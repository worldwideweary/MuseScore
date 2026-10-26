// Studio slur direction shared by prepared spacing and native rendering.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SLURDIRECTION4_H
#define MS_SLURDIRECTION4_H
#include "types.h"

namespace Ms {
class Slur;
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
}
#endif
