// Optional Studio spacing: common-notation collision policy.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacingcollision4.h"
#include "spacinggeometry4.h"
#include "chord.h"
#include "clef.h"
#include "note.h"
#include "rest.h"
#include "score.h"
#include "shape.h"
#include "spanner.h"
#include "spannermap.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Ms {
static int paddingIndex4(ElementType type)
{
    static const ElementType types[] = {
        ElementType::NOTE, ElementType::LEDGER_LINE, ElementType::HOOK, ElementType::NOTEDOT,
        ElementType::REST, ElementType::CLEF, ElementType::BAR_LINE, ElementType::KEYSIG,
        ElementType::TIMESIG, ElementType::ARPEGGIO, ElementType::ACCIDENTAL, ElementType::STEM
    };
    for (int i = 0; i < 12; ++i) {
        if (types[i] == type)
            return i;
    }
    return -1;
}

SpacingPaddingTable4::SpacingPaddingTable4(const Score* score) : _score(score)
{
    const qreal spatium = score ? score->spatium() : 0;
    const qreal minPadUnit = 0.1 * spatium;
    for (auto& row : _padding)
        row.fill(minPadUnit);
    if (!score)
        return;
    const qreal ledgerPad = 0.25 * spatium;
    const qreal ledgerLength = score->styleP(Sid::ledgerLineLength);
    auto cell = [&](ElementType a, ElementType b) -> qreal& { return _padding[paddingIndex4(a)][paddingIndex4(b)]; };
    // Studio v4.7.5 PaddingTable::createTable, restricted to these 12 types.
    cell(ElementType::NOTE, ElementType::NOTE) = minPadUnit;
    cell(ElementType::NOTE, ElementType::LEDGER_LINE) = 0.35 * spatium;
    cell(ElementType::NOTE, ElementType::ACCIDENTAL)
        = std::max(static_cast<double>(score->styleP(Sid::accidentalNoteDistance)), 0.35 * spatium);
    cell(ElementType::NOTE, ElementType::REST) = 0.5 * spatium;
    cell(ElementType::NOTE, ElementType::CLEF) = 0.8 * spatium;
    cell(ElementType::NOTE, ElementType::ARPEGGIO) = 0.6 * spatium;
    cell(ElementType::NOTE, ElementType::BAR_LINE) = score->styleP(Sid::noteBarDistance);
    cell(ElementType::NOTE, ElementType::KEYSIG) = 0.75 * spatium;
    cell(ElementType::NOTE, ElementType::TIMESIG) = 0.75 * spatium;

    cell(ElementType::LEDGER_LINE, ElementType::NOTE) = cell(ElementType::NOTE, ElementType::LEDGER_LINE);
    cell(ElementType::LEDGER_LINE, ElementType::LEDGER_LINE) = ledgerPad;
    cell(ElementType::LEDGER_LINE, ElementType::ACCIDENTAL)
        = std::max(score->styleP(Sid::accidentalNoteDistance),
                   cell(ElementType::NOTE, ElementType::ACCIDENTAL) - ledgerLength / 2);
    cell(ElementType::LEDGER_LINE, ElementType::REST) = cell(ElementType::LEDGER_LINE, ElementType::NOTE);
    cell(ElementType::LEDGER_LINE, ElementType::CLEF)
        = std::max(cell(ElementType::NOTE, ElementType::CLEF) - ledgerLength / 2, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::ARPEGGIO) = 0.5 * spatium;
    cell(ElementType::LEDGER_LINE, ElementType::BAR_LINE)
        = std::max(cell(ElementType::NOTE, ElementType::BAR_LINE) - ledgerLength, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::KEYSIG)
        = std::max(cell(ElementType::NOTE, ElementType::KEYSIG) - ledgerLength / 2, ledgerPad);
    cell(ElementType::LEDGER_LINE, ElementType::TIMESIG)
        = std::max(cell(ElementType::NOTE, ElementType::TIMESIG) - ledgerLength / 2, ledgerPad);

    cell(ElementType::HOOK, ElementType::NOTE) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::HOOK, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::HOOK, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::REST) = cell(ElementType::HOOK, ElementType::NOTE);
    cell(ElementType::HOOK, ElementType::CLEF) = 0.5 * spatium;
    cell(ElementType::HOOK, ElementType::ARPEGGIO) = 0.35 * spatium;
    cell(ElementType::HOOK, ElementType::BAR_LINE) = 1 * spatium;
    cell(ElementType::HOOK, ElementType::KEYSIG) = 1.15 * spatium;
    cell(ElementType::HOOK, ElementType::TIMESIG) = 1.15 * spatium;

    cell(ElementType::NOTEDOT, ElementType::NOTE) = std::max(score->styleP(Sid::dotNoteDistance), score->styleP(Sid::dotDotDistance));
    cell(ElementType::NOTEDOT, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::NOTEDOT, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::NOTEDOT, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::NOTEDOT, ElementType::REST) = cell(ElementType::NOTEDOT, ElementType::NOTE);
    cell(ElementType::NOTEDOT, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::NOTEDOT, ElementType::ARPEGGIO) = 0.5 * spatium;
    cell(ElementType::NOTEDOT, ElementType::BAR_LINE) = 0.8 * spatium;
    cell(ElementType::NOTEDOT, ElementType::KEYSIG) = 1.35 * spatium;
    cell(ElementType::NOTEDOT, ElementType::TIMESIG) = 1.35 * spatium;

    cell(ElementType::REST, ElementType::NOTE) = cell(ElementType::NOTE, ElementType::REST);
    cell(ElementType::REST, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::REST, ElementType::NOTE) - ledgerLength / 2, ledgerPad);
    cell(ElementType::REST, ElementType::ACCIDENTAL) = 0.45 * spatium;
    cell(ElementType::REST, ElementType::REST) = cell(ElementType::REST, ElementType::NOTE);
    cell(ElementType::REST, ElementType::CLEF) = cell(ElementType::NOTE, ElementType::CLEF);
    cell(ElementType::REST, ElementType::BAR_LINE) = 1.65 * spatium;
    cell(ElementType::REST, ElementType::KEYSIG) = 1.5 * spatium;
    cell(ElementType::REST, ElementType::TIMESIG) = 1.5 * spatium;

    cell(ElementType::CLEF, ElementType::NOTE) = score->styleP(Sid::clefKeyRightMargin);
    cell(ElementType::CLEF, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::CLEF, ElementType::NOTE) - ledgerLength / 2, ledgerPad);
    cell(ElementType::CLEF, ElementType::ACCIDENTAL) = 0.6 * spatium;
    cell(ElementType::CLEF, ElementType::REST) = cell(ElementType::CLEF, ElementType::NOTE);
    cell(ElementType::CLEF, ElementType::CLEF) = 0.75 * spatium;
    cell(ElementType::CLEF, ElementType::ARPEGGIO) = 0.65 * spatium;
    cell(ElementType::CLEF, ElementType::BAR_LINE) = score->styleP(Sid::clefBarlineDistance);
    cell(ElementType::CLEF, ElementType::KEYSIG) = score->styleP(Sid::clefKeyDistance);
    cell(ElementType::CLEF, ElementType::TIMESIG) = score->styleP(Sid::clefTimesigDistance);

    cell(ElementType::BAR_LINE, ElementType::NOTE) = score->styleP(Sid::barNoteDistance);
    cell(ElementType::BAR_LINE, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::BAR_LINE, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::BAR_LINE, ElementType::ACCIDENTAL) = score->styleP(Sid::barAccidentalDistance);
    cell(ElementType::BAR_LINE, ElementType::REST) = score->styleP(Sid::barNoteDistance);
    cell(ElementType::BAR_LINE, ElementType::CLEF) = score->styleP(Sid::clefLeftMargin);
    cell(ElementType::BAR_LINE, ElementType::ARPEGGIO) = 0.65 * spatium;
    cell(ElementType::BAR_LINE, ElementType::BAR_LINE) = 1.35 * spatium;
    cell(ElementType::BAR_LINE, ElementType::KEYSIG) = score->styleP(Sid::keysigLeftMargin);
    cell(ElementType::BAR_LINE, ElementType::TIMESIG) = score->styleP(Sid::timesigLeftMargin);

    cell(ElementType::KEYSIG, ElementType::NOTE) = 1.75 * spatium;
    cell(ElementType::KEYSIG, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::KEYSIG, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::KEYSIG, ElementType::ACCIDENTAL) = 1.6 * spatium;
    cell(ElementType::KEYSIG, ElementType::REST) = cell(ElementType::KEYSIG, ElementType::NOTE);
    cell(ElementType::KEYSIG, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::KEYSIG, ElementType::ARPEGGIO) = 1.35 * spatium;
    cell(ElementType::KEYSIG, ElementType::BAR_LINE) = score->styleP(Sid::keyBarlineDistance);
    cell(ElementType::KEYSIG, ElementType::KEYSIG) = 1 * spatium;
    cell(ElementType::KEYSIG, ElementType::TIMESIG) = score->styleP(Sid::keyTimesigDistance);

    cell(ElementType::TIMESIG, ElementType::NOTE) = 1.35 * spatium;
    cell(ElementType::TIMESIG, ElementType::LEDGER_LINE)
        = std::max(cell(ElementType::TIMESIG, ElementType::NOTE) - ledgerLength, ledgerPad);
    cell(ElementType::TIMESIG, ElementType::ACCIDENTAL) = 0.8 * spatium;
    cell(ElementType::TIMESIG, ElementType::REST) = cell(ElementType::TIMESIG, ElementType::NOTE);
    cell(ElementType::TIMESIG, ElementType::CLEF) = 1.0 * spatium;
    cell(ElementType::TIMESIG, ElementType::ARPEGGIO) = 1.35 * spatium;
    cell(ElementType::TIMESIG, ElementType::BAR_LINE) = score->styleP(Sid::timesigBarlineDistance);
    cell(ElementType::TIMESIG, ElementType::KEYSIG) = score->styleP(Sid::keyTimesigDistance);
    cell(ElementType::TIMESIG, ElementType::TIMESIG) = 1.0 * spatium;

    cell(ElementType::ARPEGGIO, ElementType::NOTE) = score->styleP(Sid::arpeggioNoteDistance);
    cell(ElementType::ARPEGGIO, ElementType::LEDGER_LINE) = 0.3 * spatium;
    cell(ElementType::ARPEGGIO, ElementType::ACCIDENTAL) = score->styleP(Sid::arpeggioAccidentalDistance);

    cell(ElementType::ACCIDENTAL, ElementType::NOTE) = score->styleP(Sid::accidentalNoteDistance);
    cell(ElementType::ACCIDENTAL, ElementType::LEDGER_LINE) = 0.18 * spatium;

    // Preserve the upstream ordering: copy note row, then note column,
    // then apply stem-specific overrides.
    _padding[paddingIndex4(ElementType::STEM)] = _padding[paddingIndex4(ElementType::NOTE)];
    for (auto& row : _padding)
        row[paddingIndex4(ElementType::STEM)] = row[paddingIndex4(ElementType::NOTE)];
    cell(ElementType::NOTE, ElementType::STEM) = score->styleP(Sid::minNoteDistance);
    cell(ElementType::REST, ElementType::STEM) = cell(ElementType::NOTE, ElementType::STEM);
    cell(ElementType::CLEF, ElementType::STEM) = 0.75 * spatium;
    cell(ElementType::ACCIDENTAL, ElementType::STEM) = cell(ElementType::ACCIDENTAL, ElementType::NOTE);
    cell(ElementType::STEM, ElementType::NOTE) = score->styleP(Sid::minNoteDistance);
    cell(ElementType::STEM, ElementType::STEM) = 0.85 * spatium;
    cell(ElementType::STEM, ElementType::ACCIDENTAL) = 0.35 * spatium;
    cell(ElementType::STEM, ElementType::LEDGER_LINE) = 0.35 * spatium;
    cell(ElementType::LEDGER_LINE, ElementType::STEM) = 0.35 * spatium;

}

