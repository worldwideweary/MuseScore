// Adapted from Studio v4.7.5 TLayout::layoutGraceNotesGroup.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "gracespacing4.h"
#include "spacingcollision4.h"
#include "spacinggeometry4.h"
#include "beam.h"
#include "barline.h"
#include "chord.h"
#include "note.h"
#include "rest.h"
#include "measure.h"
#include "stem.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "slur.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Ms {
bool spacingLocalGraceSlur4(const Slur* slur, const Chord* main, bool allowVisualEdits)
{
    if (!slur || !main || main->isGrace() || !main->segment() || main->staffMove()
        || slur->score() != main->score() || !slur->visible()
        || (!allowVisualEdits && (!slur->autoplace() || slur->isNudged())) || slur->lineType() != 0 || slur->nsegments() > 1
        || slur->endElement() != main || slur->track() != main->track() || slur->track2() != main->track()
        || slur->tick() != main->tick() || slur->tick2() != main->tick())
        return false;
    const auto graces = main->graceNotesBefore();
    if (graces.empty() || slur->startElement() != graces.front())
        return false;
    const Chord* first = graces.front();
    if (!first->isGraceBefore() || first->parent() != main || first->staffMove()
        || first->track() != main->track() || first->score() != main->score())
        return false;
    if (slur->nsegments()) {
        const SlurSegment* segment = slur->frontSegment();
        if (!allowVisualEdits && (!segment->autoplace() || segment->isNudged() || segment->isEdited()))
            return false;
    }
    return true;
}

bool spacingMergedGraceSlur4(const Slur* slur, const Chord* owner, const Chord* target, bool allowVisualEdits)
{
    if (!slur || !owner || !target || owner == target || owner->isGrace() || target->isGrace()
        || !owner->measure() || !owner->segment() || !target->segment()
        || owner->score() != target->score() || owner->track() != target->track()
        || owner->measure() != target->measure() || owner->staffMove() || target->staffMove()
        || owner->tick() + owner->actualTicks() != target->tick()
        || owner->graceNotesAfter().empty() || target->graceNotesBefore().empty())
        return false;
    if (spacingLocalGraceSlur4(slur, target, allowVisualEdits)) return true;
    const Chord* first = owner->graceNotesAfter().front();
    if (slur->score() != owner->score() || !slur->visible()
        || (!allowVisualEdits && (!slur->autoplace() || slur->isNudged())) || slur->lineType() != 0 || slur->nsegments() > 1
        || slur->startElement() != first || slur->endElement() != target
        || slur->track() != owner->track() || slur->track2() != target->track()
        || slur->tick() != owner->tick() || slur->tick2() != target->tick()
        || !first->isGraceAfter() || first->parent() != owner || first->staffMove()
        || first->score() != owner->score() || first->track() != owner->track())
        return false;
    if (slur->nsegments()) {
        const SlurSegment* segment = slur->frontSegment();
        if (!allowVisualEdits && (!segment->autoplace() || segment->isNudged() || segment->isEdited())) return false;
    }
    return true;
}

static const Chord* ownerChord4(const Element* item)
{
    for (; item; item = item->parent()) {
        if (item->isChord())
            return toChord(item);
    }
    return nullptr;
}

static bool graceComponents4(const Chord* chord, const Chord* main,
                              const SpacingPaddingTable4& table, SpacingShape4& result, const Element* afterTarget = nullptr, bool merged = false)
{
    const SpacingShape4 native = spacingElementShape4(chord);
    SpacingShape4 shape;
    for (const SpacingShapeElement4& e : native.elements) {
        // The main chord's native shape contains its already-placed graces.
        // Rebuild from its own components, never their stale positions/bounds.
        const Chord* owner = ownerChord4(e.item);
        if ((chord == main || (merged && chord == afterTarget)) && owner && owner != chord && owner->parent() == chord)
            continue;
        if (e.identity == SpacingIdentity4::LEGACY_SPACER && e.item == chord
            && e.aggregateOwners.empty() && e.sharedItems.empty())
            continue;
        if (e.identity != SpacingIdentity4::ITEM || !e.item || owner != chord
            || !e.aggregateOwners.empty() || !e.sharedItems.empty()
            || !std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
            || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
            || e.rect.width() < 0 || e.rect.height() < 0)
            return false;
        switch (e.item->type()) {
        case ElementType::NOTE:
        case ElementType::STEM:
        case ElementType::HOOK:
        case ElementType::STEM_SLASH:
        case ElementType::LEDGER_LINE:
        case ElementType::ACCIDENTAL:
            break;
        default:
            return false;
        }
        SpacingPairPolicy4 policy;
        const bool supported = merged
            ? spacingMergedGracePairPolicy4(table, main, toChord(afterTarget), e.item, e.item, chord->spatium(), policy)
            : afterTarget
            ? spacingAfterGracePairPolicy4(table, main, afterTarget, e.item, e.item, chord->spatium(), policy)
            : spacingGracePairPolicy4(table, main, e.item, e.item, chord->spatium(), policy);
        if (!supported)
            return false;
        shape.elements.push_back(e);
    }
    if (shape.elements.empty())
        return false;
    result = shape;
    return true;
}

