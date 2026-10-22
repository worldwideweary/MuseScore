// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGPLAN4_H
#define MS_SPACINGPLAN4_H
#include <QtGlobal>
#include <QString>
#include <functional>
#include <vector>
#include "fraction.h"
#include "gracespacing4.h"

namespace Ms {
class Measure;
class MeasureBase;
class Segment;
class System;
class Beam;
class Chord;
class SpacingInput4;
class SpacingAttachmentPreparation4;
class Score;

// Value-only evidence of the path actually selected by native system layout.
// NATIVE_WIDTHS with the style ON is a mixed fallback, not untouched legacy.
enum class SpacingPass4 { CANDIDATE, FINAL };
enum class SpacingPath4 { STYLE_OFF, FULL_PLAN, NATIVE_WIDTHS, POOLED_WIDTHS, NO_STRETCH };
struct SpacingDecision4 {
    SpacingPass4 pass = SpacingPass4::CANDIDATE;
    SpacingPath4 path = SpacingPath4::STYLE_OFF;
    Fraction firstTick = Fraction(0, 1), endTick = Fraction(0, 1);
    int measures = 0;
    qreal width = 0, available = 0;
    QString reason;
};
// Query/test observer, scoped to one score and the current thread. Callbacks
// consume values only: they must not mutate the score or initiate layout.
// No score, system or geometry pointers are retained in an event.
class SpacingDecisionScope4 {
    const Score* _score;
    std::function<void(const SpacingDecision4&)> _handler;
    SpacingDecisionScope4* _previous;
    friend bool spacingDecisionObserved4(const Score*);
    friend void recordSpacingDecision4(const System*, SpacingPass4, SpacingPath4, qreal, qreal, const QString&);
public:
    SpacingDecisionScope4(const Score*, std::function<void(const SpacingDecision4&)>);
    ~SpacingDecisionScope4();
    SpacingDecisionScope4(const SpacingDecisionScope4&) = delete;
    SpacingDecisionScope4& operator=(const SpacingDecisionScope4&) = delete;
};
bool spacingDecisionObserved4(const Score*);
void recordSpacingDecision4(const System*, SpacingPass4, SpacingPath4, qreal width, qreal available,
                            const QString& reason = QString());

// Certify native repeat-barline preparation for the explicit collected prefix.
// CANDIDATE is queried after appending a trial last measure, before the native
// predecessor is finalized; FINAL is queried after retained-system regeneration.
// No geometry, ownership or enabled state is changed or retained by this query.
// Ordinary nonrepeat prefixes keep their existing preparation contract.
bool spacingRepeatBarlinesReady4(const System*, SpacingPass4, QString* reason = nullptr);

// Certify ordinary interior key/time changes and terminal courtesy producers.
// Pending penultimate courtesies retain fallback until incremental spacing is
// adapted. The query never prepares or changes native header/trailer geometry.
bool spacingCandidateSignaturesReady4(const System*, QString* reason = nullptr);

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
    SpacingSlurPolicy4 slurPolicy = SpacingSlurPolicy4::COMPATIBILITY;
};

// One read-only selection contract for a collected prefix or retained system.
// Candidates return natural width even when overfull. Final requests require a
// fitting plan, then apply the shared terminal/section justification policy.
struct SpacingSystemRequest4 {
    SpacingPass4 pass = SpacingPass4::CANDIDATE;
    qreal availableWidth = 0;
    const MeasureBase* next = nullptr;
    qreal fillLimit = 0;
    SpacingSlurPolicy4 slurPolicy = SpacingSlurPolicy4::PREPARED;
};
bool spacingSystemJustified4(const Measure* last, const MeasureBase* next,
                             qreal naturalWidth, qreal availableWidth, qreal fillLimit);
bool selectSystemSpacingPlan4(System*, const SpacingSystemRequest4&,
                              const SpacingAttachmentPreparation4&, SpacingPlan4& result,
                              QString* reason = nullptr,
                              const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);

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
// Borrow one attachment preparation for the explicitly requested policy. No
// collection or native producer writes occur in planning or squeeze passes.
bool buildSpacingPlan4(System*, SpacingPlan4& result, const SpacingAttachmentPreparation4&,
                       QString* reason = nullptr,
                       const SpacingPlanOptions4& options = SpacingPlanOptions4(),
                       const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);
bool justifySpacingPlan4(SpacingPlan4&, qreal targetWidth);

// Compare a fresh plan with the active horizontal fields published by system layout.
// Query only; no preparation or DOM writes. Callers must establish that the
// existing component geometry is current before building the supplied plan.
bool spacingPlanMatches4(const SpacingPlan4&, QString* reason = nullptr);

// Preflight the entire borrowed write domain, including late grace writes.
// No query owner/geometry cache is retained and no native field changes on
// rejection. Call only within the current unchanged native publication phase.
bool spacingPlanPublicationValid4(const System*, const SpacingPlan4&, QString* reason = nullptr);
// Validate everything first, then write the segment fields only. Measure/late
// grace finalization keeps its native order in collectSystem. Failure is atomic.
bool publishSpacingPlanSegments4(System*, const SpacingPlan4&, QString* reason = nullptr);

// Try natural width, then Studio's first three positive-duration squeeze
// passes. Publish only a fitting whole-system plan; stronger compression and
// unsupported contexts leave result untouched for the existing fallback.
bool squeezeSpacingPlan4(System*, qreal targetWidth, SpacingPlan4& result,
                         QString* reason = nullptr,
                         const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr);
bool squeezeSpacingPlan4(System*, qreal targetWidth, SpacingPlan4& result,
                         const SpacingAttachmentPreparation4&, QString* reason = nullptr,
                         const std::vector<GraceMergedGeometry4>* preparedMerged = nullptr,
                         SpacingSlurPolicy4 policy = SpacingSlurPolicy4::COMPATIBILITY);
}
#endif