bool SpacingPaddingTable4::padding(ElementType first, ElementType second, qreal& result) const
{
    const int a = paddingIndex4(first), b = paddingIndex4(second);
    if (!_score || a < 0 || b < 0)
        return false;
    result = _padding[a][b];
    return true;
}

static bool commonItemContext4(const Element* item, bool allowGrace = false)
{
    if (!item || item->track() < 0 || item->onTabStaff())
        return false;
    if (item->isRest() && toRest(item)->measure() && toRest(item)->measure()->isMMRest())
        return false;
    const Chord* chord = nullptr;
    for (const Element* parent = item; parent; parent = parent->parent()) {
        if (parent->isChord()) {
            chord = toChord(parent);
            break;
        }
    }
    if ((item->isNote() || item->isStem() || item->isHook()) && !chord)
        return false;
    if (!chord)
        return true;
    // Common notation only in this stage. Studio's line attachment and chord
    // kerning-exception inputs need a separate adapter before these can enter.
    if ((!allowGrace && chord->isGrace()) || !chord->articulations().empty())
        return false;
    for (const Note* note : chord->notes()) {
        if (note->tieFor() || note->tieBack() || !note->spannerFor().empty() || !note->spannerBack().empty())
            return false;
    }
    for (const auto& entry : chord->score()->spannerMap().map()) {
        const Spanner* spanner = entry.second;
        if (!spanner->isSlur())
            continue;
        if (spanner->startElement() == chord || spanner->endElement() == chord
            || (spanner->track() == chord->track() && spanner->tick() == chord->tick())
            || (spanner->track2() == chord->track() && spanner->tick2() == chord->tick()))
            return false;
    }
    return true;
}

