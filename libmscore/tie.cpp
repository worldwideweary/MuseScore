//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "accidental.h"
#include "barline.h"
#include "barlinegeometry.h"
#include "chord.h"
#include "hook.h"
#include "ledgerline.h"
#include "measure.h"
#include "notedot.h"
#include "part.h"
#include "score.h"
#include "segment.h"
#include "stem.h"
#include "staff.h"
#include "stafftype.h"
#include "system.h"
#include "tie.h"
#include "undo.h"
#include "utils.h"

#include <cmath>

namespace Ms {

Note* Tie::editStartNote;
Note* Tie::editEndNote;

// Curve height increases sublinearly with length: short ties remain compact,
// while long ties gain enough bow without a linear growth in curvature.
// Bounds are expressed in staff spaces, matching TieSegment's layout fields.
static qreal tieCurveHeight(qreal length, qreal sp, qreal minSp, qreal maxSp)
      {
      if (sp <= 0.0)
            return 0.0;
      const qreal lengthSp = qAbs(length) / sp;
      const qreal preferredSp = 0.3 + 0.3 * std::sqrt(qAbs(lengthSp - 1.0));
      return qBound(minSp, preferredSp, qMax(minSp, maxSp)) * sp;
      }

// A short bow needs less ink at its center than a full-length tie. This
// returns the distance between the two Bezier halves, not the total pen width.
static qreal tieCurveThickness(qreal lengthSp, qreal sp, qreal endWidth,
                               qreal midWidth, qreal minimumLengthSp, qreal mag)
      {
      const qreal normal = qMax(0.0, midWidth - endWidth);
      const qreal shortLimit = 4.0 * mag;
      if (lengthSp >= shortLimit || minimumLengthSp >= shortLimit)
            return normal;
      const qreal shortest = qMax(0.0, 0.15 * sp - endWidth);
      const qreal amount = qBound(0.0,
            (lengthSp - minimumLengthSp) / (shortLimit - minimumLengthSp), 1.0);
      return qMin(normal, shortest + (normal - shortest) * amount);
      }

QPointF tieNoteAttachment(const Note* note, bool up, bool inside, bool start, bool adjust)
      {
      Chord* chord = note->chord();
      const qreal sp = note->spatium();
      const qreal sign = up ? -1.0 : 1.0;
      const qreal side = start ? 1.0 : -1.0;
      const bool tab = note->staff() && note->staff()->isTabStaff(note->tick());
      if (tab) {
            // Keep the complete fret string clear, including two-digit frets.
            return QPointF((start ? note->bbox().right() : note->bbox().left()) + side * .2 * sp,
                  (up ? note->bbox().top() : note->bbox().bottom()) + sign * .2 * sp);
            }
      const bool outer = !inside && note == (up ? chord->upNote() : chord->downNote());
      bool noteInArcDirection = false;
      for (const Note* other : chord->notes()) {
            if (other != note && qAbs(other->x() - note->x()) < .01 * sp
                && (up ? other->y() < note->y() : other->y() > note->y()))
                  noteInArcDirection = true;
            }
      const SymId head = note->noteHead();
      const QPointF cutLeft = up ? note->symCutOutNW(head) : note->symCutOutSW(head);
      const QPointF cutRight = up ? note->symCutOutNE(head) : note->symCutOutSE(head);
      const qreal center = !cutLeft.isNull() && !cutRight.isNull()
            ? .5 * (cutLeft.x() + cutRight.x()) : .5 * note->headWidth();
      const qreal inset = (inside || note->headGroup() == NoteHead::Group::HEAD_SLASH)
            ? .2 : noteInArcDirection ? .45 : .1;
      QPointF point((inside ? (start ? note->headWidth() : 0.0) : center) + side * inset * sp,
            sign * ((inside ? 0.0 : .5 * note->headHeight()) + .2 * sp));
      if (!adjust)
            return point;

      // Keep a curve close to a ledger line on the inward side of that line.
      for (const LedgerLine* ledger = chord->ledgerLines(); ledger; ledger = ledger->next()) {
            const qreal distance = sign * (ledger->y() - note->y() - point.y());
            if (distance > 0.0 && distance < .4 * sp) {
                  point.ry() -= sign * (.4 * sp - distance);
                  break;
                  }
            }
      const bool avoidStem = chord->stem() && chord->stem()->visible() && chord->up() == up;
      if (outer && !avoidStem)
            return point;

      const int dots = note->score()->styleI(Sid::tieDotsPlacement);
      const qreal padding = .2 * sp;
      const qreal y = note->y() + point.y();
      const qreal top = y - (up ? .3 : .15) * sp;
      const qreal bottom = y + (up ? .15 : .3) * sp;
      qreal x = note->x() + point.x();
      auto clear = [&](const QRectF& rect, bool force = false) {
            if (!force && (rect.bottom() < top || rect.top() > bottom))
                  return;
            x = start ? qMax(x, rect.right() + padding) : qMin(x, rect.left() - padding);
            };
      const Segment* segment = chord->segment();
      const int firstTrack = chord->staffIdx() * VOICES;
      for (int track = firstTrack; track < firstTrack + VOICES; ++track) {
            const Element* element = segment ? segment->element(track) : nullptr;
            if (!element || !element->isChord())
                  continue;
            const Chord* otherChord = toChord(element);
            if (otherChord->vStaffIdx() != chord->vStaffIdx())
                  continue;
            const QPointF offset = otherChord->pos() - chord->pos();
            if (otherChord->stem() && otherChord->stem()->visible())
                  clear(otherChord->stem()->bbox().translated(otherChord->stem()->pos() + offset));
            for (const Note* other : otherChord->notes()) {
                  const QPointF pos = other->pos() + offset;
                  if (other != note && other->visible())
                        clear(other->bbox().translated(pos));
                  if (start && !outer && dots != 1 && !other->dotsHidden()) {
                        for (const NoteDot* dot : other->dots()) {
                              if (dot->visible())
                                    clear(dot->bbox().translated(pos + dot->pos()), dots == 2);
                              }
                        }
                  if (!start && other->accidental() && other->accidental()->visible()) {
                        const bool inward = up ? other->line() >= note->line() : other->line() <= note->line();
                        if (!(outer && inward))
                              clear(other->accidental()->bbox().translated(pos + other->accidental()->pos()));
                        }
                  }
            }
      point.setX(x - note->x());
      return point;
      }

static const Note* incomingTieSource(const Note* note)
      {
      for (Measure* measure : precedingRepeatMeasures(note->chord()->measure())) {
            for (Segment* segment = measure->last(); segment; segment = segment->prev()) {
                  if (!segment->isChordRestType())
                        continue;
                  for (int track = note->part()->startTrack(); track < note->part()->endTrack(); ++track) {
                        const Element* element = segment->element(track);
                        if (!element || !element->isChord())
                              continue;
                        const Chord* chord = toChord(element);
                        if (chord->tick() + chord->actualTicks() != measure->endTick())
                              continue;
                        for (Note* source : chord->notes()) {
                              if (source->pitch() != note->pitch() || (!source->tieFor() && !source->partialTie(false)))
                                    continue;
                              const auto destinations = repeatTieDestinations(source);
                              if (std::find(destinations.begin(), destinations.end(), note) != destinations.end())
                                    return source;
                              }
                        }
                  }
            }
      return nullptr;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TieSegment::draw(QPainter* painter) const
      {
      // hide tie toward the second chord of a cross-measure value
      if (tie()->endNote() && tie()->endNote()->chord()->crossMeasure() == CrossMeasure::SECOND)
            return;

      QPen pen(curColor(getProperty(Pid::VISIBLE).toBool(), getProperty(Pid::COLOR).value<QColor>()));
      qreal mag = staff() ? staff()->mag(tie()->tick()) : 1.0;

      //Replace generic Qt dash patterns with improved equivalents to show true dots (keep in sync with slur.cpp)
      QVector<qreal> dotted     = { 0.01, 1.99 }; // tighter than Qt DotLine equivalent - woud be { 0.01, 2.99 }
      QVector<qreal> dashed     = { 3.00, 3.00 }; // Compensating for caps. Qt default DashLine is { 4.0, 2.0 }
      QVector<qreal> wideDashed = { 5.00, 6.00 };

      switch (slurTie()->lineType()) {
            case 0:
                  painter->setBrush(QBrush(pen.color()));
                  pen.setCapStyle(Qt::RoundCap);
                  pen.setJoinStyle(Qt::RoundJoin);
                  pen.setWidthF(score()->styleP(Sid::tieEndWidth) * mag);
                  break;
            case 1:
                  painter->setBrush(Qt::NoBrush);
                  pen.setCapStyle(Qt::RoundCap); // True dots
                  pen.setDashPattern(dotted);
                  pen.setWidthF(score()->styleP(Sid::tieDottedWidth) * mag);
                  break;
            case 2:
                  painter->setBrush(Qt::NoBrush);
                  pen.setDashPattern(dashed);
                  pen.setWidthF(score()->styleP(Sid::tieDottedWidth) * mag);
                  break;
            case 3:
                  painter->setBrush(Qt::NoBrush);
                  pen.setDashPattern(wideDashed);
                  pen.setWidthF(score()->styleP(Sid::tieDottedWidth) * mag);
                  break;
            }
      painter->setPen(pen);
      painter->drawPath(path);
      }

//---------------------------------------------------------
//   edit
//    return true if event is accepted
//---------------------------------------------------------

bool TieSegment::edit(EditData& ed)
      {
      SlurTie* sl = tie();

      if (ed.key == Qt::Key_X && !ed.modifiers) {
            sl->setSlurDirection(sl->up() ? Direction::DOWN : Direction::UP);
            sl->layout();
            return true;
            }
      if (ed.key == Qt::Key_Home && !ed.modifiers) {
            ups(ed.curGrip).off = QPointF();
            sl->layout();
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   changeAnchor
//---------------------------------------------------------

void TieSegment::changeAnchor(EditData& ed, Element* element)
      {
      if (ed.curGrip == Grip::START) {
            spanner()->setStartElement(element);
            Note* note = toNote(element);
            if (note->chord()->tick() <= tie()->endNote()->chord()->tick()) {
                  tie()->startNote()->setTieFor(0);
                  tie()->setStartNote(note);
                  note->setTieFor(tie());
                  }
            }
      else {
            spanner()->setEndElement(element);
            Note* note = toNote(element);
            // do not allow backward ties
            if (note->chord()->tick() >= tie()->startNote()->chord()->tick()) {
                  tie()->endNote()->setTieBack(0);
                  tie()->setEndNote(note);
                  note->setTieBack(tie());
                  }
            }

      const size_t segments  = spanner()->spannerSegments().size();
      ups(ed.curGrip).off = QPointF();
      spanner()->layout();
      if (spanner()->spannerSegments().size() != segments) {
            const std::vector<SpannerSegment*>& ss = spanner()->spannerSegments();

            TieSegment* newSegment = toTieSegment(ed.curGrip == Grip::END ? ss.back() : ss.front());
            ed.view->startEdit(newSegment, ed.curGrip);
            triggerLayout();
            }
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void TieSegment::editDrag(EditData& ed)
      {
      // The first grip edit disables automatic shoulder constraints as well
      // as endpoint clearance. Preserve both parts of the currently visible
      // curve before recording the user's movement.
      if (!isEdited() && !isNudged())
            preserveAutomaticShoulderHeight();
      consolidateAutomaticGripOffsets();
      Grip g = ed.curGrip;
      ups(g).off += ed.delta;

      if (!tie()->isPartial() && (g == Grip::START || g == Grip::END)) {
            computeBezier();
            //
            // move anchor for slurs/ties
            //
            if ((g == Grip::START && isSingleBeginType()) || (g == Grip::END && isSingleEndType())) {
                  Spanner* spanner = tie();
                  Element* e = ed.view->elementNear(ed.pos);
                  Note* note = (e && e->isNote()) ? toNote(e) : nullptr;
                  if (note && ((g == Grip::END && note->tick() > tie()->tick()) || (g == Grip::START && note->tick() < tie()->tick2()))) {
                        if (g == Grip::END) {
                              Tie* tie = toTie(spanner);
                              if (tie->startNote()->pitch() == note->pitch()
                                 && tie->startNote()->chord()->tick() < note->chord()->tick()) {
                                    ed.view->setDropTarget(note);
                                    if (note != tie->endNote()) {
                                          changeAnchor(ed, note);
                                          return;
                                          }
                                    }
                              }
                        }
                  else
                        ed.view->setDropTarget(0);
                  }
            }
      else if (g == Grip::BEZIER1 || g == Grip::BEZIER2)
            computeBezier();
      else if (g == Grip::SHOULDER) {
            ups(g).off = QPointF();
            computeBezier(ed.delta);
            }
      else if (g == Grip::DRAG) {
            ups(Grip::DRAG).off = QPointF();
            roffset() += ed.delta;
            }

      triggerLayout();
      }

//---------------------------------------------------------
//   computeBezier
//    compute help points of slur bezier segment
//---------------------------------------------------------

void TieSegment::computeBezier(QPointF shoulderOffset)
      {
      qreal _spatium = spatium();
      qreal shoulderW; // height as fraction of slur-length
      qreal shoulderH;

      QPointF tieStart = ups(Grip::START).p + ups(Grip::START).off;
      QPointF tieEnd = ups(Grip::END).p   + ups(Grip::END).off;

      QPointF tieEndNormalized = tieEnd - tieStart; // normalize to zero
      if (qFuzzyIsNull(tieEndNormalized.x())) {
            qDebug("zero tie");
            return;
            }

      qreal tieAngle = atan(tieEndNormalized.y() / tieEndNormalized.x()); // angle required from tie start to tie end--zero if horizontal
      QTransform t;
      t.rotateRadians(-tieAngle);  // rotate so that we are working with horizontal ties regardless of endpoint height difference
      tieEndNormalized = t.map(tieEndNormalized);  // apply that rotation
      shoulderOffset = t.map(shoulderOffset);  // also apply to shoulderOffset

      const qreal tieWidthInSp = tieEndNormalized.x() / _spatium;
      shoulderH = tieCurveHeight(tieEndNormalized.x(), _spatium,
                                 shoulderHeightMin, shoulderHeightMax);
      shoulderW = .6;

      shoulderH -= shoulderOffset.y();

      if (!tie()->up())
            shoulderH = -shoulderH;

      qreal tieWidth = tieEndNormalized.x();
      qreal bezier1X = (tieWidth - tieWidth * shoulderW) * .5 + shoulderOffset.x();
      qreal bezier2X = bezier1X + tieWidth * shoulderW + shoulderOffset.x();

      QPointF tieDrag = QPointF(tieWidth * .5, 0.0);

      QPointF bezier1(bezier1X, -shoulderH);
      QPointF bezier2(bezier2X, -shoulderH);

      const qreal mag = staff() ? staff()->mag(tie()->tick()) : 1.0;
      const qreal endWidth = score()->styleP(Sid::tieEndWidth);
      const qreal midWidth = score()->styleP(Sid::tieMidWidth);
      const qreal minimumLengthSp = score()->styleS(Sid::minTieLength).val() * mag;
      const qreal w = tieCurveThickness(tieWidthInSp, _spatium,
                                       endWidth, midWidth, minimumLengthSp, mag) * mag;
      QPointF tieThickness(0.0, w);

      QPointF bezier1Offset = shoulderOffset + t.map(ups(Grip::BEZIER1).off);
      QPointF bezier2Offset = shoulderOffset + t.map(ups(Grip::BEZIER2).off);

      if (!shoulderOffset.isNull()) {
            QPointF invertedShoulder = t.inverted().map(shoulderOffset);
            ups(Grip::BEZIER1).off += invertedShoulder;
            ups(Grip::BEZIER2).off += invertedShoulder;
            }

      //-----------------------------------calculate p6
      QPointF bezier1Final = bezier1 + bezier1Offset;
      QPointF bezier2Final = bezier2 + bezier2Offset;
      QPointF bezierNormalized = bezier2Final - bezier1Final;

      qreal bezierAngle = atan(bezierNormalized.y() / bezierNormalized.x());  // in case bezier1 and bezier2 are not horizontal
      t.reset();
      t.rotateRadians(-bezierAngle);
      QPointF tieShoulder = QPointF(t.map(bezierNormalized).x() * .5, 0.0);

      t.rotateRadians(2 * bezierAngle);
      tieShoulder = t.map(tieShoulder) + bezier1Final - shoulderOffset;
      //-----------------------------------

      path = QPainterPath();
      path.moveTo(QPointF());
      path.cubicTo(bezier1 + bezier1Offset - tieThickness, bezier2 + bezier2Offset - tieThickness, tieEndNormalized);
      if (tie()->lineType() == 0)
            path.cubicTo(bezier2 + bezier2Offset + tieThickness, bezier1 + bezier1Offset + tieThickness, QPointF());

      tieThickness  = QPointF(0.0, 3.0 * w);
      shapePath = QPainterPath();
      shapePath.moveTo(QPointF());
      shapePath.cubicTo(bezier1 + bezier1Offset - tieThickness, bezier2 + bezier2Offset - tieThickness, tieEndNormalized);
      shapePath.cubicTo(bezier2 + bezier2Offset + tieThickness, bezier1 + bezier1Offset + tieThickness, QPointF());

      // translate back
      double y = tieStart.y();
      const double offsetFactor = 0.2;
      if (staff()->isTabStaff(slurTie()->tick()))
          y += (_spatium * (slurTie()->up() ? -offsetFactor : offsetFactor));
      t.reset();
      t.translate(tieStart.x(), y);
      t.rotateRadians(tieAngle);
      path = t.map(path);
      shapePath = t.map(shapePath);
      ups(Grip::BEZIER1).p = t.map(bezier1);
      ups(Grip::BEZIER2).p = t.map(bezier2);
      ups(Grip::END).p = t.map(tieEndNormalized) - ups(Grip::END).off;
      ups(Grip::DRAG).p = t.map(tieDrag);
      ups(Grip::SHOULDER).p = t.map(tieShoulder);

      _shape.clear();
      QPointF start;
      start = t.map(start);

      qreal minH = qAbs(3.0 * w);
      int nbShapes = 15;
      const CubicBezier b(tieStart, ups(Grip::BEZIER1).pos(), ups(Grip::BEZIER2).pos(), ups(Grip::END).pos());
      for (int i = 1; i <= nbShapes; i++) {
            const QPointF point = b.pointAtPercent(i/float(nbShapes));
            QRectF re = QRectF(start, point).normalized();
            if (re.height() < minH) {
                  const qreal padding = (minH - re.height()) * .5;
                  re.adjust(0.0, -padding, 0.0, padding);
                  }
            _shape.add(re);
            start = point;
            }
      }

//---------------------------------------------------------
//   layoutSegment
//    Initialize tie endpoints in system coordinates. Ordinary pitched ties
//    receive vertical clearance in finalizeSegment(), after horizontal layout.
//    Keep the existing path for partial ties until their free-end geometry is
//    migrated separately.
//---------------------------------------------------------

void TieSegment::layoutSegment(const QPointF& p1, const QPointF& p2)
      {
      autoAdjustOffset = QPointF();
      automaticGripOffset[0] = QPointF();
      automaticGripOffset[1] = QPointF();
      shoulderHeightMin = 0.3;
      shoulderHeightMax = 2.0;

      setPos(QPointF());
      ups(Grip::START).p = p1;
      ups(Grip::END).p = p2;
      
      //Adjust Y pos to staff type offset before other calculations
      if (staffType())
            rypos() += staffType()->yoffset().val() * spatium();

      if (isNudged() || isEdited() || !tie()->isPartial())
            return;

      // Preserve partial-tie positioning while migrating ordinary ties.
      // Their free ends need a separate pass to avoid repeat-edge regressions.
      // Use the actual curve for staff-line checks. The old rectangular
      // estimate assumes linear shoulder growth and a constant mid-width;
      // neither holds for the shorter and longer bows now being laid out.
      computeBezier();
      const QRectF bbox = path.boundingRect();

      Tie* t = toTie(slurTie());
      qreal sp = spatium();

      // adjust position to avoid staff line if necessary
      Staff* st          = staff();

      bool collideAbove = false;
      bool collideBelow = false;

      if (slurTie()->isTie() && st && !st->isTabStaff(slurTie()->tick())) {
            // multinote chords with ties need special handling
            // otherwise, adjusted tie might crowd an unadjusted tie unnecessarily
            Note* sn  = t->startNote();
            t->setTick(t->startNote()->tick());
            Chord* sc = sn ? sn->chord() : 0;
            if (sc && sc->notes().size() > 1) {
                  for (Note* note : sc->notes()) {
                        if (note == sn || !note->tieFor())
                              continue;
                        if (note->line() == sn->line() - 1 && t->up() == note->tieFor()->up())
                              collideAbove = true;
                        if (note->line() == sn->line() + 1 && t->up() == note->tieFor()->up())
                              collideBelow = true;
                        }
                  }
            }

      if (st && !st->isTabStaff(slurTie()->tick())) {
            qreal ld = st->lineDistance(tick()) * sp;
            qreal staffLineOffset = 0.125; // sp
            staffLineOffset += (styleP(Sid::staffLineWidth) / 2) / ld;
            bool up = slurTie()->up();
            qreal tieWidth = (styleP(Sid::slurMidWidth)) / ld;
            qreal topY = bbox.top() / ld;
            qreal bottomY = bbox.bottom() / ld;
            qreal endpointY = up ? bottomY : topY;
            if (endpointY > 0 && endpointY < (st->lines(tick()) - 1) * st->lineDistance(tick())) {
                  // tie endpoints are inside the staff and may require adjustment
                  std::vector<qreal> endpointAnchors;
                  for (int i = 0; i < st->lines(tick()); ++i) {
                        endpointAnchors.push_back((qreal)i - staffLineOffset);
                        endpointAnchors.push_back((qreal)i + staffLineOffset);
                        }

                  size_t endpointAnchorIndex = 0;
                  qreal extraAdjust = 0;
                  if (!tie()->isInside()) {
                        // Find the nearest endpoint anchor
                        for (size_t i = 0; i < endpointAnchors.size(); i++) {
                              if (qAbs(endpointAnchors[i] - endpointY) <= qAbs(endpointAnchors[endpointAnchorIndex] - endpointY))
                                    endpointAnchorIndex = i;
                              else
                                    break;
                              }

                        qreal currentOffset = endpointAnchors[endpointAnchorIndex] - endpointY;
                        topY += currentOffset;
                        bottomY += currentOffset;
                        // Adjust for tie apogee colliding with staff lines
                        qreal insideTieTop = up ? topY : (bottomY - tieWidth);
                        qreal insideTieBottom = up ? (topY + tieWidth) : bottomY;
                        qreal tieTopWithMargin = insideTieTop - staffLineOffset;
                        qreal tieBottomWithMargin = insideTieBottom + staffLineOffset;
                        if ((tieTopWithMargin < 0 && tieBottomWithMargin > 0) || (int)tieTopWithMargin != (int)tieBottomWithMargin) {
                              qreal oldAnchorY = endpointAnchors[endpointAnchorIndex];
                              if (up) {
                                    endpointAnchorIndex--;
                                    tieBottomWithMargin += endpointAnchors[endpointAnchorIndex] - oldAnchorY; // update position of tie bottom
                                    if (endpointAnchorIndex & 1) {  // endpoints just below a line can be adjusted downwards
                                          extraAdjust = ((int)(tieBottomWithMargin + 1) - tieBottomWithMargin); // how far the inside of the tie is from the staff line
                                          extraAdjust = qMax(extraAdjust, 0.0); // ensure downward adjustment

                                          // clamp endpoints to at least 0.5sp of staff line
                                          qreal endpointDistanceFromUpperLine = (endpointAnchors[endpointAnchorIndex] + extraAdjust) - (int)endpointY;
                                          if (endpointDistanceFromUpperLine < 0.5) {
                                                extraAdjust += 0.5 - endpointDistanceFromUpperLine;
                                                shoulderHeightMin = 3 * (0.5 + (tieWidth / 2) + (staffLineOffset / 2)) / 2;
                                                }
                                          }
                                    }
                              else { // tie is down
                                    endpointAnchorIndex++;
                                    tieTopWithMargin += endpointAnchors[endpointAnchorIndex] - oldAnchorY; // update position of tie top
                                    if (!(endpointAnchorIndex & 1)) { // endpoint just above a line can be adjusted upwards
                                          extraAdjust = ((int)tieTopWithMargin - tieTopWithMargin);
                                          extraAdjust = qMin(extraAdjust, 0.0);

                                          // clamp endpoints to at least 0.5sp of staff line
                                          qreal endpointDistanceFromLowerLine = (int)(endpointY + 1)
                                                      - (endpointAnchors[endpointAnchorIndex] + extraAdjust);
                                          if (endpointDistanceFromLowerLine < 0.5) {
                                                // add enough to extraAdjust so that it becomes endpointY + 0.25
                                                extraAdjust -= 0.5 - endpointDistanceFromLowerLine; //((int)endpointAnchors[endpointAnchorIndex] + 0.5) - (endpointAnchors[endpointAnchorIndex]);
                                                shoulderHeightMin = 3 * (0.5 + (tieWidth / 2) + (staffLineOffset / 2)) / 2;
                                                }
                                          }
                                    }
                              }
                        }
                  else { // inside-tie
                        endpointAnchorIndex = tie()->startNote()->line() + (tie()->up() ? 0 : 1);
                        if ((up && endpointAnchorIndex & 1)
                           || (!up && !(endpointAnchorIndex & 1))) {
                              // tie endpoint is right below the line, so let's adjust the height so that the top clears the line
                              shoulderHeightMin = 3 * (staffLineOffset + (tieWidth / 2)) / 2;
                              }
                        else {
                              qreal insideTieTop = up ? topY : (bottomY - tieWidth);
                              qreal insideTieBottom = up ? (topY + tieWidth) : bottomY;
                              qreal tieTopWithMargin = insideTieTop;// -0.125;// -staffLineOffset;
                              qreal tieBottomWithMargin = insideTieBottom;// +0.125;// +staffLineOffset;
                              if ((tieTopWithMargin < 0 && tieBottomWithMargin > 0) || (int)tieTopWithMargin != (int)tieBottomWithMargin) {
                                    //shoulderHeightMin = 3 * (staffLineOffset + (tieWidth / 2)) / 2;
                                    shoulderHeightMax = 1 - staffLineOffset - (tieWidth / 2);
                                    }
                              }
                        if ((up && collideBelow) || (!up && collideAbove))
                              shoulderHeightMin = 3 * (staffLineOffset + (tieWidth / 2)) / 2;
                        if ((up && collideAbove) || (!up && collideBelow))
                              shoulderHeightMax = 1 - staffLineOffset - (tieWidth / 2);
                        }

                  qreal currentOffset = endpointAnchors[endpointAnchorIndex] - endpointY;
                  setAutoAdjust(QPointF(0, (currentOffset + extraAdjust) * ld));
                  }
            }
      }

//---------------------------------------------------------
//   adjustVertical
//    Vertical staff-line clearance for ordinary pitched ties. This operates
//    after horizontal clearance has settled the tie length, so the curve
//    is checked in its final geometry rather than against a stale bow.
//    Automatic corrections remain in the existing transient endpoint and
//    shoulder-height state, which v0.306/v0.307 preserve on first edit.
//---------------------------------------------------------

void TieSegment::adjustVertical()
      {
      if (tie()->isPartial() || isEdited() || isNudged())
            return;

      Staff* st = staff();
      if (!st || st->isTabStaff(tick()))
            return;
      const qreal sp = spatium();
      const qreal lineDist = st->lineDistance(tick()) * sp;
      const int nLines = st->lines(tick());
      if (sp <= 0.0 || lineDist <= 0.0 || nLines <= 0)
            return;

      const bool up = tie()->up();
      const qreal outward = up ? -1.0 : 1.0;
      const qreal lineHalfWidth = 0.5 * styleP(Sid::staffLineWidth);
      const qreal endpointMargin = 0.15 * sp;

      // The 4.x ledger rule is independent of horizontal note/ledger
      // clearance. When a ledger lies just outward from the note-owned tip,
      // move *both* tips inward by the same amount (at most 0.4 sp). This
      // preserves horizontal ties and the slope of cross-staff ties. Use the
      // note owned by this system segment, not a note in another system.
      const Note* owner = isSingleBeginType() ? tie()->startNote() : tie()->endNote();
      if (owner && owner->chord() && owner->chord()->ledgerLines()) {
            const Chord* chord = owner->chord();
            qreal chordY = chord->pos().y() + chord->segment()->pos().y()
                  + chord->measure()->pos().y();
            if (system() && chord->vStaffIdx() != vStaffIdx())
                  chordY += system()->staff(chord->vStaffIdx())->y()
                        - system()->staff(vStaffIdx())->y();
            const qreal gripY = ups(isSingleBeginType() ? Grip::START : Grip::END).p.y();
            const qreal margin = 0.4 * sp;
            qreal correction = 0.0;
            for (const LedgerLine* ledger = chord->ledgerLines(); ledger; ledger = ledger->next()) {
                  const qreal distance = outward * (chordY + ledger->y() - gripY);
                  if (distance > 0.0 && distance < margin)
                        correction = qMax(correction, margin - distance);
                  }
            if (correction > 0.0)
                  setAutoAdjust(QPointF(0.0, -outward * correction));
            }

      // Clear the *outward* staff line when an endpoint emerges too close
      // to its ink. Move both endpoints together to preserve any slope.
      // For an upward tie the outward line is above (floor), for a downward
      // tie it is below (ceil). Do not snap to the closest line in either
      // direction, which can move an otherwise good attachment unnecessarily.
      const qreal startY = ups(Grip::START).p.y();
      const int outwardLine = up ? int(std::floor(startY / lineDist))
                                 : int(std::ceil(startY / lineDist));
      if (outwardLine >= 0 && outwardLine < nLines) {
            const qreal lineY = outwardLine * lineDist;
            const qreal distance = qAbs(startY - lineY);
            if (distance < endpointMargin + lineHalfWidth) {
                  const qreal targetY = lineY + (up ? -1.0 : 1.0)
                        * (endpointMargin + lineHalfWidth);
                  // The ledger adjustment may already have moved the tie.
                  // setAutoAdjust() accepts a total offset, not a delta.
                  setAutoAdjust(autoAdjustOffset + QPointF(0.0, targetY - startY));
                  }
            }

      // At the crown, compare the two faces of the ink ribbon with the
      // nearest staff line. Checking only the approximate center (v0.310)
      // cannot distinguish a bow passing across the line from one that has
      // already cleared it on either side. The outward/inward choice also
      // depends on which face has more room, not just the center's sign.
      computeBezier();
      const QRectF bounds = path.boundingRect();
      const qreal length = bounds.width();
      if (length <= 0.0)
            return;

      const qreal mag = st->mag(tick());
      const qreal endWidth = styleP(Sid::tieEndWidth);
      const qreal midWidth = styleP(Sid::tieMidWidth);
      const qreal minLengthSp = score()->styleS(Sid::minTieLength).val() * mag;
      const qreal ribbonHalfWidth = tieCurveThickness(length / sp, sp,
            endWidth, midWidth, minLengthSp, mag) * mag;
      const qreal outerY = up ? bounds.top() : bounds.bottom();
      const qreal innerY = outerY - outward * 2.0 * ribbonHalfWidth;
      const qreal centerY = 0.5 * (outerY + innerY);
      const int nearestLine = int(std::round(centerY / lineDist));
      if (nearestLine < 0 || nearestLine >= nLines)
            return;

      const qreal staffY = nearestLine * lineDist;
      const qreal outerMargin = -outward
            * (outerY - (staffY - outward * lineHalfWidth));
      const qreal innerMargin = outward
            * (innerY - (staffY + outward * lineHalfWidth));
      const qreal required = (length < 3.0 * sp ? 0.10 : 0.15) * sp;
      const bool growOutwards = outerMargin - 0.5 * required < innerMargin;
      const bool needsOutward = growOutwards && innerMargin < required;
      const bool needsInward = !growOutwards && outerMargin < required;
      if (!needsOutward && !needsInward)
            return;

      // A compact inner bow may sit more cleanly between staff lines with
      // its tips translated, rather than by changing its shoulder. This is
      // restricted to non-outer ties without an adjacent tied second.
      const bool shortTie = length < 2.0 * sp && bounds.height() < 0.7 * sp;
      const bool nonOuter = !tie()->isInside() && !tie()->isOuterTieOfChord(true)
            && !tie()->isOuterTieOfChord(false);
      bool tiedSecondInside = false;
      const Note* startNote = tie()->startNote();
      if (startNote) {
            const int neighborLine = startNote->line() + (up ? 1 : -1);
            for (const Note* neighbor : startNote->chord()->notes()) {
                  if (neighbor->line() == neighborLine && neighbor->tieFor()
                      && neighbor->tieFor()->up() == up) {
                        tiedSecondInside = true;
                        break;
                        }
                  }
            }
      if (shortTie && nonOuter && !tiedSecondInside && needsInward) {
            setAutoAdjust(autoAdjustOffset + QPointF(0.0,
                  -outward * (required - outerMargin)));
            return;
            }

      // The apex moves approximately 3/4 as far as the shoulder controls.
      // Keep this correction in the transient shoulder bounds, so the first
      // manual edit can retain it through preserveAutomaticShoulderHeight().
      const QPointF delta = ups(Grip::END).p - ups(Grip::START).p;
      const qreal currentHeight = tieCurveHeight(std::hypot(delta.x(), delta.y()),
            sp, shoulderHeightMin, shoulderHeightMax);
      const qreal needed = needsOutward ? required - innerMargin
                                       : required - outerMargin;
      const qreal heightChange = needed / 0.75;
      const qreal desiredHeight = qBound(0.10 * sp,
            currentHeight + (needsOutward ? heightChange : -heightChange),
            2.0 * sp);
      if (needsOutward)
            shoulderHeightMin = qMax(shoulderHeightMin, desiredHeight / sp);
      else {
            // Some line-avoiding bows need to be flatter than the default
            // 0.3 sp minimum. This is an automatic, temporary adjustment;
            // it does not change the score-wide minimum shoulder style.
            shoulderHeightMin = qMin(shoulderHeightMin, desiredHeight / sp);
            shoulderHeightMax = qMin(shoulderHeightMax, desiredHeight / sp);
            }
      }

//---------------------------------------------------------
//   separateFromOuterTie
//    After individual placement, make space between bows that begin on
//    neighboring notes of the same chord and curve in the same direction.
//    Change the transient shoulder bounds rather than the endpoints so
//    first-edit preservation continues to work normally.
//---------------------------------------------------------

void TieSegment::separateFromOuterTie(TieSegment* outer)
      {
      if (!outer || outer == this || isEdited() || isNudged()
          || outer->isEdited() || outer->isNudged()
          || tie()->up() != outer->tie()->up())
            return;

      const qreal sp = spatium();
      if (sp <= 0.0)
            return;
      const qreal left = qMax(ups(Grip::START).p.x(), outer->ups(Grip::START).p.x());
      const qreal right = qMin(ups(Grip::END).p.x(), outer->ups(Grip::END).p.x());
      if (right - left < 0.5 * sp)
            return;

      // Compare facing edges at the crowns, not both outer edges. Each
      // path bbox includes the endpoint height, so using its opposite edge
      // would mistake the noteheads for the ribbon's inner boundary.
      const bool up = tie()->up();
      const qreal outward = up ? -1.0 : 1.0;
      const qreal innerOuterY = up ? path.boundingRect().top() : path.boundingRect().bottom();
      const qreal outerCrownY = up ? outer->path.boundingRect().top()
            : outer->path.boundingRect().bottom();
      const qreal ribbonWidth = qMax(styleP(Sid::tieMidWidth), outer->styleP(Sid::tieMidWidth));
      const qreal outerInnerY = outerCrownY - outward * ribbonWidth;
      const qreal clearance = outward * (outerInnerY - innerOuterY);
      const qreal gap = 0.15 * sp;
      if (clearance >= gap)
            return;

      // Align the gap between adjacent ribbons to the nearest half-space
      // when that boundary is near the staff. This prevents tightly packed
      // curves from straddling a staff line, while leaving distant ledger
      // chords free of an artificial staff-position snap.
      qreal midY = (innerOuterY + outerInnerY) * 0.5;
      const Staff* st = staff();
      if (st) {
            const qreal halfLineDist = st->lineDistance(tick()) * sp * 0.5;
            if (halfLineDist > 0.0) {
                  const int halfLine = int(std::round(midY / halfLineDist));
                  if (halfLine >= -1 && halfLine <= 2 * st->lines(tick()) - 1)
                        midY = halfLine * halfLineDist;
                  }
            }

      // Change shoulder heights, not notehead anchors. A cubic with equal
      // shoulder heights moves its crown approximately 3/4 of the change.
      // The transient bounds are inherited by v0.307's first-edit capture.
      const qreal insideTargetY = midY - outward * 0.5 * gap;
      const qreal outsideTargetY = midY + outward * 0.5 * gap;
      const auto curveHeight = [](const TieSegment* segment) {
            const QPointF d = segment->ups(Grip::END).p - segment->ups(Grip::START).p;
            return tieCurveHeight(std::hypot(d.x(), d.y()), segment->spatium(),
                                  segment->shoulderHeightMin, segment->shoulderHeightMax);
            };
      const qreal insideHeight = curveHeight(this);
      const qreal outsideHeight = curveHeight(outer);
      const qreal lowerInside = qMax(0.0, -outward * (insideTargetY - innerOuterY) / 0.75);
      const qreal raiseOutside = qMax(0.0, outward * (outsideTargetY - outerInnerY) / 0.75);
      const qreal insideRoom = qMax(0.0, insideHeight - shoulderHeightMin * sp);
      const qreal outsideRoom = qMax(0.0, outer->shoulderHeightMax * sp - outsideHeight);
      const qreal insideCorrection = qMin(lowerInside, insideRoom);
      const qreal outsideCorrection = qMin(raiseOutside, outsideRoom);
      if (insideCorrection <= 0.0 && outsideCorrection <= 0.0)
            return;
      if (insideCorrection > 0.0)
            shoulderHeightMax = qMin(shoulderHeightMax, (insideHeight - insideCorrection) / sp);
      if (outsideCorrection > 0.0)
            outer->shoulderHeightMin = qMax(outer->shoulderHeightMin,
                  (outsideHeight + outsideCorrection) / sp);
      computeBezier();
      setbbox(path.boundingRect());
      outer->computeBezier();
      outer->setbbox(outer->path.boundingRect());
      }

//---------------------------------------------------------
//   finalizeSegment
//    compute the bezier and adjust the bbox for the curve
//---------------------------------------------------------

void TieSegment::finalizeSegment()
      {
      // Partial ties already share their free endpoints and minimum lengths
      // with repeat spacing. Do not pull shorter chord ties across that edge.
      if (!tie()->isPartial() && !isEdited() && !isNudged()
          && (spannerSegmentType() != SpannerSegmentType::SINGLE || !tie()->endNote())) {
            const qreal minimum = score()->styleP(Sid::minHangingTieLength);
            if (ups(Grip::END).p.x() - ups(Grip::START).p.x() < minimum) {
                  if (spannerSegmentType() == SpannerSegmentType::END)
                        ups(Grip::START).p.rx() = ups(Grip::END).p.x() - minimum;
                  else
                        ups(Grip::END).p.rx() = ups(Grip::START).p.x() + minimum;
                  }
            }
      adjustVertical();
      computeBezier();
      setbbox(path.boundingRect());
      }

Element* TieSegment::propertyDelegate(Pid id)
      {
      return id == Pid::TIE_PLACEMENT ? static_cast<Element*>(tie()) : SlurTieSegment::propertyDelegate(id);
      }

void TieSegment::adjustX()
      {
      if (tie()->isPartial()) {
            if (isNudged() || isEdited())
                  return;
            // layoutSegment() may move the whole curve to clear staff lines.
            // Recheck other voices' heads at that final height, including for
            // a tie which is outer only within its own single-note chord.
            const Note* note = tie()->startNote();
            const Chord* chord = note->chord();
            const bool start = !tie()->isIncoming();
            QPointF& point = ups(start ? Grip::START : Grip::END).p;
            const qreal originalX = point.x();
            const qreal sp = spatium();
            const qreal top = point.y() - (tie()->up() ? .3 : .15) * sp;
            const qreal bottom = point.y() + (tie()->up() ? .15 : .3) * sp;
            const int firstTrack = chord->staffIdx() * VOICES;
            QPointF staffOffset;
            if (system() && chord->vStaffIdx() != vStaffIdx())
                  staffOffset.ry() = system()->staff(chord->vStaffIdx())->y() - system()->staff(vStaffIdx())->y();
            for (int track = firstTrack; track < firstTrack + VOICES; ++track) {
                  const Element* element = chord->segment()->element(track);
                  if (!element || !element->isChord())
                        continue;
                  const Chord* otherChord = toChord(element);
                  if (otherChord->vStaffIdx() != chord->vStaffIdx())
                        continue;
                  const QPointF origin = otherChord->pos() + otherChord->segment()->pos()
                        + otherChord->measure()->pos() + staffOffset;
                  for (const Note* other : otherChord->notes()) {
                        if (other == note || !other->visible())
                              continue;
                        const QRectF head = other->bbox().translated(origin + other->pos());
                        if (head.bottom() < top || head.top() > bottom)
                              continue;
                        point.setX(start ? qMax(point.x(), head.right() + .2 * sp)
                              : qMin(point.x(), head.left() - .2 * sp));
                        }
                  }
            automaticGripOffset[start ? 0 : 1].rx() += point.x() - originalX;
            return;
            }
      // Use the same initial notehead attachment and clearance rules for
      // ordinary pitched ties as for partial ties and l.v. ties. Unlike the
      // legacy offsets below, these are applied relative to the *actual*
      // endpoint rather than adding a second margin to the notehead inset.
      // Keep tablature on the existing fret/stem path for this first pass.
      const Note* startNote = tie()->startNote();
      const Note* endNote = tie()->endNote();
      const bool standardStart = startNote && startNote->staff()
            && !startNote->staff()->isTabStaff(startNote->tick());
      const bool standardEnd = endNote && endNote->staff()
            && !endNote->staff()->isTabStaff(endNote->tick());
      if (standardStart && (!endNote || standardEnd)) {
            if (isNudged() || isEdited())
                  return;
            if (isSingleBeginType()) {
                  const QPointF anchor = tieNoteAttachment(startNote, tie()->up(),
                        tie()->isInside(), true, false);
                  const QPointF cleared = tieNoteAttachment(startNote, tie()->up(),
                        tie()->isInside(), true, true);
                  const qreal shift = qMax(qreal(0.0), cleared.x() - anchor.x());
                  ups(Grip::START).p.rx() += shift;
                  automaticGripOffset[0].rx() += shift;
                  }
            if (endNote && isSingleEndType()) {
                  const QPointF anchor = tieNoteAttachment(endNote, tie()->up(),
                        tie()->isInside(), false, false);
                  const QPointF cleared = tieNoteAttachment(endNote, tie()->up(),
                        tie()->isInside(), false, true);
                  const qreal shift = qMin(qreal(0.0), cleared.x() - anchor.x());
                  ups(Grip::END).p.rx() += shift;
                  automaticGripOffset[1].rx() += shift;
                  }
            return;
            }

      const bool incoming = tie()->isPartial() && tie()->isIncoming();
      Note* sn = incoming ? nullptr : tie()->startNote();
      Note* en = incoming ? tie()->startNote() : tie()->endNote();
      Chord* sc = sn ? sn->chord() : nullptr;
      Chord* ec = en ? en->chord() : nullptr;

      bool beginGrace = (sn && sn->noteType() != NoteType::NORMAL);
      bool endGrace = (en && en->noteType() != NoteType::NORMAL);
      qreal offsetMargin = !(beginGrace || endGrace) ? spatium() * 0.25 : spatium() * 0.10;
      qreal collisionYMargin = spatium() * 0.25;

      qreal xo;
      const int dotsPlacement = score()->styleI(Sid::tieDotsPlacement);
      const bool innerStart = !tie()->isOuterTieOfChord(true);

      if (isNudged() || isEdited())
            return;

      const bool useLedgerInk = score()->styleB(Sid::spacingLayout4)
            && spannerSegmentType() == SpannerSegmentType::SINGLE && sc && ec
            && sc->measure() == ec->measure() && !sc->staffMove() && !ec->staffMove()
            && !beginGrace && !endGrace && sn->line() == en->line() && tie()->isInside();
      const auto ledgerNeedsHorizontalRoom = [&](const LedgerLine* ledger, const Chord* chord, qreal gripY) {
            // For an ordinary level tie above the staff, the curve leaves
            // the grip away from a ledger on the opposite side. Allow
            // horizontal overlap only if the ink also clears vertically.
            if (useLedgerInk && gripY < 0 && !chord->staffMove()) {
                  const qreal ledgerY = chord->y() + ledger->y();
                  const qreal away = tie()->up() ? ledgerY - gripY : gripY - ledgerY;
                  const qreal mag = staff() ? staff()->mag(tie()->tick()) : 1.0;
                  const qreal stroke = qMax(score()->styleP(Sid::tieMidWidth),
                                            score()->styleP(Sid::tieEndWidth)) * mag;
                  if (away > ledger->lineWidth() * 0.5 + stroke)
                        return false;
                  }
            return qAbs(gripY - ledger->y()) < spatium() * 0.5;
            };

      // ADJUST LEFT GRIP -----------
      if (sc && ((spannerSegmentType() == SpannerSegmentType::SINGLE || spannerSegmentType() == SpannerSegmentType::BEGIN))) {
            const qreal gripY = ups(Grip::START).p.y();
            xo = 0;
            if (!tie()->isOuterTieOfChord(true)) {  // inner ties also clear adjacent notes and stems
                  // for cross-voice collisions, we need a list of all chords at this tick
                  std::vector<Chord*> chords;
                  int strack = sc->staffIdx() * VOICES;
                  int etrack = sc->staffIdx() * VOICES + VOICES;
                  for (int track = strack; track < etrack; ++track) {
                        if (Chord* ch = sc->measure()->findChord(sc->tick(), track))
                              chords.push_back(ch);
                        }

                  for (Chord* chord : chords) {
                        qreal chordOffset = chord->x() - sc->x() - sn->x() - sn->width(); // sn for right-offset notes, width() to normalize to zero
                        // adjust for hooks
                        if (chord->hook() && chord->hook()->visible()) {
                              qreal hookHeight = chord->hook()->bbox().height();
                              // turn the hook upside down for downstems
                              qreal hookY = chord->hook()->pos().y() - (chord->up() ? 0 : hookHeight);
                              if (gripY > hookY - collisionYMargin  && gripY < hookY + hookHeight + collisionYMargin )
                                    xo = qMax(xo, chord->hook()->x() + chord->hook()->width() + chordOffset);
                              }

                        // adjust for stems
                        if (chord->stem() && chord->stem()->visible()) {
                              qreal stemLen = chord->stem()->bbox().height();
                              qreal stemY = chord->stem()->pos().y() - (chord->up() ? stemLen : 0);
                              if (gripY > stemY - collisionYMargin  && gripY < stemY + stemLen + collisionYMargin )
                                    xo = qMax(xo, chord->stem()->x() + chord->stem()->width() + chordOffset);
                              }

                        // adjust for ledger lines
                        for (LedgerLine* currLedger = chord->ledgerLines(); currLedger; currLedger = currLedger->next()) {
                              // search through ledger lines and see if any are within .5sp of tie start
                              if (ledgerNeedsHorizontalRoom(currLedger, chord, gripY)) {
                                    xo = qMax(xo, (currLedger->x() + currLedger->len() + chordOffset));
                                    break;
                                    }
                              }

                        for (auto note : chord->notes()) {
                              // adjust for dots
                              if (!(innerStart && chord == sc && dotsPlacement == 1) && note->dots().size() > 0) {
                                    qreal dotY = note->pos().y() + note->dots().last()->y();
                                    if (qAbs(gripY - dotY) < spatium() * 0.5)
                                          xo = qMax(xo, note->x() + note->dots().last()->x() + note->dots().last()->width() + chordOffset);
                                    }

                              // adjust for note collisions
                              if (note == sn)
                                    continue;
                              qreal noteTop = note->y() + note->bbox().top();
                              qreal noteHeight = note->height();
                              if (gripY > noteTop && gripY < noteTop + noteHeight)
                                    xo = qMax(xo, note->x() + note->width() + chordOffset);
                              }
                        }
                  }
            xo = (beginGrace || endGrace ? 0.0 : xo) + offsetMargin;
            if (innerStart && dotsPlacement == 2) {
                  for (const Note* note : sc->notes()) {
                        for (const NoteDot* dot : note->dots()) {
                              if (dot->visible())
                                    xo = qMax(xo, note->x() + dot->x() + dot->width()
                                          - sn->x() - sn->headWidth() + offsetMargin);
                              }
                        }
                  }
            ups(Grip::START).p += QPointF(xo, 0);
            automaticGripOffset[0].rx() += xo;
            }

      // ADJUST RIGHT GRIP ----------
      if (ec && ((spannerSegmentType() == SpannerSegmentType::SINGLE || spannerSegmentType() == SpannerSegmentType::END))) {
            const qreal gripY = ups(Grip::END).p.y();
            xo = 0;
            if (!tie()->isOuterTieOfChord(false)) {
                  // for inter-voice collisions, we need a list of all notes from all voices
                  std::vector<Chord*> chords;
                  int strack = ec->staffIdx() * VOICES;
                  int etrack = ec->staffIdx() * VOICES + VOICES;
                  for (int track = strack; track < etrack; ++track) {
                        if (Chord* ch = ec->measure()->findChord(ec->tick(), track))
                              chords.push_back(ch);
                        }

                  for (Chord* chord : chords) {
                        qreal chordOffset = (ec->x() + en->x()) - chord->x(); // en->x() for right-offset notes
                        for (LedgerLine* currLedger = chord->ledgerLines(); currLedger; currLedger = currLedger->next()) {
                              // search through ledger lines and see if any are within .5sp of tie end
                              if (ledgerNeedsHorizontalRoom(currLedger, chord, gripY))
                                    xo = qMin(xo, currLedger->x() - chordOffset);
                              }

                        if (chord->stem() && chord->stem()->visible()) {
                              // adjust for stems
                              qreal stemLen = chord->stem()->bbox().height();
                              qreal stemY = chord->stem()->pos().y() - (chord->up() ? stemLen : 0);
                              if (gripY > stemY - collisionYMargin  && gripY < stemY + stemLen + collisionYMargin )
                                    xo = qMin(xo, chord->stem()->x() - chordOffset);
                              }

                        for (Note* note : chord->notes()) {
                              // adjust for accidentals
                              Accidental* acc = note->accidental();
                              if (acc && acc->visible()) {
                                    qreal accTop = (note->y() + acc->y()) + acc->bbox().top();
                                    qreal accHeight = acc->height();
                                    if (gripY >= accTop && gripY <= accTop + accHeight)
                                          xo = qMin(xo, note->x() + acc->x() - chordOffset);
                                    }

                              if (note == en)
                                    continue;
                              // adjust for shifted notes (such as intervals of unison or second)
                              qreal noteTop = note->y() + note->bbox().top();
                              qreal noteHeight = note->headHeight();
                              if (gripY >= noteTop - collisionYMargin  && gripY <= noteTop + noteHeight + collisionYMargin )
                                    xo = qMin(xo, note->x() - chordOffset);
                              }
                        }
                  }
            xo = (beginGrace || endGrace ? 0.0 : xo) - offsetMargin;
            ups(Grip::END).p += QPointF(xo, 0);
            automaticGripOffset[1].rx() += xo;
            }
      }

//---------------------------------------------------------
//   setAutoAdjust
//---------------------------------------------------------

void TieSegment::setAutoAdjust(const QPointF& offset)
      {
      QPointF diff = offset - autoAdjustOffset;
      if (!diff.isNull()) {
            path.translate(diff);
            shapePath.translate(diff);
            _shape.translate(diff);
            for (int i = 0; i < int(Grip::GRIPS); ++i)
                  _ups[i].p += diff;
            automaticGripOffset[0] += diff;
            automaticGripOffset[1] += diff;
            autoAdjustOffset = offset;
            }
      }

//---------------------------------------------------------
//   preserveAutomaticShoulderHeight
//    A first edit bypasses layoutSegment's automatic shoulder limits. Encode
//    their current effect in the editable Bezier grips so a horizontal nudge
//    does not suddenly change the arch. Unlike temporary shoulder limits,
//    these grip offsets also survive saving and reopening an edited tie.
//---------------------------------------------------------

void TieSegment::preserveAutomaticShoulderHeight()
      {
      const QPointF start = ups(Grip::START).pos();
      const QPointF end   = ups(Grip::END).pos();
      const QPointF vector = end - start;
      const qreal sp = spatium();
      if (vector.x() <= 0.0 || sp <= 0.0)
            return;

      const qreal length = std::hypot(vector.x(), vector.y());
      const qreal height = tieCurveHeight(length, sp, shoulderHeightMin, shoulderHeightMax);
      const qreal unrestrainedHeight = tieCurveHeight(length, sp, 0.3, 2.0);
      const qreal delta = height - unrestrainedHeight;
      if (qFuzzyIsNull(delta))
            return;

      // computeBezier() derives the control-point height in coordinates
      // rotated by the tie's angle, then applies Bezier user offsets there.
      // Express the compensating displacement in the unrotated grip space.
      const qreal angle = std::atan2(vector.y(), vector.x());
      QTransform rotation;
      rotation.rotateRadians(angle);
      const QPointF correction = rotation.map(QPointF(0.0, tie()->up() ? -delta : delta));
      ups(Grip::BEZIER1).off += correction;
      ups(Grip::BEZIER2).off += correction;
      }

//---------------------------------------------------------
//   consolidateAutomaticGripOffsets
//    Keep the endpoints fixed when user editing disables automatic placement.
//    The manual offset is relative to the unadjusted notehead attachment.
//---------------------------------------------------------

void TieSegment::consolidateAutomaticGripOffsets()
      {
      for (int i = 0; i < 2; ++i) {
            const Grip grip = i == 0 ? Grip::START : Grip::END;
            const QPointF adjustment = automaticGripOffset[i];
            if (!adjustment.isNull()) {
                  ups(grip).p -= adjustment;
                  ups(grip).off += adjustment;
                  automaticGripOffset[i] = QPointF();
                  }
            }
      autoAdjustOffset = QPointF();
      }

//---------------------------------------------------------
//   isEdited
//---------------------------------------------------------

bool TieSegment::isEdited() const
      {
      for (int i = 0; i < int(Grip::GRIPS); ++i) {
            if (!_ups[i].off.isNull())
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   slurPos
//    Calculate position of start- and endpoint of slur
//    relative to System() position.
//---------------------------------------------------------

void Tie::slurPos(SlurPos* sp)
      {
      if (isPartial()) {
            const Note* note = startNote(); // local owner, also for incoming ties
            const Chord* chord = note->chord();
            calculateIsInside();
            const bool adjust = spannerSegments().empty() || (!frontSegment()->isNudged() && !frontSegment()->isEdited());
            QPointF origin = note->pos() + chord->pos() + chord->segment()->pos() + chord->measure()->pos();
            sp->system1 = sp->system2 = chord->measure()->system();
            if (sp->system1 && chord->vStaffIdx() != vStaffIdx())
                  origin.ry() += sp->system1->staff(chord->vStaffIdx())->y() - sp->system1->staff(vStaffIdx())->y();
            sp->p1 = origin + tieNoteAttachment(note, up(), isInside(), true, adjust);
            sp->p2 = origin + tieNoteAttachment(note, up(), isInside(), false, adjust);
            return;
            }
      // Partial ties keep their owner in startNote() in both directions.
      // Use that note for both local anchors, without changing the linear
      // tie chains, so the ordinary placement rules also provide an end anchor.
      const Note* sn = startNote();
      const Note* en = isPartial() ? sn : endNote();
      bool useTablature = staff() && staff()->isTabStaff(tick());
      const StaffType* stt = useTablature ? staff()->staffType(tick()) : 0;
      if (!useTablature) {
            // Pitched ties share one note-local optical attachment calculation
            // with partial ties and l.v. marks. This is the unadjusted anchor;
            // TieSegment::adjustX() separately handles nearby ink.
            Chord* sc = sn->chord();
            Chord* ec = en ? en->chord() : nullptr;
            calculateIsInside();
            sp->system1 = sc->measure()->system();
            sp->system2 = ec ? ec->measure()->system() : sp->system1;
            sp->p1 = sn->pos() + sc->pos() + sc->segment()->pos() + sc->measure()->pos()
                  + tieNoteAttachment(sn, up(), isInside(), true, false);
            if (sc->isGrace())
                  sp->p1 += sc->parent()->pos();
            if (sp->system1 && sc->vStaffIdx() != vStaffIdx())
                  sp->p1.ry() += sp->system1->staff(sc->vStaffIdx())->y()
                        - sp->system1->staff(vStaffIdx())->y();
            if (!ec) {
                  // The 3.x one-ended tie retains its existing hanging length.
                  sp->p2 = sp->p1 + QPointF(score()->styleP(Sid::minHangingTieLength), 0.0);
                  return;
                  }
            sp->p2 = en->pos() + ec->pos() + ec->segment()->pos() + ec->measure()->pos()
                  + tieNoteAttachment(en, up(), isInside(), false, false);
            if (ec->isGrace())
                  sp->p2 += ec->parent()->pos();
            if (sp->system2 && ec->vStaffIdx() != vStaffIdx())
                  sp->p2.ry() += sp->system2->staff(ec->vStaffIdx())->y()
                        - sp->system2->staff(vStaffIdx())->y();

            // Two identical staff positions should have level endpoints. If
            // the two noteheads differ in size, keep the outermost attachment.
            if (sn->line() == en->line() && sc->vStaffIdx() == ec->vStaffIdx()) {
                  const qreal y = up() ? qMin(sp->p1.y(), sp->p2.y())
                        : qMax(sp->p1.y(), sp->p2.y());
                  sp->p1.setY(y);
                  sp->p2.setY(y);
                  }
            return;
            }
      qreal _spatium    = spatium();
      qreal hw          = sn->tabHeadWidth(stt);   // if stt == 0, defaults to headWidth()
      qreal __up        = _up ? -1.0 : 1.0;
      // y offset for ties inside chord margins (typically multi-note chords): lined up with note top or bottom margin
      //    or outside (typically single-note chord): overlaps note and is above/below it
      // Outside: Tab: uses font size and may be asymmetric placed above/below line (frets ON or ABOVE line)
      //          Std: assumes notehead is 1 sp high, 1/2 sp above and 1/2 below line; add 1/4 sp to it
      // Inside:  Tab: 1/2 of Outside offset
      //          Std: use a fixed percentage of note width
      qreal yOffOutside = useTablature
            ? (_up ? stt->fretBoxY() : stt->fretBoxY() + stt->fretBoxH()) * magS()
            : 0.75 * _spatium * __up;
      qreal yOffInside  = useTablature ? yOffOutside * 0.5 : hw * .3 * __up;

      Chord* sc = sn->chord();
      Chord* ec = en ? en->chord() : nullptr;
      sp->system1 = sc->measure()->system();
      if (!sp->system1) {
            Measure* m = sc->measure();
            qDebug("No system: measure is %d has %d count %d", m->isMMRest(), m->hasMMRest(), m->mmRestCount());
            }

      qreal x1, y1;
      qreal x2, y2;

      // determine attachment points
      // similar code is used in Chord::layoutPitched()
      // to allocate extra space to enforce minTieLength
      // so keep these in sync

      sp->p1    = sc->pos() + sc->segment()->pos() + sc->measure()->pos();

      //------p1
      x1 = sn->pos().x() + hw;
      y1 = sn->pos().y();
      qreal xo = 0;
      calculateIsInside();
      if (isInside()) {
            xo = 0;  // the offset for these will be decided in TieSegment::adjustX()
            }
      else {
            if (sc->stem() && sc->up() && _up) {
                  // usually, outside ties start in the middle of the notehead, but
                  // for up-ties on up-stems, we'll start at the end of the notehead
                  // to avoid the stem
                  xo = 0;
                  }
            else {
                  // start in the middle of the notehead for outside notes
                  xo = -(hw / 2);
            }
      }
      y1 += isInside() ? yOffInside : yOffOutside;
      sp->p1 += QPointF(x1 + xo, y1);

      //------p2
      y2 = y1;
      if (!ec) {
            sp->p2 = sp->p1 + QPointF(score()->styleP(Sid::minHangingTieLength), 0.0);
            sp->system2 = sp->system1;
            return;
            }
      sp->p2 = ec->pos() + ec->segment()->pos() + ec->measure()->pos();
      sp->system2 = ec->measure()->system();

      // force tie to be horizontal except for cross-staff or if there is a difference of line (tpc, clef, tpc)
      bool horizontal = sn->line() == en->line() && sc->vStaffIdx() == ec->vStaffIdx();

      hw = en->tabHeadWidth(stt);
      x2 = en->x();
      if (!horizontal)
            y2 = en->pos().y() + (isInside() ? yOffInside : yOffOutside);
      if (isInside())
            xo = 0.0;
      else {
            if (ec->stem() && !ec->up() && !_up) {
                  // as before, xo should account for stems that could get in the way
                  xo = 0;
                  }
            else {
                  // start in the middle of the notehead for outside notes
                  xo = -(hw / 2);
                  }
            }
      sp->p2 += QPointF(x2 - xo, y2);

      if (sc->isGrace())
            sp->p1 += sc->parent()->pos();
      if (ec->isGrace())
            sp->p2 += ec->parent()->pos();

      // adjust for cross-staff
      if (sc->vStaffIdx() != vStaffIdx() && sp->system1) {
            qreal diff = sp->system1->staff(sc->vStaffIdx())->y() - sp->system1->staff(vStaffIdx())->y();
            sp->p1.ry() += diff;
            }
      if (ec->vStaffIdx() != vStaffIdx() && sp->system2) {
            qreal diff = sp->system2->staff(ec->vStaffIdx())->y() - sp->system2->staff(vStaffIdx())->y();
            sp->p2.ry() += diff;
            }
      }

//---------------------------------------------------------
//   Tie
//---------------------------------------------------------

bool Tie::isOuterTieOfChord(bool start) const
      {
      // Both anchors of a partial tie use the owner stored in startNote().
      const Note* note = start || isPartial() ? startNote() : endNote();
      return !isInside() && note && ((up() && note == note->chord()->upNote())
            || (!up() && note == note->chord()->downNote()));
      }

// Placement is resolved separately from direction, and is also used by chord spacing.
void Tie::calculateIsInside()
      {
      const Chord* sc = startNote() ? startNote()->chord() : nullptr;
      const Chord* ec = endNote() ? endNote()->chord() : nullptr;
      const bool chord = (sc && sc->notes().size() > 1) || (ec && ec->notes().size() > 1);
      const int placement = _tiePlacement ? _tiePlacement
            : score()->styleI(chord ? Sid::tiePlacementChord : Sid::tiePlacementSingleNote);
      _isInside = placement == 1;
      }

void Tie::reset()
      {
      SlurTie::reset();
      undoResetProperty(Pid::TIE_PLACEMENT);
      }

QVariant Tie::getProperty(Pid id) const
      {
      return id == Pid::TIE_PLACEMENT ? QVariant(_tiePlacement) : SlurTie::getProperty(id);
      }

bool Tie::setProperty(Pid id, const QVariant& value)
      {
      if (id != Pid::TIE_PLACEMENT)
            return SlurTie::setProperty(id, value);
      const int placement = value.toInt();
      if (placement < 0 || placement > 2)
            return false;
      _tiePlacement = placement;
      triggerLayout();
      return true;
      }

QVariant Tie::propertyDefault(Pid id) const
      {
      return id == Pid::TIE_PLACEMENT ? QVariant(0) : SlurTie::propertyDefault(id);
      }

Tie::Tie(Score* s)
   : SlurTie(s)
      {
      setAnchor(Anchor::NOTE);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Tie::write(XmlWriter& xml) const
      {
      if (isPartial()) {
            xml.stag("PartialTie");
            xml.tag("partialSpannerDirection", isIncoming() ? "incoming" : "outgoing");
            }
      else
            xml.stag(this);
      SlurTie::writeProperties(xml);
      if (_tiePlacement)
            xml.tag("tiePlacement", _tiePlacement == 1 ? "inside" : "outside");
      xml.etag();
      }

bool Tie::readProperties(XmlReader& xml)
      {
      if (xml.name() == "tiePlacement") {
            const QString value = xml.readElementText();
            _tiePlacement = value == "inside" ? 1 : value == "outside" ? 2 : 0;
            return true;
            }
      if (xml.name() == "partialSpannerDirection") {
            const QString direction = xml.readElementText();
            _partialDirection = direction == "incoming" ? 0 : 1;
            return true;
            }
      return SlurTie::readProperties(xml);
      }

// Free endpoint shared by spacing and drawing. Do not clamp it to the
// landing chord: spacing must be able to move that chord to make room.
qreal Tie::incomingStartOffset() const
      {
      const Note* note = startNote();
      if (!isPartial() || !isIncoming() || !note)
            return 0.0;
      const Chord* chord = note->chord();
      const Measure* measure = chord->measure();
      const System* system = measure->system();
      const qreal sp = spatium();
      if (!system)
            return 0.0;
      if (measure == system->firstMeasure()) {
            const Segment* firstCR = measure->first(SegmentType::ChordRest);
            const Segment* header = firstCR ? firstCR->prevActive() : nullptr;
            while (header && header->allElementsInvisible())
                  header = header->prevActive();
            if (!header)
                  return sp - measure->x();
            qreal width = 0.0;
            for (int staff = 0; staff < score()->nstaves(); ++staff) {
                  const Element* item = header->element(staff * VOICES);
                  if (system->staff(staff)->show() && item && item->addToSkyline()) {
                        const qreal right = header->isStartRepeatBarLineType() && item->isBarLine()
                              ? startRepeatRightEdge(toBarLine(item)) : item->bbox().right();
                        width = qMax(width, item->x() + right);
                        }
                  }
            return header->x() + width + (header->isStartRepeatBarLineType() ? 0.25 : 1.0) * sp;
            }
      const int firstTrack = chord->vStaffIdx() * VOICES;
      const Segment* adjacent = chord->segment()->prev1();
      while (adjacent && (!adjacent->enabled() || !adjacent->visible()
             || adjacent->allElementsInvisible() || adjacent->isBreathType()
             || !adjacent->hasElements(firstTrack, firstTrack + VOICES - 1)))
            adjacent = adjacent->prev1();
      if (!adjacent)
            return 0.25 * sp;
      const Element* item = nullptr;
      for (int track = firstTrack; track < firstTrack + VOICES && !item; ++track)
            item = adjacent->element(track);
      const bool startRepeat = item->isBarLine() && toBarLine(item)->barLineType() == BarLineType::START_REPEAT;
      const qreal right = startRepeat ? startRepeatRightEdge(toBarLine(item)) : item->shape().right();
      // The preceding measure's right edge is the current measure's origin.
      // Avoid depending on stale system positions during minimum-width layout.
      qreal x = adjacent->x();
      if (adjacent->measure() != measure) {
            const Segment* last = adjacent->measure()->lastEnabled();
            while (last && (!last->visible() || last->allElementsInvisible()))
                  last = last->prevActive();
            // width() already includes rhythmic stretch here, whereas the
            // segment positions still describe the unstretched minimum.
            // Use the actual trailing shape in either layout phase.
            if (last)
                  x -= last->x() + last->minRight();
            }
      return x + item->x() + right + (item->isBarLine() && !startRepeat ? 0.65 : 0.25) * sp;
      }

const char* Tie::spacingGripInset4(qreal& inset) const
      {
      const Note* a = startNote();
      const Note* b = endNote();
      if (isPartial() || !a || !b || a->track() != b->track() || track() != a->track()
          || a->line() != b->line() || a->isNudged() || b->isNudged())
            return "unsupported endpoints";
      const Chord* first = a->chord();
      const Chord* last = b->chord();
      const Measure* measure = first->measure();
      if (!measure || measure != last->measure())
            return "tie crosses a measure boundary";
      if (!measure->system())
            return "measure has no system yet";
      if (first->staffMove() || last->staffMove() || first->isGrace() || last->isGrace()
          || first->isNudged() || last->isNudged()
          || (first->notes().size() == 1 && last->notes().size() == 1))
            return "unsupported chord context";
      const Staff* st = a->staff();
      const StaffType* type = st ? st->staffType(a->tick()) : nullptr;
      if (!type || !st->isPitchedStaff(a->tick()) || type != st->staffType(b->tick())
          || type->yoffset().val() != 0 || type->lineDistance().val() != 1)
            return "unsupported staff geometry";
      for (size_t i = 0; i < nsegments(); ++i)
            if (segmentAt(int(i))->isNudged() || segmentAt(int(i))->isEdited()) return "edited tie segment";

      // Use the real grip adjustments on a detached tie copy. Restrict the
      // probe to grips above the staff, where native layout does not snap
      // their height to staff lines as the span changes.
      Tie copy(*this);
      copy.calculateDirection();
      SlurPos positions;
      copy.slurPos(&positions);
      if (positions.p1.y() != positions.p2.y())
            return "grips are not level";
      if (!(positions.p1.y() < 0))
            return "grips are at or below the top staff line";
      const auto originX = [](const Chord* chord) {
            return chord->measure()->x() + chord->segment()->x();
            };
      TieSegment probe(score());
      probe.setSpanner(&copy);
      probe.setTrack(track());
      probe.setSpannerSegmentType(SpannerSegmentType::SINGLE);
      probe.ups(Grip::START).p = positions.p1 - QPointF(originX(first), 0);
      probe.ups(Grip::END).p = positions.p2 - QPointF(originX(last), 0);
      probe.adjustX();
      const qreal result = probe.ups(Grip::START).p.x() - probe.ups(Grip::END).p.x();
      if (!std::isfinite(result)) return "nonfinite grip inset";
      inset = result;
      return nullptr;
      }

qreal Tie::incomingEndOffset()
      {
      Note* note = startNote();
      if (!isPartial() || !isIncoming() || !note || !note->chord()->measure()->system())
            return 0;
      calculateDirection();
      SlurPos positions;
      slurPos(&positions);
      // A local, unregistered segment reuses the real landing/collision rule.
      TieSegment probe(score());
      probe.setSpanner(this);
      probe.setTrack(track());
      probe.setParent(positions.system2);
      probe.setSpannerSegmentType(SpannerSegmentType::SINGLE);
      if (!spannerSegments().empty())
            probe.ups(Grip::END).off = frontSegment()->ups(Grip::END).off;
      qreal freeX = note->chord()->measure()->x() + incomingStartOffset();
      if (freeX >= positions.p2.x())
            freeX = positions.p2.x() - qMax(.1 * spatium(), score()->styleP(Sid::minHangingTieLength));
      // Spacing must use the same final-height head clearance as drawing.
      probe.layoutSegment(QPointF(freeX, positions.p2.y()), positions.p2);
      probe.adjustX();
      const Chord* chord = note->chord();
      const qreal result = probe.ups(Grip::END).pos().x() - chord->segment()->x() - chord->measure()->x();
      probe.setParent(nullptr);
      return result;
      }

TieSegment* Tie::layoutPartial(System* system)
      {
      Note* note = startNote(); // owner for either direction
      if (!note || !system)
            return nullptr;
      setTick(note->tick());
      if (staff()->isTabStaff(tick()) && !staff()->staffType(tick())->showBackTied()) {
            eraseSpannerSegments();
            return nullptr;
            }
      const Chord* chord = note->chord();
      const Measure* measure = chord->measure();
      calculateDirection();
      SlurPos positions;
      slurPos(&positions);
      const QPointF anchor = isIncoming() ? positions.p2 : positions.p1;
      const qreal sp = spatium();
      const qreal y = anchor.y();
      qreal left = anchor.x();
      qreal right = left;
      if (isIncoming())
            left = measure->x() + incomingStartOffset();
      else {
            // Preserve Studio's outgoing clearance from the next visible item.
            const int firstTrack = chord->vStaffIdx() * VOICES;
            const Segment* adjacent = chord->segment()->next1();
            while (adjacent && (!adjacent->enabled() || !adjacent->visible()
                   || adjacent->allElementsInvisible() || adjacent->isBreathType()
                   || !adjacent->hasElements(firstTrack, firstTrack + VOICES - 1)))
                  adjacent = adjacent->next1();
            if (adjacent) {
                  const Element* item = nullptr;
                  for (int track = firstTrack; track < firstTrack + VOICES && !item; ++track)
                        item = adjacent->element(track);
                  const Shape shape = item->shape();
                  const qreal x = adjacent->measure()->x() + adjacent->x() + item->x();
                  // Studio's default lineEndToBarlineDistance.
                  right = x + shape.left() - 0.25 * sp;
                  }
            }
      setPos(0, 0);
      fixupSegments(1);
      TieSegment* segment = segmentAt(0);
      segment->setSystem(system);
      segment->setSpannerSegmentType(SpannerSegmentType::SINGLE);
      segment->layoutSegment(QPointF(left, y), QPointF(right, y));
      segment->adjustX(); // only the note-owned endpoint, as for an ordinary tie
      segment->finalizeSegment();
      return segment;
      }

//---------------------------------------------------------
//   calculateDirection
//---------------------------------------------------------

static int compareNotesPos(const Note* n1, const Note* n2)
{
    if (n1->line() != n2->line()) {
        return n2->line() - n1->line();
    } else if (n1->string() != n2->string()) {
        return n2->string() - n1->string();
    } else {
        return n1->pitch() - n2->pitch();
    }
}

void Tie::calculateDirection()
      {
      if (!startNote() && !endNote()) {
          return;
      }
      const Note* source = isPartial() && isIncoming() ? incomingTieSource(startNote()) : startNote();
      const Note* destination = isPartial() && isIncoming() ? startNote() : endNote();
      const bool tieHasBothNotes = source && destination;

      const Note* primaryNote = source ? source : destination;
      Chord* primaryChord = primaryNote->chord();
      if (_slurDirection == Direction::AUTO && isPartial() && isIncoming() && source && !primaryChord->beam()
          && !primaryChord->staff()->isDrumStaff(primaryChord->tick())) {
            // A backwards repeat can refer to a later system whose automatic
            // stem has not been computed yet. Resolve an unbeamed source now;
            // leave beam-controlled and percussion directions to their layout.
            primaryChord->computeUp();
            }
      const Measure* primaryMeasure = primaryChord->measure();

      const Note* secondaryNote = tieHasBothNotes ? destination : nullptr;
      const Chord* secondaryChord = secondaryNote ? secondaryNote->chord() : nullptr;
      const Measure* secondaryMeasure = secondaryChord ? secondaryChord->measure() : nullptr;

      auto hasForwardTie = [](const Note* note) {
            if (note->tieFor() || note->partialTie(false))
                  return true;
            for (const Element* attached : note->el())
                  if (attached->isLaissezVib())
                        return true;
            return false;
            };
      if (_slurDirection == Direction::AUTO) {
            std::vector<Note*> notes = primaryChord->notes();
            size_t n = notes.size();
            // if there are multiple voices, the tie direction goes on stem side
            if (primaryMeasure->hasVoices(primaryChord->staffIdx(), primaryChord->tick(), primaryChord->actualTicks()))
                  _up = primaryChord->up();
            else if (tieHasBothNotes && secondaryMeasure && secondaryMeasure->hasVoices(secondaryChord->staffIdx(), secondaryChord->tick(), secondaryChord->actualTicks()))
                  _up = secondaryChord->up();
            else if (n == 1) {
                  //
                  // single note
                  //
                  if (tieHasBothNotes && primaryChord->up() != secondaryChord->up()) {
                        // if stem direction is mixed, always up
                        _up = true;
                        }
                  else
                        _up = !primaryChord->up();
                  }
            else {
                  //
                  // chords
                  //
                  // first, find pivot point in chord (below which all ties curve down and above which all ties curve up)
                  Note* pivotPoint = nullptr;
                  bool multiplePivots = false;
                  for (size_t i = 0; i < n - 1; ++i) {
                        if (!hasForwardTie(notes[i]))
                              continue; // don't include notes that don't have ties
                        for (size_t j = i + 1; j < n; ++j) {
                              if (!hasForwardTie(notes[j]))
                                    continue;
                              int noteDiff = compareNotesPos(notes[i], notes[j]);
                              if (!multiplePivots && qAbs(noteDiff) <= 1) {
                                    // TODO: Fix unison ties somehow--if noteDiff == 0 then we need to determine which of the unison is 'lower'
                                    if (pivotPoint) {
                                          multiplePivots = true;
                                          pivotPoint = nullptr;
                                          }
                                    else
                                          pivotPoint = noteDiff < 0 ? notes[i] : notes[j];
                                    }
                              }
                        }
                  if (!pivotPoint) {
                        // if the pivot point was not found (either there are no unisons/seconds or there are more than one),
                        // determine if this note is in the lower or upper half of this chord
                        int notesAbove = 0, tiesAbove = 0;
                        int notesBelow = 0, tiesBelow = 0;
                        int unisonTies = 0;
                        for (size_t i = 0; i < n; ++i) {
                              if (notes[i] == primaryNote)
                                    // skip counting if this note is the current note or if this note doesn't have a tie
                                    continue;
                              int noteDiff = compareNotesPos(primaryNote, notes[i]);
                              if (noteDiff == 0) {  // unison
                                    if (hasForwardTie(notes[i]))
                                          unisonTies++;
                                    }
                              if (noteDiff < 0) { // the note is above startNote
                                    notesAbove++;
                                    if (hasForwardTie(notes[i]))
                                          tiesAbove++;
                                    }
                              if (noteDiff > 0) { // the note is below startNote
                                    notesBelow++;
                                    if (hasForwardTie(notes[i]))
                                          tiesBelow++;
                                    }
                              }

                        if (tiesAbove == 0 && tiesBelow == 0 && unisonTies == 0) {
                              // this is the only tie in the chord.
                              if (notesAbove == notesBelow)
                                    _up = !primaryChord->up();
                              else
                                    _up = (notesAbove < notesBelow);
                              }
                        else if (tiesAbove == tiesBelow)
                              // this note is dead center, so its tie should go counter to the stem direction
                              _up = !primaryChord->up();
                        else
                              _up = (tiesAbove < tiesBelow);
                        }
                  else if (pivotPoint == primaryNote)
                        // the current note is the lower of the only second or unison in the chord; tie goes down.
                        _up = false;
                  else {
                        // if lower than the pivot, tie goes down, otherwise up
                        int noteDiff = compareNotesPos(primaryNote, pivotPoint);
                        _up = (noteDiff >= 0);
                        }
                  }
            }
      else
            _up = _slurDirection == Direction::UP ? true : false;
      }

//---------------------------------------------------------
//   layoutFor
//    layout the first SpannerSegment of a slur
//---------------------------------------------------------

TieSegment* Tie::layoutFor(System* system)
      {
      // do not layout ties in tablature if not showing back-tied fret marks
      StaffType* st = staff()->staffType(startNote() ? startNote()->tick() : Fraction(0, 1));
      if (st && st->isTabStaff() && !st->showBackTied()) {
            if (!segmentsEmpty())
                  eraseSpannerSegments();
            return nullptr;
            }
      //
      //    show short bow
      //
      if (startNote() == 0 || endNote() == 0) {
            if (startNote() == 0) {
                  qDebug("no start note");
                  return 0;
                  }
            Chord* c1 = startNote()->chord();
            if (_slurDirection == Direction::AUTO) {
                  if (c1->measure()->hasVoices(c1->staffIdx(), c1->tick(), c1->actualTicks())) {
                        // in polyphonic passage, ties go on the stem side
                        _up = c1->up();
                        }
                  else
                        _up = !c1->up();
                  }
            else
                  _up = _slurDirection == Direction::UP ? true : false;
            fixupSegments(1);
            TieSegment* segment = segmentAt(0);
            segment->setSpannerSegmentType(SpannerSegmentType::SINGLE);
            segment->setSystem(startNote()->chord()->segment()->measure()->system());
            SlurPos sPos;
            slurPos(&sPos);
            segment->layoutSegment(sPos.p1, sPos.p2);
            segment->finalizeSegment();
            return segment;
            }
      calculateDirection();

      SlurPos sPos;
      slurPos(&sPos); // get unadjusted x values and determine inside or outside

      setPos(0, 0);

      int n;
      if (sPos.system1 != sPos.system2) {
            n = 2;
            sPos.p2 = QPointF(qMax(system->lastNoteRestSegmentX(true),
                  sPos.p1.x() + score()->styleP(Sid::minHangingTieLength)), sPos.p1.y());
            }
      else
            n = 1;

      fixupSegments(n);
      TieSegment* segment = segmentAt(0);
      segment->setSystem(system); // Needed to populate System.spannerSegments
      segment->layoutSegment(sPos.p1, sPos.p2); // adjust vertically
      segment->setSpannerSegmentType(sPos.system1 != sPos.system2 ? SpannerSegmentType::BEGIN : SpannerSegmentType::SINGLE);
      segment->adjustX(); // adjust horizontally for inside-style ties

      segment->finalizeSegment(); // compute bezier and set bbox
      return segment;
      }

//---------------------------------------------------------
//   layoutBack
//    layout the second SpannerSegment of a split slur
//---------------------------------------------------------

TieSegment* Tie::layoutBack(System* system)
      {
      // do not layout ties in tablature if not showing back-tied fret marks
      StaffType* st = staff()->staffType(startNote() ? startNote()->tick() : Fraction(0, 1));
      if (st->isTabStaff() && !st->showBackTied()) {
            if (!segmentsEmpty())
                  eraseSpannerSegments();
            return nullptr;
            }

      SlurPos sPos;
      slurPos(&sPos);

      fixupSegments(2);
      TieSegment* segment = segmentAt(1);
      segment->setSystem(system);

      qreal x = qMin(system->firstNoteRestSegmentX(true),
            sPos.p2.x() - score()->styleP(Sid::minHangingTieLength));

      segment->layoutSegment(QPointF(x, sPos.p2.y()), sPos.p2);
      segment->setSpannerSegmentType(SpannerSegmentType::END);
      segment->adjustX();
      segment->finalizeSegment();
      return segment;
      }

#if 0
//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Tie::startEdit(EditData& ed)
      {
printf("tie start edit %p %p\n", editStartNote, editEndNote);
      editStartNote = startNote();
      editEndNote   = endNote();
      SlurTie::startEdit(ed);
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Tie::endEdit(EditData& ed)
      {
//printf("tie::endEdit\n");
//      if (editStartNote != startNote() || editEndNote != endNote()) {
//            score()->undoStack()->push1(new ChangeSpannerElements(this, editStartNote, editEndNote));
//            }
      SlurTie::endEdit(ed);
      }
#endif

//---------------------------------------------------------
//   setStartNote
//---------------------------------------------------------

void Tie::setStartNote(Note* note)
      {
      setStartElement(note);
      setParent(note);
      }

//---------------------------------------------------------
//   startNote
//---------------------------------------------------------

Note* Tie::startNote() const
      {
      Q_ASSERT(!startElement() || startElement()->type() == ElementType::NOTE);
      return toNote(startElement());
      }

//---------------------------------------------------------
//   endNote
//---------------------------------------------------------

Note* Tie::endNote() const
      {
      return toNote(endElement());
      }
}

