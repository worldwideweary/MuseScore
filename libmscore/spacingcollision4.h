// Optional Studio spacing: common-notation collision policy.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGCOLLISION4_H
#define MS_SPACINGCOLLISION4_H
#include <QtGlobal>
#include <QRectF>
#include <array>

namespace Ms {
class Element;
class Chord;
class Score;
struct SpacingShape4;
enum class ElementType : char;

enum class SpacingKerning4 { NON_KERNING, KERNING, ALLOW_COLLISION, LEFT_EDGE, CENTER, RIGHT_EDGE };
struct SpacingPairPolicy4 {
    qreal padding = 0;
    qreal verticalClearance = 0;
    SpacingKerning4 kerning = SpacingKerning4::NON_KERNING;
};

// Snapshot common-type padding once per pass; rebuild after style changes.
class SpacingPaddingTable4 {
    const Score* _score;
    std::array<std::array<qreal, 12>, 12> _padding;
public:
    explicit SpacingPaddingTable4(const Score*);
    const Score* score() const { return _score; }
    bool padding(ElementType first, ElementType second, qreal& result) const;
};

// False means this context is not covered: output is left untouched.
// shapeSpatium is the first shape's staff-scaled spatium; table padding uses
// score spatium and item magnification, as in Studio. They are not interchangeable.
bool spacingPairPolicy4(const SpacingPaddingTable4&, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result);
// Scoped grace-group query. Both items must belong to main or its before-graces.
// Does not broaden the ordinary collision projection/planner's admission rules.
bool spacingGracePairPolicy4(const SpacingPaddingTable4&, const Chord* main,
                             const Element* first, const Element* second,
                             qreal shapeSpatium, SpacingPairPolicy4& result);
// Pure rectangular constraint; finite, normalized geometry/policy is required.
// Returns -max(qreal) when this pair imposes no horizontal constraint.
qreal spacingPairDistance4(const QRectF&, const QRectF&, const SpacingPairPolicy4&,
                           qreal shapeSpatium, qreal squeezeFactor);
bool spacingShapeDistance4(const SpacingPaddingTable4&, const SpacingShape4&, const SpacingShape4&,
                           qreal shapeSpatium, qreal squeezeFactor, qreal& result);
// A separate, validated view for the new placement pass. Native snapshots keep
// all rectangles; only producer-tagged redundant legacy bounds are excluded.
// Proven shared noteheads expand to all owners so pair queries retain every
// voice's policy; unknown ownership or unsupported shared types reject the view.
bool spacingCollisionProjection4(const SpacingPaddingTable4&, const SpacingShape4&,
                                 qreal shapeSpatium, SpacingShape4& result);
}
#endif