static bool sameVoiceLimited4(const Element* item)
{
    return item->isNote() || item->isNoteDot() || item->isRest() || item->isStem();
}

static bool neverKernable4(const Element* item)
{
    return (item->isClef() && item->rtick().isZero()) || item->isKeySig() || item->isTimeSig() || item->isBarLine();
}

static bool pairPolicy4(const SpacingPaddingTable4& table, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result, bool allowGrace)
{
    if (!table.score() || !first || !second || first->score() != table.score() || second->score() != table.score()
        || !commonItemContext4(first, allowGrace) || !commonItemContext4(second, allowGrace)
        || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium))
        return false;
    SpacingPairPolicy4 policy;
    if (!table.padding(first->type(), second->type(), policy.padding)) {
        // Studio's default padding-table cell for the slash is 0.1sp.
        if (!allowGrace || (first->type() != ElementType::STEM_SLASH && second->type() != ElementType::STEM_SLASH))
            return false;
        policy.padding = 0.1 * table.score()->spatium();
    }
    const qreal scaling = (first->mag() + second->mag()) / 2;
    if (!(scaling > 0) || !std::isfinite(scaling))
        return false;
    if (first->isNote() && (second->isNote() || second->isRest() || second->isStem())) {
        const Note* note = toNote(first);
        if (!note->chord())
            return false;
        const bool sameVoiceNoteOrStem = (second->isNote() || second->isStem()) && note->track() == second->track();
        const bool adjacent = note->rtick() + note->chord()->actualTicks() == second->rtick();
        if ((sameVoiceNoteOrStem || adjacent)
            && note->shape().translated(note->pos()).intersects(second->shape().translated(second->pos())))
            policy.padding = std::max(policy.padding, table.score()->styleP(Sid::minNoteDistance));
    }
    policy.padding *= scaling;
    if (allowGrace && first->isNote() && second->isNote()) {
        const bool a = toNote(first)->chord()->isGrace();
        const bool b = toNote(second)->chord()->isGrace();
        if (a || b)
            policy.padding = std::max(policy.padding, (a && b ? 0.3 : 0.45) * table.score()->spatium());
    }
    if (!first->isLedgerLine() && second->isRest()) {
        switch (toRest(second)->sym()) {
        case SymId::restWholeLegerLine:
        case SymId::restDoubleWholeLegerLine:
        case SymId::restHalfLegerLine:
            policy.padding += second->bbox().left();
            break;
        default:
            break;
        }
    }
    policy.verticalClearance = (second->isAccidental() ? 0.1 : 0.2) * shapeSpatium;
    if (first->isRest() && toRest(first)->isFullMeasureRest() && first->staffIdx() != second->staffIdx())
        policy.kerning = SpacingKerning4::ALLOW_COLLISION;
    else if ((sameVoiceLimited4(first) && sameVoiceLimited4(second) && first->track() == second->track())
             || neverKernable4(first) || neverKernable4(second))
        policy.kerning = SpacingKerning4::NON_KERNING;
    else if ((first->isNote() && second->isClef()) || (first->isClef() && second->isNote()))
        policy.kerning = SpacingKerning4::RIGHT_EDGE;
    else
        policy.kerning = SpacingKerning4::KERNING;
    if (allowGrace && first->type() == ElementType::STEM_SLASH) {
        const auto chordOf = [](const Element* item) -> const Chord* {
            for (; item; item = item->parent()) {
                if (item->isChord())
                    return toChord(item);
            }
            return nullptr;
        };
        const Chord* a = chordOf(first);
        const Chord* b = chordOf(second);
        policy.kerning = a && b && a->beam() && a->beam() == b->beam()
            ? SpacingKerning4::ALLOW_COLLISION : SpacingKerning4::KERNING;
    }
    if (!std::isfinite(policy.padding))
        return false;
    result = policy;
    return true;
}

