// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacingplan4.h"
#include "spacinggeometry4.h"
#include "spacingcollision4.h"
#include "spacinglayout4.h"
#include "chord.h"
#include "beam.h"
#include "measure.h"
#include "note.h"
#include "rest.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "system.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace Ms {
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

bool eligibleType4(const Segment* s)
{
    return headerType4(s) || courtesyType4(s) || s->isChordRestType()
        || s->isEndBarLineType() || s->segmentType() == SegmentType::BeginBarLine;
}

bool cacheGeometry4(Segment* segment, const SpacingPaddingTable4& table, Geometry4& geometry)
{
    for (int staffIdx = 0; staffIdx < segment->score()->nstaves(); ++staffIdx) {
        const SpacingShape4 native = spacingStaffShape4(segment, staffIdx);
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
        if (!spacingCollisionProjection4(table, native, sp, projected))
            return false;
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
               qreal squeeze, qreal& result)
{
    qreal distance = 0;
    const bool header = headerType4(a.segment) && a.segment->rtick().isZero()
        && b.segment->measure() == firstMeasure && b.segment->isChordRestType();
    for (size_t st = 0; st < first.staff.size(); ++st) {
        qreal sp = a.segment->spatium();
        for (const SpacingShapeElement4& e : first.staff[st].elements) {
            sp = e.item->spatium();
            break;
        }
        qreal d = 0;
        if (!spacingShapeDistance4(table, first.staff[st], second.staff[st], sp, squeeze, d))
            return false;
        distance = std::max(distance, d);
    }
    if (header) {
        const Score* score = table.score();
        distance = std::max(distance, first.minRight);
        const qreal gap = score->styleP(a.segment->isTimeSigType() ? Sid::systemHeaderTimeSigDistance : Sid::systemHeaderDistance);
        distance = std::max(distance, first.minRight + gap * squeeze);
        const qreal actual = distance - first.minRight - second.minLeft;
        distance += std::max(qreal(0), 1.5 * a.segment->spatium() * squeeze - actual);
    }
    result = distance;
    return std::isfinite(result);
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
            || chord->beam() != beam || chord->notes().size() != 1 || chord->isGrace()
            || chord->tuplet() || chord->tremolo() || !chord->graceNotes().empty()
            || chord->isSmall() || chord->mag() != 1.0 || chord->noStem() || !chord->stem()
            || chord->isNudged() || chord->stemDirection() != Direction::AUTO
            || chord->durationType().type() != TDuration::DurationType::V_EIGHTH
            || chord->actualTicks() != Fraction(1, 8) || chord->segment()->ticks() != chord->actualTicks())
            return fail("Unadapted cross-staff chord, rhythm or manual context");
        const int staff = chord->vStaffIdx();
        if (staff < 0 || staff >= score->nstaves() || !score->staff(staff)->show()
            || !score->staff(staff)->isPitchedStaff(chord->tick()) || score->staff(staff)->lines(chord->tick()) != 5)
            return fail("Cross-staff context needs visible ordinary five-line staves");
        const Note* note = chord->notes().front();
        SpacingPairPolicy4 policy;
        if (!note->visible() || note->isSmall() || note->mirror()
            || note->headGroup() != NoteHead::Group::HEAD_NORMAL || note->headType() != NoteHead::Type::HEAD_AUTO
            || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
            return fail("Unadapted cross-staff note context");
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
        const int line = chord->notes().front()->line();
        if ((entry.visualStaff == upper && line > 8) || (entry.visualStaff == lower && line < 0))
            return fail("Interstaff note positions need a later stem-side adapter");
        entry.up = entry.visualStaff == lower;
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
        pair.canAdjust = true;
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
            if (a.up)
                pair.minimumWidth = std::max(2 * displacement, headWidth + stemPadding);
        }
        context.pairs.push_back(pair);
    }
    result = context;
    if (reason) reason->clear();
    return true;
}

