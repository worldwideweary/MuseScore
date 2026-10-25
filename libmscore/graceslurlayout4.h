// Optional Studio spacing: bounded automatic grace-slur curve fitting.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_GRACESLURLAYOUT4_H
#define MS_GRACESLURLAYOUT4_H
#include "spacinggeometry4.h"
#include <QString>
namespace Ms {
class Chord;
class Slur;
class GraceMergedSlurQuery4;
struct GraceSlurCurve4 {
    QPointF start, control1, control2, end; // System x / staff-relative y.
};
// Studio v4.7.5 localized collision adjustment for the validated ordinary
// combined-grace scope. The caller supplies unedited automatic anchors and
// typed physical obstacles. Endpoint balance follows the validated anchor branches.
// Success publishes a clear finite curve. Failure leaves every point intact.
bool fitGraceSlurCurve4(const SpacingShape4&, const Chord* target, qreal spatium,
                       bool up, GraceSlurCurve4&, bool startFloated = false, bool endFloated = false,
                       QString* reason = nullptr);
// Studio beam-side anchors for an automatic first-after-grace / main slur in
// the validated combined scope. Only the stem-side start branch is adapted.
// When both stems agree, adapt its forced stem-side end too; otherwise retain
// the supplied end. Query only; failure preserves both points.
// Upper starts use the user-preferred visible-tip alignment above the actual
// leading beam edge, including the rounded end-stroke radius.
bool adjustGraceMergedSlurBeamAnchors4(const Slur*, QPointF& start, QPointF& end, QString* reason = nullptr,
                                      bool allowVisualEdits = false, GraceMergedSlurQuery4* query = nullptr);
}
#endif
