// Read-only musical inputs for the optional Studio spacing pipeline.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGINPUT4_H
#define MS_SPACINGINPUT4_H
#include <QString>
#include <cstddef>
#include "fraction.h"
#include "types.h"
#include <limits>
#include <unordered_map>
#include <vector>

namespace Ms {
class Score;
class Chord;
class ChordRest;
class Element;
class Measure;
class Segment;
class Note;
class Tie;
class Spanner;
class Slur;

enum class SpacingMemberKind4 { BEFORE, MAIN, AFTER };
enum class SpacingEndpointKind4 { MISSING, CHORD, REST, NOTE, OTHER };
constexpr size_t SPACING_INPUT_NONE4 = std::numeric_limits<size_t>::max();

struct SpacingAnchorInput4 {
    const ChordRest* item = nullptr;
    const Chord* main = nullptr; // Null for a rest; owner for every grace member.
    const Measure* measure = nullptr;
    const Segment* segment = nullptr;
    Fraction tick;
    int track = -1;
    size_t orderInTrack = 0; // Musical order within this track, not across voices.
    SpacingMemberKind4 kind = SpacingMemberKind4::MAIN;
    size_t memberIndex = 0; // Accessor order within BEFORE/AFTER; MAIN uses zero.
    bool up = false;
    bool visible = false;
    bool segmentEnabled = false;
    bool segmentVisible = false;
    int staffMove = 0;
    std::vector<size_t> startingSlurs;
    std::vector<size_t> endingSlurs;
};

struct SpacingNoteInput4 {
    const Note* note = nullptr;
    size_t anchorIndex = SPACING_INPUT_NONE4;
    const Tie* tieFor = nullptr;
    const Tie* tieBack = nullptr;
    std::vector<const Spanner*> spannersFor;
    std::vector<const Spanner*> spannersBack;
};

struct SpacingEndpointInput4 {
    const Element* element = nullptr; // Exact anchor, including an individual grace/note.
    SpacingEndpointKind4 kind = SpacingEndpointKind4::MISSING;
    size_t anchorIndex = SPACING_INPUT_NONE4;
};

struct SpacingSlurInput4 {
    const Slur* slur = nullptr;
    SpacingEndpointInput4 start;
    SpacingEndpointInput4 end;
    Fraction tick;
    Fraction tick2;
    int track = -1;
    int track2 = -1;
    Direction requestedDirection = Direction::AUTO;
    bool cachedUp = false; // Observation only; not an authoritative AUTO direction.
    bool visible = false;
    int lineType = 0;
};

// A preparation-pass value, never a score-owned or cross-command cache.
// Pointers are borrowed. Captured scalars/indices are immutable through this
// interface; discard the entire view before any score edit or layout write.
// This first stage collects metadata only and does not select a spacing policy.
class SpacingInput4 {
    const Score* _score = nullptr;
    std::vector<SpacingAnchorInput4> _anchors;
    std::vector<SpacingNoteInput4> _notes;
    std::vector<SpacingSlurInput4> _slurs;
    std::unordered_map<const ChordRest*, size_t> _anchorIndex;
    std::unordered_map<const Note*, size_t> _noteIndex;
    std::unordered_map<const Slur*, size_t> _slurIndex;
    friend bool buildSpacingInput4(Score*, SpacingInput4&, QString*);
public:
    const Score* score() const { return _score; }
    const std::vector<SpacingAnchorInput4>& anchors() const { return _anchors; }
    const std::vector<SpacingNoteInput4>& notes() const { return _notes; }
    const std::vector<SpacingSlurInput4>& slurs() const { return _slurs; }
    const SpacingAnchorInput4* anchor(const ChordRest*) const;
    const SpacingNoteInput4* note(const Note*) const;
    const SpacingSlurInput4* slur(const Slur*) const;
};

// Collects all tracks, grace members and root slurs without admission whitelists.
// Uses raw endpoint accessors: unresolved anchors stay unresolved, never lazily
// computed. No positions, styles, directions, selection or history are changed.
// Structural failure leaves result unchanged. Score's map accessor is not const.
bool buildSpacingInput4(Score*, SpacingInput4& result, QString* reason = nullptr);
}
#endif
