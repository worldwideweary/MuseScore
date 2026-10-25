// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGPLAN4_H
#define MS_SPACINGPLAN4_H
#include <QtGlobal>
#include <QString>
#include <vector>
#include "gracespacing4.h"

namespace Ms {
class Measure;
class Segment;
class System;
class Beam;
class Chord;
class SpacingInput4;
class SpacingAttachmentPreparation4;

// Read-only preparation for cross-staff placement. This first context covers
// one automatic eighth-note beam of up to two unmirrored normal notes per chord
// between adjacent ordinary staves.
// Placement applies its own additional companion/system eligibility checks.
struct CrossStaffChord4 {
    const Chord* chord = nullptr;
    int visualStaff = 0;
    bool up = false; // inferred staff side; never writes Chord::up()
    qreal stemX = 0; // stem center in segment coordinates, independent of its provisional side
};
struct CrossStaffPair4 {
    const Chord* first = nullptr;
    const Chord* second = nullptr;
    bool adjacentSegments = false; // beam neighbors may have intervening voice onsets
    bool canAdjust = false;
    qreal widthOffset = 0; // fixed signed displacement, outside rhythmic springs
    qreal minimumWidth = 0;
    qreal minimumStemDistance = 0; // explicit up-to-down stem-center constraint
};
struct CrossStaffContext4 {
    const Beam* beam = nullptr;
    std::vector<CrossStaffChord4> chords;
    std::vector<CrossStaffPair4> pairs;
};
// Requires prepared layout geometry. Unsupported input leaves output untouched.
bool buildCrossStaffContext4(const Beam*, CrossStaffContext4&, QString* reason = nullptr);

struct SpacingSegmentPlan4 {
    Segment* segment;
    qreal x = 0; // system coordinates, not Segment::x()
    qreal width = 0;
    qreal widthOffset = 0; // fixed signed width, excluded from spring growth
    qreal stretch = 0;
    bool ignored = false;
    explicit SpacingSegmentPlan4(Segment* s) : segment(s) {}
};
struct SpacingMeasurePlan4 {
    Measure* measure;
    qreal x = 0;
    qreal width = 0;
    explicit SpacingMeasurePlan4(Measure* m) : measure(m) {}
};
struct SpacingPlan4 {
    std::vector<SpacingSegmentPlan4> segments;
    std::vector<SpacingMeasurePlan4> measures;
    std::vector<GraceAfterPlan4> afterGraces; // Target-relative; retained through justification/squeezing.
    std::vector<GraceMergedPlan4> mergedGraces; // Per-member ownership retained through fitting.
    qreal width = 0; // right system edge, including left margin
};
struct SpacingPlanOptions4 {
    qreal stretchReduction = 1;
    qreal squeezeFactor = 1;
    bool minimumMeasureWidths = true;
};

// Inputs must already have current shapes and segment ticks. This WIP returns
// a plan only: no DOM positions, widths, styles or durations are changed.
// All pointers are borrowed for this pass; discard the plan after edits/layout.
// Unsupported contexts leave result untouched; reason explains the boundary.
// Optional merged inputs must be complete successful geometry queries from the
// current unchanged DOM. Discard them before any geometry preparation/write;
// they are a handoff from beam preparation, never a cache across layout calls.
// Optional musical input is caller-owned and shared only within an unchanged
// preparation phase. This function never collects a score-wide view itself.
bool buildSpacingPlan4(System*, SpacingPlan4& result, QString* reason = nullptr,
                       const SpacingPlanOptions4& options = SpacingPlanOptions4(),
                       const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr,
                       const SpacingInput4* input = nullptr);
// Same existing policy, borrowing one shared attachment preparation. No input
// collection or direction resolution occurs here or in subsequent squeeze passes.
// This overload does not yet enable the general attachment admission policy.
bool buildSpacingPlan4(System*, SpacingPlan4& result, const SpacingAttachmentPreparation4&,
                       QString* reason = nullptr,
                       const SpacingPlanOptions4& options = SpacingPlanOptions4(),
                       const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);
bool justifySpacingPlan4(SpacingPlan4&, qreal targetWidth);

// Compare a fresh plan with the active horizontal fields published by system layout.
// Query only; no preparation or DOM writes. Callers must establish that the
// existing component geometry is current before building the supplied plan.
bool spacingPlanMatches4(const SpacingPlan4&, QString* reason = nullptr);

// Try natural width, then Studio's first three positive-duration squeeze
// passes. Publish only a fitting whole-system plan; stronger compression and
// unsupported contexts leave result untouched for the existing fallback.
bool squeezeSpacingPlan4(System*, qreal targetWidth, SpacingPlan4& result,
                         QString* reason = nullptr,
                         const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);
bool squeezeSpacingPlan4(System*, qreal targetWidth, SpacingPlan4& result,
                         const SpacingAttachmentPreparation4&, QString* reason = nullptr,
                         const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);
}
#endif
