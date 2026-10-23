// Optional Studio spacing: temporary item identities for native geometry.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGGEOMETRY4_H
#define MS_SPACINGGEOMETRY4_H

#include <QPointF>
#include <QRectF>
#include <vector>

namespace Ms {
class Element;
class Segment;
class Shape;

enum class SpacingIdentity4 { ITEM, UNCLASSIFIED, AMBIGUOUS };

struct SpacingShapeElement4 {
    QRectF rect;
    const Element* item;
    SpacingIdentity4 identity;
    SpacingShapeElement4(const QRectF& r, const Element* owner = nullptr)
        : rect(r), item(owner), identity(owner ? SpacingIdentity4::ITEM : SpacingIdentity4::UNCLASSIFIED) {}
};

// Coordinates remain local to the element or segment, as in its native shape.
// Pointers are borrowed for this pass only; never retain across edits/layouts.
struct SpacingShape4 {
    std::vector<SpacingShapeElement4> elements;
    // Identity coverage only, not a claim that every item's policy is ported.
    bool complete() const;
    SpacingShape4 translated(const QPointF&) const;
};

// Preserve every native rectangle, in order. Ownership is resolved only where
// all matching candidates agree. Unknown/ambiguous rectangles require fallback.
SpacingShape4 spacingResolveShape4(const Shape&, const SpacingShape4& candidates);
SpacingShape4 spacingElementShape4(const Element*);
SpacingShape4 spacingStaffShape4(const Segment*, int staffIdx);
}
#endif
