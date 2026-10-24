// Optional Studio spacing: read-only ordinary grace placement.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_GRACESPACING4_H
#define MS_GRACESPACING4_H
#include <QtGlobal>
#include <QString>
#include <vector>
#include "spacinggeometry4.h"
namespace Ms {
class Chord;
class Slur;
class Segment;
struct GracePosition4 {
    const Chord* chord;
    qreal x; // Relative to the main chord, like Chord::graceNotesBefore().
};
struct GraceBeforePlan4 {
    std::vector<GracePosition4> positions; // Musical order, left to right.
    qreal left = 0; // Leftmost component in main-chord coordinates.
};
// Uses current native component shapes, excluding legacy spacing bounds and
// old grace positions. No DOM writes; failure leaves result untouched.
// Chord::layoutPitched applies accepted plans before native shape/width collection.
// The system planner admits these groups only after validating their published
// positions; unsupported grace contexts still reject the entire system plan.
bool buildGraceBeforePlan4(const Chord*, GraceBeforePlan4&, QString* reason = nullptr);

struct GraceAfterPosition4 {
    const Chord* chord;
    qreal x; // Target-segment coordinates, NOT owner-chord coordinates.
};
struct GraceAfterPlan4 {
    const Chord* owner = nullptr;
    const Segment* target = nullptr;
    std::vector<GraceAfterPosition4> positions; // Musical order, left to right.
    qreal left = 0;
    qreal right = 0; // Typed component extent in target-segment coordinates.
};
// First scope: same-measure following ordinary chord or undotted rest; no merged before-graces.
// Query only: does not move items or publish shapes. The system planner
// separately validates its temporary projections before admission.
// Rebuild after geometry changes; failure preserves every output field.
bool buildGraceAfterPlan4(const Chord*, GraceAfterPlan4&, QString* reason = nullptr);
// Read-only staging for an ordinary normal ending barline on this staff.
// Not admitted by production geometry/publication or system planning yet.
bool buildGraceAfterBarlinePlan4(const Chord*, GraceAfterPlan4&, QString* reason = nullptr);
struct GraceAfterGeometry4 {
    GraceAfterPlan4 plan;
    SpacingShape4 source; // Owner's own components, in source-segment coordinates.
    SpacingShape4 target; // Following chord/rest plus planned graces, in target-segment coordinates.
};
// Temporary typed projection for system planning. Does not update native
// segment shapes or move musical ownership. Failure leaves result unchanged.
bool buildGraceAfterGeometry4(const Chord*, GraceAfterGeometry4&, QString* reason = nullptr);

// Local post-spacing publication only. Rebuild the plan, check every source-
// to-grace constraint in the available interval, then publish all x positions.
// Failure does not move any items. Native/system widths remain unchanged.
bool applyGraceAfterPlan4(Chord*, QString* reason = nullptr);

// Only the unedited first-before-grace to its own main chord, same staff/voice.
// This validates a spacing allowance; it does not select a new slur renderer.
bool spacingLocalGraceSlur4(const Slur*, const Chord* main);
}
#endif
