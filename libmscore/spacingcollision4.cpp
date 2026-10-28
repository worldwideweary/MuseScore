// Optional Studio spacing: common-notation collision policy.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacingprofile4.h"
#include "spacingcollision4.h"
#include "spacinginput4.h"
#include "spacinggeometry4.h"
#include "gracespacing4.h"
#include "slur.h"
#include "chord.h"
#include "beam.h"
#include "measure.h"
#include "segment.h"
#include "system.h"
#include "clef.h"
#include "note.h"
#include "rest.h"
#include "staff.h"
#include "stafftype.h"
#include "stafflines.h"
#include "score.h"
#include "shape.h"
#include "spanner.h"
#include "spannermap.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Ms {
static int paddingIndex4(ElementType type)
{
    static const ElementType types[] = {
        ElementType::NOTE, ElementType::LEDGER_LINE, ElementType::HOOK, ElementType::NOTEDOT,
        ElementType::REST, ElementType::CLEF, ElementType::BAR_LINE, ElementType::KEYSIG,
        ElementType::TIMESIG, ElementType::ARPEGGIO, ElementType::ACCIDENTAL, ElementType::STEM
    };
    for (int i = 0; i < 12; ++i) {
        if (types[i] == type)
            return i;
    }
    return -1;
}

SpacingPaddingTable4::SpacingPaddingTable4(const Score* score) : _score(score)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::PADDING_TABLE);
    const qreal spatium = score ? score->spatium() : 0;
    const qreal minPadUnit = 0.1 * spatium;
    for (auto& row : _padding)
        row.fill(minPadUnit);
    if (!score)
        return;
    const qreal ledgerPad = 0.25 * spatium;
    const qreal ledgerLength = score->styleP(Sid::ledgerLineLength);
    auto cell = [&](ElementType a, ElementType b) -> qreal& { return _padding[paddingIndex4(a)][paddingIndex4(b)]; };
    // Studio v4.7.5 PaddingTable::createTable, restricted to these 12 types.
    cell(ElementType::NOTE, ElementType::NOTE) = minPadUnit;
    cell(ElementType::NOTE, ElementType::LEDGER_LINE) = 0.35 * spatium;
    cell(ElementType::NOTE, ElementType::ACCIDENTAL)
        = std::max(static_cast<double>(score->styleP(Sid::accidentalNoteDistance)), 0.35 * spatium);
    cell(ElementType::NOTE, ElementType::REST) = 0.5 * spatium;
    cell(ElementType::NOTE, ElementType::CLEF) = 0.8 * spatium;
    cell(ElementType::NOTE, ElementType::ARPEGGIO) = 0.6 * spatium;
    cell(ElementType::NOTE, ElementType::BAR_LINE) = score->styleP(Sid::noteBarDistance);
    cell(ElementType::NOTE, ElementType::KEYSIG) = 0.75 * spatium;
    cell(ElementType::NOTE, ElementType::TIMESIG) = 0.75 * spatium;

    cell(ElementType::LEDGER_LINE, ElementType::NOTE) = cell(ElementType::NOTE, ElementType::LEDGER_LINE);
    cell(ElementType::LEDGER_LINE, ElementType::LEDGER_LINE) = ledgerPad;
    cell(ElementType::LEDGER_LINE, ElementType::ACCIDENTAL)
        = std::max(score->styleP(Sid::accidentalNoteDistance),
                   cell(ElementType::NOTE, ElementType::ACCIDENTAL) - ledgerLength / 2);
    cell(ElementType::LEDGER_LINE, ElementType::REST) = cell(ElementType::LEDGER_LINE, ElementType::NOTE);
    cell(ElementType::LEDGER_LINE, ElementType::CLEF)
        = std::max(cell(ElementType::NOTE, ElementType::CLEF) - ledgerLength / 2, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::ARPEGGIO) = 0.5 * spatium;
    cell(ElementType::LEDGER_LINE, ElementType::BAR_LINE)
        = std::max(cell(ElementType::NOTE, ElementType::BAR_LINE) - ledgerLength, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::KEYSIG)
        = std::max(cell(ElementType::NOTE, ElementType::KEYSIG) - ledgerLength / 2, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::TIMESIG)
        = std::max(cell(ElementType::NOTE, ElementType::TIMESIG) - ledgerLength / 2, ledgerPad);

    cell(ElementType::HOOK, ElementType::NOTE) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::HOOK, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::HOOK, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::REST) = cell(ElementType::HOOK, ElementType::NOTE);
    cell(ElementType::HOOK, ElementType::CLEF) = 0.5 * spatium;
    cell(ElementType::HOOK, ElementType::ARPEGGIO) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::BAR_LINE) = 1 * spatium;
    cell(ElementType::HOOK, ElementType::KEYSIG) = 1.15 * spatium;
    cell(ElementType::HOOK, ElementType::TIMESIG) = 1.15 * spatium;

    cell(ElementType::NOTEDOT, ElementType::NOTE) = std::max(score->styleP(Sid::dotNoteDistance), score->styleP(Sid::dotDotDistance));
    cell(ElementType::NOTEDOT, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::NOTEDOT, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::NOTEDOT, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::NOTEDOT, ElementType::REST) = cell(ElementType::NOTEDOT, ElementType::NOTE);
    cell(ElementType::NOTEDOT, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::NOTEDOT, ElementType::ARPEGGIO) = 0.5 * spatium;
    cell(ElementType::NOTEDOT, ElementType::BAR_LINE) = 0.8 * spatium;
    cell(ElementType::NOTEDOT, ElementType::KEYSIG) = 1.35 * spatium;
    cell(ElementType::NOTEDOT, ElementType::TIMESIG) = 1.35 * spatium;

    cell(ElementType::REST, ElementType::NOTE) = cell(ElementType::NOTE, ElementType::REST);
    cell(ElementType::REST, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::REST, ElementType::NOTE) - ledgerLength / 2, ledgerPad);
    cell(ElementType::REST, ElementType::ACCIDENTAL) = 0.45 * spatium;
    cell(ElementType::REST, ElementType::REST) = cell(ElementType::REST, ElementType::NOTE);
    cell(ElementType::REST, ElementType::CLEF) = cell(ElementType::NOTE, ElementType::CLEF);
    cell(ElementType::REST, ElementType::BAR_LINE) = 1.65 * spatium;
    cell(ElementType::REST, ElementType::KEYSIG) = 1.5 * spatium;
    cell(ElementType::REST, ElementType::TIMESIG) = 1.5 * spatium;

    cell(ElementType::CLEF, ElementType::NOTE) = score->styleP(Sid::clefKeyRightMargin);
    cell(ElementType::CLEF, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::CLEF, ElementType::NOTE) - ledgerLength / 2, ledgerPad);
    cell(ElementType::CLEF, ElementType::ACCIDENTAL) = 0.6 * spatium;
    cell(ElementType::CLEF, ElementType::REST) = cell(ElementType::CLEF, ElementType::NOTE);
    cell(ElementType::CLEF, ElementType::CLEF) = 0.75 * spatium;
    cell(ElementType::CLEF, ElementType::ARPEGGIO) = 0.65 * spatium;
    cell(ElementType::CLEF, ElementType::BAR_LINE) = score->styleP(Sid::clefBarlineDistance);
    cell(ElementType::CLEF, ElementType::KEYSIG) = score->styleP(Sid::clefKeyDistance);
    cell(ElementType::CLEF, ElementType::TIMESIG) = score->styleP(Sid::clefTimesigDistance);

    cell(ElementType::BAR_LINE, ElementType::NOTE) = score->styleP(Sid::barNoteDistance);
    cell(ElementType::BAR_LINE, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::BAR_LINE, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::BAR_LINE, ElementType::ACCIDENTAL) = score->styleP(Sid::barAccidentalDistance);
    cell(ElementType::BAR_LINE, ElementType::REST) = score->styleP(Sid::barNoteDistance);
    cell(ElementType::BAR_LINE, ElementType::CLEF) = score->styleP(Sid::clefLeftMargin);
    cell(ElementType::BAR_LINE, ElementType::ARPEGGIO) = 0.65 * spatium;
    cell(ElementType::BAR_LINE, ElementType::BAR_LINE) = 1.35 * spatium;
    cell(ElementType::BAR_LINE, ElementType::KEYSIG) = score->styleP(Sid::keysigLeftMargin);
    cell(ElementType::BAR_LINE, ElementType::TIMESIG) = score->styleP(Sid::timesigLeftMargin);

    cell(ElementType::KEYSIG, ElementType::NOTE) = 1.75 * spatium;
    cell(ElementType::KEYSIG, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::KEYSIG, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::KEYSIG, ElementType::ACCIDENTAL) = 1.6 * spatium;
    cell(ElementType::KEYSIG, ElementType::REST) = cell(ElementType::KEYSIG, ElementType::NOTE);
    cell(ElementType::KEYSIG, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::KEYSIG, ElementType::ARPEGGIO) = 1.35 * spatium;
    cell(ElementType::KEYSIG, ElementType::BAR_LINE) = score->styleP(Sid::keyBarlineDistance);
    cell(ElementType::KEYSIG, ElementType::KEYSIG) = 1 * spatium;
    cell(ElementType::KEYSIG, ElementType::TIMESIG) = score->styleP(Sid::keyTimesigDistance);

    cell(ElementType::TIMESIG, ElementType::NOTE) = 1.35 * spatium;
    cell(ElementType::TIMESIG, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::TIMESIG, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::TIMESIG, ElementType::ACCIDENTAL) = 0.8 * spatium;
    cell(ElementType::TIMESIG, ElementType::REST) = cell(ElementType::TIMESIG, ElementType::NOTE);
    cell(ElementType::TIMESIG, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::TIMESIG, ElementType::ARPEGGIO) = 1.35 * spatium;
    cell(ElementType::TIMESIG, ElementType::BAR_LINE) = score->styleP(Sid::timesigBarlineDistance);
    cell(ElementType::TIMESIG, ElementType::KEYSIG) = score->styleP(Sid::keyTimesigDistance);
    cell(ElementType::TIMESIG, ElementType::TIMESIG) = 1.0 * spatium;

    cell(ElementType::ARPEGGIO, ElementType::NOTE) = score->styleP(Sid::arpeggioNoteDistance);
    cell(ElementType::ARPEGGIO, ElementType::LEDGER_LINE) = 0.3 * spatium;
    cell(ElementType::ARPEGGIO, ElementType::ACCIDENTAL) = score->styleP(Sid::arpeggioAccidentalDistance);

    cell(ElementType::ACCIDENTAL, ElementType::NOTE) = score->styleP(Sid::accidentalNoteDistance);
    cell(ElementType::ACCIDENTAL, ElementType::LEDGER_LINE) = 0.18 * spatium;

    // Preserve the upstream ordering: copy note row, then note column,
    // then apply stem-specific overrides.
    _padding[paddingIndex4(ElementType::STEM)] = _padding[paddingIndex4(ElementType::NOTE)];
    for (auto& row : _padding)
        row[paddingIndex4(ElementType::STEM)] = row[paddingIndex4(ElementType::NOTE)];
    cell(ElementType::NOTE, ElementType::STEM) = score->styleP(Sid::minNoteDistance);
    cell(ElementType::REST, ElementType::STEM) = cell(ElementType::NOTE, ElementType::STEM);
    cell(ElementType::CLEF, ElementType::STEM) = 0.75 * spatium;
    cell(ElementType::ACCIDENTAL, ElementType::STEM) = cell(ElementType::ACCIDENTAL, ElementType::NOTE);
    cell(ElementType::STEM, ElementType::NOTE) = score->styleP(Sid::minNoteDistance);
    cell(ElementType::STEM, ElementType::STEM) = 0.85 * spatium;
    cell(ElementType::STEM, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::STEM, ElementType::LEDGER_LINE) = 0.35 * spatium;
    cell(ElementType::LEDGER_LINE, ElementType::STEM) = 0.35 * spatium;

}

