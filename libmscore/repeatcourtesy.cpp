//=============================================================================
//  MuseScore
//  Repeat/jump changes and courtesies, backported from MuseScore Studio.
//  Copyright (C) 2026 MuseScore contributors
//  SPDX-License-Identifier: GPL-2.0-or-later
//=============================================================================

#include "clef.h"
#include "keysig.h"
#include "layoutbreak.h"
#include "measure.h"
#include "repeatlist.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "stafftype.h"
#include "symbol.h"
#include "timesig.h"
#include "undo.h"
#include "xml.h"

namespace Ms {
namespace {

const Fraction epsilon = Fraction::fromTicks(1);

// The absolute tick does not change. In particular, do not remove/reinsert the
// children in the staff's musical maps when moving a segment across a barline.
class ChangeSignatureSegmentParent final : public UndoCommand {
      Segment* _segment;
      Measure* _parent;
      Fraction _rtick;
      SegmentType _type;
      void flip(EditData*) override
            {
            Measure* oldParent = _segment->measure();
            Fraction oldRtick = _segment->rtick();
            SegmentType oldType = _segment->segmentType();
            oldParent->remove(_segment);
            _segment->setRtick(_rtick);
            _segment->setSegmentType(_type);
            _parent->add(_segment);
            _parent = oldParent;
            _rtick = oldRtick;
            _type = oldType;
            }
   public:
      ChangeSignatureSegmentParent(Segment* s, Measure* m, const Fraction& rtick, SegmentType type = SegmentType::Invalid)
         : _segment(s), _parent(m), _rtick(rtick), _type(type == SegmentType::Invalid ? s->segmentType() : type) {}
      UNDO_NAME("ChangeSignatureSegmentParent")
      };

// Merge changes on different staves without changing their absolute tick or links.
class ChangeSignatureElementSegment final : public UndoCommand {
      Element* _element;
      Segment* _other;
      void flip(EditData*) override
            {
            Segment* old = toSegment(_element->parent());
            old->remove(_element);
            _element->setParent(_other);
            _other->add(_element);
            _other->setEnabled(true);
            if (old->empty()) old->setEnabled(false);
            _other = old;
            }
   public:
      ChangeSignatureElementSegment(Element* element, Segment* target) : _element(element), _other(target) {}
      UNDO_NAME("ChangeSignatureElementSegment")
      };

// Keep symbol ownership/scanning, but use Studio's tapered cubic geometry.
class CourtesyParenthesis final : public Symbol {
      qreal _height;
      QPainterPath _path;
      Shape _shape;
   public:
      CourtesyParenthesis(Score* score, SymId sym, qreal height)
         : Symbol(score, ElementFlag::ON_STAFF | ElementFlag::MOVABLE), _height(height)
            {
            setSym(sym);
            setGenerated(true);
            setIsCourtesy(true);
            }
      CourtesyParenthesis* clone() const override { return new CourtesyParenthesis(*this); }
      void setHeight(qreal height) { _height = height; }
      void layout() override
            {
            const qreal sp = spatium();
            const qreal height = qMax(sp * 0.1, _height);
            const qreal direction = sym() == SymId::timeSigParensLeft ? -1.0 : 1.0;
            // Studio evaluates the empirical powers in 1200-DPI coordinates.
            const qreal studioScale = 1200.0 / DPI;
            const qreal shoulder = qMax(0.25 * sp * mag(),
                  0.2 * qPow(height * studioScale, 0.95) * qPow(mag(), 0.1) / studioScale);
            const qreal thickness = qMin(0.2 * sp, height / 60.0 * mag());
            const qreal forward = direction * (shoulder + thickness);
            const qreal back = direction * (shoulder - thickness);
            _path = QPainterPath();
            _path.moveTo(0.0, 0.0);
            _path.cubicTo(QPointF(forward, 0.2 * height), QPointF(forward, 0.8 * height), QPointF(0.0, height));
            _path.cubicTo(QPointF(back, 0.8 * height), QPointF(back, 0.2 * height), QPointF(0.0, 0.0));
            _path.closeSubpath();
            // Courtesy parentheses retain Parenthesis::PARENTHESIS_END_WIDTH
            // (0.10sp) in Studio; the 0.05sp value belongs to note parentheses.
            const qreal halfOutline = 0.05 * sp;
            setbbox(_path.boundingRect().adjusted(-halfOutline, -halfOutline, halfOutline, halfOutline));
            // Preserve the curved contour for spacing instead of reserving
            // the whole rectangular box around a parenthesis.
            _shape = Shape();
            QPainterPath middle;
            middle.moveTo(0.0, 0.0);
            middle.cubicTo(QPointF(direction * shoulder, 0.2 * height),
                  QPointF(direction * shoulder, 0.8 * height), QPointF(0.0, height));
            const int count = qBound(3, qRound(2.0 * height / sp), 10);
            QPointF previous;
            for (int i = 1; i <= count; ++i) {
                  const qreal sine = qSin(1.5707963267948966 * i / count);
                  const qreal t = sine * sine;
                  const QPointF point = middle.pointAtPercent(t);
                  QRectF rect = QRectF(previous, point).normalized();
                  const qreal inkWidth = 2.0 * thickness * (1.0 - 2.0 * qAbs(0.5 - t));
                  if (rect.width() < inkWidth) {
                        const qreal extra = (inkWidth - rect.width()) * 0.5;
                        rect.adjust(-extra, 0.0, extra, 0.0);
                        }
                  _shape.add(rect);
                  previous = point;
                  }
            }
      Shape shape() const override { return _shape; }
      void draw(QPainter* painter) const override
            {
            painter->save();
            painter->setPen(QPen(curColor(), 0.1 * spatium(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(curColor());
            painter->drawPath(_path);
            painter->restore();
            }
      };

bool sameMeterAppearance(const TimeSig* a, const TimeSig* b)
      {
      return a && b && a->sig().identical(b->sig())
            && a->timeSigType() == b->timeSigType()
            && a->numeratorString() == b->numeratorString()
            && a->denominatorString() == b->denominatorString();
      }

bool hasRealElements(const Segment* seg)
      {
      if (seg) {
            for (const Element* e : seg->elist()) {
                  if (e && !e->generated())
                        return true;
                  }
            }
      return false;
      }

bool mergeRealSignatureSegments(Score* score, Segment* from, Segment* to)
      {
      // A conflicting real change on the same track cannot be merged safely.
      for (Element* e : from->elist()) {
            Element* occupied = e ? to->element(e->track()) : nullptr;
            if (e && !e->generated() && occupied && !occupied->generated()) return false;
            }
      const auto elements = from->elist();
      for (Element* e : elements) {
            if (!e || e->generated()) continue;
            if (Element* occupied = to->element(e->track())) score->undo(new RemoveElement(occupied));
            score->undo(new ChangeSignatureElementSegment(e, to));
            }
      // Keep the empty source attached: older undo commands can still refer to
      // this segment after a layout performed outside an active command.
      from->setEnabled(false);
      return true;
      }

Element* realSignature(Measure* m, SegmentType type, const Fraction& tick, int track)
      {
      if (!m)
            return nullptr;
      Element* result = nullptr;
      for (Segment* seg = m->first(); seg; seg = seg->next()) {
            if (seg->tick() == tick && seg->isType(type)) {
                  Element* e = seg->element(track);
                  if (e && !e->generated()) result = e;
                  }
            }
      return result;
      }

// Clef/key/time changes may be stored on either side of their barline.
Element* signatureAt(Score* score, SegmentType type, const Fraction& tick, int track)
      {
      Measure* m = score->tick2measure(tick);
      Element* e = realSignature(m, type, tick, track);
      if (!e && m)
            e = realSignature(m->prevMeasure(), type, tick, track);
      return e;
      }

bool showCourtesy(const Element* e)
      {
      if (!e || !e->visible())
            return false;
      if (e->isClef())
            return toClef(e)->showCourtesy();
      if (e->isKeySig())
            return toKeySig(e)->showCourtesy();
      if (e->isTimeSig())
            return toTimeSig(e)->showCourtesySig();
      return false;
      }

void clearCourtesySegment(Segment* seg)
      {
      if (!seg) return;
      // Keep detached display objects alive: undo commands and selection can
      // refer to them after any number of layout passes.
      const auto elements = seg->elist();
      for (Element* e : elements) {
            if (!e) continue;
            seg->measure()->cacheRepeatCourtesy(e);
            seg->remove(e);
            }
      const auto annotations = seg->annotations();
      for (Element* e : annotations) {
            seg->measure()->cacheRepeatCourtesy(e);
            seg->remove(e);
            }
      seg->setEnabled(false);
      seg->createShapes();
      }

Element* addCourtesy(Measure* m, Element* element, SegmentType type, bool continuation, int track)
      {
      const Fraction rtick = continuation ? Fraction(0,1) : m->ticks();
      Segment* seg = m->getSegmentR(type, rtick);
      if (Element* retained = m->takeRepeatCourtesy(seg, track)) {
            // Refresh the musical appearance, retaining the display object's
            // identity, offset and other user presentation properties.
            if (element->isClef()) {
                  toClef(retained)->setClefType(toClef(element)->clefTypeList());
                  toClef(retained)->setSmall(true);
                  }
            else if (element->isKeySig())
                  toKeySig(retained)->setKeySigEvent(toKeySig(element)->keySigEvent());
            else if (element->isTimeSig()) {
                  toTimeSig(retained)->setFrom(toTimeSig(element));
                  toTimeSig(retained)->setStretch(toTimeSig(element)->stretch());
                  toTimeSig(retained)->setGroups(toTimeSig(element)->groups());
                  }
            delete element;
            element = retained;
            }
      else {
            element->setTrack(track);
            element->setParent(seg);
            element->setGenerated(true);
            element->setIsCourtesy(true);
            element->setSelected(false);
            // Source offsets belong to the original, not this display copy.
            element->setOffset(QPointF());
            m->restoreRepeatCourtesyOffset(element);
            }
      element->setFlag(ElementFlag::NOT_SELECTABLE, false);
      element->setFlag(ElementFlag::MOVABLE, true);
      seg->setEnabled(true);
      seg->setTrailer(false);
      seg->add(element);
      element->layout();
      return element;
      }

// Studio's mid-measure clef shapes honor the font's SMuFL corner cutouts.
// Limit that tighter outline to courtesy-parenthesis fitting here.
Shape courtesyAnchorShape(const Element* element)
      {
      if (!element->isClef() || toSegment(element->parent())->rtick().isZero())
            return element->shape();
      const Clef* clef = toClef(element);
      const SymId id = ClefInfo::symId(clef->clefType());
      const ScoreFont* font = clef->score()->scoreFont();
      const qreal mag = clef->magS();
      const QRectF box = clef->bbox();
      const QPointF corners[] = {font->cutOutNW(id, mag), font->cutOutNE(id, mag),
                                 font->cutOutSW(id, mag), font->cutOutSE(id, mag)};
      std::vector<qreal> edges {box.top(), box.bottom()};
      for (const QPointF& corner : corners) {
            if (!corner.isNull() && corner.y() > box.top() && corner.y() < box.bottom())
                  edges.push_back(corner.y());
            }
      std::sort(edges.begin(), edges.end());
      edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
      Shape result;
      for (unsigned i = 1; i < edges.size(); ++i) {
            const qreal middle = (edges[i - 1] + edges[i]) * 0.5;
            qreal left = box.left(), right = box.right();
            if (!corners[0].isNull() && middle < corners[0].y()) left = qMax(left, corners[0].x());
            if (!corners[1].isNull() && middle < corners[1].y()) right = qMin(right, corners[1].x());
            if (!corners[2].isNull() && middle > corners[2].y()) left = qMax(left, corners[2].x());
            if (!corners[3].isNull() && middle > corners[3].y()) right = qMin(right, corners[3].x());
            if (right > left)
                  result.add(QRectF(left, edges[i - 1], right - left, edges[i] - edges[i - 1]));
            }
      return result.empty() ? element->shape() : result;
      }

void addParentheses(Measure* m, bool continuation)
      {
      const SegmentType types[] = {
            continuation ? SegmentType::ClefStartRepeatAnnounce : SegmentType::ClefRepeatAnnounce,
            continuation ? SegmentType::KeySigStartRepeatAnnounce : SegmentType::KeySigRepeatAnnounce,
            continuation ? SegmentType::TimeSigStartRepeatAnnounce : SegmentType::TimeSigRepeatAnnounce
            };
      const Fraction rtick = continuation ? Fraction(0,1) : m->ticks();
      const bool separateMeter = m->score()->styleI(Sid::timeSigPlacement) != int(TimeSigPlacement::NORMAL);
      for (int track = 0; track < m->score()->ntracks(); track += VOICES) {
        for (int group = 0; group < (separateMeter ? 2 : 1); ++group) {
            Element* first = nullptr;
            Element* last = nullptr;
            for (SegmentType type : types) {
                  Segment* seg = m->findSegmentR(type, rtick);
                  Element* e = seg && seg->enabled() ? seg->element(track) : nullptr;
                  if (!e || !e->visible() || e->bbox().isEmpty())
                        continue;
                  if (separateMeter && e->isTimeSig() != (group == 1)) continue;
                  if (!first)
                        first = e;
                  last = e;
                  }
            if (!first)
                  continue;
            // MeasureLayout::calcParenTopBottom() in Studio considers only
            // the two endpoint items, not intervening key signatures.
            const QRectF bounds = first->bbox().translated(first->pos())
                  | last->bbox().translated(last->pos());
            qreal sp = first->spatium();
            qreal top = bounds.top() + 0.25 * sp;
            qreal bottom = bounds.bottom() - 0.25 * sp;
            if (!m->score()->styleB(Sid::smallParens)) {
                  top = group == 1 ? bounds.top() - sp : -sp;
                  const Staff* staff = m->score()->staff(track / VOICES);
                  const Fraction at = m->tick() + rtick;
                  const qreal staffHeight = (staff->lines(at) - 1) * staff->lineDistance(at) * sp;
                  bottom = group == 1 ? bounds.bottom() + sp : staffHeight + sp;
                  }
            for (int side = 0; side < 2; ++side) {
                  Element* anchor = side ? last : first;
                  Segment* seg = toSegment(anchor->parent());
                  auto paren = static_cast<CourtesyParenthesis*>(m->takeRepeatCourtesy(seg, track, side ? 2 : 1));
                  if (!paren) {
                        paren = new CourtesyParenthesis(m->score(), side ? SymId::timeSigParensRight : SymId::timeSigParensLeft,
                                                      bottom - top);
                        paren->setTrack(track);
                        paren->setParent(seg);
                        m->restoreRepeatCourtesyOffset(paren);
                        }
                  paren->setHeight(bottom - top);
                  paren->setFlag(ElementFlag::NOT_SELECTABLE, false);
                  paren->setFlag(ElementFlag::MOVABLE, true);
                  paren->setTrack(track);
                  paren->setMag(qMax(first->mag(), last->mag()));
                  paren->setParent(seg);
                  paren->layout();
                  QRectF ar = anchor->bbox().translated(anchor->pos());
                  const qreal gap = (side ? 0.25 : anchor->isKeySig() ? 0.35 : 0.2)
                        * m->score()->spatium() * 0.5 * (paren->mag() + anchor->mag());
                  qreal x = side ? ar.right() + gap - paren->bbox().left()
                                 : ar.left() - gap - paren->bbox().right();
                  // top denotes the path endpoint, before the round outline.
                  const qreal y = top;
                  const Shape itemShape = courtesyAnchorShape(anchor).translated(anchor->pos());
                  const Shape parenShape = paren->shape().translated(QPointF(0.0, y));
                  const qreal distance = side ? itemShape.minHorizontalDistance(parenShape)
                                              : parenShape.minHorizontalDistance(itemShape);
                  if (qIsFinite(distance) && qAbs(distance) < 1000.0 * sp)
                        x = side ? distance + gap : -distance - gap;
                  paren->setPos(x, y);
                  seg->add(paren);
                  }
          }
        }
      }

} // namespace

namespace {
int repeatCourtesyRole(const Element* e)
      {
      if (!e->isSymbol()) return 0;
      return toSymbol(e)->sym() == SymId::timeSigParensLeft ? 1 : 2;
      }
QString repeatCourtesyKey(const Segment* segment, int track, int role)
      {
      return QString("%1:%2:%3").arg(segment->subTypeName()).arg(track).arg(role);
      }
QString repeatCourtesyKey(const Element* e)
      {
      return repeatCourtesyKey(toSegment(e->parent()), e->track(), repeatCourtesyRole(e));
      }
}

void Measure::cacheRepeatCourtesy(Element* element)
      {
      const QString key = repeatCourtesyKey(element);
      Q_ASSERT(!_repeatCourtesyCache.count(key));
      _repeatCourtesyCache[key] = element;
      element->setFlag(ElementFlag::NOT_SELECTABLE, true);
      }

Element* Measure::takeRepeatCourtesy(Segment* segment, int track, int role)
      {
      const auto it = _repeatCourtesyCache.find(repeatCourtesyKey(segment, track, role));
      if (it == _repeatCourtesyCache.end()) return nullptr;
      Element* element = it->second;
      _repeatCourtesyCache.erase(it);
      element->setParent(segment);
      element->setTrack(track);
      return element;
      }

void Measure::restoreRepeatCourtesyOffset(Element* element) const
      {
      const auto it = _repeatCourtesyOffsets.find(repeatCourtesyKey(element));
      if (it != _repeatCourtesyOffsets.end())
            element->setOffset(it->second * element->spatium());
      }

std::map<QString, QPointF> Measure::repeatCourtesyOffsets() const
      {
      auto offsets = _repeatCourtesyOffsets;
      for (const auto& entry : _repeatCourtesyCache)
            offsets[entry.first] = entry.second->offset() / entry.second->spatium();
      for (const Segment& segment : segments()) {
            if (!segment.isType(SegmentType::RepeatCourtesy | SegmentType::ContinuationCourtesy)) continue;
            for (const Element* e : segment.elist()) {
                  if (e && e->isCourtesy()) offsets[repeatCourtesyKey(e)] = e->offset() / e->spatium();
                  }
            for (const Element* e : segment.annotations()) {
                  if (e && e->isCourtesy()) offsets[repeatCourtesyKey(e)] = e->offset() / e->spatium();
                  }
            }
      return offsets;
      }

void Measure::writeRepeatCourtesyOffsets(XmlWriter& xml, int staff) const
      {
      for (const auto& entry : repeatCourtesyOffsets()) {
            if (entry.second.isNull()) continue;
            const QStringList key = entry.first.split(':');
            if (key.size() != 3) continue;
            const int track = key[1].toInt();
            if (track / VOICES != staff) continue;
            xml.tagE(QString("courtesyOffset type=\"%1\" voice=\"%2\" role=\"%3\" x=\"%4\" y=\"%5\"")
                  .arg(key[0]).arg(track % VOICES).arg(key[2])
                  .arg(entry.second.x(), 0, 'g', 16).arg(entry.second.y(), 0, 'g', 16));
            }
      }

void Measure::readRepeatCourtesyOffset(XmlReader& xml, int staff)
      {
      const QString type = xml.attribute("type");
      bool validType = false;
      for (SegmentType t : {SegmentType::ClefRepeatAnnounce, SegmentType::KeySigRepeatAnnounce,
                           SegmentType::TimeSigRepeatAnnounce, SegmentType::ClefStartRepeatAnnounce,
                           SegmentType::KeySigStartRepeatAnnounce, SegmentType::TimeSigStartRepeatAnnounce})
            validType |= type == QLatin1String(Segment::subTypeName(t));
      const int voice = xml.intAttribute("voice", 0);
      const int role = xml.intAttribute("role", 0);
      const qreal x = xml.doubleAttribute("x", 0.0);
      const qreal y = xml.doubleAttribute("y", 0.0);
      if (validType && voice >= 0 && voice < VOICES && role >= 0 && role <= 2
          && qIsFinite(x) && qIsFinite(y)) {
            const QString key = QString("%1:%2:%3").arg(type).arg(staff * VOICES + voice).arg(role);
            _repeatCourtesyOffsets[key] = QPointF(x, y);
            }
      xml.skipCurrentElement();
      }

void Measure::deselectInactiveRepeatCourtesies()
      {
      for (const auto& entry : _repeatCourtesyCache) {
            if (entry.second->selected()) score()->deselect(entry.second);
            }
      }

std::vector<Measure*> Score::followingRepeatMeasures(const Measure* m) const
      {
      std::vector<Measure*> result;
      auto it = _repeatCourtesyDestinations.find(m->endTick().ticks());
      if (it != _repeatCourtesyDestinations.end()) {
            for (int tick : it->second) {
                  Measure* target = tick2measure(Fraction::fromTicks(tick));
                  if (target)
                        result.push_back(target);
                  }
            }
      return result;
      }

// Editing/rewrite code in 3.x expects real key/meter changes at the start
// of their semantic measure. Restore that representation before such commands.
void Score::restoreSignatureSegments()
      {
      for (Score* sc : scoreList()) {
            for (Measure* m = sc->firstMeasure(); m; m = m->nextMeasure()) {
                  Measure* next = m->nextMeasure();
                  if (!next) continue;
                  for (SegmentType type : {SegmentType::KeySig, SegmentType::TimeSig}) {
                        Segment* seg = m->findSegmentR(type, m->ticks());
                        if (!hasRealElements(seg)) continue;
                        Segment* occupied = next->findSegmentR(type, Fraction(0,1));
                        if (occupied) {
                              if (hasRealElements(occupied)) {
                                    mergeRealSignatureSegments(sc, seg, occupied);
                                    continue;
                                    }
                              mergeRealSignatureSegments(sc, seg, occupied);
                              continue;
                              }
                        sc->undo(new ChangeSignatureSegmentParent(seg, next, Fraction(0,1)));
                        }
                  }
            }
      }

void Score::prepareRepeatCourtesies()
      {
      // Undoing a meter rewrite can restore whole segments without invoking
      // Segment::add() for their children. Refresh the musical meter map before
      // deciding placement; the normal layout rebuild runs later in the pass.
      for (Staff* st : staves()) st->clearTimeSig();
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            for (Segment& seg : m->segments()) {
                  if (!seg.isTimeSigType()) continue;
                  for (Element* e : seg.elist()) {
                        if (e && !e->isCourtesy()) e->staff()->addTimeSig(toTimeSig(e));
                        }
                  }
            }
      _repeatCourtesyDestinations.clear();
      // A private expanded traversal never changes the transport's Repeat toggle
      // or replaces the sequencer's cached traversal.
      RepeatList expanded(masterScore());
      expanded.update(true);
      for (int i = 0; i + 1 < expanded.size(); ++i) {
            const Measure* from = expanded[i]->lastMeasure();
            const Measure* to = expanded[i + 1]->firstMeasure();
            if (!from || !to || from->nextMeasure() == to)
                  continue;
            auto& targets = _repeatCourtesyDestinations[from->endTick().ticks()];
            int tick = to->tick().ticks();
            if (std::find(targets.begin(), targets.end(), tick) == targets.end())
                  targets.push_back(tick);
            }

      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            Measure* next = m->nextMeasure();
            if (!next)
                  continue;
            const auto targets = followingRepeatMeasures(m);
            const bool before = (m->repeatEnd() && styleB(Sid::changesBeforeBarlineRepeats))
                             || (!m->repeatEnd() && !targets.empty() && styleB(Sid::changesBeforeBarlineOtherJumps));
            auto appliesBoth = [&](Segment* seg) {
                  if (!hasRealElements(seg) || targets.empty())
                        return false;
                  bool changed = false;
                  for (int track = 0; track < ntracks(); track += VOICES) {
                        Element* e = seg->element(track);
                        if (!e || e->generated())
                              continue;
                        Staff* st = staff(track / VOICES);
                        for (Measure* target : targets) {
                              if (seg->isKeySigType()) {
                                    const KeySigEvent value = st->keySigEvent(seg->tick());
                                    if (!(value == st->keySigEvent(target->tick()))) return false;
                                    changed |= !(st->keySigEvent(seg->tick() - epsilon) == value);
                                    }
                              else if (seg->isTimeSigType()) {
                                    const TimeSig* value = st->timeSig(seg->tick());
                                    const TimeSig* dest = st->timeSig(target->tick());
                                    const TimeSig* prev = st->timeSig(seg->tick() - epsilon);
                                    if (!sameMeterAppearance(value, dest)) return false;
                                    changed |= prev && !sameMeterAppearance(prev, value);
                                    }
                              else {
                                    if (st->clef(seg->tick()) != st->clef(target->tick())) return false;
                                    changed |= st->clef(seg->tick() - epsilon) != st->clef(seg->tick());
                                    }
                              }
                        }
                  return changed;
                  };
            for (SegmentType type : {SegmentType::Clef, SegmentType::KeySig, SegmentType::TimeSig}) {
                  Segment* end = m->findSegmentR(type, m->ticks());
                  Segment* start = next->findSegmentR(type, Fraction(0,1));

                  if (type == SegmentType::Clef && !hasRealElements(start))
                        start = next->findSegmentR(SegmentType::HeaderClef, Fraction(0,1));

                  if (type == SegmentType::Clef && !m->repeatEnd() && targets.empty()) {
                        // Ordinary barlines retain the imported side of the clef.
                        // Reparenting here changes native serialization and loses
                        // MusicXML's explicit after-barline placement.
                        bool explicitPosition = false;
                        for (const Segment* candidate : {end, start}) {
                              if (!candidate) continue;
                              for (const Element* e : candidate->elist()) {
                                    if (e && !e->generated() && toClef(e)->clefToBarlinePosition())
                                          explicitPosition = true;
                                    }
                              }
                        if (!explicitPosition)
                              continue;
                        }

                  // Imported changes may be split across barline sides on different
                  // staves. Combine them at their semantic start before placement.
                  if (hasRealElements(end) && hasRealElements(start)
                      && mergeRealSignatureSegments(this, end, start))
                        end = nullptr;
                  Segment* seg = hasRealElements(end) ? end : hasRealElements(start) ? start : nullptr;
                  if (!seg)
                        continue;
                  bool moveBefore = before && appliesBoth(seg);
                  if (type == SegmentType::Clef) {
                        int overridePos = 0;
                        for (Element* e : seg->elist()) {
                              if (e && !e->generated() && toClef(e)->clefToBarlinePosition()) {
                                    overridePos = toClef(e)->clefToBarlinePosition();
                                    break;
                                    }
                              }
                        moveBefore |= !m->repeatEnd() || styleB(Sid::placeClefsBeforeRepeats);
                        if (overridePos) moveBefore = overridePos == 1;
                        }
                  Measure* parent = moveBefore ? m : next;
                  Fraction rtick = moveBefore ? m->ticks() : Fraction(0,1);
                  // A real header clef already on the following side is full
                  // size in Studio. Do not turn it into a small change clef.
                  const SegmentType placedType = type == SegmentType::Clef && !moveBefore && seg->isHeaderClefType()
                        ? SegmentType::HeaderClef : type;
                  if (seg->measure() == parent && seg->segmentType() == placedType)
                        continue;
                  // Generated headers can occupy a target slot from an earlier layout.
                  Segment* occupied = parent->findSegmentR(placedType, rtick);
                  if (occupied) {
                        if (hasRealElements(occupied))
                              continue; // never discard a real change from an imported score
                        mergeRealSignatureSegments(this, seg, occupied);
                        continue;
                        }
                  undo(new ChangeSignatureSegmentParent(seg, parent, rtick, placedType));
                  }
            m->sortRepeatSegments();
            }
      }

void Measure::layoutCourtesyParentheses()
      {
      bool repeat = false, continuation = false;
      for (Segment& seg : segments()) {
            if (!seg.isType(SegmentType::RepeatCourtesy | SegmentType::ContinuationCourtesy)) continue;
            const auto annotations = seg.annotations();
            for (Element* e : annotations) {
                  if (e->isCourtesy() && e->isSymbol()) {
                        cacheRepeatCourtesy(e);
                        seg.remove(e);
                        }
                  }
            if (seg.enabled()) {
                  if (seg.isType(SegmentType::RepeatCourtesy))
                        repeat = true;
                  else
                        continuation = true;
                  }
            }
      if (repeat && score()->styleB(repeatEnd() ? Sid::useParensRepeatCourtesies : Sid::useParensOtherJumpCourtesies))
            addParentheses(this, false);
      const Measure* prev = prevMeasureMM();
      if (continuation && prev && score()->styleB(prev->repeatEnd() ? Sid::useParensRepeatCourtesiesAfterCancelling : Sid::useParensOtherJumpCourtesiesAfterCancelling))
            addParentheses(this, true);
      deselectInactiveRepeatCourtesies();
      }

void Measure::sortRepeatSegments()
      {
      std::vector<Segment*> sorted;
      for (Segment* s = first(); s; s = s->next())
            sorted.push_back(s);
      auto rank = [&](const Segment* s) {
            int value = int(s->segmentType());
            const bool atEnd = s->rtick() == ticks();
            if (atEnd) {
                  // Actual changes precede destination courtesies and the barline.
                  if (s->isClefType())
                        return 1;
                  if (s->isKeySigType())
                        return 2;
                  if (s->isTimeSigType())
                        return 3;
                  }
            if (s->rtick().isZero()) {
                  // A repeated system-header key belongs beside the header
                  // clef, before any cancelling courtesy. In 3.x the backported
                  // continuation segment values precede KeySig numerically.
                  if (s->header() && s->isKeySigType())
                        return int(SegmentType::HeaderClef) + 1;
                  const Measure* prev = prevMeasure();
                  bool joined = prev && prev->repeatEnd() && repeatStart();
                  bool between = score()->styleB(Sid::changesBetweenEndStartRepeat);
                  if (joined && !between && hasRealElements(s)
                      && s->isType(SegmentType::Clef | SegmentType::HeaderClef | SegmentType::KeySig | SegmentType::TimeSig))
                        return int(SegmentType::StartRepeatBarLine)
                              + (s->isType(SegmentType::Clef | SegmentType::HeaderClef) ? 1 : s->isKeySigType() ? 2 : 3);
                  // At the score start a second clef follows the meter so the
                  // legacy reader can distinguish it from the initial header.
                  if (s->isClefType() && tick().isNotZero())
                        return int(SegmentType::KeySig) - 1;
                  }
            return value;
            };
      std::stable_sort(sorted.begin(), sorted.end(), [&](Segment* a, Segment* b) {
            if (a->rtick() != b->rtick())
                  return a->rtick() < b->rtick();
            return rank(a) < rank(b);
            });
      for (Segment* s : sorted)
            _segments.remove(s);
      for (Segment* s : sorted)
            _segments.push_back(s);
      }

void Measure::layoutRepeatCourtesies()
      {
      for (Segment* s = first(); s; s = s->next()) {
            if (s->isType(SegmentType::RepeatCourtesy | SegmentType::ContinuationCourtesy))
                  clearCourtesySegment(s);
            }
      Score* sc = score();
      const auto targets = sc->followingRepeatMeasures(this);
      const bool showRepeat = repeatEnd() && sc->styleB(Sid::showCourtesiesRepeats);
      const bool showJump = !repeatEnd() && !targets.empty() && sc->styleB(Sid::showCourtesiesOtherJumps);
      bool parens = showRepeat ? sc->styleB(Sid::useParensRepeatCourtesies)
                               : sc->styleB(Sid::useParensOtherJumpCourtesies);

      auto generate = [&](Measure* target, bool continuation) {
            const Fraction at = continuation ? tick() : endTick();
            const Fraction ref = target->tick();
            Measure* prev = prevMeasureMM();
            const SegmentType clefType = continuation ? SegmentType::ClefStartRepeatAnnounce : SegmentType::ClefRepeatAnnounce;
            const SegmentType keyType = continuation ? SegmentType::KeySigStartRepeatAnnounce : SegmentType::KeySigRepeatAnnounce;
            const SegmentType timeType = continuation ? SegmentType::TimeSigStartRepeatAnnounce : SegmentType::TimeSigRepeatAnnounce;
            auto previousCourtesy = [&](SegmentType type, int track) -> Element* {
                  Segment* seg = prev ? prev->findSegmentR(type, prev->ticks()) : nullptr;
                  return seg && seg->enabled() ? seg->element(track) : nullptr;
                  };
            for (int track = 0; track < sc->ntracks(); track += VOICES) {
                  Staff* st = sc->staff(track / VOICES);
                  const Fraction oldTick = at - epsilon;
                  const ClefType clef = st->clef(ref);
                  const Fraction clefTick = st->currentClefTick(ref);
                  Element* actualClef = signatureAt(sc, SegmentType::Clef | SegmentType::HeaderClef, clefTick, track);
                  Element* actualKey = signatureAt(sc, SegmentType::KeySig, Fraction::fromTicks(st->keyList()->currentKeyTick(ref.ticks())), track);
                  const TimeSig* actualTime = st->timeSig(ref);
                  Element* prevClef = previousCourtesy(SegmentType::ClefRepeatAnnounce, track);
                  Element* prevKey = previousCourtesy(SegmentType::KeySigRepeatAnnounce, track);
                  Element* prevTime = previousCourtesy(SegmentType::TimeSigRepeatAnnounce, track);
                  const bool needClef = continuation ? prevClef && toClef(prevClef)->clefType() != clef
                                                    : st->clef(oldTick) != clef;
                  const bool needKey = continuation ? prevKey && !(toKeySig(prevKey)->keySigEvent() == st->keySigEvent(ref))
                                                   : !(st->keySigEvent(oldTick) == st->keySigEvent(ref));
                  const TimeSig* oldTime = continuation ? (prevTime ? toTimeSig(prevTime) : nullptr) : st->timeSig(oldTick);
                  const bool needTime = actualTime && oldTime && !sameMeterAppearance(actualTime, oldTime);
                  // Suppress only for a real change at the visual location. A following
                  // change at the same tick must not suppress a pre-repeat destination.
                  auto realAtLocation = [&](SegmentType type) -> Element* {
                        Element* e = realSignature(this, type, at, track);
                        if (!e && continuation && prev)
                              e = realSignature(prev, type, at, track);
                        if (!e && continuation) {
                              for (Segment* seg = first(); seg && seg->tick() == at; seg = seg->next()) {
                                    if (seg->header() && seg->enabled() && seg->isType(type) && seg->element(track))
                                          e = seg->element(track);
                                    }
                              }
                        return e;
                        };
                  auto already = [&](SegmentType type) {
                        Segment* s = findSegment(type, at);
                        return s && s->element(track);
                        };
                  if (needClef && sc->genCourtesyClef() && (!actualClef || showCourtesy(actualClef))
                      && !realAtLocation(SegmentType::Clef | SegmentType::HeaderClef) && !already(clefType)) {
                        auto c = actualClef ? toClef(actualClef)->clone() : new Clef(sc);
                        if (!actualClef) {
                              c->setConcertClef(st->clefType(ref)._concertClef);
                              c->setTransposingClef(st->clefType(ref)._transposingClef);
                              }
                        c->setSmall(true);
                        addCourtesy(this, c, clefType, continuation, track);
                        }
                  // Implicit C major has no KeySig object, but still needs cancelling naturals.
                  if (needKey && sc->styleB(Sid::genCourtesyKeysig) && (!actualKey || showCourtesy(actualKey))
                      && st->isPitchedStaff(ref) && !realAtLocation(SegmentType::KeySig) && !already(keyType)) {
                        auto k = new KeySig(sc);
                        k->setKeySigEvent(st->keySigEvent(ref));
                        auto display = toKeySig(addCourtesy(this, k, keyType, continuation, track));
                        display->setCourtesyPreviousKey(continuation && prevKey ? toKeySig(prevKey)->key() : Key(100));
                        display->layout();
                        }
                  if (needTime && sc->genCourtesyTimesig() && actualTime->showCourtesySig() && actualTime->visible()
                      && !realAtLocation(SegmentType::TimeSig) && !already(timeType)) {
                        auto t = actualTime->clone();
                        addCourtesy(this, t, timeType, continuation, track);
                        }
                  }
            };
      if (showRepeat || showJump) {
            for (Measure* target : targets) {
                  LayoutBreak* section = target->prevMeasure() ? target->prevMeasure()->nextSectionBreak() : nullptr;
                  if (section && !section->showCourtesy())
                        continue;
                  generate(target, false);
                  }
            if (parens)
                  addParentheses(this, false);
            }
      Measure* prev = prevMeasureMM();
      const bool cancelRepeat = prev && prev->repeatEnd() && sc->styleB(Sid::showCourtesiesRepeats)
                                 && sc->styleB(Sid::showCourtesiesAfterCancellingRepeats);
      const bool cancelJump = prev && !prev->repeatEnd() && !sc->followingRepeatMeasures(prev).empty() && sc->styleB(Sid::showCourtesiesOtherJumps)
                                 && sc->styleB(Sid::showCourtesiesAfterCancellingOtherJumps);
      if (cancelRepeat || cancelJump) {
            generate(this, true);
            if (sc->styleB(cancelRepeat ? Sid::useParensRepeatCourtesiesAfterCancelling : Sid::useParensOtherJumpCourtesiesAfterCancelling))
                  addParentheses(this, true);
            }
      for (Segment* s = first(); s; s = s->next()) {
            if (s->isType(SegmentType::RepeatCourtesy | SegmentType::ContinuationCourtesy))
                  s->createShapes();
            }
      deselectInactiveRepeatCourtesies();
      }

} // namespace Ms
