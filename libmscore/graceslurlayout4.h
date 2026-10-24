// Optional Studio spacing: bounded automatic grace-slur curve fitting.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_GRACESLURLAYOUT4_H
#define MS_GRACESLURLAYOUT4_H
#include "spacinggeometry4.h"
#include <QString>
namespace Ms {
class Chord;
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
}
#endif
