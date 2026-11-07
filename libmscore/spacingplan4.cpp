// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacingprofile4.h"
#include "spacingplan4.h"
#include "spacinginput4.h"
#include "spacinggeometry4.h"
#include "spacingcollision4.h"
#include "spacinglayout4.h"
#include "gracespacing4.h"
#include "chord.h"
#include "clef.h"
#include "breath.h"
#include "barline.h"
#include "keysig.h"
#include "timesig.h"
#include "beam.h"
#include "measure.h"
#include "note.h"
#include "rest.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "stem.h"
#include "system.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace Ms {
namespace {
SpacingDecisionScope4*& activeSpacingDecision4()
{
    static thread_local SpacingDecisionScope4* scope = nullptr;
    return scope;
}
}

SpacingDecisionScope4::SpacingDecisionScope4(const Score* score, std::function<void(const SpacingDecision4&)> handler)
    : _score(score), _handler(std::move(handler)), _previous(activeSpacingDecision4())
{
    activeSpacingDecision4() = this;
}

SpacingDecisionScope4::~SpacingDecisionScope4()
{
    Q_ASSERT(activeSpacingDecision4() == this);
    activeSpacingDecision4() = _previous;
}

bool spacingDecisionObserved4(const Score* score)
{
    if (!score) return false;
    for (const SpacingDecisionScope4* scope = activeSpacingDecision4(); scope; scope = scope->_previous)
        if (scope->_score == score && scope->_handler) return true;
    return false;
}

void recordSpacingDecision4(const System* system, SpacingPass4 pass, SpacingPath4 path,
                            qreal width, qreal available, const QString& reason, qreal courtesyRefreshWidth)
{
    if (!system) return;
    SpacingDecisionScope4* scope = activeSpacingDecision4();
    while (scope && (scope->_score != system->score() || !scope->_handler)) scope = scope->_previous;
    if (!scope) return;
    SpacingDecision4 event;
    event.pass = pass;
    event.path = path;
    event.width = width;
    event.available = available;
    event.courtesyRefreshWidth = courtesyRefreshWidth;
    event.reason = reason;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure()) continue;
        const Measure* m = toMeasure(mb);
        if (!event.measures++) event.firstTick = m->tick();
        event.endTick = m->endTick();
    }
    scope->_handler(event);
}

namespace {
struct Geometry4 {
    std::vector<SpacingShape4> staff;
    qreal minLeft = -std::numeric_limits<qreal>::max();
    qreal minRight = 0;
    qreal leftEdge = std::numeric_limits<qreal>::max();
    bool accidentals = false;
};

bool active4(const Segment* s)
{
    return s->enabled() && s->visible() && !s->allElementsInvisible();
}

bool headerType4(const Segment* s)
{
    return s->isHeaderClefType() || s->isKeySigType() || s->isTimeSigType();
}

bool courtesyType4(const Segment* s)
{
    return s->isKeySigAnnounceType() || s->isTimeSigAnnounceType();
}

bool rightAligned4(const Segment* s)
{
    return s->isClefType() || s->isBreathType();
}

// This field is derived layout state, not manual segment leading space.
// Rebuild it below rather than borrowing a previously published compensation.
bool carriesRightAlignmentOffset4(const Segment* s)
{
    if (!s->isChordRestType()) return false;
    for (const Segment* next = s->next(); next; next = next->next()) {
        if (!active4(next)) continue;
        if (next->isChordRestType()) return false;
        if (rightAligned4(next)) return true;
    }
    return false;
}

// Studio setPositionsAndWidths: only these ordinary changes lead the next
// measure when no barline separates them from the preceding segment.
bool leadingNonRhythmic4(const Segment* s)
{
    return s->isClefType() || s->isKeySigType() || s->isTimeSigType();
}

bool eligibleType4(const Segment* s)
{
    return headerType4(s) || courtesyType4(s) || s->isClefType() || s->isBreathType() || s->isChordRestType()
        || s->isEndBarLineType() || s->isStartRepeatBarLineType() || s->segmentType() == SegmentType::BeginBarLine;
}

// The new repeat boundary borrows finalized native barline geometry. Grace
// targets, centered rests and cross-staff streams need their own boundary
// preparation; do not widen those adapters through repeat admission.
bool ordinaryRepeatBoundary4(const Measure* measure)
{
    const Measure* boundary[] = {measure->prevMeasure(), measure};
    for (const Measure* m : boundary) {
        if (!m || m->system() != measure->system()) continue;
        for (const Segment& s : m->segments()) {
            if (!s.isChordRestType()) continue;
            for (const Element* item : s.elist()) {
                if (!item) continue;
                if (item->isRest()) {
                    if (toRest(item)->isFullMeasureRest()) return false;
                }
                else if (item->isChord()) {
                    const Chord* chord = toChord(item);
                    if (chord->staffMove() || !chord->graceNotes().empty() || chord->tremolo()
                        || (chord->beam() && chord->beam()->cross())) return false;
                }
                else return false;
            }
        }
    }
    return true;
}

struct CrossStaffGeometry4 {
    std::map<const Chord*, CrossStaffChord4> chords;
    std::map<const Segment*, CrossStaffPair4> pairs;
};

bool prepareCrossStaffMeasure4(Measure* measure, CrossStaffGeometry4& result, QString* reason)
{
    const Beam* beam = nullptr;
    for (const Segment& s : measure->segments()) {
        for (const Element* item : s.elist()) {
            if (!item || !item->isChord()) continue;
            const Chord* c = toChord(item);
            if (!c->staffMove() && !(c->beam() && c->beam()->cross())) continue;
            if (!c->beam() || (beam && beam != c->beam())) {
                if (reason) *reason = "Independent cross-staff streams within a measure require a later adapter";
                return false;
            }
            beam = c->beam();
        }
    }
    if (!beam) return true;
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (measure->score()->nstaves() != 2)
        return fail("Cross-staff placement currently requires a grand staff");
    CrossStaffContext4 context;
    if (!buildCrossStaffContext4(beam, context, reason)) return false;
    if (context.chords.front().chord->measure() != measure
        || !context.chords.front().chord->rtick().isZero()
        || context.chords.back().chord->rtick() + context.chords.back().chord->actualTicks() != measure->ticks())
        return fail("Cross-staff placement requires a complete measure stream");
    CrossStaffGeometry4 prepared;
    for (const CrossStaffChord4& e : context.chords)
        prepared.chords[e.chord] = e;
    std::map<int, int> movingCounts;
    for (const Segment& segment : measure->segments()) {
        if (!segment.isChordRestType()) continue;
        for (const Element* item : segment.elist()) {
            if (!item || (item->isChord() && prepared.chords.count(toChord(item)))) continue;
            if (!item->isChord()) return fail("Cross-staff companions currently require chords");
            const Chord* c = toChord(item);
            if (!c->visible() || c->beam() || c->staffMove() || c->isGrace() || c->tuplet()
                || c->tremolo() || c->isSmall() || c->mag() != 1 || c->isNudged() || c->noStem() || !c->stem()
                || !c->graceNotes().empty() || c->notes().size() != 1)
                return fail("Unadapted moving cross-staff companion");
            const Note* note = c->notes().front();
            if (!note->visible() || note->isSmall() || !note->offset().isNull() || note->mirror()
                || note->headGroup() != NoteHead::Group::HEAD_NORMAL || note->headType() != NoteHead::Type::HEAD_AUTO)
                return fail("Unadapted moving cross-staff companion note");
            if (c->rtick().isZero() && c->actualTicks() == measure->ticks()) continue;
            // Keep each admitted un-beamed stream complete independently.
            int& count = movingCounts[c->track()];
            if (movingCounts.size() > 2 || c->actualTicks() != Fraction(1, 16)
                || c->durationType().type() != TDuration::DurationType::V_16TH
                || c->rtick() != Fraction(count, 16))
                return fail("Cross-staff companions need at most two complete sixteenth-note streams");
            ++count;
        }
    }
    for (const auto& stream : movingCounts) {
        if (Fraction(stream.second, 16) != measure->ticks())
            return fail("Incomplete moving cross-staff companion");
    }
    for (size_t i = 0; i < context.pairs.size(); ++i) {
        prepared.pairs[context.pairs[i].first->segment()] = context.pairs[i];
    }
    result = prepared;
    return true;
}

// Publish only after every measure's independently bounded context succeeds.
bool prepareCrossStaff4(System* system, CrossStaffGeometry4& result, QString* reason)
{
    CrossStaffGeometry4 prepared;
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure()) {
            if (reason) *reason = "Frames require a later cross-staff system adapter";
            return false;
        }
        CrossStaffGeometry4 local;
        if (!prepareCrossStaffMeasure4(toMeasure(mb), local, reason)) return false;
        prepared.chords.insert(local.chords.begin(), local.chords.end());
        prepared.pairs.insert(local.pairs.begin(), local.pairs.end());
    }
    result = prepared;
    return true;
}

// Validate published before-graces and prepare complete temporary after-group
// projections. No native writes; publish outputs only after every group succeeds.
bool prepareGraces4(System* system, SpacingGraceContext4& result, std::vector<GraceAfterGeometry4>& afterResult,
                    std::vector<GraceMergedGeometry4>& mergedResult, QString* reason,
                    const std::vector<GraceMergedGeometry4>* preparedMerged, SpacingContextCache4& contextCache)
{
    SpacingGraceContext4 prepared;
    std::vector<GraceAfterGeometry4> afters;
    std::vector<GraceMergedGeometry4> merged;
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure())
            continue; // The ordinary planner rejects frames itself.
        for (const Segment& segment : toMeasure(mb)->segments()) {
            if (!active4(&segment) || !segment.isChordRestType())
                continue;
            for (const Element* item : segment.elist()) {
                if (!item || !item->isChord() || toChord(item)->graceNotes().empty())
                    continue;
                const Chord* main = toChord(item);
                // Incoming combined groups already stage their target's before-graces.
                if (prepared.contains(main)) continue;
                if (!main->graceNotesAfter().empty()) {
                    GraceAfterGeometry4 geometry;
                    if (buildGraceAfterGeometry4(main, geometry, reason, &contextCache)) {
                        prepared.afterMains.push_back(main);
                        afters.push_back(geometry);
                    }
                    else {
                        GraceMergedGeometry4 joint;
                        const GraceMergedGeometry4* ready = nullptr;
                        if (preparedMerged) {
                            for (const auto& entry : *preparedMerged) {
                                if (entry.plan.owner == main) { ready = &entry; break; }
                            }
                        }
                        if (ready && ready->plan.slurPolicy == contextCache.slurPolicy()) {
                            SpacingProfileScope4 reuse(SpacingProfileStage4::PLAN_GEOMETRY_REUSE);
                            joint = *ready;
                            if (reason) reason->clear();
                        }
                        else if (!buildGraceMergedGeometry4(main, joint, reason, &contextCache)) return false;
                        prepared.afterMains.push_back(main);
                        prepared.mains.push_back(joint.plan.target);
                        prepared.mergedPairs.push_back({main, joint.plan.target});
                        merged.push_back(joint);
                    }
                    continue;
                }
                GraceBeforePlan4 plan;
                if (!buildGraceBeforePlan4(main, plan, reason, &contextCache))
                    return false;
                for (const GracePosition4& p : plan.positions) {
                    if (std::abs(p.chord->x() - p.x) > 1e-6 || !std::isfinite(p.chord->x())) {
                        if (reason) *reason = "Grace positions must be prepared before system spacing";
                        return false;
                    }
                }
                prepared.mains.push_back(main);
            }
        }
    }
    result = prepared;
    afterResult = afters;
    mergedResult = merged;
    return true;
}