bool SpacingPaddingTable4::padding(ElementType first, ElementType second, qreal& result) const
{
    const int a = paddingIndex4(first), b = paddingIndex4(second);
    if (!_score || a < 0 || b < 0)
        return false;
    result = _padding[a][b];
    return true;
}

// Horizontal admission only. The specialized merged-grace curve/anchor adapter
// retains its exact owner/target contract; this longer slur uses native layout.
static bool forwardAfterGraceSlur4(const Slur* slur)
{
    // Horizontal endpoint policy is independent of rendered system segments.
    // The specialized grace-pair curve adapter keeps its separate bounds.
    if (!slur || !slur->visible() || slur->lineType() != 0
        || slur->slurDirection() == Direction::AUTO || !slur->startElement() || !slur->endElement()
        || !slur->startElement()->isChord() || !slur->endElement()->isChord()) return false;
    const Chord* start = toChord(slur->startElement());
    const Chord* end = toChord(slur->endElement());
    if (!start->isGraceAfter() || !start->parent() || !start->parent()->isChord()) return false;
    const Chord* owner = toChord(start->parent());
    const Chord* endMain = end;
    if (end->isGrace()) {
        if (!end->parent() || !end->parent()->isChord()) return false;
        endMain = toChord(end->parent());
        const auto members = end->isGraceAfter() ? endMain->graceNotesAfter() : endMain->graceNotesBefore();
        if (std::find(members.begin(), members.end(), end) == members.end()) return false;
    }
    if (!owner->measure() || !endMain->measure() || owner->isGrace() || endMain->isGrace()
        || start->score() != slur->score() || owner->score() != slur->score()
        || end->score() != slur->score() || endMain->score() != slur->score()
        || owner->track() != start->track() || owner->track() != end->track() || owner->track() != endMain->track()
        || slur->track() != owner->track() || slur->track2() != end->track()
        || owner->staffMove() || start->staffMove() || end->staffMove() || endMain->staffMove()
        || owner->graceNotesAfter().empty() || owner->graceNotesAfter().front() != start
        || slur->tick() != owner->tick() || slur->tick2() != end->tick()) return false;
    // Grace anchors are visited by Shift+Arrow too. Tick alone cannot order
    // an owner's after-graces: all share the owner's tick.
    if (end->isGrace()) {
        if (endMain->tick() > owner->tick()) return true;
        if (endMain != owner || !end->isGraceAfter()) return false;
        const auto after = owner->graceNotesAfter();
        return std::find(after.begin() + 1, after.end(), end) != after.end();
    }
    // Keep the established adjacent main-note pair's specialized contract.
    return end->tick() > owner->tick() + owner->actualTicks();
}

