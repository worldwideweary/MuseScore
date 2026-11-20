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

static bool ordinarySlurObstacles(const SlurSegment* segment, std::vector<QRectF>& obstacles)
      {
      Slur* slur = segment->slur();
      const Staff* staff = segment->staff();
      const ChordRest* first = slur->startCR();
      const ChordRest* last = slur->endCR();
      if (!segment->system() || !staff || !first || !last || !first->isChord() || !last->isChord()
          || !first->measure() || first->measure() != last->measure()
          || first->track() != last->track() || first->track() != slur->track()
          || last->track() != slur->effectiveTrack2()
          || !staff->isPitchedStaff(first->tick()) || staff->lines(first->tick()) < 2
          || slur->partialDirection() >= 0 || slur->isHammerOnPullOff() || slur->lineType() != 0)
            return false;
      if (first->vStaffIdx() != segment->vStaffIdx() || last->vStaffIdx() != segment->vStaffIdx()
          || first->up() == slur->up() || last->up() == slur->up())
            return false;

      for (const Segment* s = first->segment(); s && s->measure() == first->measure(); s = s->next()) {
            if (!s->enabled() || s->isBreathType())
                  continue;
            if (!s->isChordRestType())
                  return false;
            const Element* e = s->element(first->track());
            if (!e || !e->isChord())
                  return false;
            for (const Element* other : s->elist()) {
                  if (other && other != e && other->vStaffIdx() == segment->vStaffIdx() && other->addToSkyline())
                        return false;
                  }
            const Chord* chord = toChord(e);
            if (!chord->addToSkyline() || chord->staffMove() || chord->isNudged() || chord->isSmall()
                || chord->isGrace() || chord->beam() || chord->tuplet() || chord->tremolo()
                || chord->arpeggio() || chord->stemSlash() || !chord->graceNotes().empty()
                || !chord->articulations().empty() || !chord->el().empty() || chord->notes().size() != 1)
                  return false;
            const Note* note = chord->notes().front();
            if (!note->addToSkyline() || note->isSmall() || note->mirror() || note->isNudged()
                || note->bbox().isEmpty() || note->accidental() || note->tieFor() || note->tieBack()
                || note->partialTie(true) || note->partialTie(false) || !note->dots().empty()
                || !note->el().empty() || !note->spannerFor().empty() || !note->spannerBack().empty())
                  return false;

            if (chord != first) {
                  const QPointF origin = s->measure()->pos() + s->pos() + chord->pos();
                  const auto add = [&](const Element* item, qreal clearance) {
                        if (!item || !item->addToSkyline() || item->bbox().isEmpty())
                              return;
                        const qreal gap = chord == last ? 0.0 : clearance * segment->spatium();
                        QRectF rect = item->bbox().translated(origin + item->pos());
                        rect.adjust(0.0, slur->up() ? -gap : 0.0, 0.0, slur->up() ? 0.0 : gap);
                        obstacles.push_back(rect);
                        };
                  add(note, 0.4);
                  add(chord->stem(), 0.1);
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

static void appendNestedSlurObstacles(const SlurSegment* outer, const QPointF& first, const QPointF& last,
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
            const CubicBezier curve(part->ups(Grip::START).pos(), part->ups(Grip::BEZIER1).pos(),
                                    part->ups(Grip::BEZIER2).pos(), part->ups(Grip::END).pos());
            // At a shared note, leave space for the two slurs to converge.
            const qreal left = first.x() + (inner->tick() == slur->tick() ? 0.9 * sp : 0.0);
            const qreal right = last.x() - (inner->tick2() == slur->tick2() ? 0.9 * sp : 0.0);
            for (int i = 0; i < 24; ++i) {
                  const QPointF a = curve.pointAtPercent(qreal(i) / 24) + part->pos();
                  const QPointF b = curve.pointAtPercent(qreal(i + 1) / 24) + part->pos();
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

static bool fitOrdinarySlur(const SlurSegment* segment, QPointF first, QPointF last,
                           std::array<QPointF, 4>& fit, std::array<QPointF, 4>& initial)
      {
      const qreal sp = segment->spatium();
      const qreal width = last.x() - first.x();
      const ChordRest* start = segment->slur()->startCR();
      const ChordRest* end = segment->slur()->endCR();
      const bool grace = start && end && (start->isGrace() || end->isGrace());
      const bool beamed = !grace && start && end && start->beam() && end->beam();
      const qreal maxSpan = beamed ? 48.0 : grace ? 40.0 : 10.0;
      if (!(sp > 0.0 && width >= (grace ? 0.4 : 2.0) * sp && width < maxSpan * sp)
          || !std::isfinite(first.y()) || !std::isfinite(last.y()))
            return false;
      std::vector<QRectF> obstacles;
      const bool eligible = grace ? nativeGraceSlurObstacles(segment, obstacles)
            : beamed ? nativeBeamedSlurObstacles(segment, obstacles) : ordinarySlurObstacles(segment, obstacles);
      if (!eligible)
            return false;
      if (beamed)
            appendNestedSlurObstacles(segment, first, last, obstacles);
      const bool up = segment->slur()->up();
      const qreal direction = up ? -1.0 : 1.0;
      const Fraction tick = segment->slur()->tick();
      const qreal lineDistance = segment->staff()->lineDistance(tick) * sp;
      if (!(lineDistance > 0.0))
            return false;
      const int lines = segment->staff()->lines(tick);
      const qreal margin = 0.175 + 0.5 * segment->score()->styleS(Sid::staffLineWidth).val() * sp / lineDistance;
      const auto snap = [&](QPointF point) {
            point.setY(clearSlurTip(point.y(), lineDistance, lines, margin, up));
            return point;
            };
      first = snap(first);
      last = snap(last);

      const qreal rise = last.y() - first.y();
      const qreal length = std::hypot(width, rise);
      // Keep the current limits for ordinary slurs. Grace tips may be close
      // horizontally while their anchor notes are several staff spaces apart.
      if (!(length < maxSpan * sp) || (!grace && qAbs(rise) > width))
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
            * (beamed && start->up() == up ? 0.75 : 1.0);
      const qreal inset = grace && length < 2.0 * sp ? 0.20 : length > 18.0 * sp ? 0.15 : length > 10.0 * sp ? 0.20 : 0.25;
      QPointF c1(inset * length, height);
      QPointF c2((1.0 - inset) * length, height);
      const auto clampShoulderX = [&](QPointF& control, qreal lastX) {
            QPointF point = toStaff(control);
            point.setX(qBound(first.x(), point.x(), lastX));
            control = toSlur(point);
            };
      if (grace) {
            clampShoulderX(c1, last.x());
            clampShoulderX(c2, last.x());
            }
      initial = {{ first, toStaff(c1), toStaff(c2), last }};
      const qreal step = direction * qMin(qreal(1.5), 0.30 * qMax(qreal(1.0), length / (16.0 * sp))) * sp;
      const qreal leftBalance = beamed ? 0.2 : grace && start->up() == up && start->up() != end->up() ? 0.9 : 0.4;
      const qreal rightBalance = beamed ? 0.2 : grace && end->up() == up && start->up() != end->up() ? 0.9 : 0.4;
      const qreal shallowSlope = 0.0872664626; // Studio's five-degree slant threshold
      const QPointF arcClearance(0.0, -direction * qBound(qreal(0.1),
            qreal(0.04 * length / sp * cosine * cosine), qreal(0.5)) * sp);

      for (int pass = 0; pass <= 30; ++pass) {
            bool collision[3] = { false, false, false };
            const CubicBezier clearance(QPointF(), c1 + arcClearance, c2 + arcClearance, p2);
            // Intersect in staff coordinates, where the obstacles lie. Studio
            // checks 19 intervals of a 20-subdivision curve.
            for (int i = 0; i < 19; ++i) {
                  const int section = i < 6 ? 0 : i < 12 ? 1 : 2;
                  if (collision[section])
                        continue;
                  const QRectF band = QRectF(toStaff(clearance.pointAtPercent(i / qreal(20))),
                                             toStaff(clearance.pointAtPercent((i + 1) / qreal(20)))).normalized();
                  for (const QRectF& item : obstacles) {
                        if (band.right() > item.left() && band.left() < item.right()
                            && (up ? band.bottom() >= item.top() : band.top() <= item.bottom())) {
                              collision[section] = true;
                              break;
                              }
                        }
                  }
            if (!collision[0] && !collision[1] && !collision[2]) {
                  // Snap only the tips so the cleared shoulders stay put.
                  fit = {{ snap(first), toStaff(c1), toStaff(c2), snap(toStaff(p2)) }};
                  return true;
                  }
            if (pass == 30)
                  return false;
            if (pass % 2 == 0) {
                  const qreal leftStep = (1.0 - leftBalance) * step;
                  const qreal rightStep = (1.0 - rightBalance) * step;
                  const QPointF left(-qAbs(leftStep) * 0.5, leftStep * 0.5);
                  const QPointF right(qAbs(rightStep) * 0.5, rightStep * 0.5);
                  if (collision[0]) {
                        c1 += left;
                        c2 += right * 0.5;
                        }
                  if (collision[1]) {
                        c1.ry() += (leftStep + rightStep) * 0.5;
                        c2.ry() += (leftStep + rightStep) * 0.5;
                        }
                  if (collision[2]) {
                        c1 += left * 0.5;
                        c2 += right;
                        }
                  }
            else {
                  const qreal slope = (toStaff(p2).y() - first.y()) / width;
                  const auto endpointStep = [&](bool left) {
                        const bool side = collision[left ? 0 : 2];
                        const bool steep = grace && (left ? rise < -width : rise > width);
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

void SlurSegment::drawOrdinaryFit(const QPointF& shoulderDelta)
      {
      ups(Grip::START).p = _ordinaryFit[0];
      ups(Grip::BEZIER1).p = _ordinaryFit[1];
      ups(Grip::BEZIER2).p = _ordinaryFit[2];
      ups(Grip::END).p = _ordinaryFit[3];
      ups(Grip::BEZIER1).off += shoulderDelta;
      ups(Grip::BEZIER2).off += shoulderDelta;
      const auto first = ups(Grip::START).pos();
      const auto c1 = ups(Grip::BEZIER1).pos();
      const auto c2 = ups(Grip::BEZIER2).pos();
      const auto last = ups(Grip::END).pos();
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

//---------------------------------------------------------
//   layoutSegment
//---------------------------------------------------------

Element* SlurSegment::propertyDelegate(Pid id)
      {
      return id == Pid::PARTIAL_SPANNER_DIRECTION ? static_cast<Element*>(slur()) : SlurTieSegment::propertyDelegate(id);
      }

void SlurSegment::layoutSegment(const QPointF& p1, const QPointF& p2)
      {
      _hasOrdinaryFit = false;
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
      // Partial slurs already receive repeat-aware contours and one-note
      // constraints from slurPosChord(). Preserve those calculated anchors.
      if (slur()->partialDirection() < 0) {
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
                  ups(Grip::END).p.ry() = up ? qMin(ups(Grip::END).p.y(), p1.y() - height) : qMax(ups(Grip::END).p.y(), p1.y() + height);
            if (startIsHanging)
                  ups(Grip::START).p.ry() = up ? qMin(ups(Grip::START).p.y(), p2.y() - height) : qMax(ups(Grip::START).p.y(), p2.y() + height);
            }
      _extraHeight = 0.0;

      //Adjust Y pos to staff type yOffset before other calculations
      if (staffType())
            rypos() += staffType()->yoffset().val() * spatium();

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

      static const bool previewEnabled = qEnvironmentVariableIntValue("MSCORE_NATIVE_SLUR_FIT") == 1;
      std::array<QPointF, 4> initialFit;
      if (previewEnabled && autoplace() && type == SpannerSegmentType::SINGLE)
            _hasOrdinaryFit = fitOrdinarySlur(this, p1, p2, _ordinaryFit, initialFit);

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
                  return QString::asprintf("kind=%s index=%d pitch=%d beam=%d rawXY/sp=%.3f,%.3f finalXY/sp=%.3f,%.3f",
                        chord->isGraceAfter() ? "after" : chord->isGrace() ? "before" : "main",
                        chord->isGrace() ? chord->graceIndex() : -1, note->pitch(), chord->beam() != nullptr,
                        before.x(), before.y(), final.x(), final.y());
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
      qreal hw1 = note1 ? note1->tabHeadWidth(stt) : scr->width();      // if stt == 0, tabHeadWidth()
      qreal hw2 = note2 ? note2->tabHeadWidth(stt) : ecr->width();      // defaults to headWidth()
      QPointF pt;
      switch (sa1) {
            case SlurAnchor::STEM:        //sc can't be null
                  {
                  // place slur starting point at stem end point
                  pt = sc->stemPos() - sc->pagePos() + sc->stem()->p2();
                  if (useTablature)                   // in tabs, stems are centred on note:
                        pt.rx() = hw1 * 0.5 + (note1 ? note1->bboxXShift() : 0.0);          // skip half notehead to touch stem, anatoly-os: incorrect. half notehead width is not always the stem position
                  // clear the stem (x)
                  // allow slight overlap (y) as per Gould
                  // don't allow overlap with hook if not disabling the autoplace checks against start/end segments in SlurSegment::layoutSegment()
                  qreal yadj = isIncoming() || isOutgoing() ? -0.5 : -0.25;
                  yadj *= _spatium * __up;
                  pt += QPointF(0.35 * _spatium, yadj);
                  // account for articulations
                  pt.ry() = fixArticulations(pt.y(), sc, __up, true);
                  sp->p1 += pt;
                  }
                  break;
            case SlurAnchor::NONE:
                  break;
            }
      switch (sa2) {
            case SlurAnchor::STEM:        //ec can't be null
                  {
                  pt = ec->stemPos() - ec->pagePos() + ec->stem()->p2();
                  if (useTablature)
                        pt.rx() = hw2 * 0.5;
                  // don't allow overlap with beam
                  qreal yadj = ec->beam() ? 0.75 : (isIncoming() || isOutgoing() ? -0.5 : -0.25);
                  yadj *= _spatium * __up;
                  pt += QPointF(-0.35 * _spatium, yadj);
                  // account for articulations
                  pt.ry() = fixArticulations(pt.y(), ec, __up, true);
                  sp->p2 += pt;
                  }
                  break;
            case SlurAnchor::NONE:
                  break;
            }

      //
      // default position:
      //    horizontal: middle of notehead
      //    vertical:   _spatium * .4 above/below notehead
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
            yo += _spatium * .9 * __up;

            // adjustments for stem and/or beam

            if (stem1) { //sc not null
                  Beam* beam1 = sc->beam();
                  if (beam1 && beam1->cross()) {
                        // TODO: stem direction is not finalized, so we cannot use it here
                        yo = fixArticulations(yo, sc, __up, false);
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
                        // Backport note: retain the 37E stem X for beamed slurs.
                        // Studio 4.7.5 instead uses stemPosX + 0.15sp * chordMag
                        // + half the scaled stem width * direction (-1 up, +1 down).
                        xo       = stem1->pos().x();

                        // account for articulations
                        yo = fixArticulations(yo, sc, __up, true);

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
                              yo = fixArticulations(yo, sc, __up, true);

                              // we may wish to force end to align to stem as well,
                              // if it is in same direction
                              // (but it won't be, so this assignment should have no effect)
                              stemPos = true;
                              }
                        else {
                              // avoid articulations
                              yo = fixArticulations(yo, sc, __up, sc->up() == _up);
                              }
                        }
                  }
            else if (sc) {
                  // avoid articulations
                  yo = fixArticulations(yo, sc, __up, sc->up() == _up);
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
                  yo += _spatium * .9 * __up;

                  // adjustments for stem and/or beam

                  if (stem2) { //ec can't be null
                        Beam* beam2 = ec->beam();
                        if (beam2 && beam2->cross()) {
                              // TODO: stem direction is not finalized, so we cannot use it here
                              yo = fixArticulations(yo, ec, __up, false);
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

                              // account for articulations
                              yo = fixArticulations(yo, ec, __up, true);
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
                                    yo = fixArticulations(yo, ec, __up, true);
                                    }
                              else {
                                    // avoid articulations
                                    yo = fixArticulations(yo, ec, __up, ec->up() == _up);
                                    }

                              }
                        }
                  else if (ec) {
                        // avoid articulations
                        yo = fixArticulations(yo, ec, __up, ec->up() == _up);
                        }

                  sp->p2 += QPointF(xo, yo);
                  }
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
      bool up = c1->up();
      for (Segment* seg = c1->segment(); seg; seg = seg->next(SegmentType::ChordRest)) {
            Element* e = seg->element(c1->track());
            if (!e || !e->isChord())
                  continue;
            Chord* c = toChord(e);
            if (c->up() != up)
                  return true;
            if (seg == c2->segment())
                  break;
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

