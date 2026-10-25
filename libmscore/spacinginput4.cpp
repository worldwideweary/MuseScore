// Read-only musical inputs for the optional Studio spacing pipeline.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacinginput4.h"
#include "score.h"
#include "measure.h"
#include "segment.h"
#include "chord.h"
#include "beam.h"
#include "rest.h"
#include "note.h"
#include "slur.h"
#include "spannermap.h"
#include <utility>
#include <map>
#include <algorithm>

namespace Ms {
const SpacingAnchorInput4* SpacingInput4::anchor(const ChordRest* item) const
{
    const auto found = _anchorIndex.find(item);
    return found == _anchorIndex.end() ? nullptr : &_anchors[found->second];
}

const SpacingSlurInput4* SpacingInput4::slur(const Slur* item) const
{
    const auto found = _slurIndex.find(item);
    return found == _slurIndex.end() ? nullptr : &_slurs[found->second];
}

const SpacingNoteInput4* SpacingInput4::note(const Note* item) const
{
    const auto found = _noteIndex.find(item);
    return found == _noteIndex.end() ? nullptr : &_notes[found->second];
}

bool SpacingSlurAttachments4::complete() const
{
    if (unmappedStarts || unmappedEnds) return false;
    for (const auto& anchor : anchors) if (!anchor.complete()) return false;
    return true;
}

bool buildSpacingSlurAttachments4(const SpacingInput4& input, SpacingSlurAttachments4& result,
                                 const std::vector<Direction>* directions, QString* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!input.score()) return fail("Missing musical inputs for slur attachments");
    if (directions && directions->size() != input.slurs().size())
        return fail("Slur direction input count differs from root count");
    SpacingSlurAttachments4 pending;
    pending.anchors.resize(input.anchors().size());
    for (size_t i = 0; i < input.slurs().size(); ++i) {
        const auto& slur = input.slurs()[i];
        Direction direction = directions ? (*directions)[i] : slur.requestedDirection;
        if ((direction != Direction::AUTO && direction != Direction::UP && direction != Direction::DOWN)
            || (slur.requestedDirection != Direction::AUTO && slur.requestedDirection != Direction::UP
                && slur.requestedDirection != Direction::DOWN))
            return fail("Invalid prepared slur direction");
        if (slur.requestedDirection != Direction::AUTO) {
            if (direction != Direction::AUTO && direction != slur.requestedDirection)
                return fail("Prepared direction contradicts explicit slur direction");
            direction = slur.requestedDirection;
        }
        const auto attach = [&](const SpacingEndpointInput4& endpoint, bool start) {
            if (endpoint.anchorIndex == SPACING_INPUT_NONE4) {
                if (start) ++pending.unmappedStarts;
                else ++pending.unmappedEnds;
                return;
            }
            auto& sides = pending.anchors[endpoint.anchorIndex];
            if (direction == Direction::AUTO) {
                if (start) ++sides.pendingStarts;
                else ++sides.pendingEnds;
            }
            else if (start) {
                if (direction == Direction::UP) sides.startUp = true;
                else sides.startDown = true;
            }
            else {
                if (direction == Direction::UP) sides.endUp = true;
                else sides.endDown = true;
            }
        };
        // Exact endpoint identities only: another grace with the same tick
        // has no attachment. Note endpoints map to their captured chord owner.
        // Visibility, line style, system count and curve contracts do not filter
        // Studio's checkStartEndSlurs flags; preserve that separation here.
        attach(slur.start, true);
        attach(slur.end, false);
    }
    result = std::move(pending);
    if (reason) reason->clear();
    return true;
}

