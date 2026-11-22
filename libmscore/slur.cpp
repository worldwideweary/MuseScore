//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <algorithm>
#include <cmath>
#include <map>

#include "accidental.h"
#include "articulation.h"
#include "beam.h"
#include "chord.h"
#include "hook.h"
#include "note.h"
#include "symbol.h"
#include "mscore.h"
#include "measure.h"
#include "navigate.h"
#include "part.h"
#include "score.h"
#include "slur.h"
#include "spannermap.h"
#include "stem.h"
#include "system.h"
#include "tie.h"
#include "tremolo.h"
#include "undo.h"
#include "utils.h"

namespace Ms {

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SlurSegment::draw(QPainter* painter) const
      {
      QPen pen(curColor(getProperty(Pid::VISIBLE).toBool(), getProperty(Pid::COLOR).value<QColor>()));
      qreal mag = staff() ? staff()->mag(slur()->tick()) : 1.0;

      //Replace generic Qt dash patterns with improved equivalents to show true dots (keep in sync with tie.cpp)
      QVector<qreal> dotted     = { 0.01, 1.99 }; // tighter than Qt DotLine equivalent - woud be { 0.01, 2.99 }
      QVector<qreal> dashed     = { 3.00, 3.00 }; // Compensating for caps. Qt default DashLine is { 4.0, 2.0 }
      QVector<qreal> wideDashed = { 5.00, 6.00 };

      switch (slurTie()->lineType()) {
            case 0:
                  painter->setBrush(QBrush(pen.color()));
                  pen.setCapStyle(Qt::RoundCap);
                  pen.setJoinStyle(Qt::RoundJoin);
                  pen.setWidthF(score()->styleP(Sid::slurEndWidth) * mag);
                  break;
            case 1:
                  painter->setBrush(Qt::NoBrush);
                  pen.setCapStyle(Qt::RoundCap); // round dots
                  pen.setDashPattern(dotted);
                  pen.setWidthF(score()->styleP(Sid::slurDottedWidth) * mag);
                  break;
            case 2:
                  painter->setBrush(Qt::NoBrush);
                  pen.setDashPattern(dashed);
                  pen.setWidthF(score()->styleP(Sid::slurDottedWidth) * mag);
                  break;
            case 3:
                  painter->setBrush(Qt::NoBrush);
                  pen.setDashPattern(wideDashed);
                  pen.setWidthF(score()->styleP(Sid::slurDottedWidth) * mag);
                  break;
            }
      painter->setPen(pen);
      painter->drawPath(path);
      if (!_hopoText.isEmpty()) {
            painter->setPen(curColor());
            painter->setFont(hopoFont(MScore::pixelRatio));
            painter->drawText(_hopoTextPos, _hopoText);
            }

      }

QFont SlurSegment::hopoFont(qreal pixelRatio) const
      {
      QFont font(score()->styleSt(Sid::hammerOnPullOffTappingFontFace));
      const FontStyle style = FontStyle(score()->styleI(Sid::hammerOnPullOffTappingFontStyle));
      font.setBold(style & FontStyle::Bold);
      font.setItalic(style & FontStyle::Italic);
      qreal size = score()->styleD(Sid::hammerOnPullOffTappingFontSize);
      if (score()->styleB(Sid::hammerOnPullOffTappingFontSpatiumDependent))
            size *= spatium() / SPATIUM20;
      font.setPointSizeF(qMax(0.1, size * pixelRatio));
      return font;
      }

void SlurSegment::layoutHopoText()
      {
      _hopoText.clear();
      _hopoTextRect = QRectF();
      if (!spanner()->isHammerOnPullOff() || !slur()->startCR() || !slur()->endCR()
            || !slur()->startCR()->isChord() || !slur()->endCR()->isChord())
            return;
      const Chord* a = toChord(slur()->startCR());
      const Chord* b = toChord(slur()->endCR());
      const bool tab = staff() && staff()->isTabStaff(tick());
      const Note* first = a->upNote();
      const Note* last = b->upNote();
      // TAB H/P describes the physical fret movement, not a pitch modified
      // simultaneously by a bend or the whammy bar.
      if (tab) {
            bool found = false;
            for (auto i = a->notes().rbegin(); i != a->notes().rend(); ++i) {
                  for (auto j = b->notes().rbegin(); j != b->notes().rend(); ++j) {
                        if ((*i)->string() == (*j)->string() && (*i)->pitch() != (*j)->pitch()) {
                              first = *i;
                              last = *j;
                              found = true;
                              break;
                              }
                        }
                  if (found)
                        break;
                  }
            }
      _hopoText = (tab ? last->fret() > first->fret() : last->pitch() > first->pitch()) ? "H" : "P";
      const QRectF ink = QFontMetricsF(hopoFont(), MScore::paintDevice()).tightBoundingRect(_hopoText);
      const QPointF firstPos = a->upNote()->pagePos() - pagePos();
      const QPointF lastPos = b->upNote()->pagePos() - pagePos();
      const qreal x = 0.5 * (firstPos.x() + a->upNote()->bbox().right() + lastPos.x() + b->upNote()->bbox().left());
      qreal bottom = qMin(firstPos.y() + a->upNote()->bbox().top(), lastPos.y() + b->upNote()->bbox().top()) - 0.5 * spatium();
      if (system())
            bottom = qMin(bottom, system()->staffYpage(staffIdx()) - pagePos().y() - 0.5 * spatium());
      if (slur()->up())
            bottom = qMin(bottom, path.boundingRect().top() - 0.25 * spatium());
      // Letter placement is independent of the slur's UP/DOWN direction.
      _hopoTextRect = ink;
      _hopoTextRect.moveBottomLeft(QPointF(x - 0.5 * ink.width(), bottom));
      _hopoTextPos = _hopoTextRect.topLeft() - ink.topLeft();
      _shape.add(_hopoTextRect);
      }

//---------------------------------------------------------
//   searchCR
//---------------------------------------------------------

static ChordRest* searchCR(Segment* segment, int startTrack, int endTrack)
      {
      // for (Segment* s = segment; s; s = s->next1MM(SegmentType::ChordRest)) {
      for (Segment* s = segment; s; s = s->next(SegmentType::ChordRest)) {     // restrict search to measure
            if (startTrack > endTrack) {
                  for (int t = startTrack-1; t >= endTrack; --t) {
                        if (s->element(t))
                              return toChordRest(s->element(t));
                        }
                  }
            else {
                  for (int t = startTrack; t < endTrack; ++t) {
                        if (s->element(t))
                              return toChordRest(s->element(t));
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   edit
//    return true if event is accepted
//---------------------------------------------------------

bool SlurSegment::edit(EditData& ed)
      {
      Slur* sl = slur();

      if (ed.key == Qt::Key_X) {
            sl->undoChangeProperty(Pid::SLUR_DIRECTION, QVariant::fromValue<Direction>(sl->up() ? Direction::DOWN : Direction::UP));
            sl->layout();
            return true;
            }
      if (ed.key == Qt::Key_Home) {
            ups(ed.curGrip).off = QPointF();          //TODO
            sl->layout();
            return true;
            }

      if (ed.modifiers == (Qt::ShiftModifier | Qt::AltModifier)
          && (ed.curGrip == Grip::START || ed.curGrip == Grip::END)
          && (ed.key == Qt::Key_Left || ed.key == Qt::Key_Right)) {
            bool incoming = sl->isIncoming();
            bool outgoing = sl->isOutgoing();
            if (ed.curGrip == Grip::START)
                  incoming = ed.key == Qt::Key_Left;
            else
                  outgoing = ed.key == Qt::Key_Right;
            const int direction = incoming ? (outgoing ? 2 : 0) : (outgoing ? 1 : -1);
            sl->undoChangeProperty(Pid::PARTIAL_SPANNER_DIRECTION, direction);
            return true;
            }

      if (!((ed.modifiers & Qt::ShiftModifier) && (isSingleType() || (isBeginType() && ed.curGrip == Grip::START) || (isEndType() && ed.curGrip == Grip::END))))
            return false;

      ChordRest* cr = 0;
      ChordRest* e;
      ChordRest* e1;
      if (ed.curGrip == Grip::START) {
            e  = sl->startCR();
            e1 = sl->endCR();
            }
      else {
            e  = sl->endCR();
            e1 = sl->startCR();
            }

      bool incoming = sl->isIncoming();
      bool outgoing = sl->isOutgoing();
      const bool horizontal = ed.key == Qt::Key_Left || ed.key == Qt::Key_Right;
      if (horizontal) {
            const bool right = ed.key == Qt::Key_Right;
            const bool start = ed.curGrip == Grip::START;
            cr = e;
            if (start) {
                  if (incoming) {
                        incoming = false;
                        if (!right)
                              cr = prevChordRest(e);
                        }
                  else if (!right && Slur::boundaryAnchor(e->measure(), e->track(), true) == e)
                        incoming = true;
                  else {
                        cr = right ? nextChordRest(e) : prevChordRest(e);
                        if (right && cr && cr->measure() != e->measure()
                            && Slur::boundaryAnchor(cr->measure(), cr->track(), true) == cr)
                              incoming = true;
                        }
                  }
            else {
                  if (outgoing) {
                        outgoing = false;
                        if (right)
                              cr = nextChordRest(e);
                        }
                  else if (right && Slur::boundaryAnchor(e->measure(), e->track(), false) == e)
                        outgoing = true;
                  else {
                        cr = right ? nextChordRest(e) : prevChordRest(e);
                        if (!right && cr && cr->measure() != e->measure()
                            && Slur::boundaryAnchor(cr->measure(), cr->track(), false) == cr)
                              outgoing = true;
                        }
                  }
            // A coincident pair of note anchors is meaningful for a partial
            // slur. Keep ordinary slurs ordered and nonzero, and never let a
            // boundary step move an endpoint past the opposite endpoint.
            if (!cr || (cr == e1 && !incoming && !outgoing)
                || (start ? e1->isBefore(cr) : cr->isBefore(e1)))
                  return true;
            }
      else if (ed.key == Qt::Key_Up) {
            Part* part     = e->part();
            int startTrack = part->startTrack();
            int endTrack   = e->track();
            cr = searchCR(e->segment(), endTrack, startTrack);
            }
      else if (ed.key == Qt::Key_Down) {
            int startTrack = e->track() + 1;
            Part* part     = e->part();
            int endTrack   = part->endTrack();
            cr = searchCR(e->segment(), startTrack, endTrack);
            }
      if (cr && (cr != e1 || (horizontal && (incoming || outgoing)))) {
            if (cr->staff() != e->staff() && (cr->staffType()->isTabStaff() || e->staffType()->isTabStaff()))
                  return false; // Cross-staff slurs don't make sense for TAB staves
            if (cr->staff()->isLinked(e->staff()))
                  return false; // Don't allow slur to cross into staff that's linked to this
            if (horizontal) {
                  const int direction = incoming ? (outgoing ? 2 : 0) : (outgoing ? 1 : -1);
                  if (direction != sl->partialDirection())
                        sl->undoChangeProperty(Pid::PARTIAL_SPANNER_DIRECTION, direction);
                  }
            if (cr != e)
                  changeAnchor(ed, cr);
            else
                  sl->layout();
            }
      return true;
      }

//---------------------------------------------------------
//   changeAnchor
//---------------------------------------------------------

void SlurSegment::changeAnchor(EditData& ed, Element* element)
      {
      ChordRest* cr = (element && element->isChordRest()) ? toChordRest(element) : nullptr;
      ChordRest* scr = spanner()->startCR();
      ChordRest* ecr = spanner()->endCR();
      if (!cr || !scr || !ecr)
            return;

      // save current start/end elements
      for (ScoreElement* e : spanner()->linkList()) {
            Spanner* sp = toSpanner(e);
            score()->undoStack()->push1(new ChangeStartEndSpanner(sp, sp->startElement(), sp->endElement()));
            }

      if (ed.curGrip == Grip::START) {
            spanner()->undoChangeProperty(Pid::SPANNER_TICK, cr->tick());
            Fraction ticks = ecr->tick() - cr->tick();
            spanner()->undoChangeProperty(Pid::SPANNER_TICKS, ticks);
            int diff = cr->track() - spanner()->track();
            for (auto e : spanner()->linkList()) {
                  Spanner* s = toSpanner(e);
                  s->undoChangeProperty(Pid::TRACK, s->track() + diff);
                  }
            scr = cr;
            }
      else {
            Fraction ticks = cr->tick() - scr->tick();
            spanner()->undoChangeProperty(Pid::SPANNER_TICKS, ticks);
            int diff = cr->track() - spanner()->track();
            for (auto e : spanner()->linkList()) {
                  Spanner* s = toSpanner(e);
                  s->undoChangeProperty(Pid::SPANNER_TRACK2, s->track() + diff);
                  }
            ecr = cr;
            }

      // update start/end elements (which could be grace notes)
      for (ScoreElement* lsp : spanner()->linkList()) {
            Spanner* sp = static_cast<Spanner*>(lsp);
            if (sp == spanner()) {
                  score()->undo(new ChangeSpannerElements(sp, scr, ecr));
                  }
            else {
                  Element* se = 0;
                  Element* ee = 0;
                  if (scr) {
                        QList<ScoreElement*> sel = scr->linkList();
                        for (ScoreElement* lcr : qAsConst(sel)) {
                              Element* le = toElement(lcr);
                              if (le->score() == sp->score() && le->track() == sp->track()) {
                                    se = le;
                                    break;
                                    }
                              }
                        }
                  if (ecr) {
                        QList<ScoreElement*> sel = ecr->linkList();
                        for (ScoreElement* lcr : qAsConst(sel)) {
                              Element* le = toElement(lcr);
                              if (le->score() == sp->score() && le->track() == sp->track2()) {
                                    ee = le;
                                    break;
                                    }
                              }
                        }
                  if (se && ee) {
                        score()->undo(new ChangeStartEndSpanner(sp, se, ee));
                        sp->layout();
                        }
                  }
            }

      const size_t segments  = spanner()->spannerSegments().size();
      ups(ed.curGrip).off = QPointF();
      spanner()->layout();
      if (spanner()->spannerSegments().size() != segments) {
            const std::vector<SpannerSegment*>& ss = spanner()->spannerSegments();
            SlurSegment* newSegment = toSlurSegment(ed.curGrip == Grip::END ? ss.back() : ss.front());
            ed.view->startEdit(newSegment, ed.curGrip);
            triggerLayout();
            }
      }

//---------------------------------------------------------
//   computeBezier
//    compute help points of slur bezier segment
//---------------------------------------------------------

void SlurSegment::computeBezier(QPointF p6o)
      {
      if (_hasOrdinaryFit) {
            drawOrdinaryFit(p6o);
            return;
            }
      qreal _spatium  = spatium();
      qreal shoulderW;              // height as fraction of slur-length
      qreal shoulderH;
      //
      // pp1 and pp2 are the end points of the slur
      //
      QPointF pp1 = ups(Grip::START).p + ups(Grip::START).off;
      QPointF pp2 = ups(Grip::END).p   + ups(Grip::END).off;

      QPointF p2 = pp2 - pp1;
      if (qFuzzyIsNull(p2.x()) && qFuzzyIsNull(p2.y())) {
            Measure* m1 = slur()->startCR()->segment()->measure();
            Measure* m2 = slur()->endCR()->segment()->measure();
            qDebug("zero slur at tick %d(%d) track %d in measure %d-%d  tick %d ticks %d",
               m1->tick().ticks(), tick().ticks(), track(), m1->no(), m2->no(), slur()->tick().ticks(), slur()->ticks().ticks());
            slur()->setBroken(true);
            return;
            }

      qreal sinb = atan(p2.y() / p2.x());
      QTransform t;
      t.rotateRadians(-sinb);
      p2  = t.map(p2);
      p6o = t.map(p6o);

      double smallH = 0.5;
      qreal d = p2.x() / _spatium;
      if (d <= 2.0) {
            shoulderH = d * 0.5 * smallH * _spatium;
            shoulderW = .6;
            }
      else {
            qreal dd = log10(1.0 + (d - 2.0) * .5) * 2.0;
            if (dd > 3.0)
                  dd = 3.0;
            shoulderH = (dd + smallH) * _spatium + _extraHeight;
            if (d > 18.0)
                  shoulderW = 0.7; // 0.8;
            else if (d > 10)
                  shoulderW = 0.6; // 0.7;
            else
                  shoulderW = 0.5; // 0.6;
            }
      if (slur()->partialDirection() >= 0 && slur()->startCR() == slur()->endCR())
            shoulderH = std::sqrt(qMax(qreal(0), d) / 4.0) * _spatium + _extraHeight;

      shoulderH -= p6o.y();

      if (!slur()->up())
            shoulderH = -shoulderH;

      qreal c    = p2.x();
      qreal c1   = (c - c * shoulderW) * .5 + p6o.x();
      qreal c2   = c1 + c * shoulderW       + p6o.x();

      QPointF p5 = QPointF(c * .5, 0.0);

      QPointF p3(c1, -shoulderH);
      QPointF p4(c2, -shoulderH);

      qreal w = score()->styleP(Sid::slurMidWidth) - score()->styleP(Sid::slurEndWidth);
      if (staff())
            w *= staff()->mag(slur()->tick());
      if ((c2 - c1) <= _spatium)
            w *= .5;
      QPointF th(0.0, w);    // thickness of slur

      QPointF p3o = p6o + t.map(ups(Grip::BEZIER1).off);
      QPointF p4o = p6o + t.map(ups(Grip::BEZIER2).off);

      if (!p6o.isNull()) {
            QPointF p6i = t.inverted().map(p6o);
            ups(Grip::BEZIER1).off += p6i ;
            ups(Grip::BEZIER2).off += p6i;
            }

      //-----------------------------------calculate p6
      QPointF pp3  = p3 + p3o;
      QPointF pp4  = p4 + p4o;
      QPointF ppp4 = pp4 - pp3;

      qreal r2 = atan(ppp4.y() / ppp4.x());
      t.reset();
      t.rotateRadians(-r2);
      QPointF p6  = QPointF(t.map(ppp4).x() * .5, 0.0);

      t.rotateRadians(2 * r2);
      p6 = t.map(p6) + pp3 - p6o;
      //-----------------------------------

      path = QPainterPath();
      path.moveTo(QPointF());
      path.cubicTo(p3 + p3o - th, p4 + p4o - th, p2);
      if (slur()->lineType() == 0)
            path.cubicTo(p4 +p4o + th, p3 + p3o + th, QPointF());

      th = QPointF(0.0, 3.0 * w);
      shapePath = QPainterPath();
      shapePath.moveTo(QPointF());
      shapePath.cubicTo(p3 + p3o - th, p4 + p4o - th, p2);
      shapePath.cubicTo(p4 +p4o + th, p3 + p3o + th, QPointF());

      // translate back
      t.reset();
      t.translate(pp1.x(), pp1.y());
      t.rotateRadians(sinb);
      path                  = t.map(path);
      shapePath             = t.map(shapePath);
      ups(Grip::BEZIER1).p  = t.map(p3);
      ups(Grip::BEZIER2).p  = t.map(p4);
      ups(Grip::END).p      = t.map(p2) - ups(Grip::END).off;
      ups(Grip::DRAG).p     = t.map(p5);
      ups(Grip::SHOULDER).p = t.map(p6);

      _shape.clear();
      QPointF start = pp1;
      int nbShapes  = 32;  // (pp2.x() - pp1.x()) / _spatium;
      qreal minH    = qAbs(3 * w);
      const CubicBezier b(pp1, ups(Grip::BEZIER1).pos(), ups(Grip::BEZIER2).pos(), ups(Grip::END).pos());
      for (int i = 1; i <= nbShapes; i++) {
            const QPointF point = b.pointAtPercent(i/float(nbShapes));
            QRectF re     = QRectF(start, point).normalized();
            if (re.height() < minH) {
                  qreal d1 = (minH - re.height()) * .5;
                  re.adjust(0.0, -d1, 0.0, d1);
                  }
            _shape.add(re);
            start = point;
            }
      }

// The ordinary-slur preview uses the same-staff subset of Studio v4.7.5
// SlurTieLayout's endpoint and collision rules.
static qreal clearSlurTip(qreal y, qreal lineDistance, int lines, qreal margin, bool up)
      {
      const qreal row = y / lineDistance;
      if (row <= -margin || row >= lines - 1 + margin)
            return y;
      const qreal f = row - std::floor(row);
      if (f < margin)
            return y + (up ? -f - margin : margin - f) * lineDistance;
      if (f > 1 - margin)
            return y + (up ? 1 - margin - f : 1 + margin - f) * lineDistance;
      return y;
      }

static const TieSegment* nativeSlurTiePart(const SlurSegment* slur, const Tie* tie, bool first)
      {
      if (!tie || tie->up() != slur->slur()->up() || !tie->visible() || tie->segmentsEmpty())
            return nullptr;
      const TieSegment* part = first ? tie->frontSegment() : tie->backSegment();
      return part && part->system() == slur->system() && part->visible() ? part : nullptr;
      }

static QPointF nativeSlurStaffOrigin(const SlurSegment* segment)
      {
      const System* system = segment->system();
      return QPointF(system->pagePos().x(), system->staffYpage(segment->vStaffIdx()) + segment->ipos().y());
      }

// Ties are laid out before slurs, but their shapes are not in the
// rhythmic segment's staff shape. Add the drawn curve in slur coordinates.
static void appendOrdinarySlurTie(const SlurSegment* segment, const TieSegment* part, std::vector<QRectF>& obstacles)
      {
      const bool up = segment->slur()->up();
      const QPointF origin = nativeSlurStaffOrigin(segment);
      const QPointF offset = part->pagePos() - origin;
      const qreal gap = 0.1 * segment->spatium();
      for (const QRectF& band : part->shape()) {
            if (band.width() > 0.0)
                  obstacles.push_back(band.translated(offset).adjusted(0.0, up ? -gap : 0.0, 0.0, up ? 0.0 : gap));
            }
      }

static const ChordRest* continuationBoundaryChord(const System*, int track, bool first);

static bool ordinarySlurObstacles(const SlurSegment* segment, std::vector<QRectF>& obstacles)
      {
      Slur* slur = segment->slur();
      const Staff* staff = segment->staff();
      const System* system = segment->system();
      const ChordRest* start = slur->startCR();
      const ChordRest* end = slur->endCR();
      if (!system || !staff || !start || !end || !start->isChord() || !end->isChord()
          || start->isGrace() || end->isGrace()
          || start->track() != slur->track() || end->track() != start->track()
          || end->track() != slur->effectiveTrack2()
          || start->vStaffIdx() != segment->vStaffIdx() || end->vStaffIdx() != segment->vStaffIdx()
          || slur->partialDirection() >= 0 || slur->isHammerOnPullOff() || slur->lineType() != 0)
            return false;

      // For a continued slur, collect only the music in this system.
      const bool anchoredStart = segment->isSingleBeginType();
      const bool anchoredEnd = segment->isSingleEndType();
      const ChordRest* first = anchoredStart ? start : continuationBoundaryChord(system, start->track(), true);
      const ChordRest* last = anchoredEnd ? end : continuationBoundaryChord(system, end->track(), false);
      const bool up = slur->up();
      const bool stemSide = start->up() == up && end->up() == up
            && toChord(start)->stem() && toChord(end)->stem();
      if (!first || !last || !first->isChord() || !last->isChord() || !first->segment() || !last->segment()
          || first->segment()->system() != system || last->segment()->system() != system
          || first->tick() >= last->tick() || !staff->isPitchedStaff(first->tick()) || staff->lines(first->tick()) < 2
          || (!stemSide && ((anchoredStart && first->up() == up) || (anchoredEnd && last->up() == up))))
            return false;

      const auto add = [&](const Element* item, const QPointF& origin, qreal clearance) {
            if (!item || !item->visible() || !item->addToSkyline() || item->bbox().isEmpty())
                  return;
            const qreal gap = clearance * segment->spatium();
            const QRectF rect = item->bbox().translated(origin + item->pos());
            obstacles.push_back(rect.adjusted(0.0, up ? -gap : 0.0, 0.0, up ? 0.0 : gap));
            };
      const auto addRest = [&](const Element* rest, const QPointF& origin) {
            if (!toChordRest(rest)->durationType().isMeasure())
                  add(rest, origin, 0.1);
            };
      const auto addAccidental = [&](const Note* note, const QPointF& chordOrigin) {
            // The accidental's position is relative to the note, not the chord.
            add(note->accidental(), chordOrigin + note->pos(), 0.1);
            };

      // Collect the visible geometry on each side of the barline. The slur
      // keeps its own endpoints; notes in other voices there are excluded.
      for (const Segment* s = first->segment(); s && s->system() == system; s = s->next1()) {
            if (!s->enabled() || s->isBreathType() || (s->segmentType() & SegmentType::BarLineType))
                  continue;
            if (!s->isChordRestType())
                  return false;
            const bool atStart = anchoredStart && s == first->segment();
            const bool atEnd = anchoredEnd && s == last->segment();
            const Element* e = s->element(first->track());
            if (!e || !e->isChordRest())
                  return false;
            const QPointF segmentOrigin = s->measure()->pos() + s->pos();
            for (const Element* other : s->elist()) {
                  if (!other || other == e || other->vStaffIdx() != segment->vStaffIdx()
                      || !other->visible() || !other->addToSkyline())
                        continue;
                  if (stemSide || other->staffIdx() != first->staffIdx())
                        return false;
                  if (atStart || atEnd || (up ? other->track() < first->track() : other->track() > first->track()))
                        continue;
                  if (other->isRest()) {
                        addRest(other, segmentOrigin);
                        continue;
                        }
                  if (!other->isChord())
                        return false;
                  const Chord* chord = toChord(other);
                  if (chord->staffMove() || chord->beam() || chord->tuplet() || chord->tremolo()
                      || chord->arpeggio() || !chord->graceNotes().empty() || !chord->articulations().empty()
                      || !chord->el().empty())
                        return false;
                  const QPointF chordOrigin = segmentOrigin + chord->pos();
                  for (const Note* note : chord->notes()) {
                        if (note->tieFor() || note->tieBack() || note->partialTie(true) || note->partialTie(false)
                            || !note->el().empty() || !note->dots().empty())
                              return false;
                        add(note, chordOrigin, 0.4);
                        addAccidental(note, chordOrigin);
                        }
                  add(chord->stem(), chordOrigin, 0.1);
                  add(chord->hook(), chordOrigin, 0.1);
                  }
            if (e->isRest()) {
                  if (toChordRest(e)->staffMove())
                        return false;
                  addRest(e, segmentOrigin);
                  continue;
                  }
            if (!e->isChord())
                  return false;
            const Chord* chord = toChord(e);
            const Tremolo* tremolo = chord->tremolo();
            if (!chord->addToSkyline() || chord->staffMove() || chord->isNudged() || chord->isSmall()
                || chord->isGrace() || chord->beam() || chord->tuplet()
                || chord->arpeggio() || chord->stemSlash() || !chord->graceNotes().empty()
                || !chord->el().empty() || chord->notes().size() != 1)
                  return false;
            if (tremolo) {
                  if (!stemSide || !tremolo->twoNotes() || !tremolo->chord1() || !tremolo->chord2()
                      || tremolo->chord1()->staffMove() || tremolo->chord2()->staffMove()
                      || tremolo->chord1()->measure()->system() != system || tremolo->chord2()->measure()->system() != system
                      || tremolo->chord1()->up() != up || tremolo->chord2()->up() != up || tremolo->bbox().isEmpty())
                        return false;
                  if (tremolo->chord1() == chord || tremolo->chord1()->tick() < first->tick()) {
                        const QPointF origin = tremolo->pagePos() - tremolo->pos() - nativeSlurStaffOrigin(segment);
                        add(tremolo, origin, 0.1);
                        }
                  }
            // Close articulations inside the span are local obstacles.
            // Keep existing tip placement for articulated anchors.
            for (const Articulation* art : chord->articulations()) {
                  if (atStart || atEnd || !art->layoutCloseToNote())
                        return false;
                  }
            const Note* note = chord->notes().front();
            if (!note->addToSkyline() || note->isSmall() || note->mirror() || note->isNudged()
                || note->bbox().isEmpty() || ((atStart || atEnd) && note->accidental())
                || note->partialTie(true) || note->partialTie(false) || !note->dots().empty()
                || !note->el().empty() || !note->spannerFor().empty() || !note->spannerBack().empty())
                  return false;

            const Tie* outgoing = note->tieFor();
            const Tie* incoming = note->tieBack();
            const bool atOpenEnd = !anchoredEnd && chord == last && outgoing && outgoing->endNote()
                  && outgoing->endNote()->tick() >= system->lastMeasure()->endTick();
            const bool atOpenStart = !anchoredStart && chord == first && incoming && incoming->startNote()
                  && incoming->startNote()->tick() < system->firstMeasure()->tick();
            // Collect ties between interior chords and the local halves
            // of ties that continue with the slur over the system break.
            if (outgoing && !atOpenEnd && (chord == first || !outgoing->endNote() || outgoing->endNote()->tick() >= last->tick()))
                  return false;
            if (incoming && !atOpenStart && (chord == last || !incoming->startNote() || incoming->startNote()->tick() <= first->tick()))
                  return false;
            const TieSegment* outgoingPart = nativeSlurTiePart(segment, outgoing, true);
            if (outgoingPart && (outgoingPart->isSingleType() || (atOpenEnd && outgoingPart->isBeginType())))
                  appendOrdinarySlurTie(segment, outgoingPart, obstacles);
            if (atOpenStart) {
                  const TieSegment* incomingPart = nativeSlurTiePart(segment, incoming, false);
                  if (incomingPart && incomingPart->isEndType())
                        appendOrdinarySlurTie(segment, incomingPart, obstacles);
                  }

            if (!atStart) {
                  const QPointF origin = segmentOrigin + chord->pos();
                  add(note, origin, atEnd ? 0.0 : 0.4);
                  addAccidental(note, origin);
                  add(chord->stem(), origin, atEnd ? 0.0 : 0.1);
                  add(chord->hook(), origin, atEnd ? 0.0 : 0.1);
                  for (const Articulation* art : chord->articulations()) {
                        if (art->up() == up)
                              add(art, origin, 0.2);
                        }
                  }
            if (chord == last)
                  return true;
            }
      return false;
      }

static QPointF nativeSlurChordOrigin(const Chord* chord)
      {
      QPointF origin = chord->measure()->pos() + chord->segment()->pos() + chord->pos();
      if (chord->isGrace())
            origin += chord->parent()->pos();
      return origin;
      }

static const QLineF* nativeGraceSlurBeam(const Chord* chord)
      {
      const Beam* beam = chord->beam();
      if (!chord->isGrace() || chord->staffMove() || !chord->staff()
          || !chord->staff()->isPitchedStaff(chord->tick()) || !chord->measure() || !beam
          || beam->cross() || !beam->parent() || beam->system() != chord->measure()->system())
            return nullptr;
      const QLineF* line = beam->primarySegment4();
      return line && line->dx() > 0.0 ? line : nullptr;
      }

static qreal nativeSlurBeamHalfWidth(const Beam* beam, const QLineF& line)
      {
      return 0.5 * beam->score()->styleS(Sid::beamWidth).val() * beam->spatium() * beam->mag()
            * std::hypot(qreal(1.0), line.dy() / line.dx());
      }

static qreal nativeGraceBeamTipY(const Chord* chord, bool up, qreal x, qreal fallback)
      {
      const QLineF* line = nativeGraceSlurBeam(chord);
      if (!line)
            return fallback;
      const Beam* beam = chord->beam();
      const QPointF origin = beam->pagePos() - chord->pagePos();
      const qreal t = (x - origin.x() - line->x1()) / line->dx();
      // The anchor uses the existing stem X, which can fall slightly inside
      // or outside the beam edge. Sample the nearest point on this beam.
      const qreal beamY = origin.y() + line->pointAt(qBound(qreal(0.0), t, qreal(1.0))).y();
      const qreal gap = qMax(0.20, 0.35 * chord->score()->styleD(Sid::graceNoteMag)) * chord->spatium();
      return beamY + (up ? -1.0 : 1.0) * (nativeSlurBeamHalfWidth(beam, *line) + gap);
      }

static void appendNativeSlurChordObstacles(const SlurSegment* segment, const Chord* chord, bool endpoint,
                                          std::vector<QRectF>& obstacles)
      {
      const auto add = [&](const Element* item, qreal padding) {
            if (!item || !item->addToSkyline() || !item->visible() || item->bbox().isEmpty())
                  return;
            const qreal gap = endpoint ? 0.0 : padding * segment->spatium();
            QRectF rect = item->bbox().translated(nativeSlurChordOrigin(chord) + item->pos());
            rect.adjust(0.0, segment->slur()->up() ? -gap : 0.0, 0.0, segment->slur()->up() ? 0.0 : gap);
            obstacles.push_back(rect);
            };
      for (const Note* note : chord->notes())
            add(note, 0.4);
      add(chord->stem(), 0.1);
      add(chord->hook(), 0.1);
      }

static bool nativeGraceSlurObstacles(const SlurSegment* segment, std::vector<QRectF>& obstacles)
      {
      Slur* slur = segment->slur();
      const ChordRest* start = slur->startCR();
      const ChordRest* end = slur->endCR();
      const Staff* staff = segment->staff();
      if (!staff || !segment->system() || !start || !end || !start->isChord() || !end->isChord()
          || (!start->isGrace() && !end->isGrace()) || start->track() != end->track()
          || start->track() != slur->track() || end->track() != slur->effectiveTrack2()
          || start->vStaffIdx() != segment->vStaffIdx() || end->vStaffIdx() != segment->vStaffIdx()
          || !staff->isPitchedStaff(start->tick()) || staff->lines(start->tick()) < 2
          || slur->partialDirection() >= 0 || slur->isHammerOnPullOff() || slur->lineType() != 0
          || !start->measure() || !end->measure()
          || start->measure()->system() != segment->system() || end->measure()->system() != segment->system()
          || !start->segment() || !end->segment()
          || (start->isGrace() && !start->parent()) || (end->isGrace() && !end->parent()))
            return false;

      const qreal startX = nativeSlurChordOrigin(toChord(start)).x();
      const qreal endX = nativeSlurChordOrigin(toChord(end)).x();
      if (endX <= startX)
            return false;
      std::vector<const Beam*> beams;
      const auto addBeam = [&](const Chord* chord) {
            if (!chord->beam() || chord->up() != slur->up())
                  return;
            const QLineF* line = nativeGraceSlurBeam(chord);
            const Beam* beam = chord->beam();
            if (!line || std::find(beams.begin(), beams.end(), beam) != beams.end())
                  return;
            beams.push_back(beam);
            const QPointF origin = beam->pagePos() - chord->pagePos() + nativeSlurChordOrigin(chord);
            const qreal margin = nativeSlurBeamHalfWidth(beam, *line);
            // A small step follows a slanted grace beam closely enough for
            // short arcs; one rectangle around the entire beam does not.
            const int count = qBound(1, int(std::ceil(line->dx() / (0.25 * segment->spatium()))), 128);
            for (int i = 0; i < count; ++i) {
                  const QPointF a = line->pointAt(qreal(i) / count) + origin;
                  const QPointF b = line->pointAt(qreal(i + 1) / count) + origin;
                  obstacles.push_back(QRectF(a, b).normalized().adjusted(0.0, -margin, 0.0, margin));
                  }
            };
      // The start chord's own glyphs are excluded, but the beam can extend
      // from that chord into the span of the slur.
      addBeam(toChord(start));
      const auto examine = [&](const Chord* chord) {
            if (chord == start)
                  return true;
            const qreal x = nativeSlurChordOrigin(chord).x();
            if (chord != end && (x <= startX || x > endX))
                  return true;
            if (chord->staffMove() || chord->vStaffIdx() != segment->vStaffIdx()
                || chord->isNudged() || chord->notes().empty() || chord->tremolo())
                  return false;
            appendNativeSlurChordObstacles(segment, chord, chord == end, obstacles);
            addBeam(chord);
            return true;
            };
      for (const Segment* s = start->segment(); s && s->measure()->system() == segment->system(); s = s->next1()) {
            if (s->enabled() && s->isChordRestType()) {
                  const Element* e = s->element(start->track());
                  for (const Element* other : s->elist()) {
                        if (other && other != e && other->vStaffIdx() == segment->vStaffIdx() && other->addToSkyline())
                              return false;
                        }
                  if (e && e->isChord()) {
                        const Chord* owner = toChord(e);
                        if (!examine(owner))
                              return false;
                        for (const Chord* grace : owner->graceNotes()) {
                              if (!examine(grace))
                                    return false;
                              }
                        }
                  else if (e && e->isRest() && e->addToSkyline())
                        obstacles.push_back(e->bbox().translated(s->measure()->pos() + s->pos() + e->pos()));
                  }
            if (s == end->segment())
                  return true;
            }
      return false;
      }

static bool nativeBeamedSlurObstacles(const SlurSegment* segment, std::vector<QRectF>& obstacles)
      {
      Slur* slur = segment->slur();
      const ChordRest* start = slur->startCR();
      const ChordRest* end = slur->endCR();
      if (!segment->system() || !segment->staff() || !start || !end || !start->beam() || !end->beam()
          || start->tick() >= end->tick() || start->track() != end->track() || start->track() != slur->track()
          || end->track() != slur->effectiveTrack2() || !start->measure() || !end->measure()
          || !start->segment() || !end->segment() || start->measure()->system() != segment->system() || end->measure()->system() != segment->system()
          || !segment->staff()->isPitchedStaff(start->tick()) || slur->partialDirection() >= 0
          || slur->lineType() != 0 || slur->isHammerOnPullOff())
            return false;

      const qreal sp = segment->spatium();
      const QPointF origin(segment->system()->pagePos().x(), segment->system()->staffYpage(segment->vStaffIdx()));
      std::vector<const Beam*> beams;
      bool complete = false;
      for (const Segment* s = start->segment(); s && s->measure()->system() == segment->system(); s = s->next1()) {
            if (s->enabled() && s->isChordRestType()) {
                  const Element* e = s->element(start->track());
                  if (!e || !e->isChord())
                        return false;
                  for (const Element* other : s->elist()) {
                        if (other && other != e && other->vStaffIdx() == segment->vStaffIdx() && other->addToSkyline())
                              return false;
                        }
                  const Chord* chord = toChord(e);
                  const Beam* beam = chord->beam();
                  if (!beam || beam->cross() || chord->staffMove() || chord->vStaffIdx() != segment->vStaffIdx()
                      || chord->isNudged() || chord->tremolo() || chord->up() != start->up()
                      || !chord->graceNotes().empty())
                        return false;
                  if (chord != start)
                        appendNativeSlurChordObstacles(segment, chord, chord == end, obstacles);
                  if (slur->up() == chord->up() && std::find(beams.begin(), beams.end(), beam) == beams.end())
                        beams.push_back(beam);
                  }
            if (s == end->segment()) {
                  complete = true;
                  break;
                  }
            }
      if (!complete)
            return false;

      for (const Beam* beam : beams) {
            const QLineF* line = beam->primarySegment4();
            if (!line)
                  return false;
            const QPointF offset = beam->pagePos() - origin;
            const qreal margin = 0.5 * segment->score()->styleP(Sid::beamWidth) * beam->mag() + 0.1 * sp;
            const int count = qBound(1, int(std::ceil(qAbs(line->dx()) / sp)), 128);
            for (int i = 0; i < count; ++i) {
                  const QPointF a = line->pointAt(qreal(i) / count) + offset;
                  const QPointF b = line->pointAt(qreal(i + 1) / count) + offset;
                  obstacles.push_back(QRectF(a, b).normalized().adjusted(0.0, -margin, 0.0, margin));
                  }
            }
      return true;
      }

// The segment's collision shape is staff-local. For a slur that joins
// two staves, put the supported glyphs into the slur's staff frame first.
static bool crossStaffSlurObstacles(const SlurSegment* segment, std::vector<QRectF>& obstacles, bool& overBeams)
      {
      Slur* slur = segment->slur();
      const ChordRest* start = slur->startCR();
      const ChordRest* end = slur->endCR();
      const System* system = segment->system();
      if (!start->segment() || !end->segment() || start->tick() >= end->tick()
          || slur->lineType() != 0)
            return false;
      // Collect both visual staves. A slur starting on a moved note and
      // ending on its owning staff can otherwise miss the notes on the
      // staff it starts from.
      const int upper = qMin(start->vStaffIdx(), end->vStaffIdx());
      const int lower = qMax(start->vStaffIdx(), end->vStaffIdx());
      // For a slur that returns to its starting staff, keep the collision
      // search on that staff. The arc can lie between the beam and the
      // moved noteheads without having to clear the entire far staff.
      if (lower - upper > 1)
            return false;

      const QPointF origin(system->pagePos().x(), system->staffYpage(segment->vStaffIdx()) + segment->ipos().y());
      const bool up = slur->up();
      const qreal sp = segment->spatium();
      overBeams = start->track() == end->track() && start->beam() && start->up() == up;
      for (const Segment* s = start->segment()->next1(); s && s->system() == system; s = s->next1()) {
            if (!s->enabled() || !s->isChordRestType())
                  continue;
            const bool last = s == end->segment();
            const Element* inVoice = s->element(start->track());
            if (!inVoice || !inVoice->isChordRest() || !toChordRest(inVoice)->beam() || toChordRest(inVoice)->up() != up)
                  overBeams = false;
            for (const Element* element : s->elist()) {
                  if (!element || !element->isChordRest() || !element->visible() || !element->addToSkyline()
                      || element->vStaffIdx() < upper || element->vStaffIdx() > lower
                      || (last && element != end))
                        continue;
                  if (element->vStaffIdx() == start->staffIdx()
                      && (up ? element->track() < start->track() : element->track() > start->track()))
                        continue;
                  const auto add = [&](const Element* item, qreal clearance) {
                        if (!item || !item->visible() || !item->addToSkyline() || item->bbox().isEmpty())
                              return;
                        const qreal gap = last ? 0.0 : clearance * sp;
                        const QRectF rect = item->bbox().translated(item->pagePos() - origin);
                        obstacles.push_back(rect.adjusted(0.0, up ? -gap : 0.0, 0.0, up ? 0.0 : gap));
                        };
                  if (!element->isChord()) {
                        // A whole-measure rest is centered independently of
                        // its tick and is not part of the slur's passage.
                        if (element->isRest() && !toChordRest(element)->durationType().isMeasure())
                              add(element, 0.1);
                        continue;
                        }
                  const Chord* chord = toChord(element);
                  // Leave combined grace/tremolo geometry on the existing
                  // path until both can be represented in this frame.
                  if (!chord->graceNotes().empty() || chord->tremolo())
                        return false;
                  for (const Note* note : chord->notes()) {
                        add(note, 0.4);
                        add(note->accidental(), 0.1);
                        }
                  // Long stems crossing the gap are not independent
                  // obstacles in the existing chord collision shape.
                  if (!chord->beam() || !chord->beam()->cross())
                        add(chord->stem(), 0.1);
                  add(chord->hook(), 0.1);
                  if (!last) {
                        for (const Articulation* articulation : chord->articulations())
                              add(articulation, 0.2);
                        }
                  }
            if (last)
                  return true;
            }
      return false;
      }

void SlurSegment::appendNestedSlurObstacles(const SlurSegment* outer, const QPointF& first, const QPointF& last,
                                           std::vector<QRectF>& obstacles)
      {
      Slur* slur = outer->slur();
      const qreal sp = outer->spatium();
      const qreal margin = 0.16 * sp + outer->score()->styleP(Sid::slurMidWidth);
      const auto intervals = outer->score()->spannerMap().findOverlapping(slur->tick().ticks(), slur->tick2().ticks());
      for (const auto& interval : intervals) {
            Spanner* candidate = interval.value;
            if (candidate == slur || !candidate->isSlur() || candidate->track() != slur->track()
                || candidate->tick() < slur->tick() || candidate->tick2() > slur->tick2()
                || candidate->ticks() >= slur->ticks())
                  continue;
            Slur* inner = toSlur(candidate);
            if (!inner->visible() || inner->up() != slur->up() || inner->partialDirection() >= 0
                || !inner->startCR() || !inner->endCR() || inner->startCR()->isGrace() || inner->endCR()->isGrace())
                  continue;
            const SlurSegment* part = inner->frontSegment();
            if (!part || part->system() != outer->system() || part->vStaffIdx() != outer->vStaffIdx()
                || part->spannerSegmentType() != SpannerSegmentType::SINGLE)
                  continue;
            // A grip edit changes the drawn curve, not the baseline used
            // to lay out its neighbors.
            const auto points = part->_hasOrdinaryFit ? part->_ordinaryFit : std::array<QPointF, 4>{{
                  part->ups(Grip::START).pos(), part->ups(Grip::BEZIER1).pos(),
                  part->ups(Grip::BEZIER2).pos(), part->ups(Grip::END).pos()
                  }};
            const CubicBezier curve(points[0], points[1], points[2], points[3]);
            const QPointF origin = part->_hasOrdinaryFit ? part->ipos() : part->pos();
            // At a shared note, leave space for the two slurs to converge.
            const qreal left = first.x() + (inner->tick() == slur->tick() ? 0.9 * sp : 0.0);
            const qreal right = last.x() - (inner->tick2() == slur->tick2() ? 0.9 * sp : 0.0);
            for (int i = 0; i < 24; ++i) {
                  const QPointF a = curve.pointAtPercent(qreal(i) / 24) + origin;
                  const QPointF b = curve.pointAtPercent(qreal(i + 1) / 24) + origin;
                  QRectF band = QRectF(a, b).normalized();
                  const qreal l = qMax(left, band.left());
                  const qreal r = qMin(right, band.right());
                  if (r <= l)
                        continue;
                  band.setLeft(l);
                  band.setRight(r);
                  obstacles.push_back(band.adjusted(0.0, -margin, 0.0, margin));
                  }
            }
      }

bool SlurSegment::fitOrdinarySlur(const SlurSegment* segment, QPointF first, QPointF last,
                                 std::array<QPointF, 4>& fit, std::array<QPointF, 4>* initial,
                                 bool crossStaff, bool fixedTips)
      {
      const qreal sp = segment->spatium();
      const qreal width = last.x() - first.x();
      const ChordRest* start = segment->slur()->startCR();
      const ChordRest* end = segment->slur()->endCR();
      const bool continued = !segment->isSingleType();
      const bool grace = !continued && start && end && (start->isGrace() || end->isGrace());
      const bool beamed = !continued && !crossStaff && !grace && start && end && start->beam() && end->beam();
      // Keep the guard for the specialized fitters. An ordinary slur can
      // span multiple measures of the same system.
      const qreal maxSpan = beamed || crossStaff ? 48.0 : grace ? 40.0 : 0.0;
      if (!(sp > 0.0 && width >= (grace || crossStaff ? 0.4 : 2.0) * sp)
          || (maxSpan > 0.0 && width >= maxSpan * sp)
          || !std::isfinite(first.y()) || !std::isfinite(last.y()))
            return false;
      std::vector<QRectF> obstacles;
      bool overBeams = beamed && start->up() == segment->slur()->up();
      const bool eligible = crossStaff ? crossStaffSlurObstacles(segment, obstacles, overBeams)
            : grace ? nativeGraceSlurObstacles(segment, obstacles)
            : beamed ? nativeBeamedSlurObstacles(segment, obstacles) : ordinarySlurObstacles(segment, obstacles);
      if (!eligible)
            return false;
      std::vector<QRectF> nestedSlurs;
      if (beamed)
            appendNestedSlurObstacles(segment, first, last, nestedSlurs);
      const bool up = segment->slur()->up();
      const qreal direction = up ? -1.0 : 1.0;
      const Fraction tick = continued ? segment->system()->firstMeasure()->tick() : segment->slur()->tick();
      const qreal lineDistance = segment->staff()->lineDistance(tick) * sp;
      if (!(lineDistance > 0.0))
            return false;
      const int lines = segment->staff()->lines(tick);
      const qreal margin = 0.175 + 0.5 * segment->score()->styleS(Sid::staffLineWidth).val() * sp / lineDistance;
      const auto snap = [&](QPointF point, const ChordRest* anchor) {
            if (fixedTips)
                  return point;
            if (crossStaff) {
                  const int index = anchor->vStaffIdx();
                  const Staff* staff = segment->score()->staff(index);
                  const Fraction tick = anchor->tick();
                  const qreal localSp = staff->spatium(tick);
                  const qreal localStep = staff->lineDistance(tick) * localSp;
                  if (localStep > 0.0) {
                        const System* system = segment->system();
                        const qreal y = system->staff(index)->y() - system->staff(segment->vStaffIdx())->y();
                        const qreal localMargin = 0.175 + 0.5 * segment->score()->styleS(Sid::staffLineWidth).val() * localSp / localStep;
                        point.setY(y + clearSlurTip(point.y() - y, localStep, staff->lines(tick), localMargin, up));
                        }
                  return point;
                  }
            point.setY(clearSlurTip(point.y(), lineDistance, lines, margin, up));
            return point;
            };
      first = snap(first, start);
      last = snap(last, end);

      const qreal rise = last.y() - first.y();
      const qreal length = std::hypot(width, rise);
      // Grace tips may be close horizontally while their anchor notes
      // are several staff spaces apart.
      if (!std::isfinite(length) || (maxSpan > 0.0 && length >= maxSpan * sp)
          || (!grace && !crossStaff && qAbs(rise) > width))
            return false;
      const qreal cosine = width / length;
      const qreal sine = rise / length;
      const auto toStaff = [&](const QPointF& point) {
            return first + QPointF(cosine * point.x() - sine * point.y(), sine * point.x() + cosine * point.y());
            };
      const auto toSlur = [&](const QPointF& point) {
            const QPointF d = point - first;
            return QPointF(cosine * d.x() + sine * d.y(), -sine * d.x() + cosine * d.y());
            };

      QPointF p2(length, 0.0);
      const qreal height = direction * std::sqrt(length / (4.0 * sp)) * sp
            * (overBeams ? 0.75 : 1.0);
      const qreal inset = grace && length < 2.0 * sp ? 0.20 : length > 18.0 * sp ? 0.15 : length > 10.0 * sp ? 0.20 : 0.25;
      QPointF c1(inset * length, height);
      QPointF c2((1.0 - inset) * length, height);
      const auto clampShoulderX = [&](QPointF& control, qreal lastX) {
            QPointF point = toStaff(control);
            point.setX(qBound(first.x(), point.x(), lastX));
            control = toSlur(point);
            };
      if (grace || crossStaff) {
            clampShoulderX(c1, last.x());
            clampShoulderX(c2, last.x());
            }
      if (initial)
            *initial = {{ first, toStaff(c1), toStaff(c2), last }};
      // Studio caps the signed step: long downward corrections are capped,
      // while long upward corrections still grow with the span.
      const qreal step = qMin(direction * 0.30 * qMax(qreal(1.0), length / (16.0 * sp)), qreal(1.5)) * sp;
      // A slur from a beamed pre-grace to its parent starts at the beam.
      // Opposite stem directions alone don't make that tip float along
      // the stem. Put more of the collision correction into the curve.
      const Chord* graceStart = grace && start->isChord() && start->isGraceBefore() && start->parent() == end
            ? toChord(start) : nullptr;
      const bool startsOnGraceBeam = graceStart && graceStart->stem() && start->up() == up
            && nativeGraceSlurBeam(graceStart) && !start->beam()->elements().empty()
            && start->beam()->elements().back() != start;
      const bool leftFloats = grace && !startsOnGraceBeam && start->up() == up && start->up() != end->up();
      const bool mixedStems = start->up() != end->up();
      const auto crossBalance = [&](const ChordRest* cr) {
            return mixedStems && !cr->beam() && cr->up() == up ? 0.9 : 0.1;
            };
      const qreal leftBalance = continued ? (segment->isBeginType() ? 0.1 : 0.9)
            : crossStaff ? crossBalance(start) : beamed ? 0.2 : leftFloats ? 0.9 : 0.4;
      const qreal rightBalance = continued ? (segment->isEndType() ? 0.1 : 0.9)
            : crossStaff ? crossBalance(end) : beamed ? 0.2 : grace && end->up() == up && mixedStems ? 0.9 : 0.4;
      const qreal shallowSlope = 0.0872664626; // Studio's five-degree slant threshold
      const QPointF arcClearance(0.0, -direction * qBound(qreal(0.1),
            qreal(0.04 * length / sp * cosine * cosine), qreal(0.5)) * sp);
      const auto hits = [&](const CubicBezier& curve, int i, const std::vector<QRectF>& shapes) {
            const QRectF band = QRectF(toStaff(curve.pointAtPercent(i / qreal(20))),
                                        toStaff(curve.pointAtPercent((i + 1) / qreal(20)))).normalized();
            return std::any_of(shapes.begin(), shapes.end(), [&](const QRectF& item) {
                  return band.right() > item.left() && band.left() < item.right()
                        && (up ? band.bottom() >= item.top() : band.top() <= item.bottom());
                  });
            };

      for (int pass = 0; pass <= 30; ++pass) {
            bool collision[3] = { false, false, false };
            const CubicBezier clearance(QPointF(), c1 + arcClearance, c2 + arcClearance, p2);
            const CubicBezier center(QPointF(), c1, c2, p2);
            // Intersect in staff coordinates, where the obstacles lie. Studio
            // checks 19 intervals of a 20-subdivision curve.
            for (int i = 0; i < 19; ++i) {
                  const int section = i < 6 ? 0 : i < 12 ? 1 : 2;
                  if (collision[section])
                        continue;
                  // Nested slur bands already reserve the combined ribbon
                  // width and a small gap. Adding beam clearance again
                  // makes the outer shoulder unnecessarily high.
                  collision[section] = hits(clearance, i, obstacles)
                        || (!nestedSlurs.empty() && hits(center, i, nestedSlurs));
                  }
            if (!collision[0] && !collision[1] && !collision[2]) {
                  // Snap only the tips so the cleared shoulders stay put.
                  const QPointF fittedFirst = fixedTips ? first : snap(first, start);
                  const QPointF fittedLast = fixedTips ? last : snap(toStaff(p2), end);
                  fit = {{ fittedFirst, toStaff(c1), toStaff(c2), fittedLast }};
                  return true;
                  }
            if (pass == 30)
                  return false;
            // With the tips locked, spend the collision correction on
            // the shoulders rather than moving either anchor.
            if (fixedTips || pass % 2 == 0) {
                  const qreal leftStep = fixedTips ? step : (1.0 - leftBalance) * step;
                  const qreal rightStep = fixedTips ? step : (1.0 - rightBalance) * step;
                  const QPointF left(-qAbs(leftStep) * 0.5, leftStep * 0.5);
                  const QPointF right(qAbs(rightStep) * 0.5, rightStep * 0.5);
                  if (collision[0]) {
                        c1 += left;
                        c2 += (crossStaff ? QPointF(-left.x(), left.y()) : right) * 0.5;
                        }
                  if (collision[1]) {
                        c1.ry() += (leftStep + rightStep) * 0.5;
                        c2.ry() += (leftStep + rightStep) * 0.5;
                        }
                  if (collision[2]) {
                        c1 += (crossStaff ? QPointF(-right.x(), right.y()) : left) * 0.5;
                        c2 += right;
                        }
                  }
            else {
                  const qreal slope = (toStaff(p2).y() - first.y()) / width;
                  const auto endpointStep = [&](bool left) {
                        const bool side = collision[left ? 0 : 2];
                        const bool steep = (grace || crossStaff) && (left ? rise < -width : rise > width);
                        if (!side && !collision[1] && !steep)
                              return qreal(0.0);
                        qreal move = (left ? leftBalance : rightBalance) * step;
                        if (!side && collision[1] && qAbs(slope) > shallowSlope && length > 8.0 * sp) {
                              const qreal ratio = qAbs(slope) / shallowSlope;
                              const bool leftIsOuter = up == (slope > 0.0);
                              const bool outer = left ? leftIsOuter : !leftIsOuter;
                              move *= outer ? 1.0 / ratio : ratio;
                              }
                        return move;
                        };

                  // A staff-vertical move has both components in the rotated
                  // frame. Move one end at a time to preserve the other tip.
                  const qreal moveFirst = endpointStep(true);
                  const QPointF leftDelta(sine * moveFirst, cosine * moveFirst);
                  c1 -= leftDelta * (c1.x() / p2.x());
                  c2 -= leftDelta * (c2.x() / p2.x());
                  p2 -= leftDelta;
                  first.ry() += moveFirst;
                  if (!(p2.x() > 1e-6 * sp))
                        return false;

                  const qreal moveLast = endpointStep(false);
                  const QPointF rightDelta(sine * moveLast, cosine * moveLast);
                  p2 += rightDelta;
                  if (!(p2.x() > 1e-6 * sp))
                        return false;
                  c1 += rightDelta * (c1.x() / p2.x());
                  c2 += rightDelta * (c2.x() / p2.x());
                  }
            const qreal lastX = toStaff(p2).x();
            if (!(lastX > first.x()))
                  return false;
            for (QPointF* control : { &c1, &c2 }) {
                  control->setY(up ? qMax(control->y(), -p2.x()) : qMin(control->y(), p2.x()));
                  clampShoulderX(*control, lastX);
                  }
            }
      return false;
      }

static qreal nativeSlurRibbonWidth(const SlurSegment* segment, const QPointF& first, const QPointF& last)
      {
      const Score* score = segment->score();
      Slur* slur = segment->slur();
      const qreal sp = segment->spatium();
      const qreal mag = segment->staff()->mag(slur->tick());
      const qreal endWidth = score->styleP(Sid::slurEndWidth);
      const qreal normal = qMax(0.0, score->styleP(Sid::slurMidWidth) - endWidth);
      const qreal length = QLineF(first, last).length() / sp;
      const qreal shortLimit = 4.0 * mag;
      const qreal minimum = score->styleS(Sid::minTieLength).val() * mag;
      qreal width = normal;
      // Studio applies its short tie/slur thickness rule in the curve's
      // own frame. A steep arc can be long even with little horizontal span.
      if (length < shortLimit && minimum < shortLimit) {
            const qreal amount = qBound(qreal(0.0), (length - minimum) / (shortLimit - minimum), qreal(1.0));
            const qreal shortest = qMax(0.0, 0.15 * sp - endWidth);
            width = qMin(normal, shortest + (normal - shortest) * amount);
            }
      const ChordRest* start = slur->startCR();
      const ChordRest* end = slur->endCR();
      // Use the grace-note scale for a local grace group. Slurs between
      // rhythmic positions retain the ordinary ribbon ceiling.
      if (start && end && start->segment() == end->segment() && (start->isGrace() || end->isGrace()))
            width = qMin(width, normal * qBound(qreal(0.0), score->styleD(Sid::graceNoteMag), qreal(1.0)));
      return width * mag;
      }

static QPointF ordinarySlurThickness(qreal width, const QPointF& first, const QPointF& last)
      {
      const qreal angle = std::atan2(last.y() - first.y(), last.x() - first.x());
      return QPointF(-width * std::sin(angle), width * std::cos(angle));
      }

static QPointF rebaseSlurControl(const QPointF& control, const QPointF& fittedStart, const QPointF& fittedEnd,
                                const QPointF& start, const QPointF& end)
      {
      if (start == fittedStart && end == fittedEnd)
            return control;
      const QPointF fittedSpan = fittedEnd - fittedStart;
      const QPointF span = end - start;
      const qreal fittedLength = std::hypot(fittedSpan.x(), fittedSpan.y());
      if (qFuzzyIsNull(fittedLength))
            return control + start - fittedStart;
      const QPointF axis = fittedSpan / fittedLength;
      const QPointF relative = control - fittedStart;
      const qreal fraction = (relative.x() * axis.x() + relative.y() * axis.y()) / fittedLength;
      const qreal height = -relative.x() * axis.y() + relative.y() * axis.x();
      const qreal length = std::hypot(span.x(), span.y());
      const QPointF direction = qFuzzyIsNull(length) ? axis : span / length;
      return start + span * fraction + QPointF(-direction.y(), direction.x()) * height;
      }

void SlurSegment::drawOrdinaryFit(const QPointF& shoulderDelta)
      {
      ups(Grip::START).p = _ordinaryFit[0];
      ups(Grip::END).p = _ordinaryFit[3];
      const auto first = ups(Grip::START).pos();
      const auto last = ups(Grip::END).pos();
      if (ups(Grip::START).off.isNull() && ups(Grip::END).off.isNull()) {
            ups(Grip::BEZIER1).p = _ordinaryFit[1];
            ups(Grip::BEZIER2).p = _ordinaryFit[2];
            }
      else {
            // Recompute the shoulders for the edited span and slope. A
            // shoulder-only edit still uses the original automatic fit.
            std::array<QPointF, 4> fit;
            if (autoplace() && fitOrdinarySlur(this, first, last, fit, nullptr, _ordinaryFitCrossStaff, true)) {
                  ups(Grip::BEZIER1).p = fit[1];
                  ups(Grip::BEZIER2).p = fit[2];
                  }
            else {
                  // Keep a usable curve if the locked tips can't clear the
                  // obstacles (for example while dragging through a note).
                  ups(Grip::BEZIER1).p = rebaseSlurControl(_ordinaryFit[1], _ordinaryFit[0], _ordinaryFit[3], first, last);
                  ups(Grip::BEZIER2).p = rebaseSlurControl(_ordinaryFit[2], _ordinaryFit[0], _ordinaryFit[3], first, last);
                  }
            }
      ups(Grip::BEZIER1).off += shoulderDelta;
      ups(Grip::BEZIER2).off += shoulderDelta;
      const auto c1 = ups(Grip::BEZIER1).pos();
      const auto c2 = ups(Grip::BEZIER2).pos();
      ups(Grip::DRAG).p = (first + last) * 0.5;
      ups(Grip::SHOULDER).p = (c1 + c2) * 0.5;

      const qreal w = nativeSlurRibbonWidth(this, first, last);
      const QPointF thickness = ordinarySlurThickness(w, first, last);
      const auto outline = [&](const QPointF& t) {
            QPainterPath result;
            result.moveTo(first);
            result.cubicTo(c1 - t, c2 - t, last);
            result.cubicTo(c2 + t, c1 + t, first);
            return result;
            };
      path = outline(thickness);
      shapePath = outline(thickness * 3.0);
      _shape.clear();
      const CubicBezier center(first, c1, c2, last);
      QPointF previous = first;
      for (int i = 1; i <= 32; ++i) {
            const QPointF point = center.pointAtPercent(i / qreal(32));
            QRectF rect = QRectF(previous, point).normalized();
            const qreal pad = qMax(qreal(0.0), (3.0 * qAbs(w) - rect.height()) * 0.5);
            rect.adjust(0.0, -pad, 0.0, pad);
            _shape.add(rect);
            previous = point;
            }
      setbbox(path.boundingRect());
      }

void SlurSegment::separateSharedTips(const std::vector<SpannerSegment*>& segments)
      {
      struct Adjustment {
            SlurSegment* segment;
            QPointF start;
            QPointF end;
            };
      struct ChordTips {
            std::vector<size_t> starts;
            std::vector<size_t> ends;
            };
      std::vector<Adjustment> adjustments;
      std::map<const ChordRest*, ChordTips> chords;
      for (SpannerSegment* segment : segments) {
            if (!segment->isSlurSegment())
                  continue;
            SlurSegment* s = toSlurSegment(segment);
            if (!s->_hasOrdinaryFit || !s->addToSkyline()
                || s->spannerSegmentType() != SpannerSegmentType::SINGLE
                || s->_ordinaryFit[3].x() - s->_ordinaryFit[0].x() < 0.8 * s->spatium())
                  continue;
            const size_t index = adjustments.size();
            adjustments.push_back({ s, QPointF(), QPointF() });
            chords[s->slur()->startCR()].starts.push_back(index);
            chords[s->slur()->endCR()].ends.push_back(index);
            }
      // Rebuild the automatic baseline even for edited slurs. The user
      // offsets are applied later by drawOrdinaryFit(), so nudging a grip
      // doesn't change which tips are separated or undo the outer refit.
      const auto tip = [&](size_t index, Grip grip) {
            const SlurSegment* s = adjustments[index].segment;
            return s->_ordinaryFit[grip == Grip::START ? 0 : 3] + s->ipos();
            };
      const auto sameSide = [&](size_t a, size_t b) {
            const SlurSegment* first = adjustments[a].segment;
            const SlurSegment* last = adjustments[b].segment;
            return first != last && first->vStaffIdx() == last->vStaffIdx() && first->slur()->up() == last->slur()->up();
            };
      const auto separateNested = [&](const std::vector<size_t>& ids, Grip grip) {
            const bool atStart = grip == Grip::START;
            const Grip other = atStart ? Grip::END : Grip::START;
            for (size_t outer : ids) {
                  const SlurSegment* s = adjustments[outer].segment;
                  const qreal sp = s->spatium();
                  const qreal sign = s->slur()->up() ? -1.0 : 1.0;
                  const QPointF p = tip(outer, grip);
                  const qreal farX = tip(outer, other).x();
                  qreal move = 0.0;
                  for (size_t inner : ids) {
                        if (!sameSide(outer, inner))
                              continue;
                        const QPointF q = tip(inner, grip);
                        const qreal innerX = tip(inner, other).x();
                        const qreal extendsBeyond = atStart ? farX - innerX : innerX - farX;
                        if (extendsBeyond <= 0.25 * sp || qAbs(p.x() - q.x()) >= 0.25 * sp)
                              continue;
                        const qreal gap = sign * (p.y() - q.y());
                        move = qMax(move, qMin(0.65 * sp, 0.65 * sp - gap));
                        }
                  // Keep the beam attachment X; only move the outer tip and
                  // its nearby shoulder away from the inner slur.
                  (atStart ? adjustments[outer].start : adjustments[outer].end).setY(sign * move);
                  }
            };
      for (const auto& entry : chords) {
            separateNested(entry.second.starts, Grip::START);
            separateNested(entry.second.ends, Grip::END);
            for (size_t in : entry.second.ends) {
                  const QPointF end = tip(in, Grip::END);
                  const qreal sp = adjustments[in].segment->spatium();
                  for (size_t out : entry.second.starts) {
                        if (!sameSide(in, out))
                              continue;
                        const ChordRest* chord = entry.first;
                        if (chord->isGraceBefore() && chord->beam() && chord->up() == adjustments[in].segment->slur()->up())
                              continue; // Keep the full span of stem-side grace slurs.
                        const QPointF start = tip(out, Grip::START);
                        const qreal gap = start.x() - end.x();
                        if (qAbs(start.y() - end.y()) >= 0.25 * sp || qAbs(gap) >= 0.4 * sp)
                              continue;
                        const qreal move = qMin(0.2 * sp, (0.4 * sp - gap) * 0.5);
                        adjustments[in].end.rx() = qMin(adjustments[in].end.x(), -move);
                        adjustments[out].start.rx() = qMax(adjustments[out].start.x(), move);
                        }
                  }
            }
      for (const auto& a : adjustments) {
            if (a.start.isNull() && a.end.isNull())
                  continue;
            Slur* slur = a.segment->slur();
            const ChordRest* start = slur->startCR();
            const ChordRest* end = slur->endCR();
            auto& fit = a.segment->_ordinaryFit;
            // A nested slur over beams can clear the inner curves with less
            // shoulder lift once its tips have already been moved apart.
            // Rebuild from those tips, using the shorter slurs already laid
            // out in this pass. A failed refit keeps the first curve.
            if (a.start.x() == 0.0 && a.end.x() == 0.0
                && start && end && !start->isGrace() && !end->isGrace()
                && start->beam() && end->beam()
                && start->up() == slur->up() && end->up() == slur->up()) {
                  if (fitOrdinarySlur(a.segment, fit[0] + a.start, fit[3] + a.end, fit)) {
                        a.segment->computeBezier();
                        continue;
                        }
                  }
            fit[0] += a.start;
            fit[1] += a.start * 0.5;
            fit[2] += a.end * 0.5;
            fit[3] += a.end;
            for (int i : { 1, 2 })
                  fit[i].setX(qBound(fit[0].x(), fit[i].x(), fit[3].x()));
            a.segment->computeBezier();
            }
      }

//---------------------------------------------------------
//   layoutSegment
//---------------------------------------------------------

Element* SlurSegment::propertyDelegate(Pid id)
      {
      return id == Pid::PARTIAL_SPANNER_DIRECTION ? static_cast<Element*>(slur()) : SlurTieSegment::propertyDelegate(id);
      }

// Studio keeps a slur on the outside of a same-side tie at its anchor.
static void clearSlurTipFromTie(const SlurSegment* segment, const ChordRest* anchor,
                                bool atStart, QPointF& tip)
      {
      if (!anchor || !anchor->isChord() || anchor->isGrace()
          || anchor->vStaffIdx() != segment->vStaffIdx())
            return;
      const Chord* chord = toChord(anchor);
      const bool up = segment->slur()->up();
      const Note* note = up ? chord->upNote() : chord->downNote();
      const qreal clearance = 0.65 * segment->spatium();
      const qreal sign = up ? -1.0 : 1.0;
      const QPointF origin = nativeSlurStaffOrigin(segment);
      const auto tieTip = [&](const Tie* tie, bool isStart, QPointF& position) {
            if (!tie || tie->isPartial() || tie->isInside())
                  return false;
            const TieSegment* part = nativeSlurTiePart(segment, tie, isStart);
            if (!part)
                  return false;
            position = part->pagePos() + part->ups(isStart ? Grip::START : Grip::END).pos() - origin;
            return true;
            };
      QPointF tiePosition;
      if (tieTip(atStart ? note->tieFor() : note->tieBack(), atStart, tiePosition)
          && sign * (tip.y() - tiePosition.y()) < clearance) {
            tip.setY(tiePosition.y() + sign * clearance);
            return;
            }
      // Retain the full stem-to-stem span requested for beam-side slurs.
      if (chord->beam() && chord->up() == up)
            return;
      if (tieTip(atStart ? note->tieBack() : note->tieFor(), !atStart, tiePosition)
          && qAbs(tip.y() - tiePosition.y()) < clearance)
            tip.rx() += (atStart ? 0.35 : -0.35) * segment->spatium();
      }

static void clearOpenSlurTipFromTie(const SlurSegment* segment, const ChordRest* boundary,
                                    bool atStart, QPointF& tip)
      {
      if (!boundary || !boundary->isChord() || boundary->vStaffIdx() != segment->vStaffIdx())
            return;
      const Chord* chord = toChord(boundary);
      const bool up = segment->slur()->up();
      const Note* note = up ? chord->upNote() : chord->downNote();
      const Tie* tie = atStart ? note->tieBack() : note->tieFor();
      if (!tie || tie->isInside() || tie->isPartial())
            return;
      const TieSegment* part = nativeSlurTiePart(segment, tie, !atStart);
      if (!part || (atStart ? !part->isEndType() : !part->isBeginType()))
            return;
      const Grip open = atStart ? Grip::START : Grip::END;
      const qreal tieY = (part->pagePos() + part->ups(open).pos() - nativeSlurStaffOrigin(segment)).y();
      const qreal gap = 0.65 * segment->spatium();
      tip.setY(up ? qMin(tip.y(), tieY - gap) : qMax(tip.y(), tieY + gap));
      }

static const ChordRest* continuationBoundaryChord(const System* system, int track, bool first)
      {
      const auto& measures = system->measures();
      for (size_t i = 0; i < measures.size(); ++i) {
            const MeasureBase* mb = measures[first ? i : measures.size() - i - 1];
            if (!mb->isMeasure())
                  continue;
            const Measure* measure = toMeasure(mb);
            for (const Segment* s = first ? measure->first() : measure->last(); s; s = first ? s->next() : s->prev()) {
                  if (!s->enabled() || !s->isChordRestType())
                        continue;
                  const Element* element = s->element(track);
                  if (element && element->isChordRest())
                        return toChordRest(element);
                  }
            }
      return nullptr;
      }

static qreal continuationChordEdge(const ChordRest* cr, const SlurSegment* segment, qreal fallback)
      {
      if (!cr || !cr->isChord() || cr->vStaffIdx() != segment->vStaffIdx())
            return fallback;
      const Chord* chord = toChord(cr);
      if (chord->notes().empty())
            return fallback;
      const bool up = segment->slur()->up();
      const Note* note = up ? chord->upNote() : chord->downNote();
      qreal y = note->y() + (up ? note->bbox().top() : note->bbox().bottom());
      const auto include = [&](const Element* item) {
            if (!item || !item->visible() || item->bbox().isEmpty())
                  return;
            const qreal edge = item->y() + (up ? item->bbox().top() : item->bbox().bottom());
            y = up ? qMin(y, edge) : qMax(y, edge);
            };
      const bool stemSide = chord->stem() && chord->up() == up;
      if (stemSide) {
            include(chord->stem());
            include(chord->hook());
            }
      else
            y += (up ? -0.4 : 0.4) * segment->spatium();
      return chord->y() + y;
      }

void SlurSegment::layoutSegment(const QPointF& p1, const QPointF& p2, bool crossStaff)
      {
      _hasOrdinaryFit = false;
      _ordinaryFitCrossStaff = crossStaff;
      setPos(QPointF());
      ups(Grip::START).p = p1;
      ups(Grip::END).p   = p2;
      const SpannerSegmentType type = spannerSegmentType();
      static const bool traceEnabled = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_TRACE") == 1;
      const ChordRest* traceStart = traceEnabled ? slur()->startCR() : nullptr;
      const ChordRest* traceEnd = traceEnabled ? slur()->endCR() : nullptr;
      const bool trace = type == SpannerSegmentType::SINGLE && slur()->partialDirection() < 0
            && system() && staff() && traceStart && traceEnd
            && traceStart->measure() && traceEnd->measure()
            && traceStart->vStaffIdx() == vStaffIdx() && traceEnd->vStaffIdx() == vStaffIdx()
            && !staff()->isTabStaff(traceStart->tick());
      const bool traceGrace = trace && (traceStart->isGrace() || traceEnd->isGrace());
      const bool incoming = slur()->isIncoming() && (type == SpannerSegmentType::SINGLE || type == SpannerSegmentType::BEGIN);
      const bool outgoing = slur()->isOutgoing() && (type == SpannerSegmentType::SINGLE || type == SpannerSegmentType::END);
      static const bool previewEnabled = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1;
      const ChordRest* startCR = slur()->startCR();
      const ChordRest* endCR = slur()->endCR();
      const bool continued = previewEnabled && type != SpannerSegmentType::SINGLE
            && slur()->partialDirection() < 0 && !slur()->isHammerOnPullOff()
            && system() && staff() && staff()->isPitchedStaff(tick())
            && startCR && endCR && !startCR->isGrace() && !endCR->isGrace()
            && startCR->vStaffIdx() == vStaffIdx() && endCR->vStaffIdx() == vStaffIdx();
      _extraHeight = 0.0;

      // Adjust Y for staff type before converting tie geometry to this frame.
      if (staffType())
            rypos() += staffType()->yoffset().val() * spatium();

      if (continued) {
            const qreal sp = spatium();
            const qreal sign = slur()->up() ? -1.0 : 1.0;
            if (type == SpannerSegmentType::END || type == SpannerSegmentType::MIDDLE) {
                  const ChordRest* first = continuationBoundaryChord(system(), slur()->effectiveTrack2(), true);
                  qreal y = continuationChordEdge(first, this, p1.y());
                  if (type == SpannerSegmentType::END && first == endCR)
                        y = p2.y() + sign * 0.25 * sp;
                  ups(Grip::START).p = QPointF(system()->firstNoteRestSegmentX(true), y);
                  if (autoplace() && slur()->lineType() == 0 && first != endCR)
                        clearOpenSlurTipFromTie(this, first, true, ups(Grip::START).p);
                  }
            if (type == SpannerSegmentType::BEGIN || type == SpannerSegmentType::MIDDLE) {
                  const ChordRest* last = continuationBoundaryChord(system(), slur()->track(), false);
                  const qreal startY = ups(Grip::START).p.y();
                  qreal y = continuationChordEdge(last, this, startY);
                  if (type == SpannerSegmentType::BEGIN && last == startCR)
                        y = startY + sign * 0.25 * sp;
                  // Limit the contour's movement toward the staff. Let it
                  // follow the music freely in the outer direction.
                  if (sign * (y - startY) < -2.5 * sp)
                        y = startY - sign * 2.5 * sp;
                  const qreal x = system()->lastMeasure()->endingXForOpenEndedLines();
                  if (x > ups(Grip::START).p.x()) {
                        ups(Grip::END).p = QPointF(x, y);
                        if (autoplace() && slur()->lineType() == 0)
                              clearOpenSlurTipFromTie(this, last, false, ups(Grip::END).p);
                        }
                  }
            }
      // Partial slurs already receive repeat-aware contours from slurPosChord().
      else if (slur()->partialDirection() < 0) {
            auto contourY = [&](Measure* measure, bool first, qreal fallback) {
                  if (!measure)
                        return fallback;
                  for (Segment* segment = first ? measure->first() : measure->last(); segment;
                        segment = first ? segment->next() : segment->prev()) {
                        if (!segment->isChordRestType())
                              continue;
                        Element* element = segment->element(first ? slur()->track() : slur()->effectiveTrack2());
                        if (!element || !element->isChord())
                              continue;
                        Chord* chord = toChord(element);
                        if (chord->notes().empty() || chord->vStaffIdx() != vStaffIdx())
                              return fallback;
                        const bool up = slur()->up();
                        qreal y = chord->y();
                        if (chord->stem() && chord->up() == up)
                              y += chord->stem()->y() + (up ? -chord->stem()->height() : chord->stem()->height());
                        else
                              y += (up ? chord->upNote()->y() - spatium() : chord->downNote()->y() + spatium());
                        return y;
                        }
                  return fallback;
                  };
            if (type == SpannerSegmentType::END && system())
                  ups(Grip::START).p.ry() = contourY(system()->firstMeasure(), true, p1.y());
            if (type == SpannerSegmentType::BEGIN && system())
                  ups(Grip::END).p.ry() = contourY(system()->lastMeasure(), false, p2.y());
            }
      const bool startIsHanging = type == SpannerSegmentType::END || incoming;
      const bool endIsHanging = type == SpannerSegmentType::BEGIN || outgoing;
      // Angle a floating end relative to its anchored opposite end. If both
      // ends float, preserve the repeat contour: neither is a fixed reference.
      if (score()->styleB(Sid::angleHangingSlursAwayFromStaff) && startIsHanging != endIsHanging) {
            const bool up = slur()->up();
            const qreal height = spatium();
            if (endIsHanging)
                  ups(Grip::END).p.ry() = up ? qMin(ups(Grip::END).p.y(), ups(Grip::START).p.y() - height) : qMax(ups(Grip::END).p.y(), ups(Grip::START).p.y() + height);
            if (startIsHanging)
                  ups(Grip::START).p.ry() = up ? qMin(ups(Grip::START).p.y(), ups(Grip::END).p.y() - height) : qMax(ups(Grip::START).p.y(), ups(Grip::END).p.y() + height);
            }
      if (autoplace() && slur()->partialDirection() >= 0
          && ups(Grip::START).off.isNull() && ups(Grip::END).off.isNull()) {
            // Studio keeps partial-slur endpoints away from staff lines.
            // Its one-note 0.25sp constraint is applied before this adjustment.
            const qreal lineDistance = (staffType() ? staffType()->lineDistance().val() : 1.0) * spatium();
            const qreal margin = 0.175 + 0.5 * score()->styleS(Sid::staffLineWidth).val() * spatium() / lineDistance;
            const int lines = staff() ? staff()->lines(slur()->tick()) : 0;
            for (Grip grip : { Grip::START, Grip::END }) {
                  qreal y = ups(grip).p.y() / lineDistance;
                  if (y <= -margin || y >= lines - 1 + margin)
                        continue;
                  const qreal fraction = y - std::floor(y);
                  qreal adjustment = 0;
                  if (slur()->up()) {
                        if (fraction < margin)
                              adjustment = -(fraction + margin);
                        else if (fraction > 1 - margin)
                              adjustment = -(fraction - (1 - margin));
                        }
                  else {
                        if (fraction < margin)
                              adjustment = margin - fraction;
                        else if (fraction > 1 - margin)
                              adjustment = 1 - fraction + margin;
                        }
                  ups(grip).p.ry() += adjustment * lineDistance;
                  }
            }

      const qreal staffOriginY = ipos().y();
      const qreal beforeStartY = ups(Grip::START).pos().y();
      const qreal beforeEndY = ups(Grip::END).pos().y();
      const auto crownY = [this]() {
            const QRectF bounds = path.boundingRect();
            return slur()->up() ? bounds.top() : bounds.bottom();
            };

      std::array<QPointF, 4> initialFit;
      if (previewEnabled && autoplace() && type == SpannerSegmentType::SINGLE) {
            QPointF first = p1;
            QPointF last = p2;
            if (!crossStaff && system() && staff() && slur()->partialDirection() < 0
                && !slur()->isHammerOnPullOff() && slur()->lineType() == 0
                && startCR && endCR && !startCR->isGrace() && !endCR->isGrace()
                && startCR->vStaffIdx() == vStaffIdx() && endCR->vStaffIdx() == vStaffIdx()
                && staff()->isPitchedStaff(slur()->tick())) {
                  clearSlurTipFromTie(this, startCR, true, first);
                  clearSlurTipFromTie(this, endCR, false, last);
                  if (last.x() - first.x() < 0.4 * spatium()) {
                        first.setX(p1.x());
                        last.setX(p2.x());
                        }
                  ups(Grip::START).p = first;
                  ups(Grip::END).p = last;
                  }
            _hasOrdinaryFit = fitOrdinarySlur(this, first, last, _ordinaryFit, trace ? &initialFit : nullptr, crossStaff);
            }

      if (continued && autoplace())
            _hasOrdinaryFit = fitOrdinarySlur(this, ups(Grip::START).p, ups(Grip::END).p, _ordinaryFit);

      computeBezier();

      qreal beforeCrownY = trace ? crownY() : 0.0;
      if (trace && _hasOrdinaryFit) {
            const bool up = slur()->up();
            const qreal w = nativeSlurRibbonWidth(this, initialFit[0], initialFit[3]);
            const QPointF t = ordinarySlurThickness(up ? -w : w, initialFit[0], initialFit[3]);
            QPainterPath initial(initialFit[0]);
            initial.cubicTo(initialFit[1] + t, initialFit[2] + t, initialFit[3]);
            const QRectF bounds = initial.boundingRect();
            beforeCrownY = up ? bounds.top() : bounds.bottom();
            }
      qreal overlap = 0.0;
      if (!_hasOrdinaryFit && autoplace() && system()) {
            bool up = slur()->up();
            Segment* ls = system()->lastMeasure()->last();
            Segment* fs = system()->firstMeasure()->first();
            Segment* ss = slur()->startSegment();
            Segment* es = slur()->endSegment();
            QPointF pp1 = ups(Grip::START).p;
            QPointF pp2 = ups(Grip::END).p;
            qreal slurMaxMove = spatium();
            bool intersection = false;
            qreal gdist = 0.0;
            qreal minDistance = score()->styleS(Sid::slurMinDistance).val() * spatium();
            for (int tries = 1; true; ++tries) {
                  for (Segment* s = fs; s && s != ls; s = s->next1()) {
                        if (!s->enabled())
                              continue;
                        // skip start and end segments on assumption start and end points were placed well already
                        // this avoids overcorrection on collision with own ledger lines and accidentals
                        // it also avoids issues where slur appears to be attached to a note in a different voice
                        if (s == ss || s == es)
                              continue;
                        // allow slurs to cross barlines
                        if (s->segmentType() & SegmentType::BarLineType)
                              continue;
                        qreal x1 = s->x() + s->measure()->x();
                        qreal x2 = x1 + s->width();
                        if (pp1.x() > x2)
                              continue;
                        if (pp2.x() < x1)
                              break;
                        const Shape& segShape = s->staffShape(staffIdx()).translated(s->pos() + s->measure()->pos());
                        if (!intersection)
                              intersection = segShape.intersects(_shape);
                        if (up) {
                              //QPointF pt = QPointF(s->x() + s->measure()->x(), s->staffShape(staffIdx()).top() + s->y() + s->measure()->y());
                              qreal dist = _shape.minVerticalDistance(segShape);
                              if (dist > 0.0)
                                    gdist = qMax(gdist, dist);
                              }
                        else {
                              //QPointF pt = QPointF(s->x() + s->measure()->x(), s->staffShape(staffIdx()).bottom() + s->y() + s->measure()->y());
                              qreal dist = segShape.minVerticalDistance(_shape);
                              if (dist > 0.0)
                                    gdist = qMax(gdist, dist);
                              }
                        }
                  if (!intersection || gdist <= slurMaxMove || tries >= 2)
                        break;
                  // slur would be moved too far
                  // try again with a steeper curve
                  _extraHeight += gdist;
                  computeBezier();
                  intersection = false;
                  gdist = 0.0;
                  }
            if (intersection && gdist > 0.0) {
                  overlap = gdist;
                  qreal min = minDistance + gdist;
                  rypos() += up ? -min : min;
                  }
            }
      if (traceGrace && traceStart->isChord() && traceEnd->isChord()) {
            const qreal sp = spatium();
            const qreal shift = ipos().y() - staffOriginY;
            const auto anchor = [&](const Chord* chord, const QPointF& raw, Grip grip) -> QString {
                  if (!chord->segment() || chord->notes().empty() || (chord->isGrace() && !chord->parent()))
                        return QStringLiteral("unavailable");
                  const Note* note = slur()->up() ? chord->upNote() : chord->downNote();
                  QPointF center = chord->measure()->pos() + chord->segment()->pos()
                        + chord->pos() + note->pos() + note->bbox().center();
                  if (chord->isGrace())
                        center += chord->parent()->pos();
                  const QPointF before = (raw - center) / sp;
                  const QPointF final = (ups(grip).pos() + QPointF(0.0, shift) - center) / sp;
                  const qreal stemLength = chord->stem() ? (chord->stem()->len() + chord->stem()->userLen()) / sp : 0.0;
                  return QString::asprintf("kind=%s index=%d pitch=%d beam=%d rawXY/sp=%.3f,%.3f finalXY/sp=%.3f,%.3f"
                        " stemLength/sp=%.3f",
                        chord->isGraceAfter() ? "after" : chord->isGrace() ? "before" : "main",
                        chord->isGrace() ? chord->graceIndex() : -1, note->pitch(), chord->beam() != nullptr,
                        before.x(), before.y(), final.x(), final.y(), stemLength);
                  };
            const QString first = anchor(toChord(traceStart), p1, Grip::START);
            const QString last = anchor(toChord(traceEnd), p2, Grip::END);
            const qreal ribbon = _hasOrdinaryFit
                  ? nativeSlurRibbonWidth(this, ups(Grip::START).pos(), ups(Grip::END).pos()) / sp : -1.0;
            qDebug("[native-grace-slur] measures=%d-%d ticks=%d-%d track=%d up=%d span/sp=%.3f"
                   " start={%s} end={%s} crownY/sp=%.3f->%.3f wholeCurveShift/sp=%.3f heightAdded/sp=%.3f"
                   " studioFit=%d ribbonOffset/sp=%.3f",
                   traceStart->measure()->no() + 1, traceEnd->measure()->no() + 1,
                   traceStart->tick().ticks(), traceEnd->tick().ticks(), slur()->track(), slur()->up(),
                   (p2.x() - p1.x()) / sp, qPrintable(first), qPrintable(last),
                   beforeCrownY / sp, (crownY() + shift) / sp, shift / sp, _extraHeight / sp, _hasOrdinaryFit, ribbon);
            }
      else if (trace && !traceGrace) {
            const qreal sp = spatium();
            const qreal shift = ipos().y() - staffOriginY;
            const qreal lineStep = staffType() ? staffType()->lineDistance().val() : 1.0;
            qDebug("[native-slur] measures=%d-%d ticks=%d-%d staff=%d up=%d autoplace=%d edited=%d"
                   " lineStep/sp=%.3f lineWidth/sp=%.3f span/sp=%.3f rawTipsY/sp=%.3f,%.3f"
                   " preFitTipsY/sp=%.3f,%.3f finalTipsY/sp=%.3f,%.3f"
                   " crownY/sp=%.3f->%.3f wholeCurveShift/sp=%.3f overlap/sp=%.3f heightAdded/sp=%.3f"
                   " studioFit=%d snappedStartY/sp=%.3f snappedEndY/sp=%.3f",
                   traceStart->measure()->no() + 1, traceEnd->measure()->no() + 1,
                   traceStart->tick().ticks(), traceEnd->tick().ticks(), staffIdx() + 1,
                   slur()->up(), autoplace(), isEdited() || isNudged(), lineStep,
                   score()->styleS(Sid::staffLineWidth).val(), (p2.x() - p1.x()) / sp,
                   p1.y() / sp, p2.y() / sp, beforeStartY / sp, beforeEndY / sp,
                   (ups(Grip::START).pos().y() + shift) / sp, (ups(Grip::END).pos().y() + shift) / sp,
                   beforeCrownY / sp, (crownY() + shift) / sp, shift / sp, overlap / sp, _extraHeight / sp,
                   _hasOrdinaryFit, (_hasOrdinaryFit ? initialFit[0].y() : beforeStartY) / sp,
                   (_hasOrdinaryFit ? initialFit[3].y() : beforeEndY) / sp);
            }
      layoutHopoText();
      QRectF bounds = path.boundingRect();
      if (!_hopoTextRect.isEmpty())
            bounds |= _hopoTextRect;
      setbbox(bounds);
      }

// Use the rendered beam in the late pass, when crossing stems have their
// final directions and lengths. An opposite-side tip stays on its notehead.
static bool crossBeamSlurTip(const SlurSegment* segment, const Chord* chord, bool atStart, QPointF& tip)
      {
      const Beam* beam = chord->beam();
      const Stem* stem = chord->stem();
      const System* system = segment->system();
      const bool up = segment->slur()->up();
      if (!stem || chord->up() != up || !beam || !beam->cross() || beam->empty()
          || !beam->parent() || beam->system() != system
          || chord == (atStart ? beam->elements().back() : beam->elements().front()))
            return false;
      const QLineF* line = beam->primarySegment4();
      if (!line || line->dx() <= 0.0)
            return false;
      const qreal pageX = chord->pagePos().x() + stem->x();
      const QPointF beamOrigin = beam->pagePos();
      const qreal x = pageX - beamOrigin.x();
      const qreal sp = segment->spatium();
      // A stem center may be slightly inset from the end of the beam.
      if (x < line->x1() - 0.25 * sp || x > line->x2() + 0.25 * sp)
            return false;
      const qreal t = qBound(qreal(0.0), (x - line->x1()) / line->dx(), qreal(1.0));
      const qreal beamY = beamOrigin.y() + line->pointAt(t).y();
      const qreal clearance = nativeSlurBeamHalfWidth(beam, *line) + 0.35 * chord->spatium();
      const qreal staffY = system->staffYpage(segment->vStaffIdx());
      tip = QPointF(pageX - system->pagePos().x(), beamY - staffY + (up ? -clearance : clearance));
      return true;
      }

// Endpoints can share a staff while the voice visits the adjacent staff
// between them. Recognize the uncomplicated single-measure cases here.
static bool returnsAcrossStaff(const ChordRest* start, const ChordRest* end)
      {
      if (start->track() != end->track() || start->measure() != end->measure()
          || !start->segment() || !end->segment() || start->tick() >= end->tick())
            return false;
      const Beam* beam = start->beam();
      if (beam != end->beam() || (beam && !beam->cross()))
            return false;
      bool moved = false;
      for (const Segment* segment = start->segment()->next1(); segment && segment != end->segment()
            && segment->tick() < end->tick(); segment = segment->next1()) {
            if (!segment->enabled() || !segment->isChordRestType())
                  continue;
            const Element* element = segment->element(start->track());
            if (!element)
                  continue;
            if (!element->isChord())
                  return false;
            const Chord* chord = toChord(element);
            if (chord->beam() != beam || chord->tremolo() || !chord->graceNotes().empty())
                  return false;
            const int distance = qAbs(chord->vStaffIdx() - start->vStaffIdx());
            if (distance > 1)
                  return false;
            moved |= distance == 1;
            }
      return moved;
      }

// System::layout2() creates cross-staff slur segments once staff Y is
// known. Page layout can still change those distances and the beam stems.
void SlurSegment::relayoutCrossStaff()
      {
      if (!isSingleType() || !system())
            return;
      Slur* s = slur();
      const ChordRest* start = s->startCR();
      const ChordRest* end = s->endCR();
      if (!start || !end || !start->isChord() || !end->isChord()
          || start->isGrace() || end->isGrace() || s->partialDirection() >= 0
          || s->isHammerOnPullOff() || start->part() != end->part()
          || !start->measure() || !end->measure()
          || start->measure()->system() != system() || end->measure()->system() != system())
            return;
      for (const ChordRest* cr : { start, end }) {
            const int index = cr->vStaffIdx();
            if (!system()->staff(index)->show() || !score()->staff(index)->isPitchedStaff(cr->tick()))
                  return;
            }
      const bool returning = start->vStaffIdx() == end->vStaffIdx();
      if (returning && (s->lineType() != 0 || !autoplace() || !returnsAcrossStaff(start, end)))
            return;
      SlurPos positions;
      s->slurPos(&positions);
      const qreal rawStartY = positions.p1.y();
      const qreal rawEndY = positions.p2.y();
      const bool beamStart = crossBeamSlurTip(this, toChord(start), true, positions.p1);
      const bool beamEnd = crossBeamSlurTip(this, toChord(end), false, positions.p2);
      if (returning && start->beam() && !(beamStart && beamEnd))
            return;
      layoutSegment(positions.p1, positions.p2, true);
      if (qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_TRACE") == 1) {
            const qreal sp = spatium();
            qDebug("[native-cross-slur] measure=%d staff=%d->%d fit=%d span/sp=%.3f rawY/sp=%.3f,%.3f fittedY/sp=%.3f,%.3f beamTips=%d,%d",
                   start->measure()->no() + 1, start->vStaffIdx() + 1, end->vStaffIdx() + 1,
                   _hasOrdinaryFit, (positions.p2.x() - positions.p1.x()) / sp,
                   rawStartY / sp, rawEndY / sp,
                   (ipos().y() + ups(Grip::START).p.y()) / sp, (ipos().y() + ups(Grip::END).p.y()) / sp,
                   beamStart, beamEnd);
            }
      }

//---------------------------------------------------------
//   isEdited
//---------------------------------------------------------

bool SlurSegment::isEdited() const
      {
      for (int i = 0; i < int(Grip::GRIPS); ++i) {
            if (!_ups[i].off.isNull())
                  return true;
            }
      return false;
      }

Slur::Slur(const Slur& s)
   : SlurTie(s), _partialDirection(s._partialDirection)
      {
      }

//---------------------------------------------------------
//   fixArticulations
//---------------------------------------------------------

static qreal fixArticulations(qreal yo, Chord* c, qreal _up, bool stemSide = false)
      {
      //
      // handle special case of tenuto and staccato
      // yo = current offset of slur from chord position
      // return unchanged position, or position of outmost "close" articulation
      //
#if 1
      for (Articulation* a : c->articulations()) {
            if (!a->layoutCloseToNote() || !a->addToSkyline())
                  continue;
            // skip if articulation on stem side but slur is not or vice versa
            if ((a->up() == c->up()) != stemSide)
                  continue;
            if (a->up())
                  yo = qMin(yo, a->y() + (a->height() + c->score()->spatium() * .3) * _up);
            else
                  yo = qMax(yo, a->y() + (a->height() + c->score()->spatium() * .3) * _up);
            }
      return yo;
#else
      const QVector<Articulation*>& al = c->articulations();
      if (al.size() >= 2) {
            Articulation* a = al.at(1);
            if (a->up() == c->up() && !stemSide)
                  return yo;
            else if (a->layoutCloseToNote())
                  return a->y() + (a->height() + c->score()->spatium() * .3) * _up;
            }
      else if (al.size() >= 1) {
            Articulation* a = al.at(0);
            if (a->up() == c->up() && !stemSide)
                  return yo;
            else if (a->layoutCloseToNote())
                  return a->y() + (a->height() + c->score()->spatium() * .3) * _up;
            }
      return yo;
#endif
      }

// 37E draws close articulations with a centered glyph box. Use that
// box to apply Studio's inward inset and clearance on the slur side.
static void placeArticulatedSlurTip(QPointF& tip, const Chord* chord, bool up, bool start, qreal slurSp)
      {
      const QPointF origin = chord->pos() + chord->segment()->pos() + chord->measure()->pos();
      QRectF ink;
      for (const Articulation* art : chord->articulations()) {
            if (!art->layoutCloseToNote() || !art->addToSkyline() || art->up() != up || art->bbox().isEmpty())
                  continue;
            const QRectF bounds = art->bbox().translated(origin + art->pos());
            if (ink.isEmpty() || (up ? bounds.top() < ink.top() : bounds.bottom() > ink.bottom()))
                  ink = bounds;
            }
      if (ink.isEmpty())
            return;

      tip.setX(ink.center().x() + (start ? 0.1 : -0.1) * slurSp);
      const qreal gap = 0.5 * chord->spatium();
      tip.setY(up ? qMin(tip.y(), ink.top() - gap) : qMax(tip.y(), ink.bottom() + gap));
      const Staff* staff = chord->staff();
      const qreal step = staff->lineDistance(chord->tick()) * slurSp;
      if (step > 0.0) {
            const qreal margin = 0.175 + 0.5 * chord->score()->styleS(Sid::staffLineWidth).val() * slurSp / step;
            tip.setY(clearSlurTip(tip.y(), step, staff->lines(chord->tick()), margin, up));
            }
      }

//---------------------------------------------------------
//   slurPos
//    Calculate position of start- and endpoint of slur
//    relative to System() position.
//---------------------------------------------------------

void Slur::slurPosChord(SlurPos* sp)
      {
      Chord* stChord;
      Chord* enChord ;
      if (startChord()->isGraceAfter()){     // grace notes after, coming in reverse order
            stChord = endChord();
            enChord = startChord();
            _up = false;
            }
      else {
            stChord = startChord();
            enChord = endChord();
            }
      Note* _startNote = stChord->downNote();
      Note* _endNote   = enChord->downNote();
      qreal hw         = _startNote->bboxRightPos();
      qreal __up       = _up ? -1.0 : 1.0;
      qreal _spatium = spatium();

      Measure* measure = endChord()->measure();
      sp->system1 = measure->system();
      if (!sp->system1) {             // DEBUG
            qDebug("no system1");
            return;
            }
      Q_ASSERT(sp->system1);
      sp->system2 = sp->system1;
      QPointF pp(sp->system1->pagePos());

      qreal xo;
      qreal yo;

      //------p1
      if (_up) {
            xo = _startNote->x() + hw * 1.12;
            yo = _startNote->pos().y() + hw * .3 * __up;
            }
      else {
            xo = _startNote->x() + hw * 0.4;
            yo = _startNote->pos().y() + _spatium * .75 * __up;
            }
      sp->p1 = stChord->pagePos() - pp + QPointF(xo, yo);

      //------p2
      if ((enChord->notes().size() > 1) || (enChord->stem() && !enChord->up() && !_up)) {
            xo = _endNote->x() - hw * 0.12;
            yo = _endNote->pos().y() + hw * .3 * __up;
            }
      else {
            xo = _endNote->x() + hw * 0.15;
            yo = _endNote->pos().y() + _spatium * .75 * __up;
            }
      sp->p2 = enChord->pagePos() - pp + QPointF(xo, yo);
      }

//---------------------------------------------------------
//   slurPos
//    calculate position of start- and endpoint of slur
//    relative to System() position
//---------------------------------------------------------

// Studio uses the chord at the measure boundary for a floating anchor, not
// the ordinary slur attachment (which can sit a quarter-spatium into a stem).
static ChordRest* partialSlurBoundaryCR(const Measure* measure, int track, bool incoming)
      {
      ChordRest* result = nullptr;
      for (Segment* segment = measure->first(SegmentType::ChordRest); segment;
           segment = segment->next(SegmentType::ChordRest)) {
            if (Element* element = segment->element(track)) {
                  result = toChordRest(element);
                  if (incoming)
                        break;
                  }
            }
      return result;
      }

// Keyboard endpoints use the same repeat roadmap as saved partial slurs.
// A system break alone is not a repeat boundary.
ChordRest* Slur::boundaryAnchor(const Measure* measure, int track, bool incoming)
      {
      if (!measure || (incoming ? precedingRepeatMeasures(measure) : followingRepeatMeasures(measure)).empty())
            return nullptr;
      ChordRest* cr = partialSlurBoundaryCR(measure, track, incoming);
      if (!cr || !cr->isChord())
            return nullptr;
      if (incoming ? cr->tick() != measure->tick() : cr->tick() + cr->actualTicks() != measure->endTick())
            return nullptr;
      return cr;
      }

static qreal partialSlurBoundaryY(const ChordRest* cr, int staffIdx, bool up, qreal sp, qreal fallback)
      {
      if (!cr || !cr->isChord() || cr->vStaffIdx() != staffIdx)
            return fallback;
      const Chord* chord = toChord(cr);
      Shape shape = chord->shape();
      for (const Note* note : chord->notes()) {
            const Accidental* accidental = note->accidental();
            if (accidental && accidental->addToSkyline()) {
                  const QRectF bounds = accidental->bbox().translated(accidental->pos() + note->pos());
                  auto i = std::find(shape.begin(), shape.end(), bounds);
                  if (i != shape.end())
                        shape.erase(i);
                  }
            }
      // 3.x also stores horizontal-only spacing rectangles in this shape.
      // Lyrics/extra leading space must not become a vertical anchor.
      shape.erase(std::remove_if(shape.begin(), shape.end(), [](const QRectF& r) {
            return qFuzzyIsNull(r.height());
            }), shape.end());
      if (shape.empty())
            return fallback;
      qreal y = up ? shape.top() : shape.bottom();
      if (!chord->stem() || chord->up() != up)
            y += (up ? -0.4 : 0.4) * sp;
      return y;
      }

void Slur::slurPos(SlurPos* sp)
      {
      qreal _spatium = spatium();

      if (endCR() == 0) {
            sp->p1 = startCR()->pagePos();
            sp->p1.rx() += startCR()->width();
            sp->p2 = sp->p1;
            sp->p2.rx() += 5 * _spatium;
            sp->system1 = startCR()->measure()->system();
            sp->system2 = sp->system1;
            return;
            }

      bool useTablature  = staff() && staff()->isTabStaff(endCR()->tick());
      bool staffHasStems = true;     // assume staff uses stems
      const StaffType* stt = 0;
      if (useTablature) {
            stt           = staff()->staffType(tick());
            staffHasStems = stt->stemThrough();   // if tab with stems beside, stems do not count for slur pos
            }

      // start and end cr, chord, and note
      ChordRest* scr = startCR();
      ChordRest* ecr = endCR();
      Chord* sc      = 0;
      Note* note1    = 0;
      if (scr->isChord()) {
            sc    = toChord(scr);
            note1 = _up ? sc->upNote() : sc->downNote();
            }
      Chord* ec = 0;
      Note* note2 = 0;
      if (ecr->isChord()) {
            ec   = toChord(ecr);
            note2 = _up ? ec->upNote() : ec->downNote();
            }

      sp->system1 = scr->measure()->system();
      sp->system2 = ecr->measure()->system();

      if (sp->system1 == 0) {
            qDebug("no system1");
            return;
            }

      sp->p1 = scr->pos() + scr->segment()->pos() + scr->measure()->pos();
      sp->p2 = ecr->pos() + ecr->segment()->pos() + ecr->measure()->pos();

      if (scr->isGrace())
            sp->p1 += scr->parent()->pos();
      if (ecr->isGrace())
            sp->p2 += ecr->parent()->pos();

      // Notation HOPO curves use the normal centred slur anchors, as in
      // Studio. Fret-side clearance is specific to the TAB representation.
      if (isHammerOnPullOff() && note1 && note2 && useTablature) {
            auto enclosed = [](const Note* note) {
                  QRectF bounds = note->bbox();
                  for (const Element* child : note->el()) {
                        if (!child->isSymbol() || !child->visible())
                              continue;
                        const SymId id = toSymbol(child)->sym();
                        if (id == SymId::noteheadParenthesisLeft || id == SymId::noteheadParenthesisRight)
                              bounds |= child->bbox().translated(child->pos());
                        }
                  const auto& notes = note->chord()->notes();
                  const int index = int(std::distance(notes.begin(), std::find(notes.begin(), notes.end(), note)));
                  for (const Note* owner : notes) {
                        if (owner == note)
                              continue;
                        for (const Element* child : owner->el()) {
                              if (child->isSymbol() && child->visible() && toSymbol(child)->parenthesisNotes().contains(index))
                                    bounds |= child->bbox().translated(child->pos() + owner->pos() - note->pos());
                              }
                        }
                  if (note->accidental() && note->accidental()->visible()) {
                        const QRectF accidental = note->accidental()->bbox().translated(note->accidental()->pos());
                        if (accidental.bottom() >= bounds.top() && accidental.top() <= bounds.bottom()) {
                              bounds.setLeft(qMin(bounds.left(), accidental.left()));
                              bounds.setRight(qMax(bounds.right(), accidental.right()));
                              }
                        }
                  return bounds;
                  };
            const QRectF first = enclosed(note1);
            const QRectF last = enclosed(note2);
            const qreal gap = 0.2 * _spatium;
            // Slur segments already carry the staff's page Y. Their path
            // points must be relative to that staff, not to the system top.
            const QPointF origin1(sp->system1->pagePos().x(), sp->system1->staffYpage(vStaffIdx()));
            const QPointF origin2(sp->system2->pagePos().x(), sp->system2->staffYpage(vStaffIdx()));
            sp->p1 = note1->pagePos() - origin1
                  + QPointF(first.right() + gap, _up ? first.top() - gap : first.bottom() + gap);
            sp->p2 = note2->pagePos() - origin2
                  + QPointF(last.left() - gap, _up ? last.top() - gap : last.bottom() + gap);
            if (sp->system1 == sp->system2 && sp->p2.x() < sp->p1.x() + 0.5 * _spatium)
                  sp->p2.setX(sp->p1.x() + 0.5 * _spatium);
            return;
            }

      // adjust for cross-staff
      if (scr->vStaffIdx() != vStaffIdx() && sp->system1) {
            qreal diff = sp->system1->staff(scr->vStaffIdx())->y() - sp->system1->staff(vStaffIdx())->y();
            sp->p1.ry() += diff;
            }
      if (ecr->vStaffIdx() != vStaffIdx() && sp->system2) {
            qreal diff = sp->system2->staff(ecr->vStaffIdx())->y() - sp->system2->staff(vStaffIdx())->y();
            sp->p2.ry() += diff;
            }

      // account for centering or other adjustments (other than mirroring)
      if (note1 && !note1->mirror())
            sp->p1.rx() += note1->x();
      if (note2 && !note2->mirror())
            sp->p2.rx() += note2->x();

      qreal xo, yo;

      Stem* stem1 = sc && staffHasStems ? sc->stem() : 0;
      Stem* stem2 = ec && staffHasStems ? ec->stem() : 0;

      enum class SlurAnchor : char {
            NONE, STEM
            };
      SlurAnchor sa1 = SlurAnchor::NONE;
      SlurAnchor sa2 = SlurAnchor::NONE;
      // if slur is 'embedded' between either stem or both (as it might happen with voices)
      // link corresponding slur end to stem position
      if ((scr->up() == ecr->up()) && !scr->beam() && !ecr->beam() && (_up == scr->up())) {
            // both chords are facing same direction and slur is also in same direction
            // and no beams
            if (stem1)
                  sa1 = SlurAnchor::STEM;
            if (stem2)
                  sa2 = SlurAnchor::STEM;
            }
      // also link start of slur to stem if start chord & slur are in same direction and there is a hook
      if (scr->up() == _up && stem1 && sc->hook()) {
            sa1 = SlurAnchor::STEM;
            // if end chord is in same direction, link end of slur to stem too
            if (ecr->up() == scr->up() && stem2 && (!ecr->beam() || !ecr->beam()->cross()))
                  sa2 = SlurAnchor::STEM;
            }

      qreal __up = _up ? -1.0 : 1.0;
      static const bool previewGraceTips = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1;
      const bool sameSystem = sp->system1 == sp->system2;
      const bool standardStemTips = previewGraceTips && sc && ec
            && partialDirection() < 0 && !isHammerOnPullOff()
            && lineType() == 0 && staff() && staff()->isPitchedStaff(tick())
            && sc->vStaffIdx() == vStaffIdx() && ec->vStaffIdx() == vStaffIdx()
            && !sc->isGrace() && !ec->isGrace() && !sc->isSmall() && !ec->isSmall()
            && !sc->beam() && !ec->beam()
            && (sameSystem || (!sc->hook() && !ec->hook() && !sc->tremolo() && !ec->tremolo()))
            && (!sc->tremolo() || sc->tremolo()->twoNotes())
            && (!ec->tremolo() || ec->tremolo()->twoNotes());
      const bool plainSlurTips = standardStemTips && sameSystem && !sc->hook() && !ec->hook()
            && !sc->tremolo() && !ec->tremolo();
      const bool tremoloStart = standardStemTips && stem1 && sc->up() == _up && sc->tremolo()
            && sc->tremolo()->chord1() == sc;
      const bool tremoloEnd = standardStemTips && stem2 && ec->up() == _up && ec->tremolo()
            && ec->tremolo()->chord2() == ec;
      if (tremoloStart)
            sa1 = SlurAnchor::STEM;
      if (tremoloEnd)
            sa2 = SlurAnchor::STEM;
      const auto legacyArticulationY = [&](qreal y, Chord* chord, bool stemSide) {
            return plainSlurTips ? y : fixArticulations(y, chord, __up, stemSide);
            };
      const qreal stemSideInset = standardStemTips ? 0.5 : 0.25;
      const bool localGrace = scr->segment() == ecr->segment();
      const bool scaleGraceTips = previewGraceTips && sc && ec && (sc->isGrace() || ec->isGrace())
            && sp->system1 == sp->system2
            && (localGrace || (sc->isGrace() && !sc->isGraceAfter()) || (ec->isGrace() && !ec->isGraceAfter()))
            && scr->track() == track() && ecr->track() == track()
            && sc->vStaffIdx() == vStaffIdx() && ec->vStaffIdx() == vStaffIdx()
            && staff() && staff()->isPitchedStaff(tick()) && staff()->lines(tick()) >= 2
            && partialDirection() < 0 && !isHammerOnPullOff() && lineType() == 0;
      // Scale the notehead clearance before checking staff lines. Stem and
      // beam anchor rules replace this default where applicable. Like a local
      // grace slur, a slur to or from a pre-grace at a different beat uses
      // the smaller clearance at its grace end.
      const auto headClearance = [&](const ChordRest* cr) {
            const qreal mag = scaleGraceTips && cr->isGrace() && (localGrace || !cr->isGraceAfter())
                  ? score()->styleD(Sid::graceNoteMag) : 1.0;
            return _spatium * .9 * __up * mag;
            };
      qreal hw1 = note1 ? note1->tabHeadWidth(stt) : scr->width();      // if stt == 0, tabHeadWidth()
      qreal hw2 = note2 ? note2->tabHeadWidth(stt) : ecr->width();      // defaults to headWidth()
      QPointF pt;
      switch (sa1) {
            case SlurAnchor::STEM:        //sc can't be null
                  {
                  if (tremoloStart) {
                        // Like a beam-side tip, start outside the tremolo.
                        pt = stem1->pos() + stem1->p2();
                        pt += QPointF(0.15 * _spatium + 0.5 * stem1->lineWidthMag() * __up,
                                      0.35 * _spatium * __up);
                        pt.ry() = legacyArticulationY(pt.y(), sc, true);
                        pt.rx() -= note1 && !note1->mirror() ? note1->x() : 0.0;
                        sp->p1 += pt;
                        break;
                        }
                  // place slur starting point at stem end point
                  pt = sc->stemPos() - sc->pagePos() + sc->stem()->p2();
                  if (useTablature)                   // in tabs, stems are centred on note:
                        pt.rx() = hw1 * 0.5 + (note1 ? note1->bboxXShift() : 0.0);          // skip half notehead to touch stem, anatoly-os: incorrect. half notehead width is not always the stem position
                  // clear the stem (x)
                  // allow slight overlap (y) as per Gould
                  // don't allow overlap with hook if not disabling the autoplace checks against start/end segments in SlurSegment::layoutSegment()
                  const Hook* hook = sc->hook();
                  const bool flagGrace = scaleGraceTips && sc->isGraceBefore() && !sc->beam();
                  const bool clearFlag = (flagGrace || standardStemTips) && hook && hook->visible() && !hook->bbox().isEmpty();
                  const qreal graceMag = clearFlag && flagGrace ? score()->styleD(Sid::graceNoteMag) : 1.0;
                  qreal yadj = isIncoming() || isOutgoing() ? -0.5 : -stemSideInset;
                  if (clearFlag && flagGrace)
                        yadj = -0.5 * graceMag;
                  yadj *= _spatium * __up;
                  pt += QPointF(qMax(0.20, 0.35 * graceMag) * _spatium, yadj);
                  // account for articulations
                  pt.ry() = legacyArticulationY(pt.y(), sc, true);
                  if (clearFlag) {
                        const QRectF bounds = hook->bbox().translated(hook->pos());
                        if (bounds.contains(pt)) {
                              // As in Studio, approximate the open side of
                              // the curved flag; its bbox encloses empty ink.
                              const QString family = score()->scoreFont()->family();
                              const qreal slope = family == "Gonville" || family == "MuseJazz" ? 1.5 : 1.0;
                              const qreal cutout = qMax(0.0, bounds.width() / slope - qAbs(yadj));
                              pt.setX(qMax(pt.x(), sc->stemPosX() + cutout));
                              }
                        // pt.x() is already relative to the chord, not the
                        // notehead, so cancel the generic notehead offset.
                        pt.rx() += sc->pageX() - sp->system1->pagePos().x() - sp->p1.x();
                        }
                  sp->p1 += pt;
                  }
                  break;
            case SlurAnchor::NONE:
                  break;
            }
      switch (sa2) {
            case SlurAnchor::STEM:        //ec can't be null
                  {
                  if (tremoloEnd) {
                        pt = stem2->pos() + stem2->p2();
                        pt += QPointF(-0.15 * _spatium + 0.5 * stem2->lineWidthMag() * __up,
                                      0.35 * _spatium * __up);
                        pt.ry() = legacyArticulationY(pt.y(), ec, true);
                        pt.rx() -= note2 && !note2->mirror() ? note2->x() : 0.0;
                        sp->p2 += pt;
                        break;
                        }
                  pt = ec->stemPos() - ec->pagePos() + ec->stem()->p2();
                  if (useTablature)
                        pt.rx() = hw2 * 0.5;
                  // don't allow overlap with beam
                  qreal yadj = ec->beam() ? 0.75 : (isIncoming() || isOutgoing() ? -0.5 : -stemSideInset);
                  yadj *= _spatium * __up;
                  pt += QPointF(-0.35 * _spatium, yadj);
                  // account for articulations
                  pt.ry() = legacyArticulationY(pt.y(), ec, true);
                  sp->p2 += pt;
                  }
                  break;
            case SlurAnchor::NONE:
                  break;
            }

      //
      // default position:
      //    horizontal: middle of notehead
      //    vertical:   clear the notehead
      //
      //------p1
      // Compute x0, y0 and stemPos
      if (sa1 == SlurAnchor::NONE || sa2 == SlurAnchor::NONE) { // need stemPos if sa2 == SlurAnchor::NONE
            bool stemPos = false;   // p1 starts at chord stem side

            // default positions
            xo = hw1 * .5 + (note1 ? note1->bboxXShift() : 0.0);
            if (note1)
                  yo = note1->pos().y();
            else if (_up)
                  yo = scr->bbox().top();
            else
                  yo = scr->bbox().top() + scr->height();
            yo += headClearance(scr);

            // adjustments for stem and/or beam

            if (stem1) { //sc not null
                  Beam* beam1 = sc->beam();
                  if (beam1 && beam1->cross()) {
                        // TODO: stem direction is not finalized, so we cannot use it here
                        yo = legacyArticulationY(yo, sc, false);
                        }
                  else if (beam1 && (beam1->elements().back() != sc) && (sc->up() == _up)) {
                        // start chord is beamed but not the last chord of beam group
                        // and slur direction is same as start chord (stem side)

                        // in these cases, layout start of slur to stem

                        qreal sh = stem1->height() + _spatium;
                        if (beam1 && qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1)
                              sh = stem1->height() + score()->styleP(Sid::beamWidth) * beam1->mag() * 0.5
                                    + qMax(0.20, 0.35 * (sc->isGrace() ? score()->styleD(Sid::graceNoteMag) : 1.0)) * _spatium;
                        if (_up)
                              yo = sc->downNote()->pos().y() - sh;
                        else
                              yo = sc->upNote()->pos().y() + sh;
                        // Keep the full horizontal span over pre-grace
                        // beams, using the drawn stem center as for an
                        // ordinary beamed note. Sample beam Y at that X.
                        xo       = stem1->pos().x();
                        if (scaleGraceTips && sc->isGraceBefore() && nativeGraceSlurBeam(sc)) {
                              yo = nativeGraceBeamTipY(sc, _up, xo, yo);
                              xo += sc->pageX() - sp->system1->pagePos().x() - sp->p1.x();
                              }
                        else if (previewGraceTips)
                              yo = nativeGraceBeamTipY(sc, _up, xo + (note1 && !note1->mirror() ? note1->x() : 0.0), yo);

                        // account for articulations
                        yo = legacyArticulationY(yo, sc, true);

                        // force end of slur to layout to stem as well,
                        // if start and end chords have same stem direction
                        stemPos = true;
                        }
                  else {
                        // start chord is not beamed or is last chord of beam group
                        // or slur direction is opposite that of start chord

                        // at this point slur is in default position relative to note on slur side
                        // but we may need to make further adjustments

                        // if stem and slur are both up
                        // we need to clear stem horizontally
                        if (sc->up() && _up)
                              xo = hw1 + _spatium * .3;

                        //
                        // handle case: stem up   - stem down
                        //              stem down - stem up
                        //
                        if ((sc->up() != ecr->up()) && (sc->up() == _up)) {
                              // start and end chord have opposite direction
                              // and slur direction is same as start chord
                              // (so slur starts on stem side)

                              // float the start point along the stem to follow direction of movement
                              // see for example Gould p. 111

                              // get position of note on slur side for start & end chords
                              Note* n1  = sc->up() ? sc->upNote() : sc->downNote();
                              Note* n2  = 0;
                              if (ec)
                                    n2 = ec->up() ? ec->upNote() : ec->downNote();

                              // differential in note positions
                              qreal yd  = (n2 ? n2->pos().y() : ecr->pos().y()) - n1->pos().y();
                              yd *= .5;

                              // float along stem according to differential
                              qreal sh = stem1->height();
                              if (_up && yd < 0.0)
                                    yo = qMax(yo + yd, sc->downNote()->pos().y() - sh - _spatium);
                              else if (!_up && yd > 0.0)
                                    yo = qMin(yo + yd, sc->upNote()->pos().y() + sh + _spatium);

                              // account for articulations
                              yo = legacyArticulationY(yo, sc, true);

                              // we may wish to force end to align to stem as well,
                              // if it is in same direction
                              // (but it won't be, so this assignment should have no effect)
                              stemPos = true;
                              }
                        else {
                              // avoid articulations
                              yo = legacyArticulationY(yo, sc, sc->up() == _up);
                              }
                        }
                  }
            else if (sc) {
                  // avoid articulations
                  yo = legacyArticulationY(yo, sc, sc->up() == _up);
                  }

            if (sa1 == SlurAnchor::NONE)
                  sp->p1 += QPointF(xo, yo);

            //------p2
            if (sa2 == SlurAnchor::NONE) {

                  // default positions
                  xo = hw2 * .5 + (note2 ? note2->bboxXShift() : 0.0);
                  if (note2)
                        yo = note2->pos().y();
                  else if (_up)
                        yo = endCR()->bbox().top();
                  else
                        yo = endCR()->bbox().top() + endCR()->height();
                  yo += headClearance(ecr);

                  // adjustments for stem and/or beam

                  if (stem2) { //ec can't be null
                        Beam* beam2 = ec->beam();
                        if (beam2 && beam2->cross()) {
                              // TODO: stem direction is not finalized, so we cannot use it here
                              yo = legacyArticulationY(yo, ec, false);
                              }
                        else if ((stemPos && (scr->up() == ec->up()))
                           || (beam2
                             && (!beam2->elements().empty())
                             && (beam2->elements().front() != ec)
                             && (ec->up() == _up)
                             && sc && (sc->noteType() == NoteType::NORMAL)
                             )
                              ) {

                              // slur start was laid out to stem and start and end have same direction
                              // OR
                              // end chord is beamed but not the first chord of beam group
                              // and slur direction is same as end chord (stem side)
                              // and start chordrest is not a grace chord

                              // in these cases, layout end of slur to stem

                              qreal sh = stem2->height() + _spatium;
                              if (beam2 && qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1)
                                    sh = stem2->height() + score()->styleP(Sid::beamWidth) * beam2->mag() * 0.5
                                          + qMax(0.20, 0.35 * (ec->isGrace() ? score()->styleD(Sid::graceNoteMag) : 1.0)) * _spatium;
                              if (_up)
                                    yo = ec->downNote()->pos().y() - sh;
                              else
                                    yo = ec->upNote()->pos().y() + sh;
                              // Backport note: retain the 37E stem X for beamed slurs.
                              // Studio 4.7.5 uses the same signed half stem width as
                              // at the start but subtracts 0.15sp * chordMag.
                              xo = stem2->pos().x();
                              if (scaleGraceTips && ec->isGraceBefore() && nativeGraceSlurBeam(ec)) {
                                    yo = nativeGraceBeamTipY(ec, _up, xo, yo);
                                    xo += ec->pageX() - sp->system2->pagePos().x() - sp->p2.x();
                                    }
                              else if (previewGraceTips)
                                    yo = nativeGraceBeamTipY(ec, _up, xo + (note2 && !note2->mirror() ? note2->x() : 0.0), yo);

                              // account for articulations
                              yo = legacyArticulationY(yo, ec, true);
                              }
                        else
                              {
                              // slur was not aligned to stem or start and end have different direction
                              // AND
                              // end chord is not beamed or is first chord of beam group
                              // or slur direction is opposite that of end chord

                              // if stem and slur are both down,
                              // we need to clear stem horizontally
                              if (!ec->up() && !_up)
                                    xo = -_spatium * .3 + note2->x();

                              //
                              // handle case: stem up   - stem down
                              //              stem down - stem up
                              //
                              if ((scr->up() != ec->up()) && (ec->up() == _up)) {
                                    // start and end chord have opposite direction
                                    // and slur direction is same as end chord
                                    // (so slur end on stem side)

                                    // float the end point along the stem to follow direction of movement
                                    // see for example Gould p. 111

                                    Note* n1 = 0;
                                    if (sc)
                                          n1 = sc->up() ? sc->upNote() : sc->downNote();
                                    Note* n2 = ec->up() ? ec->upNote() : ec->downNote();

                                    qreal yd = n2->pos().y() - (n1 ? n1->pos().y() : startCR()->pos().y());
                                    yd *= .5;

                                    qreal mh = stem2->height();
                                    if (_up && yd > 0.0)
                                          yo = qMax(yo - yd, ec->downNote()->pos().y() - mh - _spatium);
                                    else if (!_up && yd < 0.0)
                                          yo = qMin(yo - yd, ec->upNote()->pos().y() + mh + _spatium);

                                    // account for articulations
                                    yo = legacyArticulationY(yo, ec, true);
                                    }
                              else {
                                    // avoid articulations
                                    yo = legacyArticulationY(yo, ec, ec->up() == _up);
                                    }

                              }
                        }
                  else if (ec) {
                        // avoid articulations
                        yo = legacyArticulationY(yo, ec, ec->up() == _up);
                        }

                  sp->p2 += QPointF(xo, yo);
                  }
            }
      if (plainSlurTips) {
            placeArticulatedSlurTip(sp->p1, sc, _up, true, _spatium);
            placeArticulatedSlurTip(sp->p2, ec, _up, false, _spatium);
            }
      bool constrainLeftAnchor = false;
      if (isIncoming() && sp->system1) {
            const Measure* measure = startCR()->measure();
            const ChordRest* firstCR = partialSlurBoundaryCR(measure, track(), true);
            sp->p1.setX(measure->firstNoteRestSegmentX(true));
            sp->p1.setY(partialSlurBoundaryY(firstCR, vStaffIdx(), _up, _spatium, sp->p1.y()));
            constrainLeftAnchor = firstCR == endCR();
            }
      if (isOutgoing() && sp->system2) {
            const Measure* measure = endCR()->measure();
            const ChordRest* lastCR = partialSlurBoundaryCR(measure, track(), false);
            sp->p2.setX(measure->endingXForOpenEndedLines());
            if (lastCR == startCR())
                  sp->p2.setY(sp->p1.y() + __up * 0.25 * _spatium);
            else {
                  qreal y = partialSlurBoundaryY(lastCR, vStaffIdx(), _up, _spatium, sp->p1.y());
                  if (__up * (sp->p1.y() - y) > 2.5 * _spatium)
                        y = sp->p1.y() - __up * 2.5 * _spatium;
                  sp->p2.setY(y);
                  }
            }
      if (constrainLeftAnchor)
            sp->p1.setY(sp->p2.y() + __up * 0.25 * _spatium);
      }

//---------------------------------------------------------
//   Slur
//---------------------------------------------------------

Slur::Slur(Score* s)
   : SlurTie(s)
      {
      setAnchor(Anchor::CHORD);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Slur::write(XmlWriter& xml) const
      {
      if (broken()) {
            qDebug("broken slur not written");
            return;
            }
      if (!xml.canWrite(this))
            return;
      xml.stag(this);
      if (_partialDirection >= 0) {
            static const char* names[] = { "incoming", "outgoing", "both" };
            xml.tag("partialSpannerDirection", names[_partialDirection]);
            }
      SlurTie::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool Slur::readProperties(XmlReader& e)
      {
      if (e.name() == "partialSpannerDirection") {
            const QString direction = e.readElementText();
            _partialDirection = direction == "incoming" ? 0 : direction == "outgoing" ? 1 : direction == "both" ? 2 : -1;
            return true;
            }
      return SlurTie::readProperties(e);
      }

void Slur::reset()
      {
      SlurTie::reset();
      undoResetProperty(Pid::PARTIAL_SPANNER_DIRECTION);
      }

QVariant Slur::getProperty(Pid id) const
      {
      return id == Pid::PARTIAL_SPANNER_DIRECTION ? QVariant(_partialDirection) : SlurTie::getProperty(id);
      }

QVariant Slur::propertyDefault(Pid id) const
      {
      return id == Pid::PARTIAL_SPANNER_DIRECTION ? QVariant(-1) : SlurTie::propertyDefault(id);
      }

bool Slur::setProperty(Pid id, const QVariant& value)
      {
      if (id != Pid::PARTIAL_SPANNER_DIRECTION)
            return SlurTie::setProperty(id, value);
      _partialDirection = qBound(-1, value.toInt(), 2);
      triggerLayout();
      return true;
      }

//---------------------------------------------------------
//   chordsHaveTie
//---------------------------------------------------------

static bool chordsHaveTie(Chord* c1, Chord* c2)
      {
      size_t n = c1->notes().size();
      for (size_t i1 = 0; i1 < n; ++i1) {
            Note* n1 = c1->notes().at(i1);
            size_t n2 = c2->notes().size();
            for (size_t i2 = 0; i2 < n2; ++i2) {
                  Note* n3 = c2->notes().at(i2);
                  if (n1->tieFor() && n1->tieFor() == n3->tieBack())
                        return true;
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   directionMixture
//---------------------------------------------------------

static bool isDirectionMixture(Chord* c1, Chord* c2)
      {
      static const bool preview = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1;
      const bool acrossBars = preview && c1->track() == c2->track() && c1->staff()
            && c1->staff()->isPitchedStaff(c1->tick()) && !c1->isGrace() && !c2->isGrace()
            && !c1->staffMove() && !c2->staffMove();
      const bool up = c1->up();
      for (Segment* seg = c1->segment(); seg && (!acrossBars || seg->tick() <= c2->tick());
            seg = acrossBars ? seg->next1(SegmentType::ChordRest) : seg->next(SegmentType::ChordRest)) {
            Element* e = seg->element(c1->track());
            if (!e || !e->isChord())
                  continue;
            Chord* c = toChord(e);
            if (acrossBars && c->measure()->system() != c1->measure()->system()
                && !c->beam() && !c->tremolo() && !c->staffMove()) {
                  // The next system may not have resolved this plain chord's
                  // stem yet. Beams and tremolos have their own direction pass.
                  for (Note* note : c->notes())
                        note->updateLine();
                  c->computeUp();
                  }
            if (c->up() != up)
                  return true;
            if (seg == c2->segment())
                  break;
            }
      return false;
      }

// Studio gives the voice assignment priority over mixed stem directions
// across the whole slur, including the duration of the ending chord.
bool Slur::applyNativeMultiVoiceDirection()
      {
      static const bool previewEnabled = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1;
      if (!previewEnabled || partialDirection() >= 0 || isHammerOnPullOff())
            return false;
      const ChordRest* start = startCR();
      const ChordRest* end = endCR();
      if (!start || !end || !start->isChord() || start->isGrace() || end->isGrace()
          || start->staffIdx() != end->staffIdx() || start->staffMove() || end->staffMove()
          || !start->staff() || !start->staff()->isPitchedStaff(start->tick())
          || (start->beam() && start->beam()->cross()))
            return false;

      const Fraction span = ticks() + end->ticks();
      for (const Measure* m = start->measure(); m && m->tick() <= end->tick(); m = m->nextMeasure()) {
            if (m->hasVoices(start->staffIdx(), tick(), span)) {
                  _up = start->voice() == 0 && end->voice() == 0;
                  return true;
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   layoutSystem
//    layout slurSegment for system
//---------------------------------------------------------

SpannerSegment* Slur::layoutSystem(System* system)
      {
      Fraction stick = system->firstMeasure()->tick();
      Fraction etick = system->lastMeasure()->endTick();

      SlurSegment* slurSegment = toSlurSegment(getNextLayoutSystemSegment(system, [this]() { return new SlurSegment(score()); }));

      SpannerSegmentType sst;
      if (tick() >= stick) {
            //
            // this is the first call to layoutSystem,
            // processing the first line segment
            //
            if (track2() == -1)
                  setTrack2(track());
            if (startCR() == 0 || startCR()->measure() == 0) {
                  qDebug("Slur::layout(): track %d-%d  %p - %p tick %d-%d null start anchor",
                     track(), track2(), startCR(), endCR(), tick().ticks(), tick2().ticks());
                  return slurSegment;
                  }
            if (endCR() == 0) {     // sanity check
                  setEndElement(startCR());
                  setTick2(tick());
                  }
            switch (_slurDirection) {
                  case Direction::UP:
                        _up = true;
                        break;
                  case Direction::DOWN:
                        _up = false;
                        break;
                  case Direction::AUTO:
                        {
                        //
                        // assumption:
                        // slurs have only chords or rests as start/end elements
                        //
                        if (startCR() == 0 || endCR() == 0) {
                              _up = true;
                              break;
                              }
                        Chord* c1 = startCR()->isChord() ? toChord(startCR()) : 0;
                        Chord* c2 = endCR()->isChord()   ? toChord(endCR())   : 0;

                        if (c1 && c1->beam() && c1->beam()->cross()) {
                              // TODO: stem direction is not finalized, so we cannot use it here
                              _up = true;
                              break;
                              }

                        if (applyNativeMultiVoiceDirection())
                              break;

                        _up = !(startCR()->up());

                        Measure* m1 = startCR()->measure();
#if 0
                        // the following code was in place until 3.6,
                        // to force "long" slurs (duration > one measure) above
                        // but it's much too aggressive - one measure isn't necessarily long
                        if ((endCR()->tick() - startCR()->tick()) > m1->ticks()) // long slurs are always above
                              _up = true;
#endif
                        if (c1 && c2 && isDirectionMixture(c1, c2) && !c1->isGrace()) {
                              // slurs go above if start and end note have different stem directions,
                              // but grace notes are exceptions
                              _up = true;
                              }
                        else if (m1->hasVoices(startCR()->staffIdx(), tick(), ticks()) && c1 && !c1->isGrace()) {
                              // in polyphonic passage, slurs go on the stem side
                              _up = startCR()->up();
                              }
                        else if (c1 && c2 && chordsHaveTie(c1, c2)) {
                              // could confuse slur with tie, put slur on stem side
                              _up = startCR()->up();
                              }
                        }
                        break;
                  }
            sst = tick2() < etick ? SpannerSegmentType::SINGLE : SpannerSegmentType::BEGIN;
            }
      else if (tick() < stick && tick2() >= etick)
            sst = SpannerSegmentType::MIDDLE;
      else
            sst = SpannerSegmentType::END;
      slurSegment->setSpannerSegmentType(sst);

      if (isHammerOnPullOff() && _slurDirection == Direction::AUTO && staff() && staff()->isTabStaff(tick()))
            _up = true;
      SlurPos sPos;
      slurPos(&sPos);

      switch (sst) {
            case SpannerSegmentType::SINGLE:
                  slurSegment->layoutSegment(sPos.p1, sPos.p2);
                  break;
            case SpannerSegmentType::BEGIN:
                  slurSegment->layoutSegment(sPos.p1, QPointF(system->lastNoteRestSegmentX(true), sPos.p1.y()));
                  break;
            case SpannerSegmentType::MIDDLE: {
                  qreal x1 = system->firstNoteRestSegmentX(true);
                  qreal x2 = system->lastNoteRestSegmentX(true);
                  qreal y  = staffIdx() > system->staves()->size() ? system->y() : system->staff(staffIdx())->y();
                  slurSegment->layoutSegment(QPointF(x1, y), QPointF(x2, y));
                  }
                  break;
            case SpannerSegmentType::END:
                  slurSegment->layoutSegment(QPointF(system->firstNoteRestSegmentX(true), sPos.p2.y()), sPos.p2);
                  break;
            }

      return slurSegment;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Slur::layout()
      {
      if (track2() == -1)
            setTrack2(track());

      qreal _spatium = spatium();

      if (score() == gscore || tick() == Fraction(-1,1)) {
            //
            // when used in a palette, slur has no parent and
            // tick and tick2 has no meaning so no layout is
            // possible and needed
            //
            SlurSegment* s;
            if (spannerSegments().empty()) {
                  s = new SlurSegment(score());
                  s->setTrack(track());
                  add(s);
                  }
            else {
                  s = frontSegment();
                  }
            s->setSpannerSegmentType(SpannerSegmentType::SINGLE);
            s->layoutSegment(QPointF(0, 0), QPointF(_spatium * 6, 0));
            setbbox(frontSegment()->bbox());
            return;
            }

      if (startCR() == 0 || startCR()->measure() == 0) {
            qDebug("track %d-%d  %p - %p tick %d-%d null start anchor",
               track(), track2(), startCR(), endCR(), tick().ticks(), tick2().ticks());
            return;
            }
      if (endCR() == 0) {     // sanity check
            qDebug("no end CR for %d", (tick()+ticks()).ticks());
            setEndElement(startCR());
            setTick2(tick());
            }
      switch (_slurDirection) {
            case Direction::UP:
                  _up = true;
                  break;
            case Direction::DOWN:
                  _up = false;
                  break;
            case Direction::AUTO:
                  {
                  //
                  // assumption:
                  // slurs have only chords or rests as start/end elements
                  //
                  if (startCR() == 0 || endCR() == 0) {
                        _up = true;
                        break;
                        }
                  if (applyNativeMultiVoiceDirection())
                        break;

                  Measure* m1 = startCR()->measure();

                  Chord* c1 = startCR()->isChord() ? toChord(startCR()) : 0;
                  Chord* c2 = endCR()->isChord()   ? toChord(endCR())   : 0;

                  _up = !(startCR()->up());

                  if ((endCR()->tick() - startCR()->tick()) > m1->ticks()) {
                        // long slurs are always above
                        _up = true;
                        }
                  else
                        _up = !(startCR()->up());

                  if (c1 && c2 && isDirectionMixture(c1, c2) && (c1->noteType() == NoteType::NORMAL)) {
                        // slurs go above if start and end note have different stem directions,
                        // but grace notes are exceptions
                        _up = true;
                        }
                  else if (m1->hasVoices(startCR()->staffIdx(), tick(), ticks()) && c1 && c1->noteType() == NoteType::NORMAL) {
                        // in polyphonic passage, slurs go on the stem side
                        _up = startCR()->up();
                        }
                  else if (c1 && c2 && chordsHaveTie(c1, c2)) {
                        // could confuse slur with tie, put slur on stem side
                        _up = startCR()->up();
                        }
                  }
                  break;
            }

      if (isHammerOnPullOff() && _slurDirection == Direction::AUTO && staff() && staff()->isTabStaff(tick()))
            _up = true;
      SlurPos sPos;
      slurPos(&sPos);

      const QList<System*>& sl = score()->systems();
      ciSystem is = sl.begin();
      while (is != sl.end()) {
            if (*is == sPos.system1)
                  break;
            ++is;
            }
      if (is == sl.end())
            qDebug("Slur::layout  first system not found");
      setPos(0, 0);

      //---------------------------------------------------------
      //   count number of segments, if no change, all
      //    user offsets (drags) are retained
      //---------------------------------------------------------

      unsigned nsegs = 1;
      for (ciSystem iis = is; iis != sl.end(); ++iis) {
            if ((*iis)->vbox())
                  continue;
            if (*iis == sPos.system2)
                  break;
            ++nsegs;
            }

      fixupSegments(nsegs);

      for (int i = 0; is != sl.end(); ++i, ++is) {
            System* system  = *is;
            if (system->vbox()) {
                  --i;
                  continue;
                  }
            SlurSegment* segment = segmentAt(i);
            segment->setSystem(system);

            // case 1: one segment
            if (sPos.system1 == sPos.system2) {
                  segment->setSpannerSegmentType(SpannerSegmentType::SINGLE);
                  segment->layoutSegment(sPos.p1, sPos.p2);
                  }
            // case 2: start segment
            else if (i == 0) {
                  segment->setSpannerSegmentType(SpannerSegmentType::BEGIN);
                  qreal x = system->bbox().width();
                  segment->layoutSegment(sPos.p1, QPointF(x, sPos.p1.y()));
                  }
            // case 3: middle segment
            else if (i != 0 && system != sPos.system2) {
                  segment->setSpannerSegmentType(SpannerSegmentType::MIDDLE);
                  qreal x1 = system->firstNoteRestSegmentX(true);
                  qreal x2 = system->bbox().width();
                  qreal y  = staffIdx() > system->staves()->size() ? system->y() : system->staff(staffIdx())->y();
                  segment->layoutSegment(QPointF(x1, y), QPointF(x2, y));
                  }
            // case 4: end segment
            else {
                  segment->setSpannerSegmentType(SpannerSegmentType::END);
                  qreal x = system->firstNoteRestSegmentX(true);
                  segment->layoutSegment(QPointF(x, sPos.p2.y()), sPos.p2);
                  }
            if (system == sPos.system2)
                  break;
            }
      setbbox(spannerSegments().empty() ? QRectF() : frontSegment()->bbox());
      }

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void Slur::setTrack(int n)
      {
      Element::setTrack(n);
      for (SpannerSegment* ss : spannerSegments())
            ss->setTrack(n);
      }
}