bool spacingPairPolicy4(const SpacingPaddingTable4& table, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result)
{
    return pairPolicy4(table, first, second, shapeSpatium, result, false);
}

bool spacingGracePairPolicy4(const SpacingPaddingTable4& table, const Chord* main,
                             const Element* first, const Element* second,
                             qreal shapeSpatium, SpacingPairPolicy4& result)
{
    if (!main || main->isGrace())
        return false;
    const auto belongs = [main](const Element* item) {
        for (; item; item = item->parent()) {
            if (item->isChord()) {
                const Chord* chord = toChord(item);
                return chord == main || (chord->isGraceBefore() && chord->parent() == main);
            }
        }
        return false;
    };
    return belongs(first) && belongs(second) && pairPolicy4(table, first, second, shapeSpatium, result, true);
}

qreal spacingPairDistance4(const QRectF& first, const QRectF& second, const SpacingPairPolicy4& policy,
                           qreal shapeSpatium, qreal squeezeFactor)
{
    const qreal unconstrained = -std::numeric_limits<qreal>::max();
    if (first.isNull() || second.isNull() || policy.kerning == SpacingKerning4::ALLOW_COLLISION)
        return unconstrained;
    const qreal clearance = policy.verticalClearance * squeezeFactor;
    const bool intersection = first.top() != first.bottom() && second.top() != second.bottom()
        && first.bottom() + clearance > second.top() && first.top() < second.bottom() + clearance;
    if (policy.kerning == SpacingKerning4::NON_KERNING || intersection || first.width() == 0 || second.width() == 0) {
        const qreal padding = std::max(policy.padding * squeezeFactor, 0.1 * shapeSpatium * squeezeFactor);
        return first.right() - second.left() + padding;
    }
    switch (policy.kerning) {
    case SpacingKerning4::LEFT_EDGE:
        return first.left() - second.left();
    case SpacingKerning4::CENTER:
        return first.left() + 0.5 * first.width() - second.left();
    case SpacingKerning4::RIGHT_EDGE:
        return first.right() - second.left();
    default:
        return unconstrained;
    }
}

