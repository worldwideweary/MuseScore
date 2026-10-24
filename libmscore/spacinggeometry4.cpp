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

static void appendShape4(SpacingShape4& target, const SpacingShape4& source, const QPointF& offset)
{
    const SpacingShape4 translated = source.translated(offset);
    target.elements.insert(target.elements.end(), translated.elements.begin(), translated.elements.end());
}

static void appendLeaf4(SpacingShape4& target, const Element* item)
{
    if (!item)
        return;
    const Shape native = item->shape();
    for (const QRectF& rect : native)
        target.elements.push_back(SpacingShapeElement4(rect.translated(item->pos()), item));
}

static void appendLegacySpacer4(SpacingShape4& target, const Element* owner,
                                Shape::HorizontalSpacingType type, qreal left, qreal right)
{
    Shape native;
    native.addHorizontalSpacing(type, left, right);
    for (const QRectF& rect : native) {
        SpacingShapeElement4 element(rect, owner);
        element.identity = SpacingIdentity4::LEGACY_SPACER;
        target.elements.push_back(element);
    }
}

SpacingShape4 spacingElementShape4(const Element* item, SpacingShapeCache4* cache)
{
    if (cache) {
        if (const SpacingShape4* cached = cache->find(item)) {
            SpacingProfileScope4 reuse(SpacingProfileStage4::SHAPE_REUSE);
            return *cached;
        }
    }
    SpacingProfileScope4 profile(SpacingProfileStage4::ELEMENT_SHAPE);
    if (!item)
        return {};
    const Shape native = item->shape();
    SpacingShape4 candidates;
    if (item->isNote()) {
        const Note* note = toNote(item);
        candidates.elements.push_back(SpacingShapeElement4(note->bbox(), note));
        for (const NoteDot* dot : note->dots())
            candidates.elements.push_back(SpacingShapeElement4(
                note->symBbox(SymId::augmentationDot).translated(dot->pos()), dot));
        const Accidental* accidental = note->accidental();
        if (accidental && accidental->addToSkyline())
            candidates.elements.push_back(SpacingShapeElement4(accidental->bbox().translated(accidental->pos()), accidental));
        // Note::shape uses these children's bbox, not their composite shape.
        for (const Element* child : note->el()) {
            if (child->addToSkyline())
                candidates.elements.push_back(SpacingShapeElement4(child->bbox().translated(child->pos()), child));
        }
    }
    else if (item->isChord()) {
        const Chord* chord = toChord(item);
        // Reconcile against Chord::shape below, which remains authoritative
        // about omitted cross-staff stems and autoplace-dependent components.
        appendLeaf4(candidates, chord->hook());
        appendLeaf4(candidates, chord->stem());
        appendLeaf4(candidates, chord->stemSlash());
        appendLeaf4(candidates, chord->arpeggio());
        for (const Note* note : chord->notes()) {
            appendShape4(candidates, spacingElementShape4(note, cache), note->pos());
            for (const Element* child : note->el()) {
                if (child->addToSkyline())
                    candidates.elements.push_back(SpacingShapeElement4(
                        child->bbox().translated(child->pos() + note->pos()), child));
            }
        }
        for (const LedgerLine* line = chord->ledgerLines(); line; line = line->next())
            appendLeaf4(candidates, line);
        for (const Element* child : chord->el()) {
            if (child->addToSkyline())
                appendShape4(candidates, spacingElementShape4(child, cache), child->pos());
        }
        for (const Chord* grace : chord->graceNotes())
            appendShape4(candidates, spacingElementShape4(grace, cache), grace->pos());
        if (chord->addToSkyline() && (!qFuzzyIsNull(chord->spacingLeft4()) || !qFuzzyIsNull(chord->spacingRight4())))
            appendLegacySpacer4(candidates, chord, Shape::SPACING_GENERAL, -chord->spacingLeft4(), chord->spacingRight4());
    }
    else if (item->isRest()) {
        // MMRest body/number and rest-dot identity need their own adapter.
        const Rest* rest = toRest(item);
        if (!rest->isGap() && rest->measure() && !rest->measure()->isMMRest())
            candidates.elements.push_back(SpacingShapeElement4(rest->bbox(), rest));
        for (const Element* child : rest->el()) {
            if (child->addToSkyline())
                appendShape4(candidates, spacingElementShape4(child, cache), child->pos());
        }
    }
    else {
        // These native leaf shapes have one source item. Unknown composite
        // types retain their rectangles without pretending to be one leaf.
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
            for (const QRectF& rect : native)
                candidates.elements.push_back(SpacingShapeElement4(rect, item));
            break;
        default:
            break;
        }
    }
    SpacingShape4 resolved = spacingResolveShape4(native, candidates);
    if (cache) cache->remember(item, resolved);
    return resolved;
}

SpacingShape4 spacingStaffShape4(const Segment* segment, int staffIdx)
{
    if (!segment || staffIdx < 0 || static_cast<size_t>(staffIdx) >= segment->shapes().size())
        return {};
    const Shape& native = segment->staffShape(staffIdx);
    SpacingShape4 candidates;
    if (segment->segmentType() & SegmentType::BarLineType) {
        const Element* item = segment->element(staffIdx * VOICES);
        if (item && item->isBarLine()) {
            candidates.elements.push_back(SpacingShapeElement4(toBarLine(item)->layoutRect().translated(item->pos()), item));
            appendLegacySpacer4(candidates, item, Shape::SPACING_GENERAL, 0, 0);
            appendLegacySpacer4(candidates, item, Shape::SPACING_LYRICS, 0, 0);
        }
    }
    else if (segment->score()->staff(staffIdx)->show()) {
        for (const Element* item : segment->elist()) {
            if (item && item->vStaffIdx() == staffIdx && (item->addToSkyline() || item->isChord()))
                appendShape4(candidates, spacingElementShape4(item), item->pos());
        }
        for (const Element* item : segment->annotations()) {
            if (item && item->staffIdx() == staffIdx && item->addToSkyline())
                appendShape4(candidates, spacingElementShape4(item), item->pos());
        }
    }
    return spacingResolveShape4(native, candidates, true);
}
}