// Ordinary main-note starts must not discard an otherwise supported grace
// group merely because a second slur touches its owner/target. Curves remain
// native. Automatic direction is admitted only where both Evolution and
// Studio computeUp produce the same result in a bounded single-voice span.
static bool mainStartSlur4(const Slur* slur, bool* direction = nullptr)
{
    if (!slur || !slur->visible() || slur->lineType() != 0
        || !slur->startElement() || !slur->endElement()
        || !slur->startElement()->isChord() || !slur->endElement()->isChord()) return false;
    const Chord* start = toChord(slur->startElement());
    const Chord* end = toChord(slur->endElement());
    if (start->isGrace() || !start->segment() || !start->measure()) return false;
    const Chord* main = end;
    if (end->isGrace()) {
        if (!end->parent() || !end->parent()->isChord()) return false;
        main = toChord(end->parent());
        const auto members = end->isGraceAfter() ? main->graceNotesAfter() : main->graceNotesBefore();
        if (std::find(members.begin(), members.end(), end) == members.end()) return false;
    }
    if (main->isGrace() || !main->segment() || main->measure() != start->measure()
        || slur->score() != start->score() || end->score() != start->score() || main->score() != start->score()
        || start->track() != end->track() || main->track() != start->track()
        || slur->track() != start->track() || slur->track2() != end->track()
        || start->staffMove() || end->staffMove() || main->staffMove()
        || slur->tick() != start->tick() || slur->tick2() != end->tick()
        || (start->beam() && start->beam()->cross()) || (end->beam() && end->beam()->cross())
        || (main->beam() && main->beam()->cross())) return false;
    if (main->tick() < start->tick()
        || (main->tick() == start->tick() && (main != start || !end->isGraceAfter()))) return false;
    bool up = slur->slurDirection() == Direction::UP;
    if (slur->slurDirection() == Direction::AUTO) {
        // No cached Slur::up(): a newly added or reanchored slur may not have
        // been laid out yet. Reject cases needing the broader direction audit.
        if (start->voice() != 0
            || start->measure()->hasVoices(start->staffIdx(), start->tick(),
                                           main->tick() + main->actualTicks() - start->tick())) return false;
        bool reached = false;
        bool legacyMixture = false;
        bool studioMixture = end->isGrace() && end->up() != start->up();
        for (const Segment* segment = start->segment(); segment; segment = segment->next()) {
            const Element* item = segment->element(start->track());
            if (segment->isChordRestType() && item && item->isChordRest()) {
                if (!item->isChord()) return false;
                const Chord* chord = toChord(item);
                if (chord->staffMove() || (chord->beam() && chord->beam()->cross())) return false;
                legacyMixture = legacyMixture || chord->up() != start->up();
                // Studio omits the end owner's main chord for a before-grace;
                // Evolution includes it. Accept AUTO only when results agree.
                if (segment != main->segment() || !end->isGraceBefore())
                    studioMixture = studioMixture || chord->up() != start->up();
                for (const Note* note : chord->notes())
                    if (note->tieFor() || note->tieBack()) return false;
            }
            if (segment == main->segment()) { reached = true; break; }
        }
        if (!reached) return false;
        up = legacyMixture || !start->up();
        if (up != (studioMixture || !start->up())) return false;
    }
    if (direction) *direction = up;
    return true;
}

