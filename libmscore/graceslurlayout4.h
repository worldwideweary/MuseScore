// Optional Studio spacing: bounded automatic grace-slur curve fitting.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_GRACESLURLAYOUT4_H
#define MS_GRACESLURLAYOUT4_H
#include "spacinggeometry4.h"
#include <QString>
namespace Ms {
class Chord;
class ChordRest;
class Slur;
class Score;
class SlurSegment;
class GraceMergedSlurQuery4;
struct GraceSlurCurve4 {
    QPointF start, control1, control2, end; // System x / staff-relative y.
};
// Studio stores beamed stem length beyond the drawn center-line tip, and
// keeps grace-beam scaling separate from its intrinsic beam magnitude.
// Translate both metrics for modern pitched slur-clearance branches.
struct SlurStemMetrics4 {
    qreal length = 0;       // Includes manual stem length and source beam inset.
    qreal beamWidthSp = 0;  // Studio slurPos's separate start beam-width term.
};
// Beam-height and beam-end eligibility for optional Studio slur anchoring.
// Keeping these checks in the adapter avoids making the native renderer
// depend on Studio-only staff, tremolo, or cross-beam policy.
struct StudioSlurBeamEligibility4 {
    bool heights = false;
    bool ends = false;
};
StudioSlurBeamEligibility4 studioSlurBeamEligibility4(const Slur*, const ChordRest* start,
                                                     const ChordRest* end, bool tablature);
SlurStemMetrics4 slurStemMetrics4(const Chord*);
// Studio-only slurPos beam-end stem clearances. Native caller supplies its
// current defaults; these helpers adjust them only for eligible Studio paths.
// The start and end rules deliberately differ (beam metrics and beamlet edge).
void applyStudioSlurBeamStemInset4(const Chord*, bool eligible, bool start,
                                  qreal& yAdjustment, qreal& xOffsetSp);
void applyStudioSlurBeamStemHeight4(const Chord*, const Score*, bool eligible, bool start,
                                   qreal spatium, qreal upSign, qreal& height, qreal& y);
// Studio v4.7.5 localized collision adjustment for the validated ordinary
// combined-grace scope. The caller supplies unedited automatic anchors and
// typed physical obstacles. Endpoint balance follows the validated anchor branches.
// Success publishes a clear finite curve. Failure leaves every point intact.
bool fitGraceSlurCurve4(const SpacingShape4&, const Chord* target, qreal spatium,
                       bool up, GraceSlurCurve4&, bool startFloated = false, bool endFloated = false,
                       QString* reason = nullptr);
// Optional Studio Bezier fitting for the ordinary single-segment combined
// after-grace/main slur.  The caller supplies the pass's raw anchors, if any;
// on success this publishes an offset-adjusted absolute curve, leaving the
// legacy segment-local grip/Bezier publication inside computeBezier().
// Failure does not modify result or the segment.
bool fitStudioGraceSlurSegmentCurve4(const SlurSegment* segment, bool rawAnchors,
                                    const QPointF& rawStart, const QPointF& rawEnd,
                                    GraceMergedSlurQuery4* query, GraceSlurCurve4& result);
// Studio beam-side anchors for an automatic first-after-grace / main slur in
// the validated combined scope. Only the stem-side start branch is adapted.
// When both stems agree, adapt its forced stem-side end too; otherwise retain
// the supplied end. Query only; failure preserves both points.
// Upper starts use the user-preferred visible-tip alignment above the actual
// leading beam edge, including the rounded end-stroke radius.
bool adjustGraceMergedSlurBeamAnchors4(const Slur*, QPointF& start, QPointF& end, QString* reason = nullptr,
                                      bool allowVisualEdits = false, GraceMergedSlurQuery4* query = nullptr);
// Rendering entry point for ordinary, non-partial slurs.  The Studio style
// gate lives here, leaving native slurPos() with only the integration call.
bool applyStudioGraceMergedSlurBeamAnchors4(const Slur*, QPointF& start, QPointF& end,
                                           GraceMergedSlurQuery4* query);
}
#endif