// Replace only proven components on the affected staff. Unknown or mixed
// ownership must not disappear when a temporary after-grace shape is used.
bool substituteAfterGeometry4(Segment* segment, int staff, SpacingShape4& native,
                             const std::vector<GraceAfterGeometry4>& afters, bool targetBefore = false)
{
    for (const GraceAfterGeometry4& after : afters) {
        const Chord* owner = after.plan.owner;
        if (staff != owner->staffIdx()) continue;
        const bool source = segment == owner->segment();
        if (!source && segment != after.plan.target) continue;
        const Element* root = source ? owner : segment->element(owner->track());
        const bool before = targetBefore && !source;
        const auto owned = [root, before](const Element* item) {
            for (; item; item = item->parent()) {
                if (item == root) return true;
                if (item->isChord()) {
                    const Chord* chord = toChord(item);
                    return chord == root || ((before ? chord->isGraceBefore() : chord->isGraceAfter()) && chord->parent() == root);
                }
            }
            return false;
        };
        for (const auto& e : native.elements) {
            if (!std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
                || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
                || e.rect.width() < 0 || e.rect.height() < 0) return false;
            if (e.identity == SpacingIdentity4::ITEM) {
                if (!owned(e.item) || !e.aggregateOwners.empty() || !e.sharedItems.empty()) return false;
            }
            else if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
                if (!e.sharedItems.empty()) return false;
                if (e.item) {
                    if ((!e.item->isChord() && !(e.item == root && e.item->isBarLine()))
                        || !owned(e.item) || !e.aggregateOwners.empty()) return false;
                }
                else {
                    if (e.aggregateOwners.size() < 2) return false;
                    for (size_t i = 0; i < e.aggregateOwners.size(); ++i) {
                        const Element* item = e.aggregateOwners[i];
                        if (!item || !item->isChord() || !owned(item)) return false;
                        for (size_t j = 0; j < i; ++j) if (item == e.aggregateOwners[j]) return false;
                    }
                }
            }
            else if (e.identity == SpacingIdentity4::SHARED_ITEM) {
                if (e.item || !e.aggregateOwners.empty() || e.sharedItems.size() < 2) return false;
                for (size_t i = 0; i < e.sharedItems.size(); ++i) {
                    if (!owned(e.sharedItems[i])) return false;
                    for (size_t j = 0; j < i; ++j) if (e.sharedItems[i] == e.sharedItems[j]) return false;
                }
            }
            else return false;
        }
        native = source ? after.source : after.target;
        return true;
    }
    return true;
}

// Reuse the provenance validator with explicit target-before ownership. The
// adapter is private to geometry substitution; public after-only plans stay separate.
bool substituteMergedGeometry4(Segment* segment, int staff, SpacingShape4& native,
                              const std::vector<GraceMergedGeometry4>& merged)
{
    for (const GraceMergedGeometry4& joint : merged) {
        GraceAfterGeometry4 view;
        view.plan.owner = joint.plan.owner;
        view.plan.target = joint.plan.target->segment();
        view.source = joint.source;
        view.target = joint.target;
        if (!substituteAfterGeometry4(segment, staff, native, {view}, true)) return false;
    }
    return true;
}

// Studio Segment::createShape omits a full rest from a moving-voice staff,
// except when every item at this onset is a full rest. Keep native skyline
// shapes intact: apply this horizontal policy only to the validated snapshot.
// RestLayout::layoutRest starts at x=0 before spacing; an already centered
// native rest must supply that same input without resetting its DOM position.
bool projectMeasureRests4(const Segment* segment, int staff, SpacingShape4& shape)
{
    const bool hasMeasureRest = std::any_of(shape.elements.begin(), shape.elements.end(),
        [](const SpacingShapeElement4& e) {
            return e.identity == SpacingIdentity4::ITEM && e.item && e.item->isRest()
                && toRest(e.item)->isFullMeasureRest();
        });
    if (!hasMeasureRest) return true; // Preserve the existing non-rest projection path.
    SpacingShape4 projected;
    for (auto e : shape.elements) {
        // Even a source-authorized omission must not conceal malformed
        // producer geometry from the complete-snapshot rejection boundary.
        if (!std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
            || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
            || e.rect.width() < 0 || e.rect.height() < 0) return false;
        if (e.identity == SpacingIdentity4::ITEM && e.item && e.item->isRest()
            && toRest(e.item)->isFullMeasureRest()) {
            const Rest* rest = toRest(e.item);
            if (rest->segment() != segment || rest->staffIdx() != staff || rest->staffMove()
                || !std::isfinite(rest->ipos().x())) return false;
            bool allFullRests = true;
            for (int track = staff * VOICES; track < (staff + 1) * VOICES; ++track) {
                const Element* item = segment->element(track);
                if (item && !(item->isRest() && toRest(item)->isFullMeasureRest())) {
                    allFullRests = false;
                    break;
                }
            }
            if (segment->measure()->hasVoices(staff) && !allFullRests) continue;
            e.rect.translate(-rest->ipos().x(), 0); // Retain the user's offset.
        }
        projected.elements.push_back(e);
    }
    shape = std::move(projected);
    return true;
}

bool ordinaryMeasureRest4(const Rest* rest)
{
    const bool ordinary = rest && rest->measure() && !rest->measure()->isMMRest() && rest->segment()
        && rest->isFullMeasureRest() && rest->rtick().isZero()
        && rest->actualTicks() == rest->measure()->ticks()
        && rest->visible() && rest->addToSkyline() && !rest->onTabStaff()
        && !rest->isGap() && !rest->staffMove()
        && rest->mag() > 0 && std::isfinite(rest->mag())
        && !rest->tuplet() && !rest->beam() && !rest->dots()
        && rest->el().empty() && rest->lyrics().empty()
        && std::isfinite(rest->ipos().x()) && std::isfinite(rest->x()) && std::isfinite(rest->y())
        && std::isfinite(rest->bbox().x()) && std::isfinite(rest->bbox().y())
        && std::isfinite(rest->width()) && rest->width() > 0
        && std::isfinite(rest->height()) && rest->height() > 0;
    if (!ordinary) return false;
    // Studio getMeasureStartEndPos ignores internal signatures when centering
    // a full rest; Evolution's late native pass stops at any non-rhythmic item.
    // Reuse that pass only where its first boundary has the source semantics.
    for (const Segment* s = rest->segment()->nextActive(); s; s = s->nextActive()) {
        if (!s->isChordRestType() && s->element(rest->staffIdx() * VOICES))
            return s->isEndBarLineType() || s->isKeySigAnnounceType() || s->isTimeSigAnnounceType();
    }
    return true;
}

bool cacheGeometry4(Segment* segment, const SpacingPaddingTable4& table, Geometry4& geometry,
                    const CrossStaffGeometry4& cross, const SpacingGraceContext4& graces,
                    const std::vector<GraceAfterGeometry4>& afters,
                    const std::vector<GraceMergedGeometry4>& merged, SpacingContextCache4& contextCache)
{
    for (int staffIdx = 0; staffIdx < segment->score()->nstaves(); ++staffIdx) {
        SpacingShape4 native = spacingStaffShape4(segment, staffIdx);
        if (!substituteAfterGeometry4(segment, staffIdx, native, afters)
            || !substituteMergedGeometry4(segment, staffIdx, native, merged)
            || !projectMeasureRests4(segment, staffIdx, native)) return false;
        SpacingShape4 projected;
        // These first controls use ordinary visible staves. Staff scale still
        // comes from the item, rather than assuming score spatium in the kernel.
        qreal sp = segment->spatium();
        for (const SpacingShapeElement4& e : native.elements) {
            if (e.identity == SpacingIdentity4::ITEM && e.item) {
                sp = e.item->spatium();
                break;
            }
        }
        if (!spacingCollisionProjection4(table, native, sp, projected, &graces, &contextCache))
            return false;
        for (const auto& pair : cross.chords) {
            const CrossStaffChord4& e = pair.second;
            if (e.chord->segment() != segment || e.visualStaff != staffIdx) continue;
            const Note* note = e.up ? e.chord->downNote() : e.chord->upNote();
            const qreal width = e.chord->stem()->lineWidth() * e.chord->mag();
            const qreal y = e.chord->y() + note->y()
                + (e.up ? note->stemUpSE().y() : note->stemDownNW().y());
            // In this bounded context the beam lies between the staves. Use
            // the proven stem side without borrowing a provisional stem length.
            // Keep this conservative extension confined to the restricted
            // held companions or at most two un-beamed moving voices; broader cases
            // remain unsupported. This conservative extent needs visual review.
            const qreal reach = 100000;
            const QRectF rect(e.stemX - width / 2, e.up ? y - reach : y - width / 2,
                              width, reach + width / 2);
            SpacingPairPolicy4 policy;
            if (!spacingPairPolicy4(table, e.chord->stem(), e.chord->stem(), sp, policy, nullptr, &contextCache)) return false;
            projected.elements.push_back(SpacingShapeElement4(rect, e.chord->stem()));
        }
        qreal staffLeft = std::numeric_limits<qreal>::max();
        for (const SpacingShapeElement4& e : projected.elements) {
            geometry.minRight = std::max(geometry.minRight, e.rect.right());
            geometry.leftEdge = std::min(geometry.leftEdge, e.rect.left());
            if (e.rect.height() != 0)
                staffLeft = std::min(staffLeft, e.rect.left());
            geometry.accidentals = geometry.accidentals || e.item->isAccidental();
        }
        if (staffLeft != std::numeric_limits<qreal>::max())
            geometry.minLeft = std::max(geometry.minLeft, -staffLeft);
        geometry.staff.push_back(projected);
    }
    if (geometry.minLeft == -std::numeric_limits<qreal>::max())
        geometry.minLeft = 0;
    if (geometry.leftEdge == std::numeric_limits<qreal>::max())
        geometry.leftEdge = 0;
    return true;
}