static bool graceDistance4(const SpacingPaddingTable4& table, const Chord* main,
                            const SpacingShape4& a, const SpacingShape4& b, qreal sp, qreal& result, const Element* afterTarget = nullptr, bool merged = false)
{
    qreal distance = -std::numeric_limits<qreal>::max();
    for (const auto& first : a.elements) {
        for (const auto& second : b.elements) {
            SpacingPairPolicy4 policy;
            const bool supported = merged
                ? spacingMergedGracePairPolicy4(table, main, toChord(afterTarget), first.item, second.item, sp, policy)
                : afterTarget
                ? spacingAfterGracePairPolicy4(table, main, afterTarget, first.item, second.item, sp, policy)
                : spacingGracePairPolicy4(table, main, first.item, second.item, sp, policy);
            if (!supported)
                return false;
            distance = std::max(distance, spacingPairDistance4(first.rect, second.rect, policy, sp, 1));
        }
    }
    result = distance;
    return true;
}

// Ordinary rests and staged barline targets contribute their own leaf shapes
// under the explicit after-group policy. Attachments remain outside this scope.
static bool afterTargetComponents4(const Element* target, const Chord* owner,
                                    const SpacingPaddingTable4& table, SpacingShape4& result)
{
    if (target->isChord()) return graceComponents4(toChord(target), owner, table, result, target);
    if (!target->isRest() && !target->isBarLine()) return false;
    const SpacingShape4 shape = spacingElementShape4(target);
    if (shape.elements.empty()) return false;
    for (const auto& e : shape.elements) {
        SpacingPairPolicy4 policy;
        if (e.identity != SpacingIdentity4::ITEM || e.item != target
            || !e.aggregateOwners.empty() || !e.sharedItems.empty()
            || !std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
            || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
            || e.rect.width() <= 0 || e.rect.height() <= 0
            || !spacingAfterGracePairPolicy4(table, owner, target, target, target, owner->spatium(), policy))
            return false;
    }
    result = shape;
    return true;
}

