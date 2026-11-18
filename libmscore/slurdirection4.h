// Studio slur direction shared by prepared spacing and native rendering.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SLURDIRECTION4_H
#define MS_SLURDIRECTION4_H
#include "types.h"

namespace Ms {
class Slur;
class ChordRest;
class System;
enum class SpacingStemMixture4 { NONE, MIXED, PENDING };
struct SpacingSlurDirectionFacts4 {
    Direction requested = Direction::AUTO;
    bool anchorsReady = false;
    bool crossStaffStart = false;
    bool multipleVoices = false;
    bool secondaryVoice = false;
    Direction startStem = Direction::AUTO;
    SpacingStemMixture4 mixture = SpacingStemMixture4::NONE;
};
// Pure priority rule; AUTO reports pending input, never a cached direction.
Direction resolveSpacingSlurDirection4(const SpacingSlurDirectionFacts4&);

// Native producer phase, not a read-only collision query. Refreshes note lines
// and stem direction on later-system pitched chords as pinned Studio does.
// Native line refresh also updates derived note y. Does not write slur
// properties or anchors. Call before capturing immutable musical/geometry
// inputs, never inside collision queries. Failure preserves result.
bool prepareSlurDirection4(Slur*, Direction& result);

// Rendering entry point: selects the Studio direction producer only when
// the optional horizontal-spacing style is enabled.  Failure preserves result.
bool prepareStudioSlurDirection4(Slur*, Direction& result);

// Studio beam-end and across-system stem-side slur anchoring. The native
// renderer passes its already-evaluated beam-end eligibility and initial
// legacy stem selections. This helper changes nothing if Studio is off.
struct StudioSlurStemAnchorInputs4 {
    const Slur* slur;
    const ChordRest* start;
    const ChordRest* end;
    const System* startSystem;
    const System* endSystem;
    bool tablature;
    bool beamEnds;
    bool hasStartStem;
    bool hasEndStem;
};
void applyStudioSlurStemAnchors4(const StudioSlurStemAnchorInputs4&, bool& startStem, bool& endStem);

// Read-only native adapter after required note-line/stem producer work. Does
// not establish readiness itself. Uses current prepared inputs, never Slur::up,
// and never calls updateLine/computeUp or repairs raw anchors. Failure is atomic.
bool resolvePreparedSlurDirection4(const Slur*, Direction& result);
}
#endif