static bool validRect4(const QRectF& r)
{
    return std::isfinite(r.x()) && std::isfinite(r.y()) && std::isfinite(r.width()) && std::isfinite(r.height())
        && r.width() >= 0 && r.height() >= 0;
}

bool spacingShapeDistance4(const SpacingPaddingTable4& table, const SpacingShape4& first, const SpacingShape4& second,
                           qreal shapeSpatium, qreal squeezeFactor, qreal& result)
{
    if (!table.score() || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium) || !std::isfinite(squeezeFactor)
        || squeezeFactor < 0 || squeezeFactor > 1)
        return false;
    // Reject the entire query if one primitive has unadapted provenance or
    // context. Never mix partial Studio constraints with a guessed minimum.
    for (const SpacingShape4* shape : { &first, &second }) {
        for (const SpacingShapeElement4& element : shape->elements) {
            if (!validRect4(element.rect))
                return false;
            if (!element.rect.isNull() && (element.identity != SpacingIdentity4::ITEM || !element.item))
                return false;
        }
    }
    qreal distance = -std::numeric_limits<qreal>::max();
    for (const SpacingShapeElement4& b : second.elements) {
        if (b.rect.isNull())
            continue;
        for (const SpacingShapeElement4& a : first.elements) {
            if (a.rect.isNull())
                continue;
            SpacingPairPolicy4 policy;
            if (!spacingPairPolicy4(table, a.item, b.item, shapeSpatium, policy))
                return false;
            distance = std::max(distance, spacingPairDistance4(a.rect, b.rect, policy, shapeSpatium, squeezeFactor));
        }
    }
    result = distance;
    return true;
}