bool buildGraceBeforePlan4(const Chord* main, GraceBeforePlan4& result, QString* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason)
            *reason = QString::fromLatin1(message);
        return false;
    };
    if (!main || !main->score() || !main->segment() || !main->staff() || main->isGrace()
        || main->onTabStaff() || !main->staff()->isPitchedStaff(main->tick())
        || main->staff()->lines(main->tick()) != 5 || main->mag() != 1
        || !std::isfinite(main->x()) || !std::isfinite(main->y())
        || main->beam() || !main->graceNotesAfter().empty())
        return fail("Before-grace adapter needs one ordinary main chord");
    const auto graces = main->graceNotesBefore();
    if (graces.empty())
        return fail("No before-grace group");
    // Single voice on this staff: the main chord shape is then the relevant
    // segment music shape. Other voices require a full staff-shape adapter.
    for (const Element* item : main->segment()->elist()) {
        if (item && item != main && item->vStaffIdx() == main->vStaffIdx())
            return fail("Unadapted simultaneous voice");
    }
    for (const Element* item : main->segment()->annotations()) {
        if (item->staffIdx() == main->staffIdx() && item->addToSkyline() && !item->isSystemText())
            return fail("Unadapted segment attachment");
    }
    const auto validChord = [main](const Chord* chord) {
        if (!chord || chord->score() != main->score() || chord->track() != main->track()
            || chord->staffMove() || chord->isNudged() || chord->isSmall() || !chord->visible()
            || !chord->autoplace() || chord->notes().size() != 1 || chord->noStem() || !chord->stem()
            || chord->tuplet() || chord->tremolo() || chord->arpeggio()
            || !chord->el().empty() || !chord->lyrics().empty() || !chord->articulations().empty())
            return false;
        const Note* note = chord->notes().front();
        return note->headWidth() > 0 && std::isfinite(note->headWidth())
            && note->visible() && !note->isSmall() && note->offset().isNull() && !note->mirror()
            && note->headGroup() == NoteHead::Group::HEAD_NORMAL && note->headType() == NoteHead::Type::HEAD_AUTO
            && note->el().empty() && note->dots().empty() && !note->tieFor() && !note->tieBack()
            && note->spannerFor().empty() && note->spannerBack().empty();
    };
    if (!validChord(main))
        return fail("Unadapted main chord");
    const Beam* beam = graces.front()->beam();
    for (const Chord* grace : graces) {
        if (!validChord(grace) || !grace->isGraceBefore() || grace->parent() != main
            || !grace->graceNotes().empty() || grace->up() != graces.front()->up()
            || grace->beam() != beam || (beam && (beam->cross() || beam->userModified()))
            || !(grace->mag() > 0) || !std::isfinite(grace->mag()))
            return fail("Unadapted before-grace chord or beam");
    }
    if (beam) {
        if (beam->elements().size() != graces.size())
            return fail("Grace beam extends outside group");
        for (const ChordRest* chord : beam->elements()) {
            if (std::find(graces.begin(), graces.end(), chord) == graces.end())
                return fail("Grace beam extends outside group");
        }
    }
    const SpacingPaddingTable4 table(main->score());
    SpacingShape4 target;
    if (!graceComponents4(main, main, table, target))
        return fail("Unadapted main shape");
    target = target.translated(main->pos());
    GraceBeforePlan4 plan;
    plan.positions.resize(graces.size());
    SpacingShape4 accumulated;
    for (int i = graces.size() - 1; i >= 0; --i) {
        const Chord* grace = graces[i];
        SpacingShape4 shape;
        qreal distance;
        if (!graceComponents4(grace, main, table, shape)
            || !graceDistance4(table, main, shape, accumulated, grace->spatium(), distance))
            return fail("Unadapted grace shape");
        const qreal offset = -std::max(distance, qreal(0));
        plan.positions[i] = {grace, offset};
        shape = shape.translated(QPointF(offset, 0));
        accumulated.elements.insert(accumulated.elements.end(), shape.elements.begin(), shape.elements.end());
    }
    qreal distance;
    if (!graceDistance4(table, main, accumulated, target, graces.front()->spatium(), distance))
        return fail("Unadapted group-to-main constraint");
    // Studio defaults, in score spatium, independent of minNoteDistance.
    const qreal groupX = std::min(-distance,
        -(0.45 * main->score()->spatium() + graces.back()->notes().front()->headWidth() / 2));
    if (!std::isfinite(groupX))
        return fail("Nonfinite grace placement");
    for (GracePosition4& position : plan.positions)
        position.x += groupX - main->pos().x();
    plan.left = std::numeric_limits<qreal>::max();
    for (const auto& e : accumulated.elements)
        plan.left = std::min(plan.left, e.rect.left() + groupX - main->pos().x());
    if (!std::isfinite(plan.left))
        return fail("Nonfinite grace extent");
    result = plan;
    if (reason)
        reason->clear();
    return true;
}
static bool buildGraceAfterPlanImpl4(const Chord* owner, GraceAfterPlan4& result, QString* reason, bool barlineOnly, bool merged = false)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!owner || !owner->score() || !owner->measure() || !owner->segment() || owner->isGrace()
        || owner->track() < 0 || !owner->graceNotesBefore().empty() || owner->graceNotesAfter().empty())
        return fail("After-grace adapter needs an ordinary owner with only after-graces");
    const auto validChord = [](const Chord* chord) {
        if (!chord || !chord->staff() || chord->onTabStaff() || chord->staffMove()
            || !chord->staff()->isPitchedStaff(chord->tick()) || chord->staff()->lines(chord->tick()) != 5
            || !chord->visible() || !chord->autoplace() || chord->isNudged() || chord->isSmall()
            || !std::isfinite(chord->x()) || !std::isfinite(chord->y()) || chord->noStem() || !chord->stem()
            || chord->stem()->isNudged() || chord->stem()->userLen() != 0
            || chord->notes().size() != 1 || chord->tuplet() || chord->tremolo() || chord->arpeggio()
            || !chord->el().empty() || !chord->lyrics().empty() || !chord->articulations().empty())
            return false;
        const Note* note = chord->notes().front();
        return note->headWidth() > 0 && std::isfinite(note->headWidth()) && note->visible() && !note->isSmall()
            && note->offset().isNull() && !note->mirror() && note->headGroup() == NoteHead::Group::HEAD_NORMAL
            && note->headType() == NoteHead::Type::HEAD_AUTO && note->el().empty() && note->dots().empty()
            && !note->tieFor() && !note->tieBack() && note->spannerFor().empty() && note->spannerBack().empty();
    };
    if (!validChord(owner) || owner->mag() != 1 || owner->beam() || owner->actualTicks() <= Fraction(0, 1))
        return fail("Unadapted after-grace owner");
    Measure* measure = owner->measure();
    if (measure->hasVoices(owner->staffIdx(), measure->tick(), measure->ticks()))
        return fail("After-grace target needs one voice on this staff");
    Segment* targetSegment = measure->tick2segment(owner->tick() + owner->actualTicks(), SegmentType::All);
    const int firstTrack = owner->staffIdx() * VOICES;
    while (targetSegment && !targetSegment->hasElements(firstTrack, firstTrack + VOICES - 1))
        targetSegment = targetSegment->next();
    if (!targetSegment || targetSegment == owner->segment() || targetSegment->measure() != measure
        || !targetSegment->enabled() || !targetSegment->visible()
        || (barlineOnly ? !targetSegment->isEndBarLineType()
                        : !(targetSegment->isChordRestType() || targetSegment->isEndBarLineType())))
        return fail("After-grace target is not the requested same-measure segment type");
    const Element* target = targetSegment->element(owner->track());
    if (!target || (barlineOnly ? !target->isBarLine()
                              : (!target->isChord() && !target->isRest() && !target->isBarLine())))
        return fail("After-grace target is not the requested item in the same voice");
    if (target->isChord()) {
        const Chord* chord = toChord(target);
        if (!validChord(chord) || chord->score() != owner->score() || chord->mag() != 1 || chord->beam()
            || (merged ? (chord->graceNotesBefore().empty() || !chord->graceNotesAfter().empty())
                       : !chord->graceNotes().empty()))
            return fail("Unadapted or merged after-grace target");
    }
    else if (target->isRest()) {
        const Rest* rest = toRest(target);
        if (rest->score() != owner->score() || !rest->staff() || rest->onTabStaff() || rest->staffMove()
            || !rest->staff()->isPitchedStaff(rest->tick()) || rest->staff()->lines(rest->tick()) != 5
            || rest->mag() != 1 || rest->isSmall() || !rest->visible() || !rest->autoplace() || rest->isNudged()
            || !std::isfinite(rest->x()) || !std::isfinite(rest->y()) || rest->isGap() || rest->isFullMeasureRest()
            || rest->measure()->isMMRest() || rest->actualDots() || rest->beam() || rest->tuplet()
            || rest->actualTicks() <= Fraction(0, 1) || !rest->el().empty() || !rest->lyrics().empty())
            return fail("After-grace rest target needs an ordinary visible undotted rest");
    }
    else {
        const BarLine* barline = toBarLine(target);
        if (owner->track() != firstTrack || targetSegment->tick() != measure->endTick()
            || barline->score() != owner->score() || barline->mag() != 1
            || !barline->visible() || !barline->autoplace() || barline->isNudged()
            || !std::isfinite(barline->x()) || !std::isfinite(barline->y())
            || barline->barLineType() != BarLineType::NORMAL || barline->spanStaff()
            || barline->spanFrom() || barline->spanTo() || !barline->el()->empty())
            return fail("After-grace barline target needs an ordinary visible unedited single-staff barline");
    }
    if (merged && !target->isChord()) return fail("Merged grace target needs an ordinary chord");
    for (const Segment* segment : {owner->segment(), targetSegment}) {
        if (!segment->enabled() || !segment->visible())
            return fail("After-grace owner/target segment is inactive");
        for (const Element* item : segment->elist()) {
            if (item && item->track() != owner->track() && item->vStaffIdx() == owner->vStaffIdx())
                return fail("Unadapted simultaneous after-grace target voice");
        }
        for (const Element* item : segment->annotations()) {
            if (item->staffIdx() == owner->staffIdx() && item->addToSkyline() && !item->isSystemText())
                return fail("Unadapted after-grace segment attachment");
        }
    }
    auto graces = owner->graceNotesAfter();
    const Beam* beam = graces.front()->beam();
    for (const Chord* grace : graces) {
        if (!validChord(grace) || !grace->isGraceAfter() || grace->parent() != owner || !grace->graceNotes().empty()
            || grace->score() != owner->score() || grace->track() != owner->track() || grace->beam() != beam
            || grace->up() != graces.front()->up() || !(grace->mag() > 0) || !std::isfinite(grace->mag())
            || (beam && (beam->cross() || beam->userModified())))
            return fail("Unadapted after-grace group");
    }
    if (beam) {
        if (beam->elements().size() != graces.size())
            return fail("After-grace beam extends outside group");
        for (const ChordRest* item : beam->elements()) {
            if (std::find(graces.begin(), graces.end(), item) == graces.end())
                return fail("After-grace beam extends outside group");
        }
    }
    if (merged) {
        const Chord* next = toChord(target);
        const auto before = next->graceNotesBefore();
        const Beam* beforeBeam = before.front()->beam();
        for (const Chord* grace : before) {
            if (!validChord(grace) || !grace->isGraceBefore() || grace->parent() != next
                || grace->score() != owner->score() || grace->track() != owner->track()
                || !grace->graceNotes().empty() || grace->beam() != beforeBeam
                || grace->up() != graces.front()->up() || !(grace->mag() > 0) || !std::isfinite(grace->mag())
                || (beforeBeam && (beforeBeam->cross() || beforeBeam->userModified())))
                return fail("Merged query needs unedited same-direction grace groups");
        }
        if (beforeBeam) {
            if (beforeBeam->elements().size() != before.size()) return fail("Before beam extends outside merged group");
            for (const ChordRest* member : beforeBeam->elements()) {
                if (std::find(before.begin(), before.end(), member) == before.end())
                    return fail("Before beam extends outside merged group");
            }
        }
        for (Chord* grace : before) graces.push_back(grace);
    }
    const SpacingPaddingTable4 table(owner->score());
    SpacingShape4 targetShape;
    const bool targetReady = merged
        ? graceComponents4(toChord(target), owner, table, targetShape, target, true)
        : afterTargetComponents4(target, owner, table, targetShape);
    if (!targetReady)
        return fail("Unadapted after-grace target shape");
    targetShape = targetShape.translated(target->pos());
    GraceAfterPlan4 plan;
    plan.owner = owner;
    plan.target = targetSegment;
    plan.positions.resize(graces.size());
    SpacingShape4 accumulated;
    for (int i = graces.size() - 1; i >= 0; --i) {
        SpacingShape4 shape;
        qreal distance;
        if (!graceComponents4(graces[i], owner, table, shape, target, merged)
            || !graceDistance4(table, owner, shape, accumulated, graces[i]->spatium(), distance, target, merged))
            return fail("Unadapted after-grace component shape");
        const qreal x = -std::max(distance, qreal(0));
        plan.positions[i] = {graces[i], x};
        shape = shape.translated(QPointF(x, 0));
        accumulated.elements.insert(accumulated.elements.end(), shape.elements.begin(), shape.elements.end());
    }
    qreal distance;
    if (!graceDistance4(table, owner, accumulated, targetShape, graces.front()->spatium(), distance, target, merged))
        return fail("Unadapted after-grace group-to-target constraint");
    const qreal groupX = std::min(-distance,
        -(0.45 * owner->score()->spatium() + graces.back()->notes().front()->headWidth() / 2));
    if (!std::isfinite(groupX)) return fail("Nonfinite after-grace group position");
    for (GraceAfterPosition4& position : plan.positions) position.x += groupX;
    plan.left = std::numeric_limits<qreal>::max();
    plan.right = -std::numeric_limits<qreal>::max();
    for (const auto& e : accumulated.elements) {
        plan.left = std::min(plan.left, e.rect.left() + groupX);
        plan.right = std::max(plan.right, e.rect.right() + groupX);
    }
    if (!std::isfinite(plan.left) || !std::isfinite(plan.right))
        return fail("Nonfinite after-grace extent");
    result = plan;
    if (reason) reason->clear();
    return true;
}