bool distance4(const SpacingPaddingTable4& table, const SpacingSegmentPlan4& a, const Geometry4& first,
               const SpacingSegmentPlan4& b, const Geometry4& second, Measure* firstMeasure,
               qreal squeeze, qreal& result, const CrossStaffGeometry4& cross, const SpacingGraceContext4& graces,
               SpacingContextCache4& contextCache)
{
    // Studio minHorizontalDistance joins adjacent repeat barlines at the
    // thick stroke. A systemic begin barline shares the repeat's origin.
    if (a.segment->isType(SegmentType::BarLineType) && b.segment->isStartRepeatBarLineType()) {
        result = a.segment->isBeginBarLineType() ? 0
            : std::max(first.minRight - table.score()->styleP(Sid::endBarWidth), qreal(0));
        return std::isfinite(result);
    }
    qreal distance = 0;
    const bool header = headerType4(a.segment) && a.segment->rtick().isZero()
        && b.segment->measure() == firstMeasure
        && (b.segment->isChordRestType() || b.segment->isStartRepeatBarLineType());
    for (size_t st = 0; st < first.staff.size(); ++st) {
        qreal sp = a.segment->spatium();
        for (const SpacingShapeElement4& e : first.staff[st].elements) {
            sp = e.item->spatium();
            break;
        }
        qreal d = 0;
        if (!spacingShapeDistance4(table, first.staff[st], second.staff[st], sp, squeeze, d, &graces, &contextCache))
            return false;
        distance = std::max(distance, d);
    }
    const auto constraint = cross.pairs.find(a.segment);
    if (constraint != cross.pairs.end() && constraint->second.second->segment() == b.segment)
        distance = std::max(distance, std::max(constraint->second.minimumWidth, constraint->second.minimumStemDistance));
    if (header) {
        const Score* score = table.score();
        distance = std::max(distance, first.minRight);
        const qreal gap = score->styleP(a.segment->isTimeSigType() ? Sid::systemHeaderTimeSigDistance : Sid::systemHeaderDistance);
        distance = std::max(distance, first.minRight + gap * squeeze);
        if (b.segment->isStartRepeatBarLineType()) {
            // Studio measures this gap edge-to-edge. Evolution's native
            // barline renderer uses half its stored endBarDistance value.
            distance -= score->styleP(Sid::endBarWidth) + .5 * score->styleP(Sid::endBarDistance);
        }
        const qreal actual = distance - first.minRight - second.minLeft;
        distance += std::max(qreal(0), 1.5 * a.segment->spatium() * squeeze - actual);
    }
    result = distance;
    return std::isfinite(result);
}

// Studio moveRightAlignedSegments, before segment-width reconstruction.
// Geometry and DOM stay immutable; the fixed compensation belongs to the
// preceding rhythmic spring and must survive minimum widths/justification.
bool moveRightAligned4(SpacingPlan4& plan, const std::vector<Geometry4>& geometry,
                       const SpacingPaddingTable4& table, qreal squeeze, const CrossStaffGeometry4& cross,
                       const SpacingGraceContext4& graces, SpacingContextCache4& contextCache)
{
    for (size_t i = 0; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored || !rightAligned4(current.segment)) continue;
        qreal x = std::numeric_limits<qreal>::max();
        for (size_t j = i + 1; j < plan.segments.size(); ++j) {
            const SpacingSegmentPlan4& following = plan.segments[j];
            if (following.ignored) continue;
            if (following.segment->measure() != current.segment->measure() && !following.segment->rtick().isZero())
                break;
            qreal distance = 0;
            if (!distance4(table, current, geometry[i], following, geometry[j], plan.measures.front().measure,
                           squeeze, distance, cross, graces, contextCache)) return false;
            // Pinned muse::RealIsNull(double) uses an absolute 1e-9 tolerance.
            if (rightAligned4(following.segment) && qAbs(distance) <= 1e-9) continue;
            x = std::min(x, following.x - distance);
        }
        if (x == std::numeric_limits<qreal>::max()) continue;
        if (!std::isfinite(x)) return false;
        current.x = x;
        size_t next = i + 1;
        while (next < plan.segments.size() && plan.segments[next].ignored) ++next;
        if (next == plan.segments.size()) return false;
        // Use an index for the next-active lookup; never assign through a
        // reference into the plan and overwrite an ignored segment's identity.
        for (size_t previous = i; previous > 0;) {
            --previous;
            SpacingSegmentPlan4& spring = plan.segments[previous];
            if (spring.segment->measure() != current.segment->measure()) break;
            if (!spring.segment->isChordRestType()) continue;
            spring.widthOffset += x - plan.segments[next].x;
            if (!std::isfinite(spring.widthOffset)) return false;
            break;
        }
    }
    return true;
}

// Reconstruct segment positions after changing rhythmic widths in one measure.
// First-segment offset and nonrhythmic widths remain fixed, as in respaceSegments.
qreal reflowMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure)
{
    qreal cursor = measure.x;
    bool first = true;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (segment.segment->measure() != measure.measure)
            continue;
        if (segment.ignored)
            continue;
        if (first) {
            cursor = segment.x;
            first = false;
        }
        segment.x = cursor;
        cursor += segment.width;
    }
    const qreal previous = measure.width;
    measure.width = cursor - measure.x;
    return measure.width - previous;
}

void moveMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure, qreal delta)
{
    measure.x += delta;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (segment.segment->measure() == measure.measure)
            segment.x += delta;
    }
}

bool stretchMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure, qreal extra)
{
    std::vector<SpacingSpring4> springs;
    for (const SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.segment->measure() == measure.measure && segment.stretch > 0)
            springs.push_back({ segment.width - segment.widthOffset, segment.stretch });
    }
    const qreal force = spacing4SpringForce(springs, extra);
    if (!(force > 0))
        return false;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.segment->measure() == measure.measure && segment.stretch > 0)
            segment.width = std::max(segment.width - segment.widthOffset, force * segment.stretch) + segment.widthOffset;
    }
    return true;
}
}

