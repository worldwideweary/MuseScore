// Optional Studio spacing: read-only ordinary before-grace placement.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_GRACESPACING4_H
#define MS_GRACESPACING4_H
#include <QtGlobal>
#include <QString>
#include <vector>
namespace Ms {
class Chord;
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
// The full Studio system planner still rejects grace-containing systems.
bool buildGraceBeforePlan4(const Chord*, GraceBeforePlan4&, QString* reason = nullptr);
}
#endif