bool buildGraceAfterPlan4(const Chord* owner, GraceAfterPlan4& result, QString* reason)
{
    return buildGraceAfterPlanImpl4(owner, result, reason, false);
}

bool buildGraceAfterBarlinePlan4(const Chord* owner, GraceAfterPlan4& result, QString* reason)
{
    // Target-specific query shares production packing and keeps other target
    // types out of boundary-focused checks.
    return buildGraceAfterPlanImpl4(owner, result, reason, true);
}

bool buildGraceMergedPlan4(const Chord* owner, GraceMergedPlan4& result, QString* reason)
{
    GraceAfterPlan4 joint;
    if (!buildGraceAfterPlanImpl4(owner, joint, reason, false, true)) return false;
    GraceMergedPlan4 plan;
    plan.owner = owner;
    plan.target = toChord(joint.target->element(owner->track()));
    plan.positions = joint.positions;
    plan.left = joint.left;
    plan.right = joint.right;
    result = plan;
    return true;
}

static bool afterSourceComponents4(const Chord* owner, const Element* target,
                                    const SpacingPaddingTable4& table, SpacingShape4& result, bool merged = false)
{
    SpacingShape4 source;
    for (const SpacingShapeElement4& e : spacingElementShape4(owner).elements) {
        const Chord* chord = ownerChord4(e.item);
        if (chord && chord != owner && chord->parent() == owner) continue;
        if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
            const auto knownProducer = [owner](const Element* item) {
                return item && item->isChord()
                    && (item == owner || (toChord(item)->isGraceAfter() && item->parent() == owner));
            };
            if (!e.sharedItems.empty()) return false;
            if (e.item) {
                if (!knownProducer(e.item) || !e.aggregateOwners.empty()) return false;
            }
            else {
                if (e.aggregateOwners.size() < 2) return false;
                for (size_t i = 0; i < e.aggregateOwners.size(); ++i) {
                    if (!knownProducer(e.aggregateOwners[i])) return false;
                    for (size_t j = 0; j < i; ++j)
                        if (e.aggregateOwners[i] == e.aggregateOwners[j]) return false;
                }
            }
            continue;
        }
        SpacingPairPolicy4 policy;
        const bool supported = merged
            ? spacingMergedGracePairPolicy4(table, owner, toChord(target), e.item, e.item, owner->spatium(), policy, true)
            : spacingAfterGracePairPolicy4(table, owner, target, e.item, e.item, owner->spatium(), policy, true);
        if (e.identity != SpacingIdentity4::ITEM || !e.item || chord != owner
            || !e.aggregateOwners.empty() || !e.sharedItems.empty()
            || !std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
            || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
            || e.rect.width() < 0 || e.rect.height() < 0
            || !supported)
            return false;
        source.elements.push_back(e);
    }
    if (source.elements.empty()) return false;
    result = source;
    return true;
}