bool spacingCandidateSignaturesReady4(const System* system, QString* reason, bool allowPendingTrailer)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->measures().empty())
        return fail("Candidate signature prefix unavailable");
    std::vector<const Measure*> measures;
    bool signatures = false;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mb->system() != system || mb->score() != system->score())
            return fail("Candidate signature prefix ownership/frame boundary");
        const Measure* m = toMeasure(mb);
        if (!measures.empty() && measures.back()->next() != m)
            return fail("Candidate signature prefix is not contiguous");
        signatures = signatures || m->trailer() || m->hasCourtesyKeySig();
        for (const Segment& segment : m->segments())
            if (active4(&segment) && (courtesyType4(&segment) || segment.trailer()
                || (!measures.empty() && headerType4(&segment)))) signatures = true;
        measures.push_back(m);
    }
    const Score* score = system->score();
    const Measure* last = measures.back();
    if (last->nextMeasure() && score->nstaves() == 1) {
        const bool keyChange = !(score->staff(0)->keySigEvent(last->endTick() - Fraction::fromTicks(1))
            == score->staff(0)->keySigEvent(last->endTick()));
        const Segment* time = last->nextMeasure()->findSegmentR(SegmentType::TimeSig, Fraction(0, 1));
        signatures = signatures || (keyChange && score->styleB(Sid::genCourtesyKeysig))
            || (time && time->element(0) && time->element(0)->isTimeSig()
                && toTimeSig(time->element(0))->showCourtesySig() && score->styleB(Sid::genCourtesyTimesig));
    }
    if (!signatures) return true; // Preserve the existing ordinary-prefix contract.
    if (score->nstaves() != 1 || score->floatMode())
        return fail("Candidate signatures require one ordinary pitched staff");
    const auto geometryReady = [score](const Element* item, const Segment* segment) {
        if (!item || item->parent() != segment || item->score() != score || item->track() != 0
            || !item->visible() || !item->addToSkyline() || !(item->mag() > 0) || !std::isfinite(item->mag())) return false;
        for (const Element* occupant : segment->elist())
            if (occupant && occupant != item) return false;
        const QRectF rect = item->bbox();
        return rect.width() > 0 && rect.height() > 0 && std::isfinite(rect.x()) && std::isfinite(rect.y())
            && std::isfinite(rect.width()) && std::isfinite(rect.height())
            && std::isfinite(item->pos().x()) && std::isfinite(item->pos().y());
    };
    const auto ordinarySignature = [](const Element* item) {
        if (!item) return false;
        if (item->isKeySig()) {
            const KeySig* key = toKeySig(item);
            return !key->isCustom() && !key->isAtonal() && !key->forInstrumentChange();
        }
        if (!item->isTimeSig()) return false;
        const TimeSig* time = toTimeSig(item);
        return time->timeSigType() == TimeSigType::NORMAL && time->sig() > Fraction(0, 1)
            && !time->isLocal()
            && (time->numeratorString().isEmpty() || time->numeratorString() == QString::number(time->numerator()))
            && (time->denominatorString().isEmpty() || time->denominatorString() == QString::number(time->denominator()));
    };
    for (size_t i = 0; i < measures.size(); ++i) {
        const Measure* m = measures[i];
        const bool last = i + 1 == measures.size();
        if (!score->staff(0)->isPitchedStaff(m->tick()) || m->repeatStart() || m->repeatEnd()
            || m->nextSectionBreak() || !ordinaryRepeatBoundary4(m))
            return fail("Candidate signature repeat/section/grace/rest/cross-staff boundary");
        const bool pending = allowPendingTrailer && i + 2 == measures.size() && m->trailer();
        const bool terminalGeometry = last || pending;
        if (m->trailer() && !terminalGeometry)
            return fail("Candidate pending penultimate courtesy trailer");
        for (const Segment& segment : m->segments()) {
            // Even a currently hidden ordinary clef can be revealed/moved by
            // predecessor finalization. That requires a separate adapter.
            if (segment.isClefType()) return fail("Candidate signature ordinary-clef boundary");
            if (allowPendingTrailer && active4(&segment) && rightAligned4(&segment))
                return fail("Candidate pending penultimate courtesy with a right-aligned mark");
            if (!active4(&segment)) continue;
            if (!i && segment.isHeaderClefType()) continue;
            if (segment.isHeaderClefType()) return fail("Candidate interior system header not removed");
            const bool courtesy = courtesyType4(&segment);
            if (!headerType4(&segment) && !courtesy && !segment.trailer()) continue;
            if (!segment.ticks().isZero() || (courtesy ? !terminalGeometry || !segment.trailer() || segment.rtick() != m->ticks()
                : segment.trailer() || !segment.rtick().isZero()))
                return fail("Candidate signature location/trailer phase not prepared");
            const Element* item = segment.element(0);
            if (!ordinarySignature(item) || !geometryReady(item, &segment)
                || ((segment.isKeySigType() || segment.isKeySigAnnounceType()) ? !item->isKeySig() : !item->isTimeSig())
                || (courtesy && !item->generated()))
                return fail("Candidate signature glyph, ownership or dimensions not prepared");
        }
        const Measure* next = m->nextMeasure();
        const Segment* keySeg = next ? next->findSegmentR(SegmentType::KeySig, Fraction(0, 1)) : nullptr;
        const Segment* timeSeg = next ? next->findSegmentR(SegmentType::TimeSig, Fraction(0, 1)) : nullptr;
        const Element* key = keySeg ? keySeg->element(0) : nullptr;
        const Element* time = timeSeg ? timeSeg->element(0) : nullptr;
        const bool keyChange = next && !(score->staff(0)->keySigEvent(m->endTick() - Fraction::fromTicks(1))
            == score->staff(0)->keySigEvent(m->endTick()));
        const bool needKey = terminalGeometry && next && score->styleB(Sid::genCourtesyKeysig) && keyChange
            && (!key || (key->isKeySig() && toKeySig(key)->showCourtesy()));
        const bool needTime = terminalGeometry && next && score->styleB(Sid::genCourtesyTimesig)
            && time && time->isTimeSig() && toTimeSig(time)->showCourtesySig();
        const Segment* keyTrailer = m->findSegmentR(SegmentType::KeySigAnnounce, m->ticks());
        const Segment* timeTrailer = m->findSegmentR(SegmentType::TimeSigAnnounce, m->ticks());
        const bool hasKey = keyTrailer && active4(keyTrailer);
        const bool hasTime = timeTrailer && active4(timeTrailer);
        if (hasKey != needKey || hasTime != needTime || m->hasCourtesyKeySig() != needKey
            || m->trailer() != (hasKey || hasTime))
            return fail("Candidate courtesy presence not prepared");
        if (pending && ((hasKey && (!keySeg || !active4(keySeg))) || (hasTime && (!timeSeg || !active4(timeSeg)))))
            return fail("Candidate pending penultimate courtesy lacks an active following signature");
        if (hasKey && (!ordinarySignature(key)
            || !(toKeySig(keyTrailer->element(0))->keySigEvent() == toKeySig(key)->keySigEvent())))
            return fail("Candidate courtesy key does not match following signature");
        if (hasTime) {
            if (!ordinarySignature(time)) return fail("Candidate courtesy time is not ordinary");
            const TimeSig* actual = toTimeSig(time);
            const TimeSig* courtesy = toTimeSig(timeTrailer->element(0));
            // Match the fields copied by native TimeSig::setFrom. Beam groups
            // are not copied and do not affect this courtesy's drawn body.
            if (courtesy->timeSigType() != actual->timeSigType() || !courtesy->sig().identical(actual->sig())
                || courtesy->stretch() != actual->stretch() || courtesy->numeratorString() != actual->numeratorString()
                || courtesy->denominatorString() != actual->denominatorString())
                return fail("Candidate courtesy time does not match following signature");
        }
        const Segment* end = m->findSegmentR(SegmentType::EndBarLine, m->ticks());
        const Element* item = end ? end->element(0) : nullptr;
        const BarLineType expected = needKey ? BarLineType::DOUBLE : next ? BarLineType::NORMAL : BarLineType::END;
        if (!end || !active4(end) || !end->ticks().isZero() || !item || !item->isBarLine()
            || !item->generated() || toBarLine(item)->barLineType() != expected || !toBarLine(item)->el()->empty()
            || !geometryReady(item, end))
            return fail("Candidate signature end barline not prepared");
    }
    return true;
}

bool spacingRepeatBarlinesReady4(const System* system, SpacingPass4 pass, QString* reason)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->score()->nstaves() <= 0 || system->measures().empty()
        || (pass != SpacingPass4::CANDIDATE && pass != SpacingPass4::FINAL))
        return fail("Candidate repeat prefix/phase unavailable");
    bool repeats = false;
    std::vector<const Measure*> measures;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mb->system() != system || mb->score() != system->score())
            return fail("Candidate repeat prefix ownership/frame boundary");
        const Measure* m = toMeasure(mb);
        if (!measures.empty() && measures.back()->next() != m)
            return fail("Candidate repeat prefix is not contiguous");
        measures.push_back(m);
        repeats = repeats || m->repeatStart() || m->repeatEnd();
    }
    if (!repeats) return true;
    // A measure outside this explicit prefix may still point at this System
    // from a previous layout. Never use that stale parent to infer inclusion.
    const auto barlineReady = [system](const Segment* segment, BarLineType type) {
        if (!segment || !active4(segment) || !segment->ticks().isZero()) return false;
        int count = 0;
        for (const Element* item : segment->elist()) {
            if (!item) continue;
            if (!item->isBarLine() || item->parent() != segment || item->score() != system->score()
                || item->track() < 0 || item->track() % VOICES
                || item->staffIdx() >= system->score()->nstaves()) return false;
            const BarLine* barline = toBarLine(item);
            const QRectF rect = barline->layoutRect();
            if (!barline->generated() || barline->barLineType() != type || !barline->visible()
                || !barline->addToSkyline() || !barline->el()->empty()
                || !(barline->mag() > 0) || !std::isfinite(barline->mag())
                || !(rect.width() > 0) || !(rect.height() > 0)
                || !std::isfinite(rect.x()) || !std::isfinite(rect.y())
                || !std::isfinite(rect.width()) || !std::isfinite(rect.height())
                || !std::isfinite(barline->pos().x()) || !std::isfinite(barline->pos().y())) return false;
            ++count;
        }
        return count == system->score()->nstaves();
    };
    for (size_t i = 0; i < measures.size(); ++i) {
        const Measure* m = measures[i];
        if (!ordinaryRepeatBoundary4(m))
            return fail("Candidate repeat grace/rest/cross-staff boundary");
        // A hidden terminal clef may become visible after accepting the next
        // measure. Its relocation needs a separate preparation adapter too.
        for (const Segment& segment : m->segments())
            if (segment.isClefType() && segment.rtick() == m->ticks())
                return fail("Candidate repeat terminal-clef boundary");
        if (m->repeatStart() && !barlineReady(m->findSegmentR(SegmentType::StartRepeatBarLine,
                                                             Fraction(0, 1)), BarLineType::START_REPEAT))
            return fail("Candidate start-repeat geometry not prepared");
        const Segment* end = m->findSegmentR(SegmentType::EndBarLine, m->ticks());
        // createEndBarLines(true) prepared the trial last and its predecessor.
        // Only earlier accepted predecessors have received the interior call.
        const bool interior = i + 1 < measures.size()
            && (pass == SpacingPass4::FINAL || i + 2 < measures.size());
        const bool enabled = !interior || m->repeatEnd() || !measures[i + 1]->repeatStart();
        if (!enabled) {
            if (end && end->enabled()) return fail("Candidate interior repeat barline not prepared");
            continue;
        }
        const BarLineType expected = m->repeatEnd() ? BarLineType::END_REPEAT
            : m->nextMeasure() ? BarLineType::NORMAL : BarLineType::END;
        if (!barlineReady(end, expected))
            return fail("Candidate terminal repeat barline not prepared");
    }
    return true;
}

