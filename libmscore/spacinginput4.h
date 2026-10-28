// Read-only musical inputs for the optional Studio spacing pipeline.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGINPUT4_H
#define MS_SPACINGINPUT4_H
#include <QString>
#include <cstddef>
#include "fraction.h"
#include "types.h"
#include "slurdirection4.h"
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Ms {
class Score;
class System;
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

// A contiguous musical domain and its roots, in native map order. Root raw
// endpoints outside the domain retain identity/kind but have no anchor index.
struct SpacingInputScope4 {
    std::vector<const Measure*> measures;
    std::vector<const Slur*> slurs;
};

struct SpacingAnchorInput4 {
    const ChordRest* item = nullptr;
    const Chord* main = nullptr; // Null for a rest; owner for every grace member.
    const Measure* measure = nullptr;
    const Segment* segment = nullptr;
    Fraction tick;
    int track = -1;
    size_t orderInTrack = 0; // Musical order within the captured domain/track, not across voices.
    SpacingMemberKind4 kind = SpacingMemberKind4::MAIN;
    size_t memberIndex = 0; // Accessor order within BEFORE/AFTER; MAIN uses zero.
    bool up = false;
    bool visible = false;
    bool segmentEnabled = false;
    bool segmentVisible = false;
    int staffMove = 0;
    bool crossStaffBeam = false;
    bool hasStem = false;
    std::vector<size_t> startingSlurs;
    std::vector<size_t> endingSlurs;
    // Current collision-policy relevance, including track/tick aliases.
    // Kept separate from exact endpoint indices so future policy need not
    // mistake another grace member with the same tick for an attachment.
    std::vector<size_t> relatedSlurs;
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
    bool multipleVoices = false; // Studio span query, captured without layout.
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
    friend struct SpacingInputBuilder4;
public:
    const Score* score() const { return _score; }
    const std::vector<SpacingAnchorInput4>& anchors() const { return _anchors; }
    const std::vector<SpacingNoteInput4>& notes() const { return _notes; }
    const std::vector<SpacingSlurInput4>& slurs() const { return _slurs; }
    const SpacingAnchorInput4* anchor(const ChordRest*) const;
    const SpacingNoteInput4* note(const Note*) const;
    const SpacingSlurInput4* slur(const Slur*) const;
};

// Source-side attachment flags, independent of curve admission and shape readiness.
// AUTO is never inferred from cachedUp. A producer may supply a direction per
// root after preparing stems/voices; absent AUTO directions remain pending.
struct SpacingSlurSides4 {
    bool startUp = false;
    bool startDown = false;
    bool endUp = false;
    bool endDown = false;
    size_t pendingStarts = 0;
    size_t pendingEnds = 0;
    unsigned blockedKerning(bool stemUp) const {
        return ((!stemUp && (startUp || endUp)) ? 1u : 0u)
             | ((stemUp && (startDown || endDown)) ? 2u : 0u);
    }
    bool complete() const { return !pendingStarts && !pendingEnds; }
};

struct SpacingSlurAttachments4 {
    // Same order as input.anchors(); includes each exact grace/note owner.
    std::vector<SpacingSlurSides4> anchors;
    size_t unmappedStarts = 0;
    size_t unmappedEnds = 0;
    bool complete() const;
};

// One linear preparation, never a score scan per collision query. If supplied,
// directions must match input.slurs() in size/order: AUTO means not prepared.
// Explicit score directions cannot be overridden. Unresolved directions and
// unmapped endpoints are reported, not silently treated as absent attachments.
// Failure leaves result unchanged; no live DOM or cached layout data is read.
bool buildSpacingSlurAttachments4(const SpacingInput4&, SpacingSlurAttachments4& result,
                                 const std::vector<Direction>* directions = nullptr,
                                 QString* reason = nullptr);

// Pinned Studio computeUp/isDirectionMixture rules on captured musical inputs.
// The caller supplies one prepared chord/rest direction per anchor; AUTO marks
// an input not prepared yet. Captured anchor.up/cachedUp are never substituted.
// Results follow root order and feed buildSpacingSlurAttachments4 directly.
// AUTO in the result means required input is pending, not rejected notation.
// No geometry, system membership or live DOM is read. Failure is atomic.
bool buildSpacingSlurDirections4(const SpacingInput4&, const std::vector<Direction>& anchorDirections,
                                std::vector<Direction>& result, QString* reason = nullptr);

// Owns the musical view, producer directions and derived attachment tables for
// one unchanged preparation phase. Consumers borrow this owner, never separate
// tables with unrelated lifetimes. Discard before edits or geometry writes.
// Do not move or replace the owner while a consumer borrows it.
class SpacingAttachmentPreparation4 {
    SpacingInput4 _input;
    std::vector<Direction> _anchorDirections;
    std::vector<Direction> _slurDirections;
    SpacingSlurAttachments4 _attachments;
    friend bool prepareSpacingAttachments4(SpacingInput4&&, const std::vector<Direction>&,
                                           SpacingAttachmentPreparation4&, QString*);
public:
    SpacingAttachmentPreparation4() = default;
    SpacingAttachmentPreparation4(const SpacingAttachmentPreparation4&) = delete;
    SpacingAttachmentPreparation4& operator=(const SpacingAttachmentPreparation4&) = delete;
    SpacingAttachmentPreparation4(SpacingAttachmentPreparation4&&);
    SpacingAttachmentPreparation4& operator=(SpacingAttachmentPreparation4&&);
    const SpacingInput4& input() const { return _input; }
    const std::vector<Direction>& anchorDirections() const { return _anchorDirections; }
    const std::vector<Direction>& slurDirections() const { return _slurDirections; }
    const SpacingSlurAttachments4& attachments() const { return _attachments; }
    const SpacingSlurSides4* sides(const ChordRest*) const;
    const Direction* direction(const Slur*) const;
    bool complete() const { return _input.score() && _attachments.complete(); }
};

// Compose the two preparation kernels once, outside plan/pair/prefix loops.
// On success, consume and reset input; on failure preserve both input and result.
// Pending producer directions are retained as pending, never replaced with
// captured stem/slur observations. No score collection or DOM writes occur.
bool prepareSpacingAttachments4(SpacingInput4&& input, const std::vector<Direction>& anchorDirections,
                                SpacingAttachmentPreparation4& result, QString* reason = nullptr);

// Explicit producer handoff by identity, independent of snapshot index order.
// AUTO means not prepared; omitted anchors remain AUTO too. This is a caller
// readiness declaration, never inferred from a stored or live up() observation.
struct SpacingPreparedAnchor4 {
    const ChordRest* item;
    Direction direction;
    SpacingPreparedAnchor4(const ChordRest* chord, Direction d) : item(chord), direction(d) {}
};

// Capture after all native producer writes, once per caller-owned preparation
// phase. No native writes/direction production occur here. Reject null, foreign,
// duplicate or invalid declarations atomically, including unmapped anchors.
// Never call this score-wide operation from a plan/pair/prefix fitting loop.
bool captureSpacingAttachments4(Score*, const std::vector<SpacingPreparedAnchor4>& prepared,
                                SpacingAttachmentPreparation4& result, QString* reason = nullptr);

// Pass-local producer completion records. Only the native measure-production
// boundary calls produced(); invalidate before reprocessing or any skipped pass.
// Store identity certificates, not cached directions. After later beam writes,
// declarations() reads the current direction only for still-present certified
// anchors. Nothing is inferred merely from geometry existing in the score.
class SpacingDirectionReadiness4 {
    std::unordered_map<const Measure*, std::unordered_set<const ChordRest*>> _produced;
public:
    void invalidate(const Measure* measure) { _produced.erase(measure); }
    void produced(const Measure*);
    std::vector<SpacingPreparedAnchor4> declarations(const System*) const;
    std::vector<SpacingPreparedAnchor4> declarations(const Measure*) const;
};

// Collects all tracks, grace members and root slurs without admission whitelists.
// Uses raw endpoint accessors: unresolved anchors stay unresolved, never lazily
// computed. No positions, styles, directions, selection or history are changed.
// Structural failure leaves result unchanged. Score's map accessor is not const.
bool buildSpacingInput4(Score*, SpacingInput4& result, QString* reason = nullptr);
// No whole-score measure/root walk; the caller supplies the native domain.
bool buildSpacingInput4(Score*, const SpacingInputScope4&, SpacingInput4& result, QString* reason = nullptr);

// Early measure handoff using the same collector as the final system. The
// caller supplies readiness declarations after native direction production.
// External root endpoints remain pending; no cached direction is substituted.
// Works before the measure belongs to a system. Failure preserves result.
bool captureMeasureSpacingAttachments4(const Measure*, const std::vector<SpacingPreparedAnchor4>& prepared,
                                       SpacingAttachmentPreparation4& result, QString* reason = nullptr);

// Finalized-system handoff: contiguous measures plus roots from the native
// interval index, retaining native map order within equal start ticks. Current
// collision policy borrows this view across all read-only squeeze passes.
// Omitted readiness declarations remain AUTO, including external endpoints.
// No producers or geometry writes occur. Never use this in prefix fitting.
bool captureSystemSpacingAttachments4(System*, const std::vector<SpacingPreparedAnchor4>& prepared,
                                      SpacingAttachmentPreparation4& result, QString* reason = nullptr);
}
#endif