bool buildGraceMergedGeometry4(const Chord* owner, GraceMergedGeometry4& result, QString* reason)
{
    GraceMergedGeometry4 geometry;
    if (!buildGraceMergedPlan4(owner, geometry.plan, reason)) return false;
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    const Chord* target = geometry.plan.target;
    const SpacingPaddingTable4 table(owner->score());
    SpacingShape4 source;
    if (!afterSourceComponents4(owner, target, table, source, true))
        return fail("Unadapted merged-grace source projection");
    geometry.source = source.translated(owner->pos());
    SpacingShape4 following;
    if (!graceComponents4(target, owner, table, following, target, true))
        return fail("Unadapted merged-grace target projection");
    geometry.target = following.translated(target->pos());
    for (const GraceAfterPosition4& p : geometry.plan.positions) {
        SpacingShape4 member;
        if (!graceComponents4(p.chord, owner, table, member, target, true))
            return fail("Unadapted merged-grace member projection");
        member = member.translated(QPointF(p.x, 0));
        geometry.target.elements.insert(geometry.target.elements.end(), member.elements.begin(), member.elements.end());
    }
    if (!geometry.source.complete() || !geometry.target.complete())
        return fail("Incomplete merged-grace projection identity");
    result = geometry;
    if (reason) reason->clear();
    return true;
}

