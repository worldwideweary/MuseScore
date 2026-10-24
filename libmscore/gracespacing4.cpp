// Adapted from Studio v4.7.5 TLayout::layoutGraceNotesGroup.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "gracespacing4.h"
#include "spacingcollision4.h"
#include "spacinggeometry4.h"
#include "beam.h"
#include "chord.h"
#include "note.h"
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
bool spacingLocalGraceSlur4(const Slur* slur, const Chord* main)
{
    if (!slur || !main || main->isGrace() || !main->segment() || main->staffMove()
        || slur->score() != main->score() || !slur->visible() || !slur->autoplace()
        || slur->isNudged() || slur->lineType() != 0 || slur->nsegments() > 1
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
        if (!segment->autoplace() || segment->isNudged() || segment->isEdited())
            return false;
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
                              const SpacingPaddingTable4& table, SpacingShape4& result, const Chord* afterTarget = nullptr)
{
    const SpacingShape4 native = spacingElementShape4(chord);
    SpacingShape4 shape;
    for (const SpacingShapeElement4& e : native.elements) {
        // The main chord's native shape contains its already-placed graces.
        // Rebuild from its own components, never their stale positions/bounds.
        const Chord* owner = ownerChord4(e.item);
        if (chord == main && owner && owner != main && owner->parent() == main)
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
        const bool supported = afterTarget
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
                            const SpacingShape4& a, const SpacingShape4& b, qreal sp, qreal& result, const Chord* afterTarget = nullptr)
{
    qreal distance = -std::numeric_limits<qreal>::max();
    for (const auto& first : a.elements) {
        for (const auto& second : b.elements) {
            SpacingPairPolicy4 policy;
            const bool supported = afterTarget
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
bool buildGraceAfterPlan4(const Chord* owner, GraceAfterPlan4& result, QString* reason)
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
        || !targetSegment->enabled() || !targetSegment->visible() || !targetSegment->isChordRestType())
        return fail("After-grace target is not a same-measure ordinary chord segment");
    const Element* targetItem = targetSegment->element(owner->track());
    if (!targetItem || !targetItem->isChord())
        return fail("After-grace target is not an ordinary chord in the same voice");
    const Chord* target = toChord(targetItem);
    if (!validChord(target) || target->score() != owner->score() || target->mag() != 1 || target->beam()
        || !target->graceNotes().empty())
        return fail("Unadapted or merged after-grace target");
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
    const auto graces = owner->graceNotesAfter();
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
    const SpacingPaddingTable4 table(owner->score());
    SpacingShape4 targetShape;
    if (!graceComponents4(target, owner, table, targetShape, target))
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
        if (!graceComponents4(graces[i], owner, table, shape, target)
            || !graceDistance4(table, owner, shape, accumulated, graces[i]->spatium(), distance, target))
            return fail("Unadapted after-grace component shape");
        const qreal x = -std::max(distance, qreal(0));
        plan.positions[i] = {graces[i], x};
        shape = shape.translated(QPointF(x, 0));
        accumulated.elements.insert(accumulated.elements.end(), shape.elements.begin(), shape.elements.end());
    }
    qreal distance;
    if (!graceDistance4(table, owner, accumulated, targetShape, graces.front()->spatium(), distance, target))
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

}
