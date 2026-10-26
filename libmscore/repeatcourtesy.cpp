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

// A Symbol subclass keeps normal symbol scanning, painting and ownership.
// Only the vertical glyph scale differs; it is layout-generated, never saved.
class CourtesyParenthesis final : public Symbol {
      qreal _yscale = 1.0;
      qreal _height;
   public:
      CourtesyParenthesis(Score* score, SymId sym, qreal height)
         : Symbol(score, ElementFlag::ON_STAFF | ElementFlag::NOT_SELECTABLE), _height(height)
            {
            setSym(sym);
            setGenerated(true);
            setIsCourtesy(true);
            QRectF r = symBbox(sym);
            _yscale = r.height() > 0.0 ? height / r.height() : 1.0;
            }
      CourtesyParenthesis* clone() const override { return new CourtesyParenthesis(*this); }
      void layout() override
            {
            QRectF r = symBbox(sym());
            _yscale = r.height() > 0.0 ? _height / r.height() : 1.0;
            setbbox(QRectF(r.x(), r.y() * _yscale, r.width(), r.height() * _yscale));
            }
      void draw(QPainter* painter) const override
            {
            painter->save();
            painter->scale(1.0, _yscale);
            Symbol::draw(painter);
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
      if (!seg)
            return;
      // These segments contain generated visual aids only. Do not use undo:
      // none of these elements belongs to the musical document or its maps.
      const auto elements = seg->elist();
      for (Element* e : elements) {
            if (e) {
                  seg->remove(e);
                  delete e;
                  }
            }
      const auto annotations = seg->annotations();
      for (Element* e : annotations) {
            seg->remove(e);
            delete e;
            }
      seg->setEnabled(false);
      seg->createShapes();
      }

Element* addCourtesy(Measure* m, Element* element, SegmentType type, bool continuation, int track)
      {
      const Fraction rtick = continuation ? Fraction(0,1) : m->ticks();
      Segment* seg = m->getSegmentR(type, rtick);
      element->setTrack(track);
      element->setGenerated(true);
      element->setIsCourtesy(true);
      element->setSelected(false);
      element->setFlag(ElementFlag::NOT_SELECTABLE, true);
      // Source signature offsets are meaningful at its original location only.
      element->setOffset(QPointF());
      seg->setEnabled(true);
      seg->setTrailer(false);
      seg->add(element);
      element->layout();
      return element;
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
            QRectF bounds;
            for (SegmentType type : types) {
                  Segment* seg = m->findSegmentR(type, rtick);
                  Element* e = seg && seg->enabled() ? seg->element(track) : nullptr;
                  if (!e || !e->visible() || e->bbox().isEmpty())
                        continue;
                  if (separateMeter && e->isTimeSig() != (group == 1)) continue;
                  if (!first)
                        first = e;
                  last = e;
                  bounds |= e->bbox().translated(e->pos());
                  }
            if (!first)
                  continue;
            qreal sp = first->spatium();
            qreal top = bounds.top() - 0.15 * sp;
            qreal bottom = bounds.bottom() + 0.15 * sp;
            if (!m->score()->styleB(Sid::smallParens)) {
                  top = group == 1 ? bounds.top() - sp : -sp;
                  bottom = group == 1 ? bounds.bottom() + sp : m->score()->staff(track / VOICES)->height() + sp;
                  }
            for (int side = 0; side < 2; ++side) {
                  Element* anchor = side ? last : first;
                  Segment* seg = toSegment(anchor->parent());
                  auto paren = new CourtesyParenthesis(m->score(), side ? SymId::timeSigParensRight : SymId::timeSigParensLeft,
                                                       bottom - top);
                  paren->setTrack(track);
                  paren->setParent(seg);
                  paren->layout();
                  QRectF ar = anchor->bbox().translated(anchor->pos());
                  qreal x = side ? ar.right() + 0.2 * sp - paren->bbox().left()
                                 : ar.left() - 0.2 * sp - paren->bbox().right();
                  paren->setPos(x, top - paren->bbox().top());
                  seg->add(paren);
                  }
          }
        }
      }

} // namespace

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
                  if (seg->measure() == parent && seg->segmentType() == type)
                        continue;
                  // Generated headers can occupy a target slot from an earlier layout.
                  Segment* occupied = parent->findSegmentR(type, rtick);
                  if (occupied) {
                        if (hasRealElements(occupied))
                              continue; // never discard a real change from an imported score
                        mergeRealSignatureSegments(this, seg, occupied);
                        continue;
                        }
                  undo(new ChangeSignatureSegmentParent(seg, parent, rtick, type));
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
                        seg.remove(e);
                        delete e;
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
                  const Measure* prev = prevMeasure();
                  bool joined = prev && prev->repeatEnd() && repeatStart();
                  bool between = score()->styleB(Sid::changesBetweenEndStartRepeat);
                  if (joined && !between && hasRealElements(s)
                      && s->isType(SegmentType::Clef | SegmentType::KeySig | SegmentType::TimeSig))
                        return int(SegmentType::StartRepeatBarLine) + (s->isClefType() ? 1 : s->isKeySigType() ? 2 : 3);
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
                        if (continuation && prevKey)
                              k->setCourtesyPreviousKey(toKeySig(prevKey)->key());
                        addCourtesy(this, k, keyType, continuation, track);
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
      }

} // namespace Ms