bool buildGraceMergedSlurObstacles4(const Slur* slur, SpacingShape4& result, QString* reason, bool allowVisualEdits)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!slur || !slur->startElement() || !slur->startElement()->isChord()
        || !slur->endElement() || !slur->endElement()->isChord())
        return fail("Combined grace obstacles need chord anchors");
    const Chord* start = toChord(slur->startElement());
    const Chord* target = toChord(slur->endElement());
    if (!start->isGraceAfter() || !start->parent() || !start->parent()->isChord())
        return fail("Combined grace obstacles need the incoming after-grace anchor");
    const Chord* owner = toChord(start->parent());
    if (!spacingMergedGraceSlur4(slur, owner, target, allowVisualEdits)
        || !owner->measure()->system() || owner->measure()->system() != target->measure()->system())
        return fail("Unadapted combined grace slur obstacles");
    GraceMergedGeometry4 geometry;
    if (!buildGraceMergedGeometry4(owner, geometry, reason)) return false;
    SpacingShape4 obstacles;
    // Geometry already contains each validated physical component. Rebase
    // those rectangles from temporary target coordinates to CURRENT published
    // coordinates instead of resolving and validating every chord again.
    const auto append = [&](const Chord* chord, const QPointF& plannedOrigin) {
        QPointF position = chord->pos() + chord->segment()->pos() + chord->measure()->pos();
        if (chord->isGrace()) position += chord->parent()->pos();
        const QPointF shift = position - plannedOrigin;
        if (!std::isfinite(shift.x()) || !std::isfinite(shift.y())) return false;
        bool found = false;
        for (const SpacingShapeElement4& component : geometry.target.elements) {
            if (ownerChord4(component.item) != chord || component.item->isLedgerLine()) continue;
            SpacingShapeElement4 projected = component;
            projected.rect.translate(shift);
            if (!std::isfinite(projected.rect.left()) || !std::isfinite(projected.rect.right())
                || !std::isfinite(projected.rect.top()) || !std::isfinite(projected.rect.bottom())) return false;
            obstacles.elements.push_back(projected);
            found = true;
        }
        return found;
    };
    for (const GraceAfterPosition4& member : geometry.plan.positions) {
        if (member.chord != start && !append(member.chord, QPointF(member.x, 0)))
            return fail("Unadapted published combined grace obstacle");
    }
    if (!append(target, target->pos()) || !obstacles.complete() || obstacles.elements.empty())
        return fail("Incomplete combined grace slur obstacles");
    result = obstacles;
    if (reason) reason->clear();
    return true;
}