bool spacingCollisionProjection4(const SpacingPaddingTable4& table, const SpacingShape4& native,
                                 qreal shapeSpatium, SpacingShape4& result)
{
    SpacingShape4 projected;
    if (!table.score() || !(shapeSpatium > 0) || !std::isfinite(shapeSpatium))
        return false;
    const auto supportedAggregateOwner = [&](const Element* owner) {
        if (!owner)
            return false;
        if (owner->isChord()) {
            const Chord* chord = toChord(owner);
            if (chord->notes().empty())
                return false;
            for (const Note* note : chord->notes()) {
                SpacingPairPolicy4 policy;
                if (!spacingPairPolicy4(table, note, note, shapeSpatium, policy))
                    return false;
            }
        }
        else if (!owner->isBarLine())
            return false;
        // Every producer needs its own identified component footprint.
        for (const SpacingShapeElement4& component : native.elements) {
            const auto belongsToOwner = [owner](const Element* item) {
                return item && (item == owner || (item->isNote() && toNote(item)->chord() == owner));
            };
            if (component.identity == SpacingIdentity4::ITEM && belongsToOwner(component.item))
                return true;
            if (component.identity == SpacingIdentity4::SHARED_ITEM) {
                for (const Element* item : component.sharedItems) {
                    if (belongsToOwner(item))
                        return true;
                }
            }
        }
        return false;
    };
    for (const SpacingShapeElement4& e : native.elements) {
        if (!validRect4(e.rect))
            return false;
        if (e.identity == SpacingIdentity4::SHARED_ITEM) {
            if (e.item || !e.aggregateOwners.empty() || e.sharedItems.size() < 2)
                return false;
            for (size_t i = 0; i < e.sharedItems.size(); ++i) {
                const Element* item = e.sharedItems[i];
                SpacingPairPolicy4 policy;
                // This stage admits shared noteheads only. Other coincident
                // component types still require their own context adapter.
                if (!item || !item->isNote() || !spacingPairPolicy4(table, item, item, shapeSpatium, policy))
                    return false;
                for (size_t j = 0; j < i; ++j) {
                    if (item == e.sharedItems[j])
                        return false;
                }
                projected.elements.push_back(SpacingShapeElement4(e.rect, item));
            }
            continue;
        }
        if (!e.sharedItems.empty())
            return false;
        if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
            if (e.item) {
                if (!e.aggregateOwners.empty() || !supportedAggregateOwner(e.item))
                    return false;
            }
            else {
                if (e.aggregateOwners.size() < 2)
                    return false;
                for (size_t i = 0; i < e.aggregateOwners.size(); ++i) {
                    if (!supportedAggregateOwner(e.aggregateOwners[i]))
                        return false;
                    for (size_t j = 0; j < i; ++j) {
                        if (e.aggregateOwners[i] == e.aggregateOwners[j])
                            return false;
                    }
                }
            }
            continue;
        }
        if (!e.item || !e.aggregateOwners.empty())
            return false;
        SpacingPairPolicy4 policy;
        if (e.identity != SpacingIdentity4::ITEM || !spacingPairPolicy4(table, e.item, e.item, shapeSpatium, policy))
            return false;
        projected.elements.push_back(e);
    }
    result = projected;
    return true;
}
}
