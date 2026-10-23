// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGPLAN4_H
#define MS_SPACINGPLAN4_H
#include <QtGlobal>
#include <QString>
#include <vector>

namespace Ms {
class Measure;
class Segment;
class System;
class Beam;
class Chord;

// Read-only preparation for cross-staff placement. This first context covers
// one automatic, single-note eighth-note beam between adjacent ordinary staves.
// It does not authorize the placement planner to accept cross-staff geometry.
struct CrossStaffChord4 {
    const Chord* chord = nullptr;
    int visualStaff = 0;
    bool up = false; // inferred staff side; never writes Chord::up()
};
struct CrossStaffPair4 {
    const Chord* first = nullptr;
    const Chord* second = nullptr;
    bool canAdjust = false;
    qreal widthOffset = 0; // fixed signed displacement, outside rhythmic springs
    qreal minimumWidth = 0;
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
bool buildSpacingPlan4(System*, SpacingPlan4& result, QString* reason = nullptr,
                       const SpacingPlanOptions4& options = SpacingPlanOptions4());
bool justifySpacingPlan4(SpacingPlan4&, qreal targetWidth);

// Try natural width, then Studio's first three positive-duration squeeze
// passes. Publish only a fitting whole-system plan; stronger compression and
// unsupported contexts leave result untouched for the existing fallback.
bool squeezeSpacingPlan4(System*, qreal targetWidth, SpacingPlan4& result,
                         QString* reason = nullptr);
}
#endif