bool prepareGraceMergedBeams4(Chord* owner, QString* reason)
{
    GraceMergedGeometry4 geometry;
    if (!buildGraceMergedGeometry4(owner, geometry, reason)) return false;
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    std::vector<Chord*> members;
    for (Chord* grace : owner->graceNotesAfter()) members.push_back(grace);
    const size_t afterCount = members.size();
    for (Chord* grace : geometry.plan.target->graceNotesBefore()) members.push_back(grace);
    if (!afterCount || members.size() == afterCount || members.size() != geometry.plan.positions.size())
        return fail("Merged beam preparation needs both complete groups");
    std::vector<qreal> original;
    std::vector<Beam*> beams;
    for (Chord* grace : members) {
        original.push_back(grace->x());
        if (grace->beam() && std::find(beams.begin(), beams.end(), grace->beam()) == beams.end())
            beams.push_back(grace->beam());
    }
    if (beams.empty()) return true;
    const auto sameShape = [](const SpacingShape4& a, const SpacingShape4& b) {
        if (a.elements.size() != b.elements.size()) return false;
        for (size_t i = 0; i < a.elements.size(); ++i) {
            const auto& x = a.elements[i];
            const auto& y = b.elements[i];
            if (x.item != y.item || x.identity != y.identity
                || x.aggregateOwners != y.aggregateOwners || x.sharedItems != y.sharedItems
                || qAbs(x.rect.x() - y.rect.x()) > 1e-9 || qAbs(x.rect.y() - y.rect.y()) > 1e-9
                || qAbs(x.rect.width() - y.rect.width()) > 1e-9
                || qAbs(x.rect.height() - y.rect.height()) > 1e-9)
                return false;
        }
        return true;
    };
    for (int pass = 0; pass < 8; ++pass) {
        // Each independent beam needs its own group's relative distances.
        // Anchor that group at its original first x; do not move musical parents.
        for (size_t i = 0; i < members.size(); ++i) {
            const size_t base = i < afterCount ? 0 : afterCount;
            members[i]->rxpos() = original[base] + geometry.plan.positions[i].x
                - geometry.plan.positions[base].x;
        }
        for (Beam* beam : beams) beam->layout();
        for (size_t i = 0; i < members.size(); ++i) members[i]->rxpos() = original[i];
        owner->segment()->createShape(owner->vStaffIdx());
        geometry.plan.target->segment()->createShape(geometry.plan.target->vStaffIdx());
        GraceMergedGeometry4 next;
        if (!buildGraceMergedGeometry4(owner, next, reason)) return false;
        if (next.plan.positions.size() != members.size()) return fail("Merged grace membership changed during beam preparation");
        for (size_t i = 0; i < members.size(); ++i) {
            if (next.plan.positions[i].chord != members[i])
                return fail("Merged grace order changed during beam preparation");
        }
        if (sameShape(geometry.source, next.source) && sameShape(geometry.target, next.target)) return true;
        geometry = next;
    }
    return fail("Merged grace beam geometry did not stabilize");
}

