// Optional Studio spacing: common-notation collision policy.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGCOLLISION4_H
#define MS_SPACINGCOLLISION4_H
#include <QtGlobal>
#include <QRectF>
#include <array>
#include <vector>

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

// Borrowed roots validated by before-group plans or complete after-group
// projections for this spacing pass.
// An empty/null context retains ordinary grace rejection. Do not retain across
// edits/layout; preparing a context does not itself move any score items.
struct SpacingGraceContext4 {
    std::vector<const Chord*> mains;
    std::vector<const Chord*> afterMains; // Validated temporary after-grace projections.
    bool contains(const Chord* chord) const;
    bool containsAfter(const Chord* chord) const;
};

// False means this context is not covered: output is left untouched.
// shapeSpatium is the first shape's staff-scaled spatium; table padding uses
// score spatium and item magnification, as in Studio. They are not interchangeable.
bool spacingPairPolicy4(const SpacingPaddingTable4&, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result, const SpacingGraceContext4* = nullptr);
// Scoped grace-group query. Both items must belong to main or its before-graces.
// Does not broaden the ordinary collision projection/planner's admission rules.
bool spacingGracePairPolicy4(const SpacingPaddingTable4&, const Chord* main,
                             const Element* first, const Element* second,
                             qreal shapeSpatium, SpacingPairPolicy4& result);
// Read-only after-grace packing scope: owner's after-graces or ordinary chord/rest target.
// includeOwner also allows source-chord components for a publication clearance check.
// Does not admit after-graces into default/system spacing queries.
bool spacingAfterGracePairPolicy4(const SpacingPaddingTable4&, const Chord* owner, const Element* target,
                                  const Element* first, const Element* second,
                                  qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner = false);
// Pure rectangular constraint; finite, normalized geometry/policy is required.
// Returns -max(qreal) when this pair imposes no horizontal constraint.
qreal spacingPairDistance4(const QRectF&, const QRectF&, const SpacingPairPolicy4&,
                           qreal shapeSpatium, qreal squeezeFactor);
bool spacingShapeDistance4(const SpacingPaddingTable4&, const SpacingShape4&, const SpacingShape4&,
                           qreal shapeSpatium, qreal squeezeFactor, qreal& result, const SpacingGraceContext4* = nullptr);
// A separate, validated view for the new placement pass. Native snapshots keep
// all rectangles; only producer-tagged redundant legacy bounds are excluded.
// Proven shared noteheads expand to all owners so pair queries retain every
// voice's policy; unknown ownership or unsupported shared types reject the view.
bool spacingCollisionProjection4(const SpacingPaddingTable4&, const SpacingShape4&,
                                 qreal shapeSpatium, SpacingShape4& result, const SpacingGraceContext4* = nullptr);
}
#endif