// Studio Chord::computeKerningExceptions: a slur on the side opposite the
// stem prohibits note kerning on that side. The forward after-grace and
// main-start spans use this rule; established local/merged pairs keep theirs.
static unsigned forwardSlurKerning4(const Chord* chord, const SpacingInput4* input = nullptr)
{
    unsigned blocked = 0;
    const auto include = [chord, &blocked](const Slur* slur) {
        if (slur->startElement() != chord && slur->endElement() != chord) return;
        bool up = slur->slurDirection() == Direction::UP;
        if (!forwardAfterGraceSlur4(slur) && !mainStartSlur4(slur, &up)) return;
        if (up && !chord->up()) blocked |= 1;
        if (!up && chord->up()) blocked |= 2;
    };
    if (input) {
        const auto* anchor = input->anchor(chord);
        if (anchor)
            for (size_t index : anchor->relatedSlurs) include(input->slurs()[index].slur);
    }
    else {
        for (const auto& entry : chord->score()->spannerMap().map())
            if (entry.second->isSlur()) include(toSlur(entry.second));
    }
    return blocked;
}

SpacingContextCache4::SpacingContextCache4(const SpacingAttachmentPreparation4& preparation, SpacingSlurPolicy4 policy)
    : _input(&preparation.input()), _preparation(&preparation), _slurPolicy(policy)
{
}

bool SpacingContextCache4::preparedSlursReady(const Chord* chord) const
{
    if (!_preparation || !chord || _input->score() != chord->score()) return false;
    const auto* anchor = _input->anchor(chord);
    const auto* sides = _preparation->sides(chord);
    if (!anchor || !sides || !sides->complete()) return false;
    const size_t index = size_t(anchor - _input->anchors().data());
    if (_preparation->anchorDirections()[index] == Direction::AUTO) return false;
    for (size_t rootIndex : anchor->relatedSlurs) {
        const auto& root = _input->slurs()[rootIndex];
        // Tick aliases do not create attachments. They only identify unknown
        // raw anchors that could otherwise be silently treated as absent.
        if (!root.start.element || !root.end.element
            || root.start.kind == SpacingEndpointKind4::OTHER || root.end.kind == SpacingEndpointKind4::OTHER) return false;
    }
    return true;
}

unsigned SpacingContextCache4::forwardSlurKerning(const Chord* chord)
{
    for (const auto& entry : _slurKerning) if (entry.first == chord) return entry.second;
    unsigned blocked = 0;
    if (_slurPolicy == SpacingSlurPolicy4::PREPARED && _preparation) {
        const auto* anchor = _input->anchor(chord);
        const auto* sides = _preparation->sides(chord);
        if (anchor && sides) {
            const Direction stem = _preparation->anchorDirections()[size_t(anchor - _input->anchors().data())];
            if (stem != Direction::AUTO) blocked = sides->blockedKerning(stem == Direction::UP);
        }
    }
    else blocked = forwardSlurKerning4(chord, _input);
    _slurKerning.push_back({chord, blocked});
    return blocked;
}

