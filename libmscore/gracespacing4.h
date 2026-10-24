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
// Same-measure ordinary chord, undotted rest or normal ending barline; no merged before-graces.
// Query only: does not move items or publish shapes. The system planner
// separately validates its temporary projections before admission.
// Rebuild after geometry changes; failure preserves every output field.
bool buildGraceAfterPlan4(const Chord*, GraceAfterPlan4&, QString* reason = nullptr);
// Read-only target-specific query for an ordinary normal ending barline.
bool buildGraceAfterBarlinePlan4(const Chord*, GraceAfterPlan4&, QString* reason = nullptr);
struct GraceMergedPlan4 {
    const Chord* owner = nullptr; // Owns the incoming after-graces.
    const Chord* target = nullptr; // Retains ownership of its before-graces.
    std::vector<GraceAfterPosition4> positions; // After then before, in TARGET SEGMENT coordinates.
    qreal left = 0;
    qreal right = 0;
};
// Read-only same-direction joint packing for temporary system projection.
// Every member keeps its parent; failure leaves the entire output untouched.
bool buildGraceMergedPlan4(const Chord*, GraceMergedPlan4&, QString* reason = nullptr);
struct GraceMergedGeometry4 {
    GraceMergedPlan4 plan;
    SpacingShape4 source; // Incoming owner's own components in its segment.
    SpacingShape4 target; // Target's own components plus all planned graces in target segment.
};
// Read-only joint projection for system planning.
bool buildGraceMergedGeometry4(const Chord*, GraceMergedGeometry4&, QString* reason = nullptr);
// Preparation before system spacing. Updates derived beam/
// stem geometry at planned intra-group distances, restoring every member x.
// Failure after preparation may leave derived geometry needing native relayout.
// Optional output is the final validated geometry, published only on success.
bool prepareGraceMergedBeams4(Chord*, QString* reason = nullptr, GraceMergedGeometry4* prepared = nullptr);
struct GraceAfterGeometry4 {
    GraceAfterPlan4 plan;
    SpacingShape4 source; // Owner's own components, in source-segment coordinates.
    SpacingShape4 target; // Following chord/rest/barline plus planned graces, in target-segment coordinates.
};
// Temporary typed projection for system planning. Does not update native
// segment shapes or move musical ownership. Failure leaves result unchanged.
bool buildGraceAfterGeometry4(const Chord*, GraceAfterGeometry4&, QString* reason = nullptr);

// Local post-spacing publication only. Rebuild the plan, check every source-
// to-grace constraint in the available interval, then publish all x positions.
// Failure does not move any items. Native/system widths remain unchanged.
bool applyGraceAfterPlan4(Chord*, QString* reason = nullptr);

// Automatic slur within a validated same-direction combined group: first
// incoming after-grace (or the existing local before anchor) to target main.
bool spacingMergedGraceSlur4(const Slur*, const Chord* owner, const Chord* target, bool allowVisualEdits = false);

// Read-only physical obstacles for first-after-grace to target-main slurs in
// the validated combined scope. Uses CURRENT published positions, not planned
// grace offsets. Coordinates match Slur::slurPos (system x, staff-relative y).
// Excludes the start anchor, ledgers and synthetic spacing bounds; includes
// other grace members and the target's own components. No clearance inflation
// or curve adjustment yet. Failure leaves result unchanged.
// allowVisualEdits is for computing an unedited base before applying user offsets.
bool buildGraceMergedSlurObstacles4(const Slur*, SpacingShape4&, QString* reason = nullptr, bool allowVisualEdits = false);

// First-before-grace to its own main chord, same staff/voice.
// Visual edits may be admitted explicitly for horizontal spacing or computing
// an automatic curve base; anchors, ownership and slur-count guards still apply.
// This validates a spacing allowance; it does not select a new slur renderer.
bool spacingLocalGraceSlur4(const Slur*, const Chord* main, bool allowVisualEdits = false);
}
#endif
