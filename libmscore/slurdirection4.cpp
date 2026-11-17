// Studio slur direction shared by prepared spacing and native rendering.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "slurdirection4.h"
#include "slur.h"
#include "chord.h"
#include "beam.h"
#include "measure.h"
#include "note.h"
#include "segment.h"
#include "staff.h"
#include "system.h"

namespace Ms {
Direction resolveSpacingSlurDirection4(const SpacingSlurDirectionFacts4& f)
{
    if (f.requested == Direction::UP || f.requested == Direction::DOWN) return f.requested;
    if (f.requested != Direction::AUTO || !f.anchorsReady) return Direction::AUTO;
    if (f.crossStaffStart) return Direction::UP;
    if (f.multipleVoices) return f.secondaryVoice ? Direction::DOWN : Direction::UP;
    if (f.startStem != Direction::UP && f.startStem != Direction::DOWN) return Direction::AUTO;
    if (f.mixture == SpacingStemMixture4::MIXED) return Direction::UP;
    if (f.mixture == SpacingStemMixture4::PENDING) return Direction::AUTO;
    return f.startStem == Direction::UP ? Direction::DOWN : Direction::UP;
}

static void refreshChordDirection4(Chord* chord)
{
    for (Note* note : chord->notes()) note->updateLine();
    chord->computeUp();
}

static bool slurDirectionMixture4(const Chord* start, const Chord* end, bool refresh)
{
    if (start->track() != end->track()) return false;
    const bool up = start->up();
    if (end->isGrace() && end->up() != up) return true;
    if (start->isGraceBefore() && end->isGraceAfter() && start->parent() == end->parent()) {
        const Chord* main = toChord(start->parent());
        if (main->stem() && main->up() != up) return true;
    }
    for (const Segment* segment = start->segment(); segment && segment->tick() <= end->tick();
         segment = segment->next1(SegmentType::ChordRest)) {
        if ((start->isGrace() || end->isGraceBefore()) && segment->tick() == end->tick()) return false;
        const Element* item = segment->element(start->track());
        if (!item || !item->isChord()) continue;
        const Chord* chord = toChord(item);
        const System* firstSystem = start->measure()->system();
        const System* system = chord->measure()->system();
        if (refresh && chord->staff() && !chord->staff()->isDrumStaff(chord->tick())
            && firstSystem && system && firstSystem != system) refreshChordDirection4(const_cast<Chord*>(chord));
        if (chord->up() != up) return true;
    }
    return false;
}

static bool slurDirectionImpl4(const Slur* slur, Direction& result, bool refresh)
{
    if (!slur) return false;
    SpacingSlurDirectionFacts4 facts;
    facts.requested = slur->slurDirection();
    if (facts.requested == Direction::UP || facts.requested == Direction::DOWN) {
        result = resolveSpacingSlurDirection4(facts);
        return true;
    }
    if (facts.requested != Direction::AUTO) return false;
    const Element* a = slur->startElement();
    const Element* b = slur->endElement();
    if (!a || !b || !a->isChordRest() || !b->isChordRest()) return false;
    const ChordRest* start = toChordRest(a);
    const ChordRest* end = toChordRest(b);
    if (!start->measure() || !end->measure() || start->score() != slur->score() || end->score() != slur->score())
        return false;
    // Tablature stays on Evolution's existing renderer until its separate audit.
    if (!start->staff() || !end->staff() || start->staff()->isTabStaff(start->tick())
        || end->staff()->isTabStaff(end->tick())) return false;
    const Chord* c1 = start->isChord() ? toChord(start) : nullptr;
    const Chord* c2 = end->isChord() ? toChord(end) : nullptr;
    if (refresh && c2 && c2->staff() && !c2->staff()->isDrumStaff(c2->tick())
        && start->measure()->system() != end->measure()->system()) refreshChordDirection4(const_cast<Chord*>(c2));
    facts.anchorsReady = true;
    facts.crossStaffStart = c1 && c1->beam() && c1->beam()->cross();
    if (!facts.crossStaffStart) {
        for (const Measure* measure = start->measure(); measure && measure->tick() <= end->tick();
             measure = measure->nextMeasure()) {
            if (c1 && measure->hasVoices(start->staffIdx(), slur->tick(), slur->ticks() + end->ticks())) {
                facts.multipleVoices = true;
                break;
            }
        }
        facts.secondaryVoice = start->voice() > 0 || end->voice() > 0;
        facts.startStem = start->up() ? Direction::UP : Direction::DOWN;
        if (!facts.multipleVoices && c1 && c2 && (!c1->isGrace() || c2 != c1->parent())
            && slurDirectionMixture4(c1, c2, refresh)) facts.mixture = SpacingStemMixture4::MIXED;
    }
    const Direction prepared = resolveSpacingSlurDirection4(facts);
    if (prepared == Direction::AUTO) return false;
    result = prepared;
    return true;
}

bool prepareSlurDirection4(Slur* slur, Direction& result)
{
    return slurDirectionImpl4(slur, result, true);
}

bool resolvePreparedSlurDirection4(const Slur* slur, Direction& result)
{
    return slurDirectionImpl4(slur, result, false);
}

}