static bool commonChordContext4(const Chord* chord, bool allowGrace,
                                const SpacingGraceContext4* context, const Chord* scopedMain, const SpacingContextCache4* cache)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::CONTEXT);
    // Common notation only in this stage. Studio's line attachment and chord
    // kerning-exception inputs need a separate adapter before these can enter.
    if ((!allowGrace && chord->isGrace()) || !chord->articulations().empty())
        return false;
    for (const Note* note : chord->notes()) {
        if (note->tieFor() || note->tieBack() || !note->spannerFor().empty() || !note->spannerBack().empty())
            return false;
    }
    if (cache && cache->slurPolicy() == SpacingSlurPolicy4::PREPARED)
        return cache->preparedSlursReady(chord);
    const SpacingInput4* input = cache ? cache->input() : nullptr;
    const Chord* main = chord;
    if (chord->isGrace() && chord->parent() && chord->parent()->isChord())
        main = toChord(chord->parent());
    const bool localScope = main == scopedMain || (context && context->contains(main));
    bool jointScope = false;
    if (context) {
        for (const auto& pair : context->mergedPairs)
            jointScope = jointScope || main == pair.first || main == pair.second;
    }
    int localSlurs = 0;
    const auto supportedSlur = [&](const Slur* slur) {
        // Selection of relevant roots is independent of this existing policy.
        // Preserve current behavior here until the general consumer is ready.
        if (forwardAfterGraceSlur4(slur) || mainStartSlur4(slur)) return true;
        bool supported = localScope && spacingLocalGraceSlur4(slur, main, true);
        if (context) {
            for (const auto& pair : context->mergedPairs)
                if ((main == pair.first || main == pair.second)
                    && spacingMergedGraceSlur4(slur, pair.first, pair.second, true)) supported = true;
        }
        return supported && ++localSlurs <= (jointScope ? 2 : 1);
    };
    if (input) {
        const auto* anchor = input->anchor(chord);
        if (!anchor) return false;
        for (size_t index : anchor->relatedSlurs)
            if (!supportedSlur(input->slurs()[index].slur)) return false;
    }
    else {
        for (const auto& entry : chord->score()->spannerMap().map()) {
            const Spanner* spanner = entry.second;
            if (!spanner->isSlur()) continue;
            if (spanner->startElement() == chord || spanner->endElement() == chord
                || (spanner->track() == chord->track() && spanner->tick() == chord->tick())
                || (spanner->track2() == chord->track() && spanner->tick2() == chord->tick()))
                if (!supportedSlur(toSlur(spanner))) return false;
        }
    }
    return true;
}

bool SpacingContextCache4::lookup(const Chord* chord, bool allowGrace,
                                   const SpacingGraceContext4* context, const Chord* scopedMain, bool& result)
{
    // Compare scope values, not an address: merged-policy wrappers construct
    // equivalent temporary contexts at different addresses for each pair.
    const bool sameContext = _hasContext == bool(context) && (!context
        || (_context.mains == context->mains && _context.afterMains == context->afterMains
            && _context.mergedPairs == context->mergedPairs));
    if (!_initialized || _allowGrace != allowGrace || _scopedMain != scopedMain || !sameContext) {
        _initialized = true;
        _allowGrace = allowGrace;
        _hasContext = bool(context);
        _scopedMain = scopedMain;
        _context = context ? *context : SpacingGraceContext4();
        _chords.clear();
    }
    for (const auto& entry : _chords) {
        if (entry.first == chord) {
            result = entry.second;
            return true;
        }
    }
    return false;
}

const SpacingGraceContext4* SpacingContextCache4::mergedContext(const Chord* owner, const Chord* target)
{
    if (_mergedOwner != owner || _mergedTarget != target) {
        _mergedOwner = owner;
        _mergedTarget = target;
        _mergedContext.mains = {target};
        _mergedContext.afterMains = {owner};
        _mergedContext.mergedPairs = {{owner, target}};
    }
    return &_mergedContext;
}

static bool commonItemContext4(const Element* item, bool allowGrace,
                               const SpacingGraceContext4* context, const Chord* scopedMain, SpacingContextCache4* cache)
{
    if (!item || item->track() < 0 || item->onTabStaff())
        return false;
    if (cache && cache->input() && cache->input()->score() != item->score()) return false;
    if (item->isRest() && toRest(item)->measure() && toRest(item)->measure()->isMMRest())
        return false;
    const Chord* chord = nullptr;
    for (const Element* parent = item; parent; parent = parent->parent()) {
        if (parent->isChord()) {
            chord = toChord(parent);
            break;
        }
    }
    if ((item->isNote() || item->isStem() || item->isHook()) && !chord)
        return false;
    if (!chord)
        return true;
    bool supported;
    if (cache && cache->lookup(chord, allowGrace, context, scopedMain, supported))
        return supported;
    supported = commonChordContext4(chord, allowGrace, context, scopedMain, cache);
    if (cache) cache->remember(chord, supported);
    return supported;
}

static bool sameVoiceLimited4(const Element* item)
{
    return item->isNote() || item->isNoteDot() || item->isRest() || item->isStem();
}

static bool neverKernable4(const Element* item)
{
    return (item->isClef() && item->rtick().isZero()) || item->isKeySig() || item->isTimeSig() || item->isBarLine();
}

static bool nativeShapesIntersect4(const Element* first, const Element* second)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::PAIR_OVERLAP);
    return first->shape().translated(first->pos()).intersects(second->shape().translated(second->pos()));
}

bool SpacingContextCache4::shapesIntersect(const Element* first, const Element* second)
{
    auto& row = _overlaps[first];
    const auto found = row.find(second);
    if (found != row.end())
        return found->second;
    const bool overlap = nativeShapesIntersect4(first, second);
    row.emplace(second, overlap);
    return overlap;
}

// Compare within one staff frame before system/page placement exists. Match
// Evolution's native pagePos overrides: ledger lines ignore chord position;
// grace chords use their own y plus staff-type offset, not the main chord y.
bool spacingLocalSlurSide4(const Note* first, const Element* second, unsigned& result)
{
    if (!first || !second || first->score() != second->score()) return false;
    const Measure* measure = first->findMeasure();
    if (!measure || second->findMeasure() != measure) return false;
    const int staff = first->vStaffIdx();
    const int otherStaff = second->isLedgerLine() ? second->staffIdx() : second->vStaffIdx();
    if (staff < 0 || staff != otherStaff || staff >= first->score()->nstaves()) return false;
    const auto ordinate = [measure](const Element* item, qreal& y) {
        if (item->isLedgerLine()) {
            y = item->y();
            return std::isfinite(y);
        }
        y = 0;
        for (const Element* part = item; part; part = part->parent()) {
            y += part->y();
            if (!part->isChordRest()) continue;
            const ChordRest* cr = toChordRest(part);
            if (cr->isChord() && toChord(cr)->isGrace()) {
                if (!cr->staff()) return false;
                y += cr->staff()->staffType(cr->tick())->yoffset().val() * cr->spatium();
            }
            else {
                const StaffLines* lines = const_cast<Measure*>(measure)->staffLines(cr->vStaffIdx());
                if (!lines) return false;
                y += lines->y();
            }
            return std::isfinite(y);
        }
        return false; // No supported staff-coordinate root.
    };
    qreal a = 0, b = 0;
    if (!ordinate(first, a) || !ordinate(second, b)) return false;
    result = b < a ? 1 : 2;
    return true;
}