bool buildSpacingPlan4(System* system, SpacingPlan4& result, QString* reason, const SpacingPlanOptions4& options)
{
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!system || system->measures().empty())
        return fail("No system measures");
    if (!std::isfinite(options.squeezeFactor) || options.squeezeFactor < 0 || options.squeezeFactor > 1
        || !std::isfinite(options.stretchReduction) || options.stretchReduction <= 0)
        return fail("Invalid spacing factors");
    Score* score = system->score();
    if (!score)
        return fail("No score");
    for (int st = 0; st < score->nstaves(); ++st) {
        if (!score->staff(st)->show() || !system->staff(st)->show())
            return fail("Hidden staves require a later adapter");
    }
    SpacingPlan4 plan;
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
                if (courtesyType4(&segment) && (!segment.trailer() || measure != system->lastMeasure()
                                               || segment.rtick() != measure->ticks()))
                    return fail("Courtesy signatures require a terminal measure-end trailer");
                if (!eligibleType4(&segment) || (segment.trailer() && !courtesyType4(&segment))
                    || segment.extraLeadingSpace().val() != 0
                    || segment.spacingWidthOffset4() != 0)
                    return fail("Unadapted segment type, trailer, manual space or cross-beam offset");
                for (const Element* annotation : segment.annotations()) {
                    if (!annotation)
                        continue;
                    if (!annotation->isSystemText() && !annotation->isStaffText() && !annotation->isRehearsalMark()
                        && !annotation->isTempoText() && !annotation->isDynamic())
                        return fail("Annotations require a later adapter");
                }
                for (const Element* item : segment.elist()) {
                    if (item && item->isRest() && (toRest(item)->isGap() || toRest(item)->isSmall()
                                                  || toRest(item)->isFullMeasureRest()))
                        return fail("Gap, cue and centered measure rests require a later adapter");
                    if (!item || !item->isChord())
                        continue;
                    const Chord* chord = toChord(item);
                    if (chord->tremolo())
                        return fail("Tremolo spacing requires a later adapter");
                    if (chord->staffMove() != 0 || (chord->beam() && chord->beam()->cross()))
                        return fail("Cross-staff geometry requires its later stem constraints");
                    if (chord->isSmall() || !chord->graceNotes().empty())
                        return fail("Cue and grace-note rhythm requires a later adapter");
                    for (const Note* note : chord->notes()) {
                        SpacingPairPolicy4 policy;
                        if (!note->visible() || note->isSmall() || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
                            return fail("Unadapted note visibility, cue or decoration context");
                    }
                }
                if (!cacheGeometry4(&segment, table, g))
                    return fail(QString("Unadapted geometry at tick %1").arg(segment.tick().ticks()));
                if (segment.isChordRestType()) {
                    if (!(segment.ticks() > Fraction(0, 1)))
                        return fail("Chord/rest duration is not ready");
                    entry.stretch = segmentSpacingStretch4(&segment);
                    if (!(entry.stretch > 0) || !std::isfinite(entry.stretch))
                        return fail("Invalid rhythmic stretch");
                }
            }
            plan.segments.push_back(entry);
            geometry.push_back(g);
        }
    }
    qreal cursor = system->leftMargin();
    bool placed = false;
    for (size_t i = 0; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored) {
            current.x = cursor;
            continue;
        }
        if (!placed) {
            qreal offset = 0;
            if (current.segment->isChordRestType()) {
                offset = std::max(qreal(0), -geometry[i].leftEdge);
                offset += score->styleP(geometry[i].accidentals ? Sid::barAccidentalDistance : Sid::barNoteDistance);
                // Studio default for its style absent from Evolution.
                offset = std::max(offset, 1.25 * current.segment->spatium());
            }
            else if (current.segment->isHeaderClefType())
                offset = score->styleP(Sid::clefLeftMargin);
            else if (current.segment->isKeySigType())
                offset = score->styleP(Sid::keysigLeftMargin);
            else if (current.segment->isTimeSigType())
                offset = score->styleP(Sid::timesigLeftMargin);
            cursor += offset * options.squeezeFactor;
            placed = true;
        }
        else {
            qreal x = -std::numeric_limits<qreal>::max();
            int crCount = 0;
            for (size_t j = i; j > 0;) {
                --j;
                SpacingSegmentPlan4& previous = plan.segments[j];
                if (previous.ignored)
                    continue;
                if (previous.segment->isChordRestType())
                    ++crCount;
                qreal distance = 0;
                if (!distance4(table, previous, geometry[j], current, geometry[i], plan.measures.front().measure,
                               options.squeezeFactor, distance))
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
                if (previousEnd < current.segment->tick()) {
                    if (!current.segment->isChordRestType()
                        || cursor + geometry[i].leftEdge > previous.x + geometry[j].minRight
                        || (previous.segment->measure() != current.segment->measure()
                            && previous.segment->measure() != current.segment->measure()->prevMeasure()))
                        break;
                }
            }
        }
        current.x = cursor;
        if (current.segment->isChordRestType()) {
            if (current.segment->measure() == plan.measures.front().measure && current.segment->rtick().isZero()) {
                cursor = std::max(cursor, system->leftMargin() + 1.25 * current.segment->spatium());
                current.x = cursor;
            }
            cursor += 3.5 * current.segment->spatium() * current.stretch * options.stretchReduction;
        }
    }
    if (!placed)
        return fail("No active segments");
    plan.measures.front().x = system->leftMargin();
    std::map<Measure*, size_t> measureIndex;
    for (size_t i = 0; i < plan.measures.size(); ++i)
        measureIndex[plan.measures[i].measure] = i;
    for (size_t i = 0; i < plan.segments.size(); ++i) {
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
            const bool leadingHeader = headerType4(following.segment) && !current.segment->isType(SegmentType::BarLineType);
            current.width = sameMeasure || leadingHeader ? following.x - current.x : geometry[i].minRight;
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
        for (size_t mi = 0; mi < plan.measures.size(); ++mi) {
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

bool squeezeSpacingPlan4(System* system, qreal targetWidth, SpacingPlan4& result, QString* reason)
{
    if (!std::isfinite(targetWidth) || targetWidth <= 0) {
        if (reason)
            *reason = "Invalid squeeze target";
        return false;
    }
    SpacingPlan4 candidate;
    if (!buildSpacingPlan4(system, candidate, reason))
        return false;
    if (candidate.width <= targetWidth) {
        result = candidate;
        return true;
    }
    // Studio 4.7.5 squeezeSystemToFit recomputes from native geometry each
    // pass, with minimum-measure widths overridden. Stop before its negative
    // duration advances and subsequent collision-allowing width reduction.
    SpacingPlanOptions4 options;
    options.minimumMeasureWidths = false;
    options.stretchReduction = 1 - 0.33;
    options.squeezeFactor = 1 - 0.2;
    for (int pass = 0; pass < 3; ++pass) {
        if (!buildSpacingPlan4(system, candidate, reason, options))
            return false;
        if (candidate.width <= targetWidth) {
            result = candidate;
            return true;
        }
        options.stretchReduction -= 0.33;
        options.squeezeFactor -= 0.2;
    }
    if (reason)
        *reason = "Target needs stronger compression than the adapted squeeze passes";
    return false;
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
}