bool buildCrossStaffContext4(const Beam* beam, CrossStaffContext4& result, QString* reason)
{
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!beam || !beam->score() || !beam->cross() || beam->elements().size() < 3
        || beam->beamDirection() != Direction::AUTO || beam->userModified())
        return fail("Cross-staff context needs a prepared automatic beam");
    const Score* score = beam->score();
    const SpacingPaddingTable4 table(score);
    CrossStaffContext4 context;
    context.beam = beam;
    int upper = score->nstaves(), lower = -1;
    const Chord* previous = nullptr;
    for (const ChordRest* cr : beam->elements()) {
        if (!cr || !cr->isChord())
            return fail("Cross-staff context does not yet support beamed rests");
        const Chord* chord = toChord(cr);
        if (chord->score() != score || !chord->segment() || !chord->measure() || !chord->visible()
            || chord->beam() != beam || chord->notes().empty() || chord->notes().size() > 2 || chord->isGrace()
            || chord->tuplet() || chord->tremolo() || !chord->graceNotes().empty()
            || chord->isSmall() || chord->mag() != 1.0 || chord->noStem() || !chord->stem()
            || chord->isNudged() || chord->stemDirection() != Direction::AUTO
            || chord->durationType().type() != TDuration::DurationType::V_EIGHTH
            || chord->actualTicks() != Fraction(1, 8) || chord->segment()->ticks() <= Fraction(0, 1)
            || chord->segment()->ticks() > chord->actualTicks())
            return fail("Unadapted cross-staff chord, rhythm or manual context");
        const int staff = chord->vStaffIdx();
        if (staff < 0 || staff >= score->nstaves() || !score->staff(staff)->show()
            || !score->staff(staff)->isPitchedStaff(chord->tick()) || score->staff(staff)->lines(chord->tick()) != 5)
            return fail("Cross-staff context needs visible ordinary five-line staves");
        for (const Note* note : chord->notes()) {
            SpacingPairPolicy4 policy;
            if (!note->visible() || note->isSmall() || note->mirror() || !note->offset().isNull()
                || note->headGroup() != NoteHead::Group::HEAD_NORMAL || note->headType() != NoteHead::Type::HEAD_AUTO
                || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
                return fail("Unadapted cross-staff note context");
        }
        if (previous && (chord->measure() != previous->measure() || chord->track() != previous->track()
                         || chord->tick() != previous->tick() + previous->actualTicks()))
            return fail("Cross-staff context needs one adjacent stream within one measure");
        CrossStaffChord4 entry;
        entry.chord = chord;
        entry.visualStaff = staff;
        context.chords.push_back(entry);
        upper = std::min(upper, staff);
        lower = std::max(lower, staff);
        previous = chord;
    }
    if (lower != upper + 1)
        return fail("Cross-staff context needs exactly two adjacent staves");
    for (CrossStaffChord4& entry : context.chords) {
        const Chord* chord = entry.chord;
        // Evolution's initial stem sides are provisional. Infer only the
        // unambiguous within-staff case, as in the existing bounded adapter.
        for (const Note* note : chord->notes()) {
            if ((entry.visualStaff == upper && note->line() > 8) || (entry.visualStaff == lower && note->line() < 0))
                return fail("Interstaff note positions need a later stem-side adapter");
        }
        entry.up = entry.visualStaff == lower;
        const Note* note = entry.up ? chord->upNote() : chord->downNote();
        // Match Chord::stemPosX/layoutStem1 without changing the provisional
        // Chord::up(). Stem length and final vertical beam position are unused.
        const QPointF attach = entry.up ? note->stemUpSE() : note->stemDownNW();
        const qreal nominalWidth = chord->noteHeadWidth();
        const qreal anchor = attach.isNull() ? (entry.up ? nominalWidth : 0)
            : attach.x() - (entry.up ? note->headBodyWidth() - nominalWidth : 0);
        const qreal stemWidth = chord->stem()->lineWidth() * chord->mag();
        entry.stemX = chord->x() + anchor + (entry.up ? -.5 : .5) * stemWidth;
        if (!(stemWidth > 0) || !std::isfinite(stemWidth) || !std::isfinite(entry.stemX))
            return fail("Invalid cross-staff stem anchor");
        for (const Element* item : chord->segment()->elist()) {
            if (!item || item == chord)
                continue;
            if (!item->isChord() || !item->visible() || !item->staff()->show())
                return fail("Unadapted companion voice in cross-staff context");
            const Chord* companion = toChord(item);
            if (companion->beam() || companion->staffMove() || companion->isGrace() || companion->tuplet()
                || companion->tremolo() || companion->isSmall() || companion->notes().empty())
                return fail("Independent moving or decorated voices need a later cross-staff adapter");
            for (const Note* note : companion->notes()) {
                SpacingPairPolicy4 policy;
                if (!note->visible() || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
                    return fail("Unadapted companion note context");
            }
        }
    }
    const qreal headWidth = score->noteHeadWidth();
    const qreal displacement = headWidth - score->styleP(Sid::stemWidth);
    qreal stemPadding = 0;
    if (!(headWidth > 0) || !(displacement > 0) || !std::isfinite(displacement)
        || !table.padding(ElementType::STEM, ElementType::STEM, stemPadding)
        || stemPadding < 0 || !std::isfinite(stemPadding))
        return fail("Invalid cross-staff spacing dimensions");
    for (size_t i = 1; i < context.chords.size(); ++i) {
        const CrossStaffChord4& a = context.chords[i - 1];
        const CrossStaffChord4& b = context.chords[i];
        CrossStaffPair4 pair;
        pair.first = a.chord;
        pair.second = b.chord;
        pair.adjacentSegments = a.chord->segment()->next(SegmentType::ChordRest) == b.chord->segment();
        pair.canAdjust = pair.adjacentSegments;
        for (int side = 0; side < 2; ++side) {
            const Chord* chord = side ? b.chord : a.chord;
            for (const Element* item : chord->segment()->elist()) {
                if (item && item != chord && item->isChordRest()
                    && (side || toChordRest(item)->actualTicks() <= chord->segment()->ticks()))
                    pair.canAdjust = false;
            }
        }
        if (a.up != b.up) {
            pair.widthOffset = pair.canAdjust ? (a.up ? displacement : -displacement) : 0;
            if (a.up) {
                if (pair.adjacentSegments)
                    pair.minimumWidth = std::max(2 * displacement, headWidth + stemPadding);
                // Source minStemDistOnNonAdjacentCross also applies to
                // adjacent segments. Keep it explicit for later lookback.
                pair.minimumStemDistance = std::max(qreal(0), a.stemX - b.stemX + stemPadding);
                if (pair.adjacentSegments)
                    pair.minimumWidth = std::max(pair.minimumWidth, pair.minimumStemDistance);
            }
        }
        context.pairs.push_back(pair);
    }
    result = context;
    if (reason) reason->clear();
    return true;
}

static bool buildSpacingPlanImpl4(System* system, SpacingPlan4& result, QString* reason, const SpacingPlanOptions4& options,
                                 const std::vector<GraceMergedGeometry4>* preparedMerged, const SpacingInput4* input,
                                 const SpacingAttachmentPreparation4* preparation,
                                 const SpacingPlan4* prefix = nullptr)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::PLAN);
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!system || system->measures().empty())
        return fail("No system measures");
    if (!std::isfinite(options.squeezeFactor) || options.squeezeFactor < 0 || options.squeezeFactor > 1
        || !std::isfinite(options.stretchReduction) || options.stretchReduction < 0)
        return fail("Invalid spacing factors");
    Score* score = system->score();
    if (!score)
        return fail("No score");
    if (input && input->score() != score)
        return fail("Prepared musical inputs belong to another score");
    const Measure* predecessor = nullptr;
    const bool refresh = prefix && prefix->trialOnly;
    if (prefix) {
        if (!spacingCandidateSignaturesReady4(system, reason, true)) return false;
        const auto& measures = system->measures();
        if (measures.size() < 2 || prefix->measures.size() != measures.size() - (refresh ? 0 : 1)
            || !prefix->afterGraces.empty() || !prefix->mergedGraces.empty())
            return fail("Candidate courtesy has no valid prefix seed");
        const Measure* penultimate = toMeasure(measures[measures.size() - 2]);
        if (refresh) {
            if (penultimate->trailer() || prefix->measures.back().measure != measures.back()
                || !system->lastMeasure()->trailer())
                return fail("Accepted courtesy trial has not been finalized");
        } else {
            if (!penultimate->trailer())
                return fail("Candidate prefix seed has no pending penultimate courtesy");
            predecessor = penultimate;
        }
    }
    for (int st = 0; st < score->nstaves(); ++st) {
        if (!score->staff(st)->show() || !system->staff(st)->show())
            return fail("Hidden staves require a later adapter");
    }
    CrossStaffGeometry4 cross;
    if (!prepareCrossStaff4(system, cross, reason)) return false;
    SpacingGraceContext4 graces;
    std::vector<GraceAfterGeometry4> afters;
    std::vector<GraceMergedGeometry4> merged;
    // Grace packing/projection and the final system constraints are all
    // read-only. Borrow the same input owner and query-local memo throughout;
    // no standalone grace query silently drops the prepared attachment view.
    if (options.slurPolicy != SpacingSlurPolicy4::COMPATIBILITY
        && options.slurPolicy != SpacingSlurPolicy4::PREPARED) return fail("Invalid slur packing policy");
    if (options.slurPolicy == SpacingSlurPolicy4::PREPARED && !preparation)
        return fail("Prepared slur policy requires an attachment owner");
    SpacingContextCache4 contextCache = preparation ? SpacingContextCache4(*preparation, options.slurPolicy)
                                                  : SpacingContextCache4(input);
    if (!prepareGraces4(system, graces, afters, merged, reason, preparedMerged, contextCache)) return false;
    SpacingPlan4 plan;
    for (const auto& after : afters) plan.afterGraces.push_back(after.plan);
    for (const auto& joint : merged) plan.mergedGraces.push_back(joint.plan);
    std::vector<Geometry4> geometry;
    const SpacingPaddingTable4 table(score);
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure() || toMeasure(mb)->isMMRest())
            return fail("Frames and MMRest measures are outside the common plan");
        Measure* measure = toMeasure(mb);
        plan.measures.push_back(SpacingMeasurePlan4(measure));
        for (Segment& segment : measure->segments()) {
            SpacingSegmentPlan4 entry(&segment);
            entry.ignored = !active4(&segment);
            if (entry.ignored && segment.isChordRestType())
                return fail("Inactive rhythmic segments require a later adapter");
            Geometry4 g;
            if (!entry.ignored) {
                if (courtesyType4(&segment) && (!segment.trailer()
                    || (measure != system->lastMeasure() && measure != predecessor)
                    || segment.rtick() != measure->ticks()))
                    return fail("Courtesy signatures require a terminal measure-end trailer");
                if (!eligibleType4(&segment) || (segment.trailer() && !courtesyType4(&segment))
                    || segment.extraLeadingSpace().val() != 0
                    || (segment.spacingWidthOffset4() != 0 && !cross.pairs.count(&segment)
                        && !carriesRightAlignmentOffset4(&segment)))
                    return fail("Unadapted segment type, trailer, manual space or cross-beam offset");
                if (segment.isBreathType() && (!segment.ticks().isZero() || segment.rtick() <= Fraction(0, 1)
                                               || segment.rtick() > measure->ticks()))
                    return fail("Breath spacing requires a zero-duration internal or end boundary");
                if (segment.isStartRepeatBarLineType()
                    && (!measure->repeatStart() || !segment.rtick().isZero() || !segment.ticks().isZero()
                        || !ordinaryRepeatBoundary4(measure)))
                    return fail("Start-repeat spacing requires an ordinary zero-duration boundary");
                for (const Element* annotation : segment.annotations()) {
                    if (!annotation)
                        continue;
                    if (!annotation->isSystemText() && !annotation->isStaffText() && !annotation->isRehearsalMark()
                        && !annotation->isTempoText() && !annotation->isDynamic())
                        return fail("Annotations require a later adapter");
                }
                for (const Element* item : segment.elist()) {
                    if (item && segment.isStartRepeatBarLineType()) {
                        if (!item->isBarLine()) return fail("Unexpected start-repeat item");
                        const BarLine* barline = toBarLine(item);
                        if (barline->barLineType() != BarLineType::START_REPEAT
                            || !barline->staff() || !barline->staff()->isPitchedStaff(barline->tick())
                            || !barline->visible() || !barline->addToSkyline() || !barline->el()->empty()
                            || !(barline->mag() > 0) || !std::isfinite(barline->mag())
                            || !(barline->layoutRect().width() > 0) || !(barline->layoutRect().height() > 0))
                            return fail("Unadapted start-repeat glyph, staff, decoration or dimensions");
                    }
                    if (item && segment.isBreathType()) {
                        if (!item->isBreath()) return fail("Unexpected breath-segment item");
                        const Breath* breath = toBreath(item);
                        const bool known = std::any_of(Breath::breathList.begin(), Breath::breathList.end(),
                            [breath](const BreathType& type) { return type.id == breath->symId(); });
                        if (!known || !breath->staff() || !breath->staff()->isPitchedStaff(breath->tick())
                            || !breath->visible() || !breath->addToSkyline()
                            || !(breath->mag() > 0) || !std::isfinite(breath->mag())
                            || !(breath->bbox().width() > 0) || !(breath->bbox().height() > 0))
                            return fail("Unadapted breath glyph, staff, visibility or dimensions");
                    }
                    if (item && segment.isClefType()) {
                        if (!item->isClef()) return fail("Unexpected clef-segment item");
                        const Clef* clef = toClef(item);
                        const ClefType type = clef->clefType();
                        if (type == ClefType::INVALID || type == ClefType::MAX
                            || ClefInfo::staffGroup(type) != StaffGroup::STANDARD
                            || !clef->visible() || !clef->addToSkyline()
                            || !(clef->mag() > 0) || !std::isfinite(clef->mag()))
                            return fail("Unadapted clef visibility, staff family or magnitude");
                    }
                    if (item && item->isRest()) {
                        const Rest* rest = toRest(item);
                        if (rest->isGap())
                            return fail("Gap rests require a later adapter");
                        if (rest->isFullMeasureRest() && !ordinaryMeasureRest4(rest))
                            return fail("Decorated, hidden or moved measure rests require a later adapter");
                    }
                    if (!item || !item->isChord())
                        continue;
                    const Chord* chord = toChord(item);
                    if (chord->tremolo())
                        return fail("Tremolo spacing requires a later adapter");
                    if ((chord->staffMove() != 0 || (chord->beam() && chord->beam()->cross()))
                        && !cross.chords.count(chord))
                        return fail("Cross-staff geometry requires its later stem constraints");
                    if (!chord->graceNotes().empty() && !graces.contains(chord) && !graces.containsAfter(chord))
                        return fail("Unadapted grace-note rhythm");
                    for (const Note* note : chord->notes()) {
                        SpacingPairPolicy4 policy;
                        if (!note->visible() || !spacingPairPolicy4(table, note, note, note->spatium(), policy, &graces, &contextCache))
                            return fail("Unadapted note visibility or decoration context");
                    }
                }
                if (!cacheGeometry4(&segment, table, g, cross, graces, afters, merged, contextCache))
                    return fail(QString("Unadapted geometry at tick %1").arg(segment.tick().ticks()));
                if (segment.isChordRestType()) {
                    if (!(segment.ticks() > Fraction(0, 1)))
                        return fail("Chord/rest duration is not ready");
                    entry.stretch = segmentSpacingStretch4(&segment);
                    if (!(entry.stretch > 0) || !std::isfinite(entry.stretch))
                        return fail("Invalid rhythmic stretch");
                }
            }
            const auto correction = cross.pairs.find(&segment);
            if (correction != cross.pairs.end()) entry.widthOffset = correction->second.widthOffset;
            plan.segments.push_back(entry);
            geometry.push_back(g);
        }
    }
    qreal cursor = system->leftMargin();
    bool placed = false;
    size_t firstNew = 0;
    size_t collisionFirst = 0;
    size_t firstMutableMeasure = 0;
    if (prefix) {
        // Studio updates only the predecessor/new-measure pair. Earlier
        // measures retain both their positions and their occupied widths.
        const size_t frozenMeasures = prefix->measures.size() - (refresh ? 1 : 0);
        firstMutableMeasure = frozenMeasures - 1;
        const Segment* lastActive = nullptr;
        bool removedCourtesy = false;
        for (size_t mi = 0; mi < frozenMeasures; ++mi) {
            auto& measure = plan.measures[mi];
            const auto& savedMeasure = prefix->measures[mi];
            const bool finalizedPredecessor = refresh && mi == firstMutableMeasure;
            if (mi == firstMutableMeasure) collisionFirst = firstNew;
            if (measure.measure != savedMeasure.measure || !std::isfinite(savedMeasure.x)
                || !std::isfinite(savedMeasure.width) || savedMeasure.width <= 0
                || qAbs(savedMeasure.x - cursor) > 1e-6)
                return fail("Candidate prefix measure identity or extent changed");
            measure = savedMeasure;
            placed = false;
            for (; firstNew < plan.segments.size()
                && plan.segments[firstNew].segment->measure() == measure.measure; ++firstNew) {
                auto& current = plan.segments[firstNew];
                if (firstNew >= prefix->segments.size())
                    return fail("Candidate prefix segment list changed");
                const auto& saved = prefix->segments[firstNew];
                if (saved.segment != current.segment)
                    return fail("Candidate prefix segment identity changed");
                if (saved.ignored != current.ignored) {
                    if (finalizedPredecessor && current.ignored && courtesyType4(current.segment)
                        && current.segment->trailer() && current.segment->rtick() == measure.measure->ticks()
                        && lastActive && lastActive->measure() == measure.measure && lastActive->isEndBarLineType()) {
                        if (!removedCourtesy && (!std::isfinite(saved.x) || qAbs(cursor - saved.x) > 1e-6))
                            return fail("Accepted trial's former courtesy edge changed");
                        removedCourtesy = true;
                        continue;
                    }
                    return fail("Candidate prefix segment identity or visibility changed");
                }
                if (current.ignored) continue;
                if (!std::isfinite(saved.x) || !std::isfinite(saved.width) || saved.width < 0
                    || saved.widthOffset != 0 || saved.stretch != current.stretch
                    || saved.x < cursor - 1e-6 || (placed && qAbs(saved.x - cursor) > 1e-6))
                    return fail("Candidate prefix segment positions or advances are invalid");
                current.x = saved.x;
                current.width = saved.width;
                cursor = saved.x + saved.width;
                lastActive = current.segment;
                placed = true;
            }
            // Studio loads the surviving end barline's saved advance even
            // when the courtesy that used to follow it has been disabled.
            // Reconstruct this measure's width below with the new barline.
            if (!placed || !std::isfinite(cursor)
                || (!finalizedPredecessor && qAbs(cursor - (measure.x + measure.width)) > 1e-6))
                return fail("Candidate prefix measure has no matching segment edge");
        }
        if (refresh) {
            if (!removedCourtesy || !lastActive || !lastActive->isEndBarLineType()
                || lastActive->measure() != plan.measures[firstMutableMeasure].measure
                || firstNew == plan.segments.size() || firstNew >= prefix->segments.size()
                || prefix->segments[firstNew].segment != plan.segments[firstNew].segment)
                return fail("Accepted trial has no matching finalized courtesy boundary");
        } else {
            if (firstNew != prefix->segments.size() || firstNew == plan.segments.size()
                || !lastActive || !courtesyType4(lastActive) || !std::isfinite(prefix->width)
                || qAbs(cursor - prefix->width) > 1e-6)
                return fail("Candidate prefix has no matching terminal courtesy edge");
            plan.trialOnly = true;
        }
    }
    for (size_t i = firstNew; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored) {
            current.x = cursor;
            continue;
        }
        qreal positionedX = cursor;
        if (!placed) {
            qreal offset = 0;
            if (current.segment->isChordRestType()) {
                offset = std::max(qreal(0), -geometry[i].leftEdge);
                offset += score->styleP(geometry[i].accidentals ? Sid::barAccidentalDistance : Sid::barNoteDistance);
                // Studio default for its style absent from Evolution.
                offset = std::max(offset, 1.25 * current.segment->spatium());
            }
            else if (current.segment->isHeaderClefType() || current.segment->isClefType())
                offset = score->styleP(Sid::clefLeftMargin);
            else if (current.segment->isKeySigType())
                offset = score->styleP(Sid::keysigLeftMargin);
            else if (current.segment->isTimeSigType())
                offset = score->styleP(Sid::timesigLeftMargin);
            cursor += offset * options.squeezeFactor;
            positionedX = cursor;
            placed = true;
        }
        else {
            qreal x = -std::numeric_limits<qreal>::max();
            // A beam neighbor can lie beyond the ordinary geometry lookback.
            const Segment* requiredPrevious = nullptr;
            for (const auto& pair : cross.pairs) {
                if (pair.second.minimumStemDistance > 0 && pair.second.second->segment() == current.segment)
                    requiredPrevious = pair.second.first->segment();
            }
            int crCount = 0;
            for (size_t j = i; j > collisionFirst;) {
                --j;
                SpacingSegmentPlan4& previous = plan.segments[j];
                if (previous.ignored)
                    continue;
                if (previous.segment->isChordRestType())
                    ++crCount;
                qreal distance = 0;
                if (!distance4(table, previous, geometry[j], current, geometry[i], plan.measures.front().measure,
                               options.squeezeFactor, distance, cross, graces, contextCache))
                    return fail("Unsupported segment distance");
                x = std::max(x, previous.x + distance);
                if (x > cursor) {
                    if (crCount > 0) {
                        const qreal increase = (x - cursor) / crCount;
                        qreal movement = increase;
                        for (size_t k = j + 1; k < i; ++k) {
                            if (plan.segments[k].ignored)
                                continue;
                            plan.segments[k].x += movement;
                            if (plan.segments[k].segment->isChordRestType())
                                movement += increase;
                        }
                    }
                    cursor = x;
                }
                const Fraction previousEnd = previous.segment->tick() + previous.segment->ticks();
                if (previousEnd < current.segment->tick()
                    && (!requiredPrevious || previous.segment->tick() <= requiredPrevious->tick())) {
                    if (!current.segment->isChordRestType()
                        || cursor + geometry[i].leftEdge > previous.x + geometry[j].minRight
                        || (previous.segment->measure() != current.segment->measure()
                            && previous.segment->measure() != current.segment->measure()->prevMeasure()))
                        break;
                }
            }
            // Studio spaces right-aligned marks from collision geometry,
            // independently of the rhythmic cursor. Reserving the entire
            // cursor advance here would widen the surrounding note interval.
            positionedX = rightAligned4(current.segment) ? x : cursor;
        }
        current.x = positionedX;
        if (current.segment->isChordRestType()) {
            if (current.segment->measure() == plan.measures.front().measure && current.segment->rtick().isZero()) {
                cursor = std::max(cursor, system->leftMargin() + 1.25 * current.segment->spatium());
                current.x = cursor;
            }
            qreal advance = 3.5 * current.segment->spatium() * current.stretch * options.stretchReduction
                + current.widthOffset;
            const auto correction = cross.pairs.find(current.segment);
            if (correction != cross.pairs.end() && correction->second.minimumWidth > 0)
                advance = std::max(advance, correction->second.minimumWidth);
            if (advance < 0) return fail("Cross-staff advance needs stronger compression support");
            cursor += advance;
        }
    }
    if (!placed)
        return fail("No active segments");
    if (!moveRightAligned4(plan, geometry, table, options.squeezeFactor, cross, graces, contextCache))
        return fail("Unadapted right-aligned segment geometry");
    plan.measures.front().x = system->leftMargin();
    std::map<Measure*, size_t> measureIndex;
    for (size_t i = 0; i < plan.measures.size(); ++i)
        measureIndex[plan.measures[i].measure] = i;
    for (size_t i = collisionFirst; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored)
            continue;
        size_t next = i + 1;
        while (next < plan.segments.size() && plan.segments[next].ignored)
            ++next;
        const size_t mi = measureIndex[current.segment->measure()];
        if (next == plan.segments.size()) {
            current.width = geometry[i].minRight;
            if (current.segment->trailer())
                current.width += score->styleP(Sid::systemTrailerRightMargin);
            plan.measures[mi].width = current.x + current.width - plan.measures[mi].x;
        }
        else {
            SpacingSegmentPlan4& following = plan.segments[next];
            const bool sameMeasure = current.segment->measure() == following.segment->measure();
            const bool leadingHeader = leadingNonRhythmic4(following.segment) && !current.segment->isType(SegmentType::BarLineType);
            current.width = sameMeasure || following.segment->isStartRepeatBarLineType() || leadingHeader
                ? following.x - current.x : geometry[i].minRight;
            if (!sameMeasure) {
                plan.measures[mi].width = current.x + current.width - plan.measures[mi].x;
                plan.measures[measureIndex[following.segment->measure()]].x = current.x + current.width;
            }
        }
        if (current.width < 0 || !std::isfinite(current.width))
            return fail("Invalid reconstructed segment width");
    }
    if (options.minimumMeasureWidths) {
        const qreal available = system->width() - system->leftMargin();
        for (size_t mi = firstMutableMeasure; mi < plan.measures.size(); ++mi) {
            SpacingMeasurePlan4& measure = plan.measures[mi];
            qreal minimum = score->styleP(Sid::minMeasureWidth);
            const qreal cap = available > 0 ? available : minimum;
            minimum = std::min(minimum, cap);
            if (measure.measure->ticks() < measure.measure->timesig())
                minimum *= (measure.measure->ticks() / measure.measure->timesig()).toDouble();
            for (size_t i = 0; i < plan.segments.size(); ++i) {
                const SpacingSegmentPlan4& firstCR = plan.segments[i];
                if (firstCR.ignored || firstCR.segment->measure() != measure.measure || !firstCR.segment->isChordRestType())
                    continue;
                if (firstCR.segment != measure.measure->firstEnabled()) {
                    const qreal start = firstCR.x - measure.x - geometry[i].minLeft
                        - score->styleP(geometry[i].accidentals ? Sid::barAccidentalDistance : Sid::barNoteDistance);
                    minimum = std::min(minimum + start, cap);
                }
                break;
            }
            if (minimum > measure.width) {
                const qreal extra = minimum - measure.width;
                if (!stretchMeasure4(plan, measure, extra))
                    return fail("Minimum-width springs cannot be solved");
                const qreal delta = reflowMeasure4(plan, measure);
                if (qAbs(delta - extra) > 1e-6 * measure.measure->spatium())
                    return fail("Minimum-width reconstruction lost space");
                for (size_t j = mi + 1; j < plan.measures.size(); ++j)
                    moveMeasure4(plan, plan.measures[j], delta);
            }
        }
    }
    for (const SpacingMeasurePlan4& measure : plan.measures) {
        if (!std::isfinite(measure.x) || !std::isfinite(measure.width) || measure.width <= 0)
            return fail("Invalid reconstructed measure width");
    }
    plan.width = plan.measures.back().x + plan.measures.back().width;
    result = plan;
    if (reason)
        reason->clear();
    return true;
}