static bool pairPolicy4(const SpacingPaddingTable4& table, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result, bool allowGrace,
                        const SpacingGraceContext4* context = nullptr, const Chord* scopedMain = nullptr,
                        SpacingContextCache4* cache = nullptr)
{
    if (!table.score() || !first || !second || first->score() != table.score() || second->score() != table.score()
        || !commonItemContext4(first, allowGrace, context, scopedMain, cache)
        || !commonItemContext4(second, allowGrace, context, scopedMain, cache)
        || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium))
        return false;
    SpacingPairPolicy4 policy;
    if (!table.padding(first->type(), second->type(), policy.padding)) {
        // Studio's default padding-table cell for the slash is 0.1sp.
        if (!allowGrace || (first->type() != ElementType::STEM_SLASH && second->type() != ElementType::STEM_SLASH))
            return false;
        policy.padding = 0.1 * table.score()->spatium();
    }
    const qreal scaling = (first->mag() + second->mag()) / 2;
    if (!(scaling > 0) || !std::isfinite(scaling))
        return false;
    if (first->isNote() && (second->isNote() || second->isRest() || second->isStem())) {
        const Note* note = toNote(first);
        if (!note->chord())
            return false;
        const bool sameVoiceNoteOrStem = (second->isNote() || second->isStem()) && note->track() == second->track();
        const bool adjacent = note->rtick() + note->chord()->actualTicks() == second->rtick();
        if ((sameVoiceNoteOrStem || adjacent)
            && (cache ? cache->shapesIntersect(note, second) : nativeShapesIntersect4(note, second)))
            policy.padding = std::max(policy.padding, table.score()->styleP(Sid::minNoteDistance));
    }
    policy.padding *= scaling;
    if (allowGrace && first->isNote() && (second->isNote() || second->isRest())) {
        const bool a = toNote(first)->chord()->isGrace();
        const bool b = second->isNote() && toNote(second)->chord()->isGrace();
        if (a || b)
            policy.padding = std::max(policy.padding, (a && b ? 0.3 : 0.45) * table.score()->spatium());
    }
    if (!first->isLedgerLine() && second->isRest()) {
        switch (toRest(second)->sym()) {
        case SymId::restWholeLegerLine:
        case SymId::restDoubleWholeLegerLine:
        case SymId::restHalfLegerLine:
            policy.padding += second->bbox().left();
            break;
        default:
            break;
        }
    }
    policy.verticalClearance = (second->isAccidental() ? 0.1 : 0.2) * shapeSpatium;
    if (first->isRest() && toRest(first)->isFullMeasureRest() && first->staffIdx() != second->staffIdx())
        policy.kerning = SpacingKerning4::ALLOW_COLLISION;
    else if ((sameVoiceLimited4(first) && sameVoiceLimited4(second) && first->track() == second->track())
             || neverKernable4(first) || neverKernable4(second))
        policy.kerning = SpacingKerning4::NON_KERNING;
    else if ((first->isNote() && second->isClef()) || (first->isClef() && second->isNote()))
        policy.kerning = SpacingKerning4::RIGHT_EDGE;
    else
        policy.kerning = SpacingKerning4::KERNING;
    if (first->isNote() && policy.kerning == SpacingKerning4::KERNING) {
        const Note* note = toNote(first);
        const Chord* chord = note->chord();
        // Studio permits offset up-stem seconds to kern on either side.
        if (!(chord->up() && note->pos().x() > 0)) {
            const unsigned blocked = cache ? cache->forwardSlurKerning(chord) : forwardSlurKerning4(chord);
            if (blocked) {
                unsigned side = 0;
                const bool local = cache && cache->slurPolicy() == SpacingSlurPolicy4::PREPARED
                    && spacingLocalSlurSide4(note, second, side);
                if (!local) {
                    // Grace preparation also queries this policy before measures
                    // belong to systems. LedgerLine::pagePos dereferences that
                    // system, so only inspect page coordinates in a ready frame.
                    const Measure* a = note->findMeasure();
                    const Measure* b = second->findMeasure();
                    const System* system = a ? a->system() : nullptr;
                    if (!system || !b || b->system() != system) return false;
                    const int staves = system->staves()->size();
                    if (note->vStaffIdx() < 0 || note->vStaffIdx() >= staves
                        || second->vStaffIdx() < 0 || second->vStaffIdx() >= staves
                        || second->staffIdx() < 0 || second->staffIdx() >= staves) return false;
                    const qreal firstY = note->pagePos().y();
                    const qreal secondY = second->pagePos().y();
                    if (!std::isfinite(firstY) || !std::isfinite(secondY)) return false;
                    side = secondY < firstY ? 1 : 2;
                }
                if (blocked & side) policy.kerning = SpacingKerning4::NON_KERNING;
            }
        }
    }
    if (allowGrace && first->type() == ElementType::STEM_SLASH) {
        const auto chordOf = [](const Element* item) -> const Chord* {
            for (; item; item = item->parent()) {
                if (item->isChord())
                    return toChord(item);
            }
            return nullptr;
        };
        const Chord* a = chordOf(first);
        const Chord* b = chordOf(second);
        policy.kerning = a && b && a->beam() && a->beam() == b->beam()
            ? SpacingKerning4::ALLOW_COLLISION : SpacingKerning4::KERNING;
    }
    if (!std::isfinite(policy.padding))
        return false;
    result = policy;
    return true;
}