bool buildSpacingSlurDirections4(const SpacingInput4& input, const std::vector<Direction>& directions,
                                std::vector<Direction>& result, QString* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    const auto valid = [](Direction d) {
        return d == Direction::AUTO || d == Direction::UP || d == Direction::DOWN;
    };
    if (!input.score()) return fail("Missing musical inputs for slur directions");
    if (directions.size() != input.anchors().size())
        return fail("Prepared direction count differs from anchor count");
    for (Direction direction : directions)
        if (!valid(direction)) return fail("Invalid prepared chord/rest direction");
    // Index each track once. Range queries below visit normal anchors only,
    // rather than rescanning all voices or consulting rendered slur segments.
    std::unordered_map<int, std::vector<size_t>> mains;
    std::unordered_map<const Chord*, size_t> mainIndex;
    for (size_t i = 0; i < input.anchors().size(); ++i) {
        const auto& anchor = input.anchors()[i];
        if (anchor.kind == SpacingMemberKind4::MAIN) {
            mains[anchor.track].push_back(i);
            if (anchor.main) mainIndex.emplace(anchor.main, i);
        }
    }
    std::vector<Direction> pending;
    pending.reserve(input.slurs().size());
    for (const auto& slur : input.slurs()) {
        if (!valid(slur.requestedDirection)) return fail("Invalid requested slur direction");
        if (slur.requestedDirection != Direction::AUTO) {
            pending.push_back(slur.requestedDirection);
            continue;
        }
        const auto resolve = [&]() -> Direction {
            // A raw NOTE/missing/foreign endpoint is retained as pending. The
            // renderer's lazy anchor repair is not a spacing-query operation.
            if (slur.start.anchorIndex == SPACING_INPUT_NONE4 || slur.end.anchorIndex == SPACING_INPUT_NONE4
                || (slur.start.kind != SpacingEndpointKind4::CHORD && slur.start.kind != SpacingEndpointKind4::REST)
                || (slur.end.kind != SpacingEndpointKind4::CHORD && slur.end.kind != SpacingEndpointKind4::REST))
                return Direction::AUTO;
            const auto& start = input.anchors()[slur.start.anchorIndex];
            const auto& end = input.anchors()[slur.end.anchorIndex];
            const bool startChord = slur.start.kind == SpacingEndpointKind4::CHORD;
            const bool endChord = slur.end.kind == SpacingEndpointKind4::CHORD;
            if (startChord && start.crossStaffBeam) return Direction::UP;
            // Studio gives voice selection priority over the mixture rule.
            if (slur.multipleVoices)
                return start.track % VOICES || end.track % VOICES ? Direction::DOWN : Direction::UP;
            const Direction stem = directions[slur.start.anchorIndex];
            if (stem == Direction::AUTO) return Direction::AUTO;
            const Direction opposite = stem == Direction::UP ? Direction::DOWN : Direction::UP;
            if (!startChord || !endChord || start.track != end.track
                || (start.kind != SpacingMemberKind4::MAIN && end.item == start.main)) return opposite;
            // A definite mixture resolves UP even when another stem is still
            // pending. Without a definite mixture, pending inputs stay pending.
            bool unknown = false;
            const auto mixed = [&](size_t index) {
                if (directions[index] == Direction::AUTO) unknown = true;
                return directions[index] != Direction::AUTO && directions[index] != stem;
            };
            if (end.kind != SpacingMemberKind4::MAIN && mixed(slur.end.anchorIndex)) return Direction::UP;
            if (start.kind == SpacingMemberKind4::BEFORE && end.kind == SpacingMemberKind4::AFTER
                && start.main == end.main) {
                const auto found = mainIndex.find(start.main);
                if (found != mainIndex.end() && input.anchors()[found->second].hasStem && mixed(found->second))
                    return Direction::UP;
            }
            const auto found = mains.find(start.track);
            if (found != mains.end()) {
                const auto& track = found->second;
                const auto first = std::lower_bound(track.begin(), track.end(), start.tick,
                    [&input](size_t index, const Fraction& tick) { return input.anchors()[index].tick < tick; });
                for (auto it = first; it != track.end(); ++it) {
                    const auto& anchor = input.anchors()[*it];
                    if (anchor.tick > end.tick) break;
                    if ((start.kind != SpacingMemberKind4::MAIN || end.kind == SpacingMemberKind4::BEFORE)
                        && anchor.tick == end.tick) break;
                    if (anchor.main && mixed(*it)) return Direction::UP;
                }
            }
            return unknown ? Direction::AUTO : opposite;
        };
        pending.push_back(resolve());
    }
    result = std::move(pending);
    if (reason) reason->clear();
    return true;
}

SpacingAttachmentPreparation4::SpacingAttachmentPreparation4(SpacingAttachmentPreparation4&& other)
{
    *this = std::move(other);
}

SpacingAttachmentPreparation4& SpacingAttachmentPreparation4::operator=(SpacingAttachmentPreparation4&& other)
{
    if (this != &other) {
        _input = std::move(other._input);
        _anchorDirections = std::move(other._anchorDirections);
        _slurDirections = std::move(other._slurDirections);
        _attachments = std::move(other._attachments);
        other._input = SpacingInput4();
        other._anchorDirections.clear();
        other._slurDirections.clear();
        other._attachments = SpacingSlurAttachments4();
    }
    return *this;
}