bool buildSpacingPlan4(System* system, SpacingPlan4& result, QString* reason, const SpacingPlanOptions4& options,
                       const std::vector<GraceMergedGeometry4>* preparedMerged, const SpacingInput4* input)
{
    return buildSpacingPlanImpl4(system, result, reason, options, preparedMerged, input, nullptr);
}

bool buildSpacingPlan4(System* system, SpacingPlan4& result, const SpacingAttachmentPreparation4& preparation,
                       QString* reason, const SpacingPlanOptions4& options,
                       const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    return buildSpacingPlanImpl4(system, result, reason, options, preparedMerged, &preparation.input(), &preparation);
}

static bool squeezeSpacingPlanImpl4(System* system, qreal targetWidth, SpacingPlan4& result, QString* reason,
                                   const std::vector<GraceMergedGeometry4>* preparedMerged,
                                   const SpacingAttachmentPreparation4* preparation, SpacingSlurPolicy4 policy,
                                   bool retainedOverfullGroup = false)
{
    if (!std::isfinite(targetWidth) || targetWidth <= 0) {
        if (reason)
            *reason = "Invalid squeeze target";
        return false;
    }
    SpacingPlan4 candidate;
    const SpacingInput4* input = preparation ? &preparation->input() : nullptr;
    SpacingPlanOptions4 natural;
    natural.slurPolicy = policy;
    if (!buildSpacingPlanImpl4(system, candidate, reason, natural, preparedMerged, input, preparation))
        return false;
    if (candidate.width <= targetWidth) {
        result = candidate;
        return true;
    }
    // Studio 4.7.5 squeezeSystemToFit recomputes from native geometry each
    // pass, with minimum-measure widths overridden. For a retained group,
    // also try the two tighter padding levels, with nonnegative advances.
    const bool tightGroup = retainedOverfullGroup && system->score()->nstaves() == 1
        && candidate.afterGraces.empty() && candidate.mergedGraces.empty()
        && std::all_of(candidate.segments.begin(), candidate.segments.end(), [](const SpacingSegmentPlan4& s) {
            if (s.ignored) return true;
            if (s.widthOffset != 0 || rightAligned4(s.segment)) return false;
            if (!s.segment->isChordRestType())
                return headerType4(s.segment) || s.segment->isEndBarLineType() || s.segment->isBeginBarLineType();
            for (const Element* item : s.segment->elist())
                if (item && !item->isChord()) return false;
            return true;
        });
    SpacingPlanOptions4 options;
    options.slurPolicy = policy;
    options.minimumMeasureWidths = false;
    options.stretchReduction = 1 - 0.33;
    options.squeezeFactor = 1 - 0.2;
    for (int pass = 0; pass < (tightGroup ? 5 : 3); ++pass) {
        if (pass >= 3) {
            options.stretchReduction = 0;
            options.squeezeFactor = pass == 3 ? 0.2 : 0;
        }
        if (!buildSpacingPlanImpl4(system, candidate, reason, options, preparedMerged, input, preparation))
            return false;
        if (candidate.width <= targetWidth) {
            result = candidate;
            return true;
        }
        options.stretchReduction -= 0.33;
        options.squeezeFactor -= 0.2;
    }
    if (tightGroup) {
        // Last resort in Studio: reduce only rhythmic advances and rebuild
        // segment and measure positions together, even if notes overlap.
        for (int percent = 95; percent > 0; percent -= 5) {
            for (auto& s : candidate.segments)
                if (!s.ignored && s.segment->isChordRestType())
                    s.width *= percent / 100.0;
            qreal edge = system->leftMargin();
            for (auto& m : candidate.measures) {
                moveMeasure4(candidate, m, edge - m.x);
                reflowMeasure4(candidate, m);
                edge += m.width;
            }
            candidate.width = edge;
            if (edge <= targetWidth) {
                result = std::move(candidate);
                return true;
            }
        }
    }
    if (reason)
        *reason = "Target needs stronger compression than the adapted squeeze passes";
    return false;
}