bool SpacingGraceContext4::contains(const Chord* chord) const
{
    return std::find(mains.begin(), mains.end(), chord) != mains.end();
}

bool SpacingGraceContext4::containsAfter(const Chord* chord) const
{
    return std::find(afterMains.begin(), afterMains.end(), chord) != afterMains.end();
}

bool spacingPairPolicy4(const SpacingPaddingTable4& table, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result, const SpacingGraceContext4* context, SpacingContextCache4* cache)
{
    bool hasGrace = false;
    for (const Element* item : {first, second}) {
        for (; item; item = item->parent()) {
            if (!item->isChord())
                continue;
            const Chord* chord = toChord(item);
            if (chord->isGrace()) {
                if (!context || !chord->parent() || !chord->parent()->isChord()) return false;
                const Chord* root = toChord(chord->parent());
                if (!(chord->isGraceBefore() ? context->contains(root)
                      : chord->isGraceAfter() && context->containsAfter(root)))
                    return false;
                hasGrace = true;
            }
            break;
        }
    }
    return pairPolicy4(table, first, second, shapeSpatium, result, hasGrace, context, nullptr, cache);
}

bool spacingGracePairPolicy4(const SpacingPaddingTable4& table, const Chord* main,
                             const Element* first, const Element* second,
                             qreal shapeSpatium, SpacingPairPolicy4& result, SpacingContextCache4* cache)
{
    if (!main || main->isGrace())
        return false;
    const auto belongs = [main](const Element* item) {
        for (; item; item = item->parent()) {
            if (item->isChord()) {
                const Chord* chord = toChord(item);
                return chord == main || (chord->isGraceBefore() && chord->parent() == main);
            }
        }
        return false;
    };
    return belongs(first) && belongs(second) && pairPolicy4(table, first, second, shapeSpatium, result, true, nullptr, main, cache);
}

static bool afterGraceScopePolicy4(const SpacingPaddingTable4& table, const Chord* owner, const Element* target,
                                  const Element* first, const Element* second,
                                  qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner, bool includeBefore, SpacingContextCache4* cache)
{
    if (!owner || !target || owner == target || owner->isGrace()
        || (!target->isChord() && !target->isRest() && !target->isBarLine()) || (target->isChord() && toChord(target)->isGrace())
        || owner->score() != target->score() || owner->track() != target->track())
        return false;
    const auto belongs = [owner, target, includeOwner, includeBefore](const Element* item) {
        for (; item; item = item->parent()) {
            if (item == target) return true;
            if (item->isChord()) {
                const Chord* chord = toChord(item);
                return chord == target || (includeOwner && chord == owner) || (chord->isGraceAfter() && chord->parent() == owner)
                    || (includeBefore && chord->isGraceBefore() && chord->parent() == target);
            }
        }
        return false;
    };
    SpacingGraceContext4 local;
    const SpacingGraceContext4* joint = nullptr;
    if (includeBefore) {
        if (cache)
            joint = cache->mergedContext(owner, toChord(target));
        else {
            local.mains.push_back(toChord(target));
            local.afterMains.push_back(owner);
            local.mergedPairs.push_back({owner, toChord(target)});
            joint = &local;
        }
    }
    return belongs(first) && belongs(second)
        && pairPolicy4(table, first, second, shapeSpatium, result, true, joint, nullptr, cache);
}

bool spacingAfterGracePairPolicy4(const SpacingPaddingTable4& table, const Chord* owner, const Element* target,
                                  const Element* first, const Element* second,
                                  qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner, SpacingContextCache4* cache)
{
    return afterGraceScopePolicy4(table, owner, target, first, second, shapeSpatium, result, includeOwner, false, cache);
}

bool spacingMergedGracePairPolicy4(const SpacingPaddingTable4& table, const Chord* owner, const Chord* target,
                                   const Element* first, const Element* second,
                                   qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner, SpacingContextCache4* cache)
{
    return afterGraceScopePolicy4(table, owner, target, first, second, shapeSpatium, result, includeOwner, true, cache);
}

qreal spacingPairDistance4(const QRectF& first, const QRectF& second, const SpacingPairPolicy4& policy,
                           qreal shapeSpatium, qreal squeezeFactor)
{
    const qreal unconstrained = -std::numeric_limits<qreal>::max();
    if (first.isNull() || second.isNull() || policy.kerning == SpacingKerning4::ALLOW_COLLISION)
        return unconstrained;
    const qreal clearance = policy.verticalClearance * squeezeFactor;
    const bool intersection = first.top() != first.bottom() && second.top() != second.bottom()
        && first.bottom() + clearance > second.top() && first.top() < second.bottom() + clearance;
    if (policy.kerning == SpacingKerning4::NON_KERNING || intersection || first.width() == 0 || second.width() == 0) {
        const qreal padding = std::max(policy.padding * squeezeFactor, 0.1 * shapeSpatium * squeezeFactor);
        return first.right() - second.left() + padding;
    }
    switch (policy.kerning) {
    case SpacingKerning4::LEFT_EDGE:
        return first.left() - second.left();
    case SpacingKerning4::CENTER:
        return first.left() + 0.5 * first.width() - second.left();
    case SpacingKerning4::RIGHT_EDGE:
        return first.right() - second.left();
    default:
        return unconstrained;
    }
}

