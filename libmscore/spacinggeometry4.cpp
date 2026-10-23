// Optional Studio spacing: temporary item identities for native geometry.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacinggeometry4.h"
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

namespace Ms {

bool SpacingShape4::complete() const
{
    for (const SpacingShapeElement4& e : elements) {
        if (e.identity != SpacingIdentity4::ITEM || !e.item)
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

SpacingShape4 spacingResolveShape4(const Shape& native, const SpacingShape4& candidates)
{
    SpacingShape4 result;
    result.elements.reserve(native.size());
    for (const QRectF& rect : native) {
        SpacingShapeElement4 resolved(rect);
        bool found = false;
        bool unknown = false;
        bool ambiguous = false;
        size_t matches = 0;
        const Element* owner = nullptr;
        for (const SpacingShapeElement4& candidate : candidates.elements) {
            // Exact native geometry only. Do not infer identity by proximity,
            // shape bounds, debug names or candidate order.
            if (!sameRect4(candidate.rect, rect))
                continue;
            found = true;
            ++matches;
            if (candidate.identity == SpacingIdentity4::AMBIGUOUS)
                ambiguous = true;
            else if (candidate.identity != SpacingIdentity4::ITEM || !candidate.item)
                unknown = true;
            else if (owner && owner != candidate.item)
                ambiguous = true;
            else
                owner = candidate.item;
        }
        // A single known primitive cannot account for several coincident
        // native primitives whose other owners have not been identified.
        size_t nativeMatches = 0;
        for (const QRectF& other : native) {
            if (sameRect4(other, rect))
                ++nativeMatches;
        }
        unknown = unknown || matches < nativeMatches;
        if (ambiguous)
            resolved.identity = SpacingIdentity4::AMBIGUOUS;
        else if (found && !unknown && owner) {
            resolved.item = owner;
            resolved.identity = SpacingIdentity4::ITEM;
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

SpacingShape4 spacingElementShape4(const Element* item)
{
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
            appendShape4(candidates, spacingElementShape4(note), note->pos());
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
                appendShape4(candidates, spacingElementShape4(child), child->pos());
        }
        for (const Chord* grace : chord->graceNotes())
            appendShape4(candidates, spacingElementShape4(grace), grace->pos());
    }
    else if (item->isRest()) {
        // MMRest body/number and rest-dot identity need their own adapter.
        const Rest* rest = toRest(item);
        if (!rest->isGap() && rest->measure() && !rest->measure()->isMMRest())
            candidates.elements.push_back(SpacingShapeElement4(rest->bbox(), rest));
        for (const Element* child : rest->el()) {
            if (child->addToSkyline())
                appendShape4(candidates, spacingElementShape4(child), child->pos());
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
    return spacingResolveShape4(native, candidates);
}

SpacingShape4 spacingStaffShape4(const Segment* segment, int staffIdx)
{
    if (!segment || staffIdx < 0 || static_cast<size_t>(staffIdx) >= segment->shapes().size())
        return {};
    const Shape& native = segment->staffShape(staffIdx);
    SpacingShape4 candidates;
    if (segment->segmentType() & SegmentType::BarLineType) {
        const Element* item = segment->element(staffIdx * VOICES);
        if (item && item->isBarLine())
            candidates.elements.push_back(SpacingShapeElement4(toBarLine(item)->layoutRect().translated(item->pos()), item));
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
    return spacingResolveShape4(native, candidates);
}
}