bool squeezeSpacingPlan4(System* system, qreal targetWidth, SpacingPlan4& result, QString* reason,
                         const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    return squeezeSpacingPlanImpl4(system, targetWidth, result, reason, preparedMerged, nullptr, SpacingSlurPolicy4::COMPATIBILITY);
}

bool squeezeSpacingPlan4(System* system, qreal targetWidth, SpacingPlan4& result,
                         const SpacingAttachmentPreparation4& preparation, QString* reason,
                         const std::vector<GraceMergedGeometry4>* preparedMerged, SpacingSlurPolicy4 policy)
{
    return squeezeSpacingPlanImpl4(system, targetWidth, result, reason, preparedMerged, &preparation, policy);
}

bool spacingSystemJustified4(const Measure* last, const MeasureBase* next,
                             qreal naturalWidth, qreal availableWidth, qreal fillLimit)
{
    if (last && last->sectionBreak()) next = nullptr;
    while (next) {
        if (next->isMeasure() || next->isHBox()) return true;
        if (next->sectionBreak()) break;
        next = next->next();
    }
    return !(availableWidth > 0 && naturalWidth / availableWidth < fillLimit);
}

bool selectSystemSpacingPlan4(System* system, const SpacingSystemRequest4& request,
                              const SpacingAttachmentPreparation4& attachments, SpacingPlan4& result,
                              QString* reason, const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->measures().empty()) return fail("Missing spacing system");
    if (request.pass != SpacingPass4::CANDIDATE && request.pass != SpacingPass4::FINAL)
        return fail("Invalid spacing system pass");
    if (!(request.availableWidth > 0) || !std::isfinite(request.availableWidth))
        return fail("Invalid spacing system width");
    if (request.pass == SpacingPass4::FINAL && !std::isfinite(request.fillLimit))
        return fail("Invalid spacing system fill limit");
    if (request.pass == SpacingPass4::FINAL && request.previousPrefix)
        return fail("Final spacing cannot use a pending courtesy seed");
    SpacingPlan4 plan;
    if (request.pass == SpacingPass4::CANDIDATE) {
        SpacingPlanOptions4 options;
        options.slurPolicy = request.slurPolicy;
        if (!buildSpacingPlanImpl4(system, plan, reason, options, preparedMerged, &attachments.input(),
                                  &attachments, request.previousPrefix)) return false;
    }
    else {
        if (!squeezeSpacingPlanImpl4(system, request.availableWidth, plan, reason, preparedMerged,
                                    &attachments, request.slurPolicy, request.retainedOverfullGroup)) return false;
        if (spacingSystemJustified4(system->lastMeasure(), request.next, plan.width, request.availableWidth,
                                    request.fillLimit) && !justifySpacingPlan4(plan, request.availableWidth))
            return fail("Final plan justification rejected");
    }
    result = std::move(plan);
    if (reason) reason->clear();
    return true;
}