static bool validRect4(const QRectF& r)
{
    return std::isfinite(r.x()) && std::isfinite(r.y()) && std::isfinite(r.width()) && std::isfinite(r.height())
        && r.width() >= 0 && r.height() >= 0;
}

bool spacingShapeDistance4(const SpacingPaddingTable4& table, const SpacingShape4& first, const SpacingShape4& second,
                           qreal shapeSpatium, qreal squeezeFactor, qreal& result, const SpacingGraceContext4* context, SpacingContextCache4* queryCache)
{
    if (!table.score() || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium) || !std::isfinite(squeezeFactor)
        || squeezeFactor < 0 || squeezeFactor > 1)
        return false;
    // Reject the entire query if one primitive has unadapted provenance or
    // context. Never mix partial Studio constraints with a guessed minimum.
    for (const SpacingShape4* shape : { &first, &second }) {
        for (const SpacingShapeElement4& element : shape->elements) {
            if (!validRect4(element.rect))
                return false;
            if (!element.rect.isNull() && (element.identity != SpacingIdentity4::ITEM || !element.item))
                return false;
        }
    }
    SpacingContextCache4 localCache;
    SpacingContextCache4* cache = queryCache ? queryCache : &localCache;
    qreal distance = -std::numeric_limits<qreal>::max();
    for (const SpacingShapeElement4& b : second.elements) {
        if (b.rect.isNull())
            continue;
        for (const SpacingShapeElement4& a : first.elements) {
            if (a.rect.isNull())
                continue;
            SpacingPairPolicy4 policy;
            if (!spacingPairPolicy4(table, a.item, b.item, shapeSpatium, policy, context, cache))
                return false;
            distance = std::max(distance, spacingPairDistance4(a.rect, b.rect, policy, shapeSpatium, squeezeFactor));
        }
    }
    result = distance;
    return true;
}

bool spacingCollisionProjection4(const SpacingPaddingTable4& table, const SpacingShape4& native,
                                 qreal shapeSpatium, SpacingShape4& result, const SpacingGraceContext4* context, SpacingContextCache4* queryCache)
{
    SpacingContextCache4 localCache;
    SpacingContextCache4* cache = queryCache ? queryCache : &localCache;
    SpacingShape4 projected;
    if (!table.score() || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium))
        return false;
    const auto supportedAggregateOwner = [&](const Element* owner) {
        if (!owner)
            return false;
        if (owner->isChord()) {
            const Chord* chord = toChord(owner);
            if (chord->notes().empty())
                return false;
            for (const Note* note : chord->notes()) {
                SpacingPairPolicy4 policy;
                if (!spacingPairPolicy4(table, note, note, shapeSpatium, policy, context, cache))
                    return false;
            }
        }
        else if (!owner->isBarLine())
            return false;
        // Every producer needs its own identified component footprint.
        for (const SpacingShapeElement4& component : native.elements) {
            const auto belongsToOwner = [owner](const Element* item) {
                return item && (item == owner || (item->isNote() && toNote(item)->chord() == owner));
            };
            if (component.identity == SpacingIdentity4::ITEM && belongsToOwner(component.item))
                return true;
            if (component.identity == SpacingIdentity4::SHARED_ITEM) {
                for (const Element* item : component.sharedItems) {
                    if (belongsToOwner(item))
                        return true;
                }
            }
        }
        return false;
    };
    for (const SpacingShapeElement4& e : native.elements) {
        if (!validRect4(e.rect))
            return false;
        if (e.identity == SpacingIdentity4::SHARED_ITEM) {
            if (e.item || !e.aggregateOwners.empty() || e.sharedItems.size() < 2)
                return false;
            for (size_t i = 0; i < e.sharedItems.size(); ++i) {
                const Element* item = e.sharedItems[i];
                SpacingPairPolicy4 policy;
                // This stage admits shared noteheads only. Other coincident
                // component types still require their own context adapter.
                if (!item || !item->isNote() || !spacingPairPolicy4(table, item, item, shapeSpatium, policy, context, cache))
                    return false;
                for (size_t j = 0; j < i; ++j) {
                    if (item == e.sharedItems[j])
                        return false;
                }
                projected.elements.push_back(SpacingShapeElement4(e.rect, item));
            }
            continue;
        }
        if (!e.sharedItems.empty())
            return false;
        if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
            if (e.item) {
                if (!e.aggregateOwners.empty() || !supportedAggregateOwner(e.item))
                    return false;
            }
            else {
                if (e.aggregateOwners.size() < 2)
                    return false;
                for (size_t i = 0; i < e.aggregateOwners.size(); ++i) {
                    if (!supportedAggregateOwner(e.aggregateOwners[i]))
                        return false;
                    for (size_t j = 0; j < i; ++j) {
                        if (e.aggregateOwners[i] == e.aggregateOwners[j])
                            return false;
                    }
                }
            }
            continue;
        }
        if (!e.item || !e.aggregateOwners.empty())
            return false;
        SpacingPairPolicy4 policy;
        if (e.identity != SpacingIdentity4::ITEM || !spacingPairPolicy4(table, e.item, e.item, shapeSpatium, policy, context, cache))
            return false;
        projected.elements.push_back(e);
    }
    result = projected;
    return true;
}
}