const SpacingSlurSides4* SpacingAttachmentPreparation4::sides(const ChordRest* chord) const
{
    const auto* anchor = _input.anchor(chord);
    return anchor ? &_attachments.anchors[size_t(anchor - _input.anchors().data())] : nullptr;
}

const Direction* SpacingAttachmentPreparation4::direction(const Slur* slur) const
{
    const auto* root = _input.slur(slur);
    return root ? &_slurDirections[size_t(root - _input.slurs().data())] : nullptr;
}

bool prepareSpacingAttachments4(SpacingInput4&& input, const std::vector<Direction>& anchorDirections,
                                SpacingAttachmentPreparation4& result, QString* reason)
{
    SpacingAttachmentPreparation4 pending;
    if (!buildSpacingSlurDirections4(input, anchorDirections, pending._slurDirections, reason)
        || !buildSpacingSlurAttachments4(input, pending._attachments, &pending._slurDirections, reason)) return false;
    pending._anchorDirections = anchorDirections;
    pending._input = std::move(input);
    input = SpacingInput4();
    result = std::move(pending);
    if (reason) reason->clear();
    return true;
}

bool buildSpacingInput4(Score* score, SpacingInput4& result, QString* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!score) return fail("Missing score for spacing inputs");
    SpacingInput4 pending;
    pending._score = score;
    std::unordered_map<int, size_t> nextOrder;
    const auto append = [&](const ChordRest* item, const Chord* main, const Measure* measure,
                            const Segment* segment, SpacingMemberKind4 kind, size_t member) {
        if (!item || item->score() != score || item->track() < 0
            || item->track() >= score->ntracks() || pending._anchorIndex.count(item)) return false;
        SpacingAnchorInput4 input;
        input.item = item;
        input.main = main;
        input.measure = measure;
        input.segment = segment;
        input.tick = item->tick();
        input.track = item->track();
        input.orderInTrack = nextOrder[input.track]++;
        input.kind = kind;
        input.memberIndex = member;
        input.up = item->up();
        input.visible = item->visible();
        input.segmentEnabled = segment->enabled();
        input.segmentVisible = segment->visible();
        input.staffMove = item->staffMove();
        if (item->isChord()) {
            const Chord* chord = toChord(item);
            input.crossStaffBeam = chord->beam() && chord->beam()->cross();
            input.hasStem = chord->stem();
        }
        const size_t index = pending._anchors.size();
        pending._anchorIndex.emplace(item, index);
        pending._anchors.push_back(std::move(input));
        if (item->isChord()) {
            for (const Note* note : toChord(item)->notes()) {
                if (!note || note->score() != score || note->chord() != item
                    || pending._noteIndex.count(note)) return false;
                SpacingNoteInput4 n;
                n.note = note;
                n.anchorIndex = index;
                n.tieFor = note->tieFor();
                n.tieBack = note->tieBack();
                for (const Spanner* spanner : note->spannerFor()) n.spannersFor.push_back(spanner);
                for (const Spanner* spanner : note->spannerBack()) n.spannersBack.push_back(spanner);
                pending._noteIndex.emplace(note, pending._notes.size());
                pending._notes.push_back(std::move(n));
            }
        }
        return true;
    };
    for (const Measure* measure = score->firstMeasure(); measure; measure = measure->nextMeasure()) {
        for (const Segment& segment : measure->segments()) {
            if (!segment.isChordRestType()) continue;
            for (const Element* element : segment.elist()) {
                if (!element || !element->isChordRest()) continue;
                const ChordRest* item = toChordRest(element);
                const Chord* main = item->isChord() ? toChord(item) : nullptr;
                if (item->segment() != &segment || item->measure() != measure || (main && main->isGrace()))
                    return fail("Invalid main spacing anchor ownership");
                if (main) {
                    size_t index = 0;
                    for (const Chord* grace : main->graceNotesBefore()) {
                        if (!grace || grace->parent() != main || !grace->isGraceBefore()
                            || grace->track() != main->track()
                            || !append(grace, main, measure, &segment, SpacingMemberKind4::BEFORE, index++))
                            return fail("Invalid before-grace spacing ownership");
                    }
                }
                if (!append(item, main, measure, &segment, SpacingMemberKind4::MAIN, 0))
                    return fail("Invalid or duplicate main spacing anchor");
                if (main) {
                    size_t index = 0;
                    // Evolution stores after-graces in reverse; the accessor
                    // supplies their actual navigation/musical order.
                    for (const Chord* grace : main->graceNotesAfter()) {
                        if (!grace || grace->parent() != main || !grace->isGraceAfter()
                            || grace->track() != main->track()
                            || !append(grace, main, measure, &segment, SpacingMemberKind4::AFTER, index++))
                            return fail("Invalid after-grace spacing ownership");
                    }
                }
            }
        }
    }
    std::map<std::pair<int, Fraction>, std::vector<size_t>> anchorsAt;
    for (size_t i = 0; i < pending._anchors.size(); ++i) {
        const auto& anchor = pending._anchors[i];
        anchorsAt[{anchor.track, anchor.tick}].push_back(i);
    }
    const auto endpoint = [&pending](const Element* element) {
        SpacingEndpointInput4 end;
        end.element = element;
        const ChordRest* anchor = nullptr;
        if (!element) return end;
        if (element->isChordRest()) {
            anchor = toChordRest(element);
            end.kind = element->isChord() ? SpacingEndpointKind4::CHORD : SpacingEndpointKind4::REST;
        }
        else if (element->isNote()) {
            anchor = toNote(element)->chord();
            end.kind = SpacingEndpointKind4::NOTE;
        }
        else end.kind = SpacingEndpointKind4::OTHER;
        const auto found = pending._anchorIndex.find(anchor);
        if (found != pending._anchorIndex.end()) end.anchorIndex = found->second;
        return end;
    };
    for (const auto& entry : score->spannerMap().map()) {
        if (!entry.second || !entry.second->isSlur()) continue;
        const Slur* slur = toSlur(entry.second);
        if (slur->score() != score || pending._slurIndex.count(slur))
            return fail("Invalid or duplicate root slur input");
        SpacingSlurInput4 input;
        input.slur = slur;
        input.start = endpoint(slur->startElement());
        input.end = endpoint(slur->endElement());
        input.tick = slur->tick();
        input.tick2 = slur->tick2();
        input.track = slur->track();
        input.track2 = slur->track2();
        input.requestedDirection = slur->slurDirection();
        input.cachedUp = slur->up();
        input.visible = slur->visible();
        input.lineType = slur->lineType();
        // Match Studio computeUp's span, including the end chord/rest's
        // duration. Do not resolve raw endpoints or update any stem here.
        if (input.start.kind == SpacingEndpointKind4::CHORD
            && input.end.anchorIndex != SPACING_INPUT_NONE4
            && (input.end.kind == SpacingEndpointKind4::CHORD || input.end.kind == SpacingEndpointKind4::REST)
            && input.start.anchorIndex != SPACING_INPUT_NONE4) {
            const auto& start = pending._anchors[input.start.anchorIndex];
            const auto& end = pending._anchors[input.end.anchorIndex];
            for (const Measure* measure = start.measure; measure && measure->tick() <= end.tick;
                 measure = measure->nextMeasure()) {
                if (measure->hasVoices(start.item->staffIdx(), input.tick, slur->ticks() + end.item->ticks())) {
                    input.multipleVoices = true;
                    break;
                }
            }
        }
        const size_t index = pending._slurs.size();
        pending._slurIndex.emplace(slur, index);
        pending._slurs.push_back(std::move(input));
        const auto& stored = pending._slurs.back();
        if (stored.start.anchorIndex != SPACING_INPUT_NONE4)
            pending._anchors[stored.start.anchorIndex].startingSlurs.push_back(index);
        if (stored.end.anchorIndex != SPACING_INPUT_NONE4)
            pending._anchors[stored.end.anchorIndex].endingSlurs.push_back(index);
        std::vector<size_t> related;
        const auto addAt = [&anchorsAt, &related](int track, const Fraction& tick) {
            const auto found = anchorsAt.find({track, tick});
            if (found != anchorsAt.end()) related.insert(related.end(), found->second.begin(), found->second.end());
        };
        addAt(stored.track, stored.tick);
        addAt(stored.track2, stored.tick2);
        if (stored.start.kind != SpacingEndpointKind4::NOTE && stored.start.anchorIndex != SPACING_INPUT_NONE4)
            related.push_back(stored.start.anchorIndex);
        if (stored.end.kind != SpacingEndpointKind4::NOTE && stored.end.anchorIndex != SPACING_INPUT_NONE4)
            related.push_back(stored.end.anchorIndex);
        std::sort(related.begin(), related.end());
        related.erase(std::unique(related.begin(), related.end()), related.end());
        // Root iteration order is preserved once per recipient, even when
        // start/end/tick conditions all match the same grace member.
        for (size_t anchor : related) pending._anchors[anchor].relatedSlurs.push_back(index);
    }
    result = std::move(pending);
    if (reason) reason->clear();
    return true;
}
}