bool justifySpacingPlan4(SpacingPlan4& target, qreal targetWidth)
{
    if (target.measures.empty() || !std::isfinite(targetWidth) || targetWidth < target.width)
        return false;
    if (targetWidth == target.width)
        return true;
    SpacingPlan4 plan = target;
    std::vector<SpacingSpring4> springs;
    for (const SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.stretch > 0)
            springs.push_back({ segment.width - segment.widthOffset, segment.stretch });
    }
    const qreal force = spacing4SpringForce(springs, targetWidth - plan.width);
    if (!(force > 0))
        return false;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.stretch > 0)
            segment.width = std::max(segment.width - segment.widthOffset, force * segment.stretch) + segment.widthOffset;
    }
    qreal movement = 0;
    for (SpacingMeasurePlan4& measure : plan.measures) {
        moveMeasure4(plan, measure, movement);
        movement += reflowMeasure4(plan, measure);
    }
    plan.width = plan.measures.back().x + plan.measures.back().width;
    if (qAbs(plan.width - targetWidth) > 1e-6)
        return false;
    target = plan;
    return true;
}

bool spacingPlanPublicationValid4(const System* system, const SpacingPlan4& plan, QString* reason)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || plan.measures.empty() || !(plan.width > 0) || !std::isfinite(plan.width))
        return fail("Invalid publication system or width");
    if (plan.trialOnly) return fail("Pending courtesy trial cannot be published");
    std::map<const Segment*, qreal> segmentX;
    std::set<const Chord*> mains;
    std::vector<const Rest*> measureRests;
    size_t mi = 0, si = 0;
    qreal edge = system->leftMargin();
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mi >= plan.measures.size()) return fail("Changed publication measure domain");
        const Measure* measure = toMeasure(mb);
        const auto& m = plan.measures[mi++];
        if (m.measure != measure || measure->system() != system || !std::isfinite(m.x)
            || !(m.width > 0) || !std::isfinite(m.width) || !std::isfinite(edge)
            || qAbs(m.x - edge) > 1e-6) return fail("Invalid publication measure placement");
        edge = m.x + m.width;
        for (const Segment& segment : measure->segments()) {
            if (si >= plan.segments.size()) return fail("Incomplete publication segment domain");
            const auto& s = plan.segments[si++];
            if (s.segment != &segment || !segmentX.emplace(&segment, s.x - m.x).second || s.ignored != !active4(&segment)
                || !std::isfinite(s.x) || !std::isfinite(s.width) || s.width < 0
                || !std::isfinite(s.x - m.x)
                || !std::isfinite(s.stretch) || s.stretch < 0 || !std::isfinite(s.widthOffset)
                || (!s.ignored && segment.isChordRestType() && !(s.stretch > 0)))
                return fail("Invalid publication segment placement");
            if (!s.ignored) for (const Element* item : segment.elist()) {
                if (item && item->isChord()) mains.insert(toChord(item));
                if (item && item->isRest() && toRest(item)->isFullMeasureRest()) {
                    if (!ordinaryMeasureRest4(toRest(item))) return fail("Changed publication measure-rest context");
                    measureRests.push_back(toRest(item));
                }
            }
        }
    }
    if (mi != plan.measures.size() || si != plan.segments.size() || !std::isfinite(edge)
        || qAbs(edge - plan.width) > 1e-6) return fail("Incomplete publication plan extent");
    // Measure::layoutElementsAfterSpacing centers these bodies after segment
    // publication. Validate that exact conversion now, before any native write.
    // Admitted rests start at tick zero: preceding shapes are fixed headers,
    // and the following non-rhythmic boundary is a barline/courtesy signature.
    for (const Rest* rest : measureRests) {
        const Segment* segment = rest->segment();
        qreal x1 = 0, x2 = 0;
        for (const auto& m : plan.measures) if (m.measure == rest->measure()) x2 = m.width;
        const Segment* previous = segment->prevActive();
        while (previous && previous->allElementsInvisible()) previous = previous->prevActive();
        for (const Segment* s = previous; s; s = s->prevActive()) {
            const qreal right = segmentX.at(s) + s->minRight();
            if (!std::isfinite(right)) return fail("Invalid publication rest header extent");
            x1 = std::max(x1, right);
        }
        for (const Segment* s = segment->nextActive(); s; s = s->nextActive()) {
            if (!s->isChordRestType() && s->element(rest->staffIdx() * VOICES)) {
                x2 = segmentX.at(s) - s->minLeft();
                break;
            }
        }
        const qreal x = (x2 - x1 - rest->width()) * .5 + x1 - segmentX.at(segment) - rest->bbox().x();
        if (!std::isfinite(x2) || !std::isfinite(x) || !std::isfinite(x + rest->offset().x()))
            return fail("Invalid publication centered-rest position");
    }
    std::set<const Chord*> owners, members;
    const auto positions = [&](const std::vector<GraceAfterPosition4>& written, const QVector<Chord*>& expected,
                               const Segment* target) {
        const auto targetX = segmentX.find(target);
        if (targetX == segmentX.end()) return false;
        if (written.size() != static_cast<size_t>(expected.size())) return false;
        for (size_t i = 0; i < written.size(); ++i) {
            const Chord* chord = expected[int(i)];
            if (!chord || written[i].chord != chord || !std::isfinite(written[i].x)
                || !chord->parent() || !chord->parent()->isChord()
                || !mains.count(toChord(chord->parent())) || !std::isfinite(toChord(chord->parent())->x())
                || !members.insert(chord).second) return false;
            const Chord* parent = toChord(chord->parent());
            const auto parentX = segmentX.find(parent->segment());
            if (parentX == segmentX.end()
                || !std::isfinite(targetX->second - parentX->second - parent->x() + written[i].x)) return false;
        }
        return true;
    };
    for (const auto& group : plan.afterGraces) {
        if (!mains.count(group.owner) || !segmentX.count(group.target) || !owners.insert(group.owner).second
            || !positions(group.positions, group.owner->graceNotesAfter(), group.target))
            return fail("Invalid publication after-grace ownership");
    }
    for (const auto& group : plan.mergedGraces) {
        if (!mains.count(group.owner) || !mains.count(group.target) || group.owner == group.target
            || !owners.insert(group.owner).second) return fail("Invalid publication merged-grace ownership");
        auto expected = group.owner->graceNotesAfter();
        for (Chord* chord : group.target->graceNotesBefore()) expected.push_back(chord);
        if (!positions(group.positions, expected, group.target->segment())) return fail("Invalid publication merged-grace members");
    }
    for (const Chord* main : mains)
        if (!main->graceNotesAfter().empty() && !owners.count(main)) return fail("Missing publication after-grace group");
    if (reason) reason->clear();
    return true;
}

bool publishSpacingPlanSegments4(System* system, const SpacingPlan4& plan, QString* reason)
{
    if (!spacingPlanPublicationValid4(system, plan, reason)) return false;
    std::map<const Measure*, qreal> origins;
    for (const auto& m : plan.measures) origins[m.measure] = m.x;
    // No fallible validation/query follows the first write. All segment and
    // late grace destinations have already passed complete-domain preflight.
    for (const auto& s : plan.segments) {
        s.segment->rxpos() = s.x - origins.at(s.segment->measure());
        s.segment->setWidth(s.width);
        s.segment->setStretch(s.stretch);
        s.segment->setSpacingWidthOffset4(s.widthOffset);
    }
    return true;
}

bool spacingPlanMatches4(const SpacingPlan4& plan, QString* reason)
{
    const auto equal = [reason](qreal actual, qreal expected, const char* field) {
        if (std::isfinite(actual) && std::isfinite(expected) && qAbs(actual - expected) <= 1e-6)
            return true;
        if (reason) *reason = QString("Changed horizontal %1").arg(field);
        return false;
    };
    if (plan.measures.empty() || !plan.measures.front().measure
        || !plan.measures.front().measure->system()) return false;
    const System* system = plan.measures.front().measure->system();
    if (!equal(system->width(), plan.width, "system width")) return false;
    for (const auto& m : plan.measures) {
        if (!m.measure || m.measure->system() != system
            || !equal(m.measure->x(), m.x, "measure x")
            || !equal(m.measure->width(), m.width, "measure width")) return false;
    }
    for (const auto& s : plan.segments) {
        // Inactive headers can retain cache-only positions after earlier
        // break choices. The fresh plan excludes their geometry as well.
        if (s.ignored) continue;
        if (!s.segment || !s.segment->measure() || s.segment->measure()->system() != system
            || !equal(s.segment->measure()->x() + s.segment->x(), s.x, "segment x")
            || !equal(s.segment->width(), s.width, "segment width")
            || !equal(s.segment->stretch(), s.stretch, "segment stretch")
            || !equal(s.segment->spacingWidthOffset4(), s.widthOffset, "segment width offset")) return false;
    }
    const auto graceMatches = [&equal](const GraceAfterPosition4& g, const Segment* target) {
        if (!g.chord || !g.chord->parent() || !g.chord->parent()->isChord() || !target) return false;
        const Chord* parent = toChord(g.chord->parent());
        if (!parent->segment()) return false;
        return equal(g.chord->x() + parent->x() + parent->segment()->x() - target->x(), g.x, "grace x");
    };
    for (const auto& after : plan.afterGraces)
        for (const auto& g : after.positions)
            if (!graceMatches(g, after.target)) return false;
    for (const auto& merged : plan.mergedGraces) {
        if (!merged.target) return false;
        for (const auto& g : merged.positions)
            if (!graceMatches(g, merged.target->segment())) return false;
    }
    return true;
}

}
