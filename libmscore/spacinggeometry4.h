// Optional Studio spacing: temporary item identities for native geometry.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGGEOMETRY4_H
#define MS_SPACINGGEOMETRY4_H

#include <QPointF>
#include <QRectF>
#include <vector>
#include <utility>
#include "shape.h"

namespace Ms {
class Element;
class Segment;
class Shape;
class ChordRest;

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

// Optional side output of the native shape producer. The same add operation
// emits geometry and its actual owner; native Shape keeps no DOM pointers.
// A null output performs native geometry assembly only. A non-null output is
// local to this call and requires already laid-out children (no lazy layout).
class SpacingShapeBuilder4 {
    Shape& _native;
    SpacingShape4* _items;
public:
    SpacingShapeBuilder4(Shape& native, SpacingShape4* items);
    void add(const QRectF&, const Element* owner = nullptr);
    void add(const Shape&, const SpacingShape4&, const QPointF& offset = QPointF());
    void addElement(const Element*, const QPointF& offset = QPointF());
    void addChordRest(const ChordRest*);
    void addHorizontalSpacing(Shape::HorizontalSpacingType, qreal left, qreal right,
                              const Element* owner = nullptr);
};

// Strict ordered coverage, not rectangle-to-candidate matching. Coincident
// rectangles retain their individual producers. Failure leaves result intact.
bool captureProducedSpacingShape4(const Shape&, const SpacingShape4& produced, SpacingShape4& result);

// Compatibility/diagnostic resolver only; live spacing capture uses the native
// producer's ordered side output above, never this candidate search.
// Preserve every native rectangle, in order. Ownership is resolved only where
// all matching candidates agree. Coincident legacy aggregates may retain a
// complete producer set when multiplicities match. Previously resolved sets
// survive another resolution only with identical complete sets and exact counts.
// At staff assembly only,
// allowSharedItems also retains distinct direct producers with exact counts.
// Other ambiguity falls back; shared geometry still requires projection.
SpacingShape4 spacingResolveShape4(const Shape&, const SpacingShape4& candidates, bool allowSharedItems = false);
// One read-only query only: native shapes and all positions must remain fixed.
// Borrowed item identities and snapshots must not survive an edit or layout.
class SpacingShapeCache4 {
    std::vector<std::pair<const Element*, SpacingShape4>> _entries;
public:
    const SpacingShape4* find(const Element* item) const {
        for (const auto& entry : _entries) if (entry.first == item) return &entry.second;
        return nullptr;
    }
    void remember(const Element* item, const SpacingShape4& shape) { _entries.push_back({item, shape}); }
};
SpacingShape4 spacingElementShape4(const Element*, SpacingShapeCache4* = nullptr);
SpacingShape4 spacingStaffShape4(const Segment*, int staffIdx);
}
#endif
