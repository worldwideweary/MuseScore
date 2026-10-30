// Optional Studio spacing: temporary item identities for native geometry.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacinggeometry4.h"
#include "spacingprofile4.h"
#include "accidental.h"
#include "arpeggio.h"
#include "barline.h"
#include "chord.h"
#include "hook.h"
#include "ledgerline.h"
#include "measure.h"
#include "note.h"
#include "notedot.h"
#include "rest.h"
#include "score.h"
#include "segment.h"
#include "shape.h"
#include "staff.h"
#include "stem.h"
#include "stemslash.h"
#include <algorithm>
#include <functional>

namespace Ms {

bool SpacingShape4::complete() const
{
    for (const SpacingShapeElement4& e : elements) {
        if (e.identity != SpacingIdentity4::ITEM || !e.item || !e.aggregateOwners.empty() || !e.sharedItems.empty())
            return false;
    }
    return true;
}

SpacingShape4 SpacingShape4::translated(const QPointF& offset) const
{
    SpacingShape4 result = *this;
    for (SpacingShapeElement4& e : result.elements)
        e.rect.translate(offset);
    return result;
}

static bool sameRect4(const QRectF& a, const QRectF& b)
{
    // QRectF's operator== can use fuzzy comparisons on some Qt versions.
    return a.x() == b.x() && a.y() == b.y() && a.width() == b.width() && a.height() == b.height();
}

static Shape producedElementShape4(const Element* item, SpacingShape4& items)
{
    if (item->isNote()) return toNote(item)->shapeWithItems4(&items);
    if (item->isChord()) return toChord(item)->shapeWithItems4(&items);
    if (item->isRest()) return toRest(item)->shapeWithItems4(&items);
    const Shape native = item->shape();
    const Element* owner = nullptr;
    switch (item->type()) {
    case ElementType::STEM:
    case ElementType::HOOK:
    case ElementType::STEM_SLASH:
    case ElementType::LEDGER_LINE:
    case ElementType::ACCIDENTAL:
    case ElementType::NOTEDOT:
    case ElementType::CLEF:
    case ElementType::KEYSIG:
    case ElementType::TIMESIG:
    case ElementType::BAR_LINE:
    case ElementType::ARPEGGIO:
    case ElementType::BREATH:
        owner = item;
        break;
    default:
        break; // Unknown composites must not masquerade as one leaf.
    }
    items.elements.clear();
    for (const QRectF& rect : native)
        items.elements.push_back(SpacingShapeElement4(rect, owner));
    return native;
}

SpacingShapeBuilder4::SpacingShapeBuilder4(Shape& native, SpacingShape4* items)
    : _native(native), _items(items)
{
    if (_items) _items->elements.clear();
}

void SpacingShapeBuilder4::add(const QRectF& rect, const Element* owner)
{
#ifndef NDEBUG
    _native.add(rect, owner ? owner->name() : nullptr);
#else
    _native.add(rect);
#endif
    if (_items) _items->elements.push_back(SpacingShapeElement4(rect, owner));
}

void SpacingShapeBuilder4::add(const Shape& native, const SpacingShape4& items, const QPointF& offset)
{
    _native.add(native.translated(offset));
    if (_items) {
        const SpacingShape4 translated = items.translated(offset);
        _items->elements.insert(_items->elements.end(), translated.elements.begin(), translated.elements.end());
    }
}

void SpacingShapeBuilder4::addElement(const Element* item, const QPointF& offset)
{
    if (!item) return;
    if (!_items) {
        _native.add(item->shape().translated(offset));
        return;
    }
    SpacingShape4 items;
    const Shape native = producedElementShape4(item, items);
    add(native, items, offset);
}

void SpacingShapeBuilder4::addChordRest(const ChordRest* item)
{
    if (!_items) {
        _native.add(item->ChordRest::shape());
        return;
    }
    SpacingShape4 items;
    _native.add(item->ChordRest::shapeWithItems4(&items));
    _items->elements.insert(_items->elements.end(), items.elements.begin(), items.elements.end());
}

void SpacingShapeBuilder4::addHorizontalSpacing(Shape::HorizontalSpacingType type, qreal left, qreal right,
                                              const Element* owner)
{
    _native.addHorizontalSpacing(type, left, right);
    if (_items) {
        SpacingShapeElement4 element(_native.back(), owner);
        // Known general bounds and empty barline lyric bounds have the
        // legacy-spacer policy. Actual lyric/harmony aggregates stay opaque.
        if (owner && (type == Shape::SPACING_GENERAL || (owner->isBarLine() && type == Shape::SPACING_LYRICS)))
            element.identity = SpacingIdentity4::LEGACY_SPACER;
        else { element.item = nullptr; element.identity = SpacingIdentity4::UNCLASSIFIED; }
        _items->elements.push_back(element);
    }
}

bool captureProducedSpacingShape4(const Shape& native, const SpacingShape4& produced, SpacingShape4& result)
{
    if (native.size() != produced.elements.size()) return false;
    for (size_t i = 0; i < native.size(); ++i) {
        const auto& e = produced.elements[i];
        if (!sameRect4(native[i], e.rect) || !e.item || !e.aggregateOwners.empty() || !e.sharedItems.empty()
            || (e.identity != SpacingIdentity4::ITEM && e.identity != SpacingIdentity4::LEGACY_SPACER)
            || (e.identity == SpacingIdentity4::LEGACY_SPACER && !e.item->isChord() && !e.item->isBarLine())) return false;
    }
    result = produced;
    return true;
}

SpacingShape4 spacingResolveShape4(const Shape& native, const SpacingShape4& candidates, bool allowSharedItems)
{
    SpacingShape4 result;
    result.elements.reserve(native.size());
    for (const QRectF& rect : native) {
        SpacingShapeElement4 resolved(rect);
        bool found = false;
        bool unknown = false;
        bool ambiguous = false;
        bool allAggregates = true;
        bool allResolvedAggregates = true;
        std::vector<const Element*> resolvedAggregateOwners;
        bool allItems = true;
        std::vector<const Element*> sharedItems;
        std::vector<const Element*> aggregateOwners;
        size_t matches = 0;
        const Element* owner = nullptr;
        SpacingIdentity4 identity = SpacingIdentity4::UNCLASSIFIED;
        for (const SpacingShapeElement4& candidate : candidates.elements) {
            // Exact native geometry only. Do not infer identity by proximity,
            // shape bounds, debug names or candidate order.
            if (!sameRect4(candidate.rect, rect))
                continue;
            found = true;
            ++matches;
            // A child chord can already have resolved coincident spacing
            // bounds. Preserve that complete set through staff assembly only
            // when every matching record agrees and multiplicities still fit.
            bool resolvedAggregate = candidate.identity == SpacingIdentity4::LEGACY_SPACER
                && !candidate.item && candidate.sharedItems.empty() && candidate.aggregateOwners.size() > 1;
            if (resolvedAggregate) {
                auto owners = candidate.aggregateOwners;
                std::sort(owners.begin(), owners.end(), std::less<const Element*>());
                resolvedAggregate = std::find(owners.begin(), owners.end(), nullptr) == owners.end()
                    && std::adjacent_find(owners.begin(), owners.end()) == owners.end();
                if (resolvedAggregateOwners.empty()) resolvedAggregateOwners = owners;
                else resolvedAggregate = resolvedAggregate && resolvedAggregateOwners == owners;
            }
            allResolvedAggregates = allResolvedAggregates && resolvedAggregate;
            allAggregates = allAggregates && candidate.identity == SpacingIdentity4::LEGACY_SPACER
                && candidate.item && candidate.aggregateOwners.empty() && candidate.sharedItems.empty();
            allItems = allItems && candidate.identity == SpacingIdentity4::ITEM
                && candidate.item && candidate.aggregateOwners.empty() && candidate.sharedItems.empty();
            if (candidate.identity == SpacingIdentity4::ITEM && candidate.item
                && std::find(sharedItems.begin(), sharedItems.end(), candidate.item) == sharedItems.end())
                sharedItems.push_back(candidate.item);
            if (candidate.identity == SpacingIdentity4::LEGACY_SPACER && candidate.item
                && std::find(aggregateOwners.begin(), aggregateOwners.end(), candidate.item) == aggregateOwners.end())
                aggregateOwners.push_back(candidate.item);
            if (candidate.identity == SpacingIdentity4::AMBIGUOUS)
                ambiguous = true;
            else if ((candidate.identity != SpacingIdentity4::ITEM && candidate.identity != SpacingIdentity4::LEGACY_SPACER)
                     || !candidate.item || !candidate.aggregateOwners.empty() || !candidate.sharedItems.empty())
                unknown = true;
            else if (owner && (owner != candidate.item || identity != candidate.identity))
                ambiguous = true;
            else {
                owner = candidate.item;
                identity = candidate.identity;
            }
        }
        // A single known primitive cannot account for several coincident
        // native primitives whose other owners have not been identified.
        size_t nativeMatches = 0;
        for (const QRectF& other : native) {
            if (sameRect4(other, rect))
                ++nativeMatches;
        }
        unknown = unknown || matches < nativeMatches;
        if (allAggregates && !unknown && matches == nativeMatches && aggregateOwners.size() > 1) {
            // Multi-voice Chord::shape bounds can coincide exactly. Preserve
            // the complete producer set; projection must validate every owner.
            std::sort(aggregateOwners.begin(), aggregateOwners.end(), std::less<const Element*>());
            resolved.identity = SpacingIdentity4::LEGACY_SPACER;
            resolved.aggregateOwners = aggregateOwners;
        }
        else if (allResolvedAggregates && matches == nativeMatches
                 && matches == resolvedAggregateOwners.size() && matches > 1) {
            resolved.identity = SpacingIdentity4::LEGACY_SPACER;
            resolved.aggregateOwners = resolvedAggregateOwners;
        }
        else if (allowSharedItems && allItems && !unknown && matches == nativeMatches
                 && sharedItems.size() == matches && matches > 1) {
            std::sort(sharedItems.begin(), sharedItems.end(), std::less<const Element*>());
            resolved.identity = SpacingIdentity4::SHARED_ITEM;
            resolved.sharedItems = sharedItems;
        }
        else if (ambiguous)
            resolved.identity = SpacingIdentity4::AMBIGUOUS;
        else if (found && !unknown && owner) {
            resolved.item = owner;
            resolved.identity = identity;
        }
        result.elements.push_back(resolved);
    }
    return result;
}

static SpacingShape4 unclassifiedShape4(const Shape& native)
{
    SpacingShape4 result;
    result.elements.reserve(native.size());
    for (const QRectF& rect : native) result.elements.push_back(SpacingShapeElement4(rect));
    return result;
}

SpacingShape4 spacingElementShape4(const Element* item, SpacingShapeCache4* cache)
{
    if (!item) return {};
    if (cache) {
        if (const SpacingShape4* cached = cache->find(item)) {
            SpacingProfileScope4 reuse(SpacingProfileStage4::SHAPE_REUSE);
            return *cached;
        }
    }
    SpacingProfileScope4 profile(SpacingProfileStage4::ELEMENT_SHAPE);
    SpacingShape4 produced, result;
    const Shape native = producedElementShape4(item, produced);
    if (!captureProducedSpacingShape4(native, produced, result)) result = unclassifiedShape4(native);
    if (cache) cache->remember(item, result);
    return result;
}

SpacingShape4 spacingStaffShape4(const Segment* segment, int staffIdx)
{
    if (!segment || staffIdx < 0 || static_cast<size_t>(staffIdx) >= segment->shapes().size()) return {};
    const Shape& native = segment->staffShape(staffIdx);
    SpacingShape4 produced, result;
    const Shape emitted = segment->shapeWithItems4(staffIdx, &produced);
    // First prove that the side output covers its own emitted geometry; then
    // prove it is the currently published native staff shape. Never repair a
    // stale, reordered or incomplete snapshot by searching coincident owners.
    if (!captureProducedSpacingShape4(emitted, produced, result)
        || !captureProducedSpacingShape4(native, result, result)) return unclassifiedShape4(native);
    return result;
}
}