bool buildGraceAfterGeometry4(const Chord* owner, GraceAfterGeometry4& result, QString* reason)
{
    GraceAfterGeometry4 geometry;
    if (!buildGraceAfterPlan4(owner, geometry.plan, reason)) return false;
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    const Element* target = geometry.plan.target->element(owner->track());
    const SpacingPaddingTable4 table(owner->score());
    SpacingShape4 source;
    if (!afterSourceComponents4(owner, target, table, source))
        return fail("Unadapted after-grace source projection");
    geometry.source = source.translated(owner->pos());
    SpacingShape4 following;
    if (!afterTargetComponents4(target, owner, table, following))
        return fail("Unadapted after-grace target projection");
    geometry.target = following.translated(target->pos());
    for (const GraceAfterPosition4& p : geometry.plan.positions) {
        SpacingShape4 member;
        if (!graceComponents4(p.chord, owner, table, member, target))
            return fail("Unadapted after-grace member projection");
        member = member.translated(QPointF(p.x, 0));
        geometry.target.elements.insert(geometry.target.elements.end(), member.elements.begin(), member.elements.end());
    }
    if (!geometry.source.complete() || !geometry.target.complete())
        return fail("Incomplete after-grace projection identity");
    result = geometry;
    if (reason) reason->clear();
    return true;
}

bool applyGraceAfterPlan4(Chord* owner, QString* reason)
{
    GraceAfterPlan4 plan;
    if (!buildGraceAfterPlan4(owner, plan, reason)) return false;
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    const Element* target = plan.target->element(owner->track());
    const qreal translation = plan.target->x() - owner->segment()->x() - owner->x();
    if (!std::isfinite(translation)) return fail("Nonfinite after-grace publication offset");
    const SpacingPaddingTable4 table(owner->score());
    SpacingShape4 source;
    if (!afterSourceComponents4(owner, target, table, source))
        return fail("Unadapted after-grace source shape");
    std::vector<qreal> positions;
    for (const GraceAfterPosition4& p : plan.positions) {
        const qreal x = translation + p.x;
        if (!std::isfinite(x)) return fail("Nonfinite after-grace member position");
        SpacingShape4 shape;
        if (!graceComponents4(p.chord, owner, table, shape, target))
            return fail("Unadapted after-grace publication shape");
        // The segment interval still comes from native spacing at this stage.
        // Refuse to repack into it unless every source-to-grace constraint fits.
        for (const auto& first : source.elements) {
            for (const auto& second : shape.elements) {
                SpacingPairPolicy4 policy;
                if (!spacingAfterGracePairPolicy4(table, owner, target, first.item, second.item,
                                                  owner->spatium(), policy, true))
                    return fail("Unadapted source-to-after-grace constraint");
                const qreal required = spacingPairDistance4(first.rect, second.rect, policy, owner->spatium(), 1);
                if (x + 1e-6 < required)
                    return fail("Native segment interval is too narrow for after-grace plan");
            }
        }
        positions.push_back(x);
    }
    const auto graces = owner->graceNotesAfter();
    for (int i = 0; i < graces.size(); ++i) graces[i]->rxpos() = positions[i];
    owner->segment()->createShape(owner->vStaffIdx());
    if (reason) reason->clear();
    return true;
}

}
