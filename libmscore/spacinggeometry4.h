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

enum class SpacingIdentity4 { ITEM, UNCLASSIFIED, AMBIGUOUS, LEGACY_SPACER, SHARED_ITEM };
// LEGACY_SPACER records a known producer's aggregate bound. It is retained in
// snapshots and excluded only by the separately validated collision projection.

struct SpacingShapeElement4 {
    QRectF rect;
    const Element* item;
    SpacingIdentity4 identity;
    // For coincident known aggregates only: every native producer must be
    // accounted for exactly. item stays null rather than choosing one owner.
    std::vector<const Element*> aggregateOwners;
    // Proven coincident direct producers; projection validates every item and
    // expands the rectangle for all pair policies. Never select one voice.
    std::vector<const Element*> sharedItems;
    SpacingShapeElement4(const QRectF& r, const Element* owner = nullptr)
        : rect(r), item(owner), identity(owner ? SpacingIdentity4::ITEM : SpacingIdentity4::UNCLASSIFIED) {}
};

// Coordinates remain local to the element or segment, as in its native shape.
// Pointers are borrowed for this pass only; never retain across edits/layouts.
struct SpacingShape4 {
    std::vector<SpacingShapeElement4> elements;
    // Direct item geometry only. Known legacy aggregates still need projection;
    // true does not claim that every item's collision policy is ported.
    bool complete() const;
    SpacingShape4 translated(const QPointF&) const;
};

// Preserve every native rectangle, in order. Ownership is resolved only where
// all matching candidates agree. Coincident legacy aggregates may retain a
// complete producer set when multiplicities match. At staff assembly only,
// allowSharedItems also retains distinct direct producers with exact counts.
// Other ambiguity falls back; shared geometry still requires projection.
SpacingShape4 spacingResolveShape4(const Shape&, const SpacingShape4& candidates, bool allowSharedItems = false);
SpacingShape4 spacingElementShape4(const Element*);
SpacingShape4 spacingStaffShape4(const Segment*, int staffIdx);
}
#endif
