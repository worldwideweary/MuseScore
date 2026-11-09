// Optional Studio spacing: nonmutating common-notation placement plan.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#include "spacingprofile4.h"
#include "spacingplan4.h"
#include "spacinginput4.h"
#include "spacinggeometry4.h"
#include "spacingcollision4.h"
#include "spacinglayout4.h"
#include "gracespacing4.h"
#include "chord.h"
#include "articulation.h"
#include "sym.h"
#include "symbol.h"
#include <QStringList>
#include "clef.h"
#include "breath.h"
#include "barline.h"
#include "keysig.h"
#include "timesig.h"
#include "beam.h"
#include "measure.h"
#include "note.h"
#include "tie.h"
#include "rest.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "stem.h"
#include "system.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <set>

namespace Ms {
namespace {
SpacingDecisionScope4*& activeSpacingDecision4()
{
    static thread_local SpacingDecisionScope4* scope = nullptr;
    return scope;
}
}

SpacingDecisionScope4::SpacingDecisionScope4(const Score* score, std::function<void(const SpacingDecision4&)> handler)
    : _score(score), _handler(std::move(handler)), _previous(activeSpacingDecision4())
{
    activeSpacingDecision4() = this;
}

SpacingDecisionScope4::~SpacingDecisionScope4()
{
    Q_ASSERT(activeSpacingDecision4() == this);
    activeSpacingDecision4() = _previous;
}

bool spacingDecisionObserved4(const Score* score)
{
    if (!score) return false;
    for (const SpacingDecisionScope4* scope = activeSpacingDecision4(); scope; scope = scope->_previous)
        if (scope->_score == score && scope->_handler) return true;
    return false;
}

void recordSpacingDecision4(const System* system, SpacingPass4 pass, SpacingPath4 path,
                            qreal width, qreal available, const QString& reason, qreal courtesyRefreshWidth)
{
    if (!system) return;
    SpacingDecisionScope4* scope = activeSpacingDecision4();
    while (scope && (scope->_score != system->score() || !scope->_handler)) scope = scope->_previous;
    if (!scope) return;
    SpacingDecision4 event;
    event.pass = pass;
    event.path = path;
    event.width = width;
    event.available = available;
    event.courtesyRefreshWidth = courtesyRefreshWidth;
    event.reason = reason;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure()) continue;
        const Measure* m = toMeasure(mb);
        if (!event.measures++) event.firstTick = m->tick();
        event.endTick = m->endTick();
    }
    scope->_handler(event);
}

namespace {
struct Geometry4 {
    std::vector<SpacingShape4> staff;
    qreal minLeft = -std::numeric_limits<qreal>::max();
    qreal minRight = 0;
    qreal leftEdge = std::numeric_limits<qreal>::max();
    bool accidentals = false;
    bool outgoingTie = false;
    const Segment* earliestTieStart = nullptr;
    const Segment* incomingTieHeader = nullptr;
    const Segment* outgoingTieBoundary = nullptr;
    qreal incomingTieRoom = 0;
    qreal outgoingTieRoom = 0;
};

bool active4(const Segment* s)
{
    return s->enabled() && s->visible() && !s->allElementsInvisible();
}

bool headerType4(const Segment* s)
{
    return s->isHeaderClefType() || s->isKeySigType() || s->isTimeSigType();
}

bool ordinarySignature4(const Element* item)
{
    if (!item) return false;
    if (item->isKeySig()) {
        const KeySig* key = toKeySig(item);
        return !key->isCustom() && !key->isAtonal() && !key->forInstrumentChange();
    }
    if (!item->isTimeSig()) return false;
    const TimeSig* time = toTimeSig(item);
    if (time->sig() <= Fraction(0, 1) || time->isLocal()) return false;
    const auto type = time->timeSigType();
    if (type == TimeSigType::FOUR_FOUR || type == TimeSigType::ALLA_BREVE)
        return time->sig().identical(type == TimeSigType::FOUR_FOUR ? Fraction(4, 4) : Fraction(2, 2))
            && time->numeratorString().isEmpty() && time->denominatorString().isEmpty();
    return time->timeSigType() == TimeSigType::NORMAL
        && (time->numeratorString().isEmpty() || time->numeratorString() == QString::number(time->numerator()))
        && (time->denominatorString().isEmpty() || time->denominatorString() == QString::number(time->denominator()));
}

bool signatureGeometryReady4(const Element* item, const Segment* segment, int track)
{
    if (!item || item->parent() != segment || item->score() != segment->score() || item->track() != track
        || !item->visible() || !item->addToSkyline() || !(item->mag() > 0) || !std::isfinite(item->mag()))
        return false;
    const QRectF rect = item->bbox();
    return rect.width() > 0 && rect.height() > 0 && std::isfinite(rect.x()) && std::isfinite(rect.y())
        && std::isfinite(rect.width()) && std::isfinite(rect.height())
        && std::isfinite(item->pos().x()) && std::isfinite(item->pos().y());
}

bool sharedSignatureReady4(const Segment* segment)
{
    const int staves = segment->score()->nstaves();
    if (std::count_if(segment->elist().begin(), segment->elist().end(),
            [](const Element* item) { return item != nullptr; }) != staves)
        return false;
    for (int st = 0; st < staves; ++st) {
        const int track = st * VOICES;
        const Element* item = segment->element(track);
        if (!ordinarySignature4(item) || !signatureGeometryReady4(item, segment, track)
            || ((segment->isKeySigType() || segment->isKeySigAnnounceType()) ? !item->isKeySig() : !item->isTimeSig()))
            return false;
    }
    return true;
}

bool courtesyType4(const Segment* s)
{
    return s->isKeySigAnnounceType() || s->isTimeSigAnnounceType();
}

// Measure::addSystemTrailer uses the first staff with a time signature
// to decide whether to generate the courtesy for the whole system.
bool timeCourtesyRequested4(const Segment* segment)
{
    if (!segment || !segment->score()->genCourtesyTimesig()) return false;
    for (int st = 0; st < segment->score()->nstaves(); ++st) {
        const Element* item = segment->element(st * VOICES);
        if (item) return item->isTimeSig() && toTimeSig(item)->showCourtesySig();
    }
    return false;
}

// Mirror createEndBarLines: an opted-out key change on one staff
// does not request a trailer, but another staff can still request it.
bool keyCourtesyRequested4(const Measure* measure, const Segment* nextKey)
{
    const Score* score = measure->score();
    if (!measure->nextMeasure() || !score->styleB(Sid::genCourtesyKeysig)) return false;
    const Fraction tick = measure->endTick();
    for (int st = 0; st < score->nstaves(); ++st) {
        const Staff* staff = score->staff(st);
        if (staff->keySigEvent(tick - Fraction::fromTicks(1)) == staff->keySigEvent(tick))
            continue;
        const Element* item = nextKey ? nextKey->element(st * VOICES) : nullptr;
        if (!item || (item->isKeySig() && toKeySig(item)->showCourtesy()))
            return true;
    }
    return false;
}

bool rightAligned4(const Segment* s)
{
    return s->isClefType() || s->isBreathType();
}

bool manualInteriorLeadingSpace4(const Segment* s)
{
    if (!s || !active4(s) || !s->isChordRestType() || s->rtick() <= Fraction(0, 1))
        return false;
    const Segment* prev = s->prev();
    return prev && active4(prev) && prev->isChordRestType() && prev->measure() == s->measure()
        && prev->rtick() < s->rtick();
}

qreal manualSpringOffset4(const Segment* segment, qreal advance, qreal systemSpatium)
{
    const Segment* next = segment->next();
    return manualInteriorLeadingSpace4(next)
        ? std::max(next->extraLeadingSpace().val() * systemSpatium, -advance) : 0;
}

bool generatedBarlines4(const Segment* segment, BarLineType type)
{
    for (int st = 0; st < segment->score()->nstaves(); ++st) {
        const Element* item = segment->element(st * VOICES);
        if (!item || !item->isBarLine() || !item->generated() || toBarLine(item)->barLineType() != type)
            return false;
    }
    return true;
}

// At a bar start the edit belongs to the onset alone, including when a
// generated start repeat or an ordinary key/time change precedes it.
bool manualBarStartLeadingSpace4(const Segment& target, const Measure* first,
                                 const std::vector<SpacingSegmentPlan4>& earlier,
                                 const Measure* pendingCourtesy)
{
    if (!active4(&target) || !target.isChordRestType() || !target.rtick().isZero())
        return false;
    const int staves = target.score()->nstaves();
    for (const Element* item : target.elist())
        if (item && item->isRest() && toRest(item)->isFullMeasureRest()) return false;
    const auto activeEntry = [](const SpacingSegmentPlan4& s) {
        return !s.ignored;
    };
    auto prev = std::find_if(earlier.rbegin(), earlier.rend(), activeEntry);
    if (target.measure()->repeatStart()) {
        if (prev == earlier.rend() || !prev->segment->isStartRepeatBarLineType()
            || prev->segment->measure() != target.measure() || !prev->segment->rtick().isZero()
            || !prev->segment->ticks().isZero())
            return false;
        return generatedBarlines4(prev->segment, BarLineType::START_REPEAT);
    }
    if (target.measure() == first) {
        return std::all_of(earlier.begin(), earlier.end(), [](const SpacingSegmentPlan4& s) {
            return s.ignored || headerType4(s.segment) || s.segment->isBeginBarLineType();
        });
    }
    // Walk across the in-measure prefix to certify the preceding barline.
    while (prev != earlier.rend() && prev->segment->measure() == target.measure()) {
        const Segment* sig = prev->segment;
        const Element* item = sig->element(0);
        if ((!sig->isKeySigType() && !sig->isTimeSigType()) || sig->header() || sig->trailer()
            || !sig->rtick().isZero() || !sig->ticks().isZero()
            || (staves == 1 ? !ordinarySignature4(item)
                || (sig->isKeySigType() ? !item->isKeySig() : !item->isTimeSig()) : !sharedSignatureReady4(sig)))
            return false;
        prev = std::find_if(std::next(prev), earlier.rend(), activeEntry);
    }
    // Only cross the courtesy already certified for this trial. Keep its
    // geometry in the plan: the walk here only checks the edit's boundary.
    bool courtesyKey = false;
    while (pendingCourtesy && prev != earlier.rend() && prev->segment->measure() == pendingCourtesy
        && courtesyType4(prev->segment)) {
        courtesyKey = courtesyKey || prev->segment->isKeySigAnnounceType();
        prev = std::find_if(std::next(prev), earlier.rend(), activeEntry);
    }
    if (prev == earlier.rend() || !prev->segment->isEndBarLineType()
        || prev->segment->measure() != target.measure()->prevMeasure()
        || prev->segment->tick() != target.tick())
        return false;
    const auto expected = courtesyKey ? BarLineType::DOUBLE : BarLineType::NORMAL;
    return generatedBarlines4(prev->segment, expected);
}

// This field is derived layout state, not manual segment leading space.
// Rebuild it below rather than borrowing a previously published compensation.
bool carriesRightAlignmentOffset4(const Segment* s)
{
    if (!s->isChordRestType()) return false;
    for (const Segment* next = s->next(); next; next = next->next()) {
        if (!active4(next)) continue;
        if (next->isChordRestType()) return false;
        if (rightAligned4(next)) return true;
    }
    return false;
}

// Studio setPositionsAndWidths: only these ordinary changes lead the next
// measure when no barline separates them from the preceding segment.
bool leadingNonRhythmic4(const Segment* s)
{
    return s->isClefType() || s->isKeySigType() || s->isTimeSigType();
}

bool eligibleType4(const Segment* s)
{
    return headerType4(s) || courtesyType4(s) || s->isClefType() || s->isBreathType() || s->isChordRestType()
        || s->isEndBarLineType() || s->isStartRepeatBarLineType() || s->segmentType() == SegmentType::BeginBarLine;
}

bool ordinaryMeasureRest4(const Rest* rest);

// Reuse the ordinary rest projection and late centering at repeat barlines.
// Signature/courtesy fitting keeps its separate, narrower boundary policy.
bool ordinaryRepeatBoundary4(const Measure* measure, bool allowMeasureRests)
{
    const Measure* boundary[] = {measure->prevMeasure(), measure};
    for (const Measure* m : boundary) {
        if (!m || m->system() != measure->system()) continue;
        for (const Segment& s : m->segments()) {
            if (!s.isChordRestType()) continue;
            for (const Element* item : s.elist()) {
                if (!item) continue;
                if (item->isRest()) {
                    const Rest* rest = toRest(item);
                    if (rest->isFullMeasureRest() && (!allowMeasureRests || !ordinaryMeasureRest4(rest)))
                        return false;
                }
                else if (item->isChord()) {
                    const Chord* chord = toChord(item);
                    if (chord->staffMove() || !chord->graceNotes().empty() || chord->tremolo()
                        || (chord->beam() && chord->beam()->cross())) return false;
                }
                else return false;
            }
        }
    }
    return true;
}

struct CrossStaffGeometry4 {
    std::map<const Chord*, CrossStaffChord4> chords;
    std::map<const Segment*, CrossStaffPair4> pairs;
};

// A beam can move as a group without spanning two visual staves.
// Native shapes already route its noteheads to the destination staff,
// but omit the stems of moved beamed chords.
// Return the first rejection, or nullptr after successful preparation.
const char* prepareRelocatedBeam4(const Beam* beam, const Measure* measure, CrossStaffGeometry4& geometry)
{
    const Score* score = measure->score();
    if (score->nstaves() != 2) return "Relocated beam requires a grand staff";
    if (beam->score() != score || beam->cross()) return "Relocated beam has incompatible ownership or staff span";
    if (beam->beamDirection() != Direction::AUTO) return "Relocated beam has an explicit beam direction";
    if (beam->userModified()) return "Relocated beam has a manual adjustment";
    if (beam->elements().size() < 2) return "Relocated beam has fewer than two members";
    const Chord* previous = nullptr;
    for (const ChordRest* item : beam->elements()) {
        if (!item || !item->isChord()) return "Relocated beam has a rest or missing member";
        const Chord* chord = toChord(item);
        if (chord->measure() != measure) return "Relocated beam crosses a measure boundary";
        if (!chord->segment() || chord->beam() != beam) return "Relocated beam has an unattached chord";
        if (!chord->staffMove()) return "Relocated beam has an unmoved chord";
        if (!chord->visible()) return "Relocated beam has a hidden chord";
        if (chord->isGrace() || !chord->graceNotes().empty()) return "Relocated beam has grace notation";
        if (chord->tuplet()) return "Relocated beam has a tuplet chord";
        if (chord->tremolo()) return "Relocated beam has a tremolo chord";
        // Native note shapes use chord->pos(), and the projected stem below
        // uses chord->x() and chord->y(). All three include the user's offset.
        if (!std::isfinite(chord->x()) || !std::isfinite(chord->y()))
            return "Relocated beam has a non-finite chord position";
        if (chord->isSmall()) return "Relocated beam has a small chord";
        if (chord->noStem() || !chord->stem()) return "Relocated beam has a chord without a stem";
        if (chord->stemDirection() != Direction::AUTO) return "Relocated beam has a chord with an explicit stem direction";
        if (chord->up() != beam->up()) return "Relocated beam and chord disagree on stem side";
        if (chord->notes().empty()) return "Relocated beam has an empty chord";
        if (chord->actualTicks() != Fraction(1, 8)
            || chord->durationType().type() != TDuration::DurationType::V_EIGHTH)
            return "Relocated beam has a member other than a plain eighth note";
        const int visual = chord->vStaffIdx();
        if (visual < 0 || visual >= score->nstaves() || !score->staff(visual)->show()
            || !score->staff(visual)->isPitchedStaff(chord->tick()) || !chord->staff()
            || !chord->staff()->isPitchedStaff(chord->tick()))
            return "Relocated beam requires visible pitched destination and pitched source staves";
        if (previous && (chord->track() != previous->track() || visual != previous->vStaffIdx()))
            return "Relocated beam changes source voice or destination staff";
        if (previous && chord->tick() != previous->tick() + previous->actualTicks())
            return "Relocated beam has noncontiguous onsets";
        for (const Note* note : chord->notes()) {
            if (!note->visible()) return "Relocated beam has a hidden notehead";
            if (note->mirror()) return "Relocated beam has a mirrored notehead";
            if (note->isNudged()) return "Relocated beam has a nudged notehead";
            if (note->isSmall()) return "Relocated beam has a small notehead";
        }
        CrossStaffChord4 entry;
        entry.chord = chord;
        entry.visualStaff = visual;
        entry.up = beam->up();
        const qreal width = chord->stem()->lineWidth() * chord->mag();
        entry.stemX = chord->x() + chord->stemPosX() + (entry.up ? -.5 : .5) * width;
        if (!(width > 0) || !std::isfinite(width) || !std::isfinite(entry.stemX))
            return "Relocated beam has invalid stem geometry";
        geometry.chords.emplace(chord, entry);
        previous = chord;
    }
    return nullptr;
}

bool prepareCrossStaffMeasure4(Measure* measure, CrossStaffGeometry4& result, QString* reason)
{
    const Beam* beam = nullptr;
    std::set<const Beam*> relocated;
    for (const Segment& s : measure->segments()) {
        for (const Element* item : s.elist()) {
            if (!item || !item->isChord()) continue;
            const Chord* c = toChord(item);
            if (!c->staffMove() && !(c->beam() && c->beam()->cross())) continue;
            if (!c->beam()) {
                if (reason) *reason = "Cross-staff chord has no beam";
                return false;
            }
            if (!c->beam()->cross()) {
                relocated.insert(c->beam());
                continue;
            }
            if (beam && beam != c->beam()) {
                if (reason) *reason = "Independent cross-staff streams within a measure require a later adapter";
                return false;
            }
            beam = c->beam();
        }
    }
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!relocated.empty()) {
        if (beam) return fail("Mixed relocated and interstaff beams require separate preparation");
        CrossStaffGeometry4 prepared;
        for (const Beam* group : relocated) {
            if (const char* rejection = prepareRelocatedBeam4(group, measure, prepared))
                return fail(rejection);
        }
        result = std::move(prepared);
        return true;
    }
    if (!beam) return true;
    if (measure->score()->nstaves() != 2)
        return fail("Cross-staff placement currently requires a grand staff");
    CrossStaffContext4 context;
    if (!buildCrossStaffContext4(beam, context, reason)) return false;
    if (context.chords.front().chord->measure() != measure
        || !context.chords.front().chord->rtick().isZero()
        || context.chords.back().chord->rtick() + context.chords.back().chord->actualTicks() != measure->ticks())
        return fail("Cross-staff placement requires a complete measure stream");
    CrossStaffGeometry4 prepared;
    for (const CrossStaffChord4& e : context.chords)
        prepared.chords[e.chord] = e;
    std::map<int, int> movingCounts;
    for (const Segment& segment : measure->segments()) {
        if (!segment.isChordRestType()) continue;
        for (const Element* item : segment.elist()) {
            if (!item || (item->isChord() && prepared.chords.count(toChord(item)))) continue;
            if (!item->isChord()) return fail("Cross-staff companions currently require chords");
            const Chord* c = toChord(item);
            if (!c->visible() || c->beam() || c->staffMove() || c->isGrace() || c->tuplet()
                || c->tremolo() || c->isSmall() || c->mag() != 1 || c->isNudged() || c->noStem() || !c->stem()
                || !c->graceNotes().empty() || c->notes().size() != 1)
                return fail("Unadapted moving cross-staff companion");
            const Note* note = c->notes().front();
            if (!note->visible() || note->isSmall() || !note->offset().isNull() || note->mirror()
                || note->headGroup() != NoteHead::Group::HEAD_NORMAL || note->headType() != NoteHead::Type::HEAD_AUTO)
                return fail("Unadapted moving cross-staff companion note");
            if (c->rtick().isZero() && c->actualTicks() == measure->ticks()) continue;
            // Keep each admitted un-beamed stream complete independently.
            int& count = movingCounts[c->track()];
            if (movingCounts.size() > 2 || c->actualTicks() != Fraction(1, 16)
                || c->durationType().type() != TDuration::DurationType::V_16TH
                || c->rtick() != Fraction(count, 16))
                return fail("Cross-staff companions need at most two complete sixteenth-note streams");
            ++count;
        }
    }
    for (const auto& stream : movingCounts) {
        if (Fraction(stream.second, 16) != measure->ticks())
            return fail("Incomplete moving cross-staff companion");
    }
    for (size_t i = 0; i < context.pairs.size(); ++i) {
        prepared.pairs[context.pairs[i].first->segment()] = context.pairs[i];
    }
    result = prepared;
    return true;
}

// Publish only after every measure's independently bounded context succeeds.
bool prepareCrossStaff4(System* system, CrossStaffGeometry4& result, QString* reason)
{
    CrossStaffGeometry4 prepared;
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure()) {
            if (reason) *reason = "Frames require a later cross-staff system adapter";
            return false;
        }
        CrossStaffGeometry4 local;
        Measure* measure = toMeasure(mb);
        if (!prepareCrossStaffMeasure4(measure, local, reason)) {
            if (reason) *reason += QStringLiteral(" [measure %1]").arg(measure->no() + 1);
            return false;
        }
        prepared.chords.insert(local.chords.begin(), local.chords.end());
        prepared.pairs.insert(local.pairs.begin(), local.pairs.end());
    }
    result = prepared;
    return true;
}

// Validate published before-graces and prepare complete temporary after-group
// projections. No native writes; publish outputs only after every group succeeds.
bool prepareGraces4(System* system, SpacingGraceContext4& result, std::vector<GraceAfterGeometry4>& afterResult,
                    std::vector<GraceMergedGeometry4>& mergedResult, QString* reason,
                    const std::vector<GraceMergedGeometry4>* preparedMerged, SpacingContextCache4& contextCache)
{
    SpacingGraceContext4 prepared;
    std::vector<GraceAfterGeometry4> afters;
    std::vector<GraceMergedGeometry4> merged;
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure())
            continue; // The ordinary planner rejects frames itself.
        for (const Segment& segment : toMeasure(mb)->segments()) {
            if (!active4(&segment) || !segment.isChordRestType())
                continue;
            for (const Element* item : segment.elist()) {
                if (!item || !item->isChord() || toChord(item)->graceNotes().empty())
                    continue;
                const Chord* main = toChord(item);
                // Incoming combined groups already stage their target's before-graces.
                if (prepared.contains(main)) continue;
                if (!main->graceNotesAfter().empty()) {
                    GraceAfterGeometry4 geometry;
                    if (buildGraceAfterGeometry4(main, geometry, reason, &contextCache)) {
                        prepared.afterMains.push_back(main);
                        afters.push_back(geometry);
                    }
                    else {
                        GraceMergedGeometry4 joint;
                        const GraceMergedGeometry4* ready = nullptr;
                        if (preparedMerged) {
                            for (const auto& entry : *preparedMerged) {
                                if (entry.plan.owner == main) { ready = &entry; break; }
                            }
                        }
                        if (ready && ready->plan.slurPolicy == contextCache.slurPolicy()) {
                            SpacingProfileScope4 reuse(SpacingProfileStage4::PLAN_GEOMETRY_REUSE);
                            joint = *ready;
                            if (reason) reason->clear();
                        }
                        else if (!buildGraceMergedGeometry4(main, joint, reason, &contextCache)) return false;
                        prepared.afterMains.push_back(main);
                        prepared.mains.push_back(joint.plan.target);
                        prepared.mergedPairs.push_back({main, joint.plan.target});
                        merged.push_back(joint);
                    }
                    continue;
                }
                GraceBeforePlan4 plan;
                if (!buildGraceBeforePlan4(main, plan, reason, &contextCache))
                    return false;
                for (const GracePosition4& p : plan.positions) {
                    if (std::abs(p.chord->x() - p.x) > 1e-6 || !std::isfinite(p.chord->x())) {
                        if (reason) *reason = "Grace positions must be prepared before system spacing";
                        return false;
                    }
                }
                prepared.mains.push_back(main);
            }
        }
    }
    result = prepared;
    afterResult = afters;
    mergedResult = merged;
    return true;
}

// Replace only proven components on the affected staff. Unknown or mixed
// ownership must not disappear when a temporary after-grace shape is used.
bool substituteAfterGeometry4(Segment* segment, int staff, SpacingShape4& native,
                             const std::vector<GraceAfterGeometry4>& afters, bool targetBefore = false)
{
    for (const GraceAfterGeometry4& after : afters) {
        const Chord* owner = after.plan.owner;
        if (staff != owner->staffIdx()) continue;
        const bool source = segment == owner->segment();
        if (!source && segment != after.plan.target) continue;
        const Element* root = source ? owner : segment->element(owner->track());
        const bool before = targetBefore && !source;
        const auto owned = [root, before](const Element* item) {
            for (; item; item = item->parent()) {
                if (item == root) return true;
                if (item->isChord()) {
                    const Chord* chord = toChord(item);
                    return chord == root || ((before ? chord->isGraceBefore() : chord->isGraceAfter()) && chord->parent() == root);
                }
            }
            return false;
        };
        for (const auto& e : native.elements) {
            if (!std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
                || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
                || e.rect.width() < 0 || e.rect.height() < 0) return false;
            if (e.identity == SpacingIdentity4::ITEM) {
                if (!owned(e.item) || !e.aggregateOwners.empty() || !e.sharedItems.empty()) return false;
            }
            else if (e.identity == SpacingIdentity4::LEGACY_SPACER) {
                if (!e.sharedItems.empty()) return false;
                if (e.item) {
                    if ((!e.item->isChord() && !(e.item == root && e.item->isBarLine()))
                        || !owned(e.item) || !e.aggregateOwners.empty()) return false;
                }
                else {
                    if (e.aggregateOwners.size() < 2) return false;
                    for (size_t i = 0; i < e.aggregateOwners.size(); ++i) {
                        const Element* item = e.aggregateOwners[i];
                        if (!item || !item->isChord() || !owned(item)) return false;
                        for (size_t j = 0; j < i; ++j) if (item == e.aggregateOwners[j]) return false;
                    }
                }
            }
            else if (e.identity == SpacingIdentity4::SHARED_ITEM) {
                if (e.item || !e.aggregateOwners.empty() || e.sharedItems.size() < 2) return false;
                for (size_t i = 0; i < e.sharedItems.size(); ++i) {
                    if (!owned(e.sharedItems[i])) return false;
                    for (size_t j = 0; j < i; ++j) if (e.sharedItems[i] == e.sharedItems[j]) return false;
                }
            }
            else return false;
        }
        native = source ? after.source : after.target;
        return true;
    }
    return true;
}

// Reuse the provenance validator with explicit target-before ownership. The
// adapter is private to geometry substitution; public after-only plans stay separate.
bool substituteMergedGeometry4(Segment* segment, int staff, SpacingShape4& native,
                              const std::vector<GraceMergedGeometry4>& merged)
{
    for (const GraceMergedGeometry4& joint : merged) {
        GraceAfterGeometry4 view;
        view.plan.owner = joint.plan.owner;
        view.plan.target = joint.plan.target->segment();
        view.source = joint.source;
        view.target = joint.target;
        if (!substituteAfterGeometry4(segment, staff, native, {view}, true)) return false;
    }
    return true;
}

// Studio Segment::createShape omits a full rest from a moving-voice staff,
// except when every item at this onset is a full rest. Keep native skyline
// shapes intact: apply this horizontal policy only to the validated snapshot.
// RestLayout::layoutRest starts at x=0 before spacing; an already centered
// native rest must supply that same input without resetting its DOM position.
bool projectMeasureRests4(const Segment* segment, int staff, SpacingShape4& shape)
{
    const bool hasMeasureRest = std::any_of(shape.elements.begin(), shape.elements.end(),
        [](const SpacingShapeElement4& e) {
            return e.identity == SpacingIdentity4::ITEM && e.item && e.item->isRest()
                && toRest(e.item)->isFullMeasureRest();
        });
    if (!hasMeasureRest) return true; // Preserve the existing non-rest projection path.
    SpacingShape4 projected;
    for (auto e : shape.elements) {
        // Even a source-authorized omission must not conceal malformed
        // producer geometry from the complete-snapshot rejection boundary.
        if (!std::isfinite(e.rect.x()) || !std::isfinite(e.rect.y())
            || !std::isfinite(e.rect.width()) || !std::isfinite(e.rect.height())
            || e.rect.width() < 0 || e.rect.height() < 0) return false;
        if (e.identity == SpacingIdentity4::ITEM && e.item && e.item->isRest()
            && toRest(e.item)->isFullMeasureRest()) {
            const Rest* rest = toRest(e.item);
            if (rest->segment() != segment || rest->staffIdx() != staff || rest->staffMove()
                || !std::isfinite(rest->ipos().x())) return false;
            bool allFullRests = true;
            for (int track = staff * VOICES; track < (staff + 1) * VOICES; ++track) {
                const Element* item = segment->element(track);
                if (item && !(item->isRest() && toRest(item)->isFullMeasureRest())) {
                    allFullRests = false;
                    break;
                }
            }
            if (segment->measure()->hasVoices(staff) && !allFullRests) continue;
            e.rect.translate(-rest->ipos().x(), 0); // Retain the user's offset.
        }
        projected.elements.push_back(e);
    }
    shape = std::move(projected);
    return true;
}

bool ordinaryMeasureRest4(const Rest* rest)
{
    const bool ordinary = rest && rest->measure() && !rest->measure()->isMMRest() && rest->segment()
        && rest->isFullMeasureRest() && rest->rtick().isZero()
        && rest->actualTicks() == rest->measure()->ticks()
        && rest->visible() && rest->addToSkyline() && !rest->onTabStaff()
        && !rest->isGap() && !rest->staffMove()
        && rest->mag() > 0 && std::isfinite(rest->mag())
        && !rest->tuplet() && !rest->beam() && !rest->dots()
        && rest->el().empty() && rest->lyrics().empty()
        && std::isfinite(rest->ipos().x()) && std::isfinite(rest->x()) && std::isfinite(rest->y())
        && std::isfinite(rest->bbox().x()) && std::isfinite(rest->bbox().y())
        && std::isfinite(rest->width()) && rest->width() > 0
        && std::isfinite(rest->height()) && rest->height() > 0;
    if (!ordinary) return false;
    // Studio getMeasureStartEndPos ignores internal signatures when centering
    // a full rest; Evolution's late native pass stops at any non-rhythmic item.
    // Reuse that pass only where its first boundary has the source semantics.
    for (const Segment* s = rest->segment()->nextActive(); s; s = s->nextActive()) {
        if (!s->isChordRestType() && s->element(rest->staffIdx() * VOICES))
            return s->isEndBarLineType() || s->isKeySigAnnounceType() || s->isTimeSigAnnounceType();
    }
    return true;
}

bool cacheGeometry4(Segment* segment, const SpacingPaddingTable4& table, Geometry4& geometry,
                    const CrossStaffGeometry4& cross, const SpacingGraceContext4& graces,
                    const std::vector<GraceAfterGeometry4>& afters,
                    const std::vector<GraceMergedGeometry4>& merged, SpacingContextCache4& contextCache, QString* reason)
{
    const auto fail = [reason](const char* why) { if (reason) *reason = QString::fromLatin1(why); return false; };
    for (int staffIdx = 0; staffIdx < segment->score()->nstaves(); ++staffIdx) {
        SpacingShape4 native = spacingStaffShape4(segment, staffIdx);
        if (!substituteAfterGeometry4(segment, staffIdx, native, afters)) return fail("after-grace substitution");
        if (!substituteMergedGeometry4(segment, staffIdx, native, merged)) return fail("merged grace substitution");
        if (!projectMeasureRests4(segment, staffIdx, native)) return fail("measure rest projection");
        SpacingShape4 projected;
        // These first controls use ordinary visible staves. Staff scale still
        // comes from the item, rather than assuming score spatium in the kernel.
        qreal sp = segment->spatium();
        for (const SpacingShapeElement4& e : native.elements) {
            if (e.identity == SpacingIdentity4::ITEM && e.item) {
                sp = e.item->spatium();
                break;
            }
        }
        if (!spacingCollisionProjection4(table, native, sp, projected, &graces, &contextCache, reason))
            return false;
        for (const auto& pair : cross.chords) {
            const CrossStaffChord4& e = pair.second;
            if (e.chord->segment() != segment || e.visualStaff != staffIdx) continue;
            const Note* note = e.up ? e.chord->downNote() : e.chord->upNote();
            const qreal width = e.chord->stem()->lineWidth() * e.chord->mag();
            const qreal y = e.chord->y() + note->y()
                + (e.up ? note->stemUpSE().y() : note->stemDownNW().y());
            // Use the prepared stem side without borrowing a provisional
            // stem length. Relocated beams use the side from native beam
            // preparation; interstaff beams use the independently inferred side.
            const qreal reach = 100000;
            const QRectF rect(e.stemX - width / 2, e.up ? y - reach : y - width / 2,
                              width, reach + width / 2);
            SpacingPairPolicy4 policy;
            if (!spacingPairPolicy4(table, e.chord->stem(), e.chord->stem(), sp, policy, nullptr, &contextCache)) return fail("cross-staff stem policy");
            projected.elements.push_back(SpacingShapeElement4(rect, e.chord->stem()));
        }
        qreal staffLeft = std::numeric_limits<qreal>::max();
        for (const SpacingShapeElement4& e : projected.elements) {
            geometry.minRight = std::max(geometry.minRight, e.rect.right());
            geometry.leftEdge = std::min(geometry.leftEdge, e.rect.left());
            if (e.rect.height() != 0)
                staffLeft = std::min(staffLeft, e.rect.left());
            geometry.accidentals = geometry.accidentals || e.item->isAccidental();
        }
        if (staffLeft != std::numeric_limits<qreal>::max())
            geometry.minLeft = std::max(geometry.minLeft, -staffLeft);
        geometry.staff.push_back(projected);
    }
    if (geometry.minLeft == -std::numeric_limits<qreal>::max())
        geometry.minLeft = 0;
    if (geometry.leftEdge == std::numeric_limits<qreal>::max())
        geometry.leftEdge = 0;
    return true;
}

bool distance4(const SpacingPaddingTable4& table, const SpacingSegmentPlan4& a, const Geometry4& first,
               const SpacingSegmentPlan4& b, const Geometry4& second, Measure* firstMeasure,
               qreal squeeze, qreal& result, const CrossStaffGeometry4& cross, const SpacingGraceContext4& graces,
               SpacingContextCache4& contextCache)
{
    // Studio minHorizontalDistance joins adjacent repeat barlines at the
    // thick stroke. A systemic begin barline shares the repeat's origin.
    if (a.segment->isType(SegmentType::BarLineType) && b.segment->isStartRepeatBarLineType()) {
        result = a.segment->isBeginBarLineType() ? 0
            : std::max(first.minRight - table.score()->styleP(Sid::endBarWidth), qreal(0));
        return std::isfinite(result);
    }
    qreal distance = 0;
    const bool header = headerType4(a.segment) && a.segment->rtick().isZero()
        && b.segment->measure() == firstMeasure
        && (b.segment->isChordRestType() || b.segment->isStartRepeatBarLineType());
    for (size_t st = 0; st < first.staff.size(); ++st) {
        qreal sp = a.segment->spatium();
        for (const SpacingShapeElement4& e : first.staff[st].elements) {
            sp = e.item->spatium();
            break;
        }
        qreal d = 0;
        if (!spacingShapeDistance4(table, first.staff[st], second.staff[st], sp, squeeze, d, &graces, &contextCache))
            return false;
        distance = std::max(distance, d);
    }
    if (first.outgoingTie && b.segment->isChordRestType()) {
        for (const Element* item : a.segment->elist()) {
            if (!item || !item->isChord()) continue;
            for (const Note* note : toChord(item)->notes()) {
                const Tie* tie = note->tieFor();
                const Note* end = tie ? tie->endNote() : nullptr;
                if (!end || end->chord()->segment() != b.segment) continue;
                // The native tie adjusts its grips around endpoint ink, then
                // adds a quarter-space inset at each end. Reserve that room
                // outside the projected endpoint shapes, even when squeezing.
                const qreal left = second.leftEdge;
                const qreal inset = 0.5 * std::max(note->spatium(), end->spatium());
                distance = std::max(distance, first.minRight - left + inset
                    + table.score()->styleP(Sid::minTieLength) * std::max(note->mag(), end->mag()));
            }
        }
    }
    if (second.incomingTieHeader == a.segment)
        distance = std::max(distance, first.minRight - second.leftEdge + second.incomingTieRoom);
    if (first.outgoingTieBoundary == b.segment)
        distance = std::max(distance, first.minRight + first.outgoingTieRoom);
    const auto constraint = cross.pairs.find(a.segment);
    if (constraint != cross.pairs.end() && constraint->second.second->segment() == b.segment)
        distance = std::max(distance, std::max(constraint->second.minimumWidth, constraint->second.minimumStemDistance));
    if (header) {
        const Score* score = table.score();
        distance = std::max(distance, first.minRight);
        const qreal gap = score->styleP(a.segment->isTimeSigType() ? Sid::systemHeaderTimeSigDistance : Sid::systemHeaderDistance);
        distance = std::max(distance, first.minRight + gap * squeeze);
        if (b.segment->isStartRepeatBarLineType()) {
            // Studio measures this gap edge-to-edge. Evolution's native
            // barline renderer uses half its stored endBarDistance value.
            distance -= score->styleP(Sid::endBarWidth) + .5 * score->styleP(Sid::endBarDistance);
        }
        const qreal actual = distance - first.minRight - second.minLeft;
        distance += std::max(qreal(0), 1.5 * a.segment->spatium() * squeeze - actual);
    }
    result = distance;
    return std::isfinite(result);
}

// Studio moveRightAlignedSegments, before segment-width reconstruction.
// Geometry and DOM stay immutable; the fixed compensation belongs to the
// preceding rhythmic spring and must survive minimum widths/justification.
bool moveRightAligned4(SpacingPlan4& plan, const std::vector<Geometry4>& geometry,
                       const SpacingPaddingTable4& table, qreal squeeze, const CrossStaffGeometry4& cross,
                       const SpacingGraceContext4& graces, SpacingContextCache4& contextCache)
{
    for (size_t i = 0; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored || !rightAligned4(current.segment)) continue;
        qreal x = std::numeric_limits<qreal>::max();
        for (size_t j = i + 1; j < plan.segments.size(); ++j) {
            const SpacingSegmentPlan4& following = plan.segments[j];
            if (following.ignored) continue;
            if (following.segment->measure() != current.segment->measure() && !following.segment->rtick().isZero())
                break;
            qreal distance = 0;
            if (!distance4(table, current, geometry[i], following, geometry[j], plan.measures.front().measure,
                           squeeze, distance, cross, graces, contextCache)) return false;
            // Pinned muse::RealIsNull(double) uses an absolute 1e-9 tolerance.
            if (rightAligned4(following.segment) && qAbs(distance) <= 1e-9) continue;
            x = std::min(x, following.x - distance);
        }
        if (x == std::numeric_limits<qreal>::max()) continue;
        if (!std::isfinite(x)) return false;
        current.x = x;
        size_t next = i + 1;
        while (next < plan.segments.size() && plan.segments[next].ignored) ++next;
        if (next == plan.segments.size()) return false;
        // Use an index for the next-active lookup; never assign through a
        // reference into the plan and overwrite an ignored segment's identity.
        for (size_t previous = i; previous > 0;) {
            --previous;
            SpacingSegmentPlan4& spring = plan.segments[previous];
            if (spring.segment->measure() != current.segment->measure()) break;
            if (!spring.segment->isChordRestType()) continue;
            spring.widthOffset += x - plan.segments[next].x;
            if (!std::isfinite(spring.widthOffset)) return false;
            break;
        }
    }
    return true;
}

// Reconstruct segment positions after changing rhythmic widths in one measure.
// First-segment offset and nonrhythmic widths remain fixed, as in respaceSegments.
qreal reflowMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure)
{
    qreal cursor = measure.x;
    bool first = true;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (segment.segment->measure() != measure.measure)
            continue;
        if (segment.ignored)
            continue;
        if (first) {
            cursor = segment.x;
            first = false;
        }
        segment.x = cursor;
        cursor += segment.width;
    }
    const qreal previous = measure.width;
    measure.width = cursor - measure.x;
    return measure.width - previous;
}

void moveMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure, qreal delta)
{
    measure.x += delta;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (segment.segment->measure() == measure.measure)
            segment.x += delta;
    }
}

bool stretchMeasure4(SpacingPlan4& plan, SpacingMeasurePlan4& measure, qreal extra)
{
    std::vector<SpacingSpring4> springs;
    for (const SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.segment->measure() == measure.measure && segment.stretch > 0)
            springs.push_back({ segment.width - segment.widthOffset, segment.stretch });
    }
    const qreal force = spacing4SpringForce(springs, extra);
    if (!(force > 0))
        return false;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.segment->measure() == measure.measure && segment.stretch > 0)
            segment.width = std::max(segment.width - segment.widthOffset, force * segment.stretch) + segment.widthOffset;
    }
    return true;
}
}

bool spacingCandidateSignaturesReady4(const System* system, QString* reason, bool allowPendingTrailer)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->measures().empty())
        return fail("Candidate signature prefix unavailable");
    std::vector<const Measure*> measures;
    bool signatures = false;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mb->system() != system || mb->score() != system->score())
            return fail("Candidate signature prefix ownership/frame boundary");
        const Measure* m = toMeasure(mb);
        if (!measures.empty() && measures.back()->next() != m)
            return fail("Candidate signature prefix is not contiguous");
        signatures = signatures || m->trailer() || m->hasCourtesyKeySig();
        for (const Segment& segment : m->segments())
            if (active4(&segment) && (courtesyType4(&segment) || segment.trailer()
                || (!measures.empty() && headerType4(&segment)))) signatures = true;
        measures.push_back(m);
    }
    const Score* score = system->score();
    const Measure* last = measures.back();
    if (const Measure* next = last->nextMeasure()) {
        const Fraction zero(0, 1);
        signatures = signatures
            || timeCourtesyRequested4(next->findSegmentR(SegmentType::TimeSig, zero))
            || keyCourtesyRequested4(last, next->findSegmentR(SegmentType::KeySig, zero));
    }
    if (!signatures) return true; // Preserve the existing ordinary-prefix contract.
    if (score->floatMode())
        return fail("Candidate signatures do not support float layout");
    for (size_t i = 0; i < measures.size(); ++i) {
        const Measure* m = measures[i];
        const bool last = i + 1 == measures.size();
        for (int st = 0; st < score->nstaves(); ++st)
            if (!score->staff(st)->isPitchedStaff(m->tick()))
                return fail("Candidate signatures require pitched staves");
        if (m->repeatStart() || m->repeatEnd()
            || m->nextSectionBreak() || !ordinaryRepeatBoundary4(m, false))
            return fail("Candidate signature repeat/section/grace/rest/cross-staff boundary");
        const bool pending = allowPendingTrailer && i + 2 == measures.size() && m->trailer();
        const bool terminalGeometry = last || pending;
        if (m->trailer() && !terminalGeometry)
            return fail("Candidate pending penultimate courtesy trailer");
        for (const Segment& segment : m->segments()) {
            // Even a currently hidden ordinary clef can be revealed/moved by
            // predecessor finalization. That requires a separate adapter.
            if (segment.isClefType()) return fail("Candidate signature ordinary-clef boundary");
            if (allowPendingTrailer && active4(&segment) && rightAligned4(&segment))
                return fail("Candidate pending penultimate courtesy with a right-aligned mark");
            if (!active4(&segment)) continue;
            if (!i && segment.isHeaderClefType()) continue;
            if (segment.isHeaderClefType()) return fail("Candidate interior system header not removed");
            const bool courtesy = courtesyType4(&segment);
            if (!headerType4(&segment) && !courtesy && !segment.trailer()) continue;
            if (!segment.ticks().isZero() || (courtesy ? !terminalGeometry || !segment.trailer() || segment.rtick() != m->ticks()
                : segment.trailer() || !segment.rtick().isZero()))
                return fail("Candidate signature location/trailer phase not prepared");
            if (!sharedSignatureReady4(&segment))
                return fail("Candidate signature glyph, ownership or dimensions not prepared");
            if (courtesy)
                for (const Element* item : segment.elist())
                    if (item && !item->generated())
                        return fail("Candidate courtesy is not generated");
        }
        const Measure* next = m->nextMeasure();
        const Segment* keySeg = next ? next->findSegmentR(SegmentType::KeySig, Fraction(0, 1)) : nullptr;
        const Segment* timeSeg = next ? next->findSegmentR(SegmentType::TimeSig, Fraction(0, 1)) : nullptr;
        const Segment* keyTrailer = m->findSegmentR(SegmentType::KeySigAnnounce, m->ticks());
        const Segment* timeTrailer = m->findSegmentR(SegmentType::TimeSigAnnounce, m->ticks());
        const bool hasKey = keyTrailer && active4(keyTrailer);
        const bool hasTime = timeTrailer && active4(timeTrailer);
        if (m->hasCourtesyKeySig() != hasKey || m->trailer() != (hasKey || hasTime))
            return fail("Candidate courtesy presence not prepared");
        if (pending && ((hasKey && (!keySeg || !active4(keySeg))) || (hasTime && (!timeSeg || !active4(timeSeg)))))
            return fail("Candidate pending penultimate courtesy lacks an active following signature");

        const bool needTime = terminalGeometry && timeCourtesyRequested4(timeSeg);
        if (hasTime != needTime)
            return fail("Candidate courtesy presence differs from the following signature");

        const bool needKey = terminalGeometry && keyCourtesyRequested4(m, keySeg);
        if (hasKey != needKey)
            return fail("Candidate courtesy presence differs from the following signature");
        for (int st = 0; st < score->nstaves(); ++st) {
            const int track = st * VOICES;
            const Element* key = keySeg ? keySeg->element(track) : nullptr;
            const Element* time = timeSeg ? timeSeg->element(track) : nullptr;
            if (hasKey && (!ordinarySignature4(key)
                || !(toKeySig(keyTrailer->element(track))->keySigEvent() == toKeySig(key)->keySigEvent())))
                return fail("Candidate courtesy key does not match following signature");
            if (hasTime) {
                if (!ordinarySignature4(time)) return fail("Candidate courtesy time is not ordinary");
                const TimeSig* actual = toTimeSig(time);
                const TimeSig* courtesy = toTimeSig(timeTrailer->element(track));
                if (courtesy->timeSigType() != actual->timeSigType() || !courtesy->sig().identical(actual->sig())
                    || courtesy->stretch() != actual->stretch() || courtesy->numeratorString() != actual->numeratorString()
                    || courtesy->denominatorString() != actual->denominatorString())
                    return fail("Candidate courtesy time does not match following signature");
            }
        }
        const Segment* end = m->findSegmentR(SegmentType::EndBarLine, m->ticks());
        const BarLineType expected = hasKey ? BarLineType::DOUBLE : next ? BarLineType::NORMAL : BarLineType::END;
        if (!end || !active4(end) || !end->ticks().isZero() || !generatedBarlines4(end, expected)
            || std::count_if(end->elist().begin(), end->elist().end(),
                [](const Element* item) { return item != nullptr; }) != score->nstaves())
            return fail("Candidate signature end barline not prepared");
        for (int st = 0; st < score->nstaves(); ++st) {
            const int track = st * VOICES;
            const Element* bar = end->element(track);
            if (!toBarLine(bar)->el()->empty() || !signatureGeometryReady4(bar, end, track))
                return fail("Candidate signature end barline geometry not prepared");
        }
    }
    return true;
}

bool spacingRepeatBarlinesReady4(const System* system, SpacingPass4 pass, QString* reason)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->score()->nstaves() <= 0 || system->measures().empty()
        || (pass != SpacingPass4::CANDIDATE && pass != SpacingPass4::FINAL))
        return fail("Candidate repeat prefix/phase unavailable");
    bool repeats = false;
    std::vector<const Measure*> measures;
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mb->system() != system || mb->score() != system->score())
            return fail("Candidate repeat prefix ownership/frame boundary");
        const Measure* m = toMeasure(mb);
        if (!measures.empty() && measures.back()->next() != m)
            return fail("Candidate repeat prefix is not contiguous");
        measures.push_back(m);
        repeats = repeats || m->repeatStart() || m->repeatEnd();
    }
    if (!repeats) return true;
    // A measure outside this explicit prefix may still point at this System
    // from a previous layout. Never use that stale parent to infer inclusion.
    const auto barlineReady = [system](const Segment* segment, BarLineType type) {
        if (!segment || !active4(segment) || !segment->ticks().isZero()) return false;
        int count = 0;
        for (const Element* item : segment->elist()) {
            if (!item) continue;
            if (!item->isBarLine() || item->parent() != segment || item->score() != system->score()
                || item->track() < 0 || item->track() % VOICES
                || item->staffIdx() >= system->score()->nstaves()) return false;
            const BarLine* barline = toBarLine(item);
            const QRectF rect = barline->layoutRect();
            if (!barline->generated() || barline->barLineType() != type || !barline->visible()
                || !barline->addToSkyline() || !barline->el()->empty()
                || !(barline->mag() > 0) || !std::isfinite(barline->mag())
                || !(rect.width() > 0) || !(rect.height() > 0)
                || !std::isfinite(rect.x()) || !std::isfinite(rect.y())
                || !std::isfinite(rect.width()) || !std::isfinite(rect.height())
                || !std::isfinite(barline->pos().x()) || !std::isfinite(barline->pos().y())) return false;
            ++count;
        }
        return count == system->score()->nstaves();
    };
    for (size_t i = 0; i < measures.size(); ++i) {
        const Measure* m = measures[i];
        if (!ordinaryRepeatBoundary4(m, true))
            return fail("Candidate repeat grace/rest/cross-staff boundary");
        // A hidden terminal clef may become visible after accepting the next
        // measure. Its relocation needs a separate preparation adapter too.
        for (const Segment& segment : m->segments())
            if (segment.isClefType() && segment.rtick() == m->ticks())
                return fail("Candidate repeat terminal-clef boundary");
        if (m->repeatStart() && !barlineReady(m->findSegmentR(SegmentType::StartRepeatBarLine,
                                                             Fraction(0, 1)), BarLineType::START_REPEAT))
            return fail("Candidate start-repeat geometry not prepared");
        const Segment* end = m->findSegmentR(SegmentType::EndBarLine, m->ticks());
        // createEndBarLines(true) prepared the trial last and its predecessor.
        // Only earlier accepted predecessors have received the interior call.
        const bool interior = i + 1 < measures.size()
            && (pass == SpacingPass4::FINAL || i + 2 < measures.size());
        const bool enabled = !interior || m->repeatEnd() || !measures[i + 1]->repeatStart();
        if (!enabled) {
            if (end && end->enabled()) return fail("Candidate interior repeat barline not prepared");
            continue;
        }
        const BarLineType expected = m->repeatEnd() ? BarLineType::END_REPEAT
            : m->nextMeasure() ? BarLineType::NORMAL : BarLineType::END;
        if (!barlineReady(end, expected))
            return fail("Candidate terminal repeat barline not prepared");
    }
    return true;
}

bool buildCrossStaffContext4(const Beam* beam, CrossStaffContext4& result, QString* reason)
{
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!beam || !beam->score())
        return fail("Cross-staff beam is missing or unattached");
    if (!beam->cross())
        return fail("Beam is not flagged as cross-staff");
    if (beam->elements().size() < 3)
        return fail("Cross-staff beam has fewer than three members");
    if (beam->beamDirection() != Direction::AUTO)
        return fail("Cross-staff beam has an explicit direction");
    if (beam->userModified())
        return fail("Cross-staff beam has a manual adjustment");
    const Score* score = beam->score();
    const SpacingPaddingTable4 table(score);
    CrossStaffContext4 context;
    context.beam = beam;
    int upper = score->nstaves(), lower = -1;
    const Chord* previous = nullptr;
    for (const ChordRest* cr : beam->elements()) {
        if (!cr || !cr->isChord())
            return fail("Cross-staff context does not yet support beamed rests");
        const Chord* chord = toChord(cr);
        if (chord->score() != score || !chord->segment() || !chord->measure() || !chord->visible()
            || chord->beam() != beam || chord->notes().empty() || chord->notes().size() > 2 || chord->isGrace()
            || chord->tuplet() || chord->tremolo() || !chord->graceNotes().empty()
            || chord->isSmall() || chord->mag() != 1.0 || chord->noStem() || !chord->stem()
            || chord->isNudged() || chord->stemDirection() != Direction::AUTO
            || chord->durationType().type() != TDuration::DurationType::V_EIGHTH
            || chord->actualTicks() != Fraction(1, 8) || chord->segment()->ticks() <= Fraction(0, 1)
            || chord->segment()->ticks() > chord->actualTicks())
            return fail("Unadapted cross-staff chord, rhythm or manual context");
        const int staff = chord->vStaffIdx();
        if (staff < 0 || staff >= score->nstaves() || !score->staff(staff)->show()
            || !score->staff(staff)->isPitchedStaff(chord->tick()) || score->staff(staff)->lines(chord->tick()) != 5)
            return fail("Cross-staff context needs visible ordinary five-line staves");
        for (const Note* note : chord->notes()) {
            SpacingPairPolicy4 policy;
            if (!note->visible() || note->isSmall() || note->mirror() || !note->offset().isNull()
                || note->headGroup() != NoteHead::Group::HEAD_NORMAL || note->headType() != NoteHead::Type::HEAD_AUTO
                || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
                return fail("Unadapted cross-staff note context");
        }
        if (previous && (chord->measure() != previous->measure() || chord->track() != previous->track()
                         || chord->tick() != previous->tick() + previous->actualTicks()))
            return fail("Cross-staff context needs one adjacent stream within one measure");
        CrossStaffChord4 entry;
        entry.chord = chord;
        entry.visualStaff = staff;
        context.chords.push_back(entry);
        upper = std::min(upper, staff);
        lower = std::max(lower, staff);
        previous = chord;
    }
    if (lower != upper + 1)
        return fail("Cross-staff context needs exactly two adjacent staves");
    for (CrossStaffChord4& entry : context.chords) {
        const Chord* chord = entry.chord;
        // Evolution's initial stem sides are provisional. Infer only the
        // unambiguous within-staff case, as in the existing bounded adapter.
        for (const Note* note : chord->notes()) {
            if ((entry.visualStaff == upper && note->line() > 8) || (entry.visualStaff == lower && note->line() < 0))
                return fail("Interstaff note positions need a later stem-side adapter");
        }
        entry.up = entry.visualStaff == lower;
        const Note* note = entry.up ? chord->upNote() : chord->downNote();
        // Match Chord::stemPosX/layoutStem1 without changing the provisional
        // Chord::up(). Stem length and final vertical beam position are unused.
        const QPointF attach = entry.up ? note->stemUpSE() : note->stemDownNW();
        const qreal nominalWidth = chord->noteHeadWidth();
        const qreal anchor = attach.isNull() ? (entry.up ? nominalWidth : 0)
            : attach.x() - (entry.up ? note->headBodyWidth() - nominalWidth : 0);
        const qreal stemWidth = chord->stem()->lineWidth() * chord->mag();
        entry.stemX = chord->x() + anchor + (entry.up ? -.5 : .5) * stemWidth;
        if (!(stemWidth > 0) || !std::isfinite(stemWidth) || !std::isfinite(entry.stemX))
            return fail("Invalid cross-staff stem anchor");
        for (const Element* item : chord->segment()->elist()) {
            if (!item || item == chord)
                continue;
            if (!item->isChord() || !item->visible() || !item->staff()->show())
                return fail("Unadapted companion voice in cross-staff context");
            const Chord* companion = toChord(item);
            if (companion->beam() || companion->staffMove() || companion->isGrace() || companion->tuplet()
                || companion->tremolo() || companion->isSmall() || companion->notes().empty())
                return fail("Independent moving or decorated voices need a later cross-staff adapter");
            for (const Note* note : companion->notes()) {
                SpacingPairPolicy4 policy;
                if (!note->visible() || !spacingPairPolicy4(table, note, note, note->spatium(), policy))
                    return fail("Unadapted companion note context");
            }
        }
    }
    const qreal headWidth = score->noteHeadWidth();
    const qreal displacement = headWidth - score->styleP(Sid::stemWidth);
    qreal stemPadding = 0;
    if (!(headWidth > 0) || !(displacement > 0) || !std::isfinite(displacement)
        || !table.padding(ElementType::STEM, ElementType::STEM, stemPadding)
        || stemPadding < 0 || !std::isfinite(stemPadding))
        return fail("Invalid cross-staff spacing dimensions");
    for (size_t i = 1; i < context.chords.size(); ++i) {
        const CrossStaffChord4& a = context.chords[i - 1];
        const CrossStaffChord4& b = context.chords[i];
        CrossStaffPair4 pair;
        pair.first = a.chord;
        pair.second = b.chord;
        pair.adjacentSegments = a.chord->segment()->next(SegmentType::ChordRest) == b.chord->segment();
        pair.canAdjust = pair.adjacentSegments;
        for (int side = 0; side < 2; ++side) {
            const Chord* chord = side ? b.chord : a.chord;
            for (const Element* item : chord->segment()->elist()) {
                if (item && item != chord && item->isChordRest()
                    && (side || toChordRest(item)->actualTicks() <= chord->segment()->ticks()))
                    pair.canAdjust = false;
            }
        }
        if (a.up != b.up) {
            pair.widthOffset = pair.canAdjust ? (a.up ? displacement : -displacement) : 0;
            if (a.up) {
                if (pair.adjacentSegments)
                    pair.minimumWidth = std::max(2 * displacement, headWidth + stemPadding);
                // Source minStemDistOnNonAdjacentCross also applies to
                // adjacent segments. Keep it explicit for later lookback.
                pair.minimumStemDistance = std::max(qreal(0), a.stemX - b.stemX + stemPadding);
                if (pair.adjacentSegments)
                    pair.minimumWidth = std::max(pair.minimumWidth, pair.minimumStemDistance);
            }
        }
        context.pairs.push_back(pair);
    }
    result = context;
    if (reason) reason->clear();
    return true;
}

// Keep both notes in native chord layout (including fingering placement)
// and project their existing shapes normally. Only admit an invisible note
// here when it is an ordinary unison with a visible note in the same chord.
static bool hiddenUnison4(const Note* note, const Chord* chord)
{
    if (!chord->visible() || chord->isGrace() || chord->staffMove() || chord->notes().size() != 2)
        return false;
    const auto ordinaryHead = [](const Note* n) {
        return !n->isSmall() && n->offset().isNull() && !n->mirror()
            && n->headGroup() == NoteHead::Group::HEAD_NORMAL && n->headType() == NoteHead::Type::HEAD_AUTO;
    };
    if (!ordinaryHead(note))
        return false;
    for (const Note* other : chord->notes()) {
        if (other->visible() && other->pitch() == note->pitch() && other->line() == note->line()
            && other->tpc() == note->tpc() && ordinaryHead(other))
            return true;
    }
    return false;
}

// Called only after the existing note policy rejects a note. Keep the
// admission checks in spacingPairPolicy4; these labels explain the failure.
static QString notePolicyDiagnostic4(const Note* note, const Chord* chord, const SpacingContextCache4& cache)
{
    if (!note->visible()) return QStringLiteral("invisible note");
    if (note->track() < 0) return QStringLiteral("invalid note track");
    if (note->onTabStaff()) return QStringLiteral("tablature note");
    if (!spacingChordArticulationsSupported4(chord)) {
        QStringList marks;
        for (const Articulation* mark : chord->articulations()) {
            // subtype() groups above/below variants of the same mark.
            marks.push_back(mark ? QString::fromLatin1(Sym::id2name(static_cast<SymId>(mark->subtype())))
                                 : QStringLiteral("missing articulation"));
        }
        marks.sort();
        return QStringLiteral("chord articulation: ") + marks.join(QStringLiteral(" + "));
    }
    for (const Note* member : chord->notes()) {
        for (const Tie* tie : {member->tieFor(), member->tieBack()}) {
            TieObstacleDiagnostic4 detail;
            if (const char* rejection = ordinaryTieRejection4(tie, &detail)) {
                QString message = QStringLiteral("chord with tie: ") + QString::fromLatin1(rejection);
                if (detail.item && detail.item->isChord() && toChord(detail.item)->measure()) {
                    const Chord* obstacle = toChord(detail.item);
                    message += QStringLiteral(" {obstacle: m.%1, beat %2, tick %3, staff %4, voice %5: %6}")
                        .arg(obstacle->measure()->no() + 1).arg(obstacle->barbeat().second)
                        .arg(obstacle->tick().ticks()).arg(obstacle->vStaffIdx() + 1)
                        .arg(obstacle->track() % VOICES + 1).arg(QString::fromLatin1(detail.reason));
                }
                return message;
            }
        }
        if (!member->spannerFor().empty() || !member->spannerBack().empty())
            return QStringLiteral("chord with note spanner");
    }
    if (cache.slurPolicy() == SpacingSlurPolicy4::PREPARED && !cache.preparedSlursReady(chord))
        return QStringLiteral("unready prepared slur context");
    if (!(note->spatium() > 0) || !std::isfinite(note->spatium()) || !(note->mag() > 0) || !std::isfinite(note->mag()))
        return QStringLiteral("invalid note scale");
    return QStringLiteral("other note pair policy");
}

static bool buildSpacingPlanImpl4(System* system, SpacingPlan4& result, QString* reason, const SpacingPlanOptions4& options,
                                 const std::vector<GraceMergedGeometry4>* preparedMerged, const SpacingInput4* input,
                                 const SpacingAttachmentPreparation4* preparation,
                                 const SpacingPlan4* prefix = nullptr)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::PLAN);
    const auto fail = [&](const QString& message) { if (reason) *reason = message; return false; };
    if (!system || system->measures().empty())
        return fail("No system measures");
    if (!std::isfinite(options.squeezeFactor) || options.squeezeFactor < 0 || options.squeezeFactor > 1
        || !std::isfinite(options.stretchReduction) || options.stretchReduction < 0)
        return fail("Invalid spacing factors");
    Score* score = system->score();
    if (!score)
        return fail("No score");
    if (input && input->score() != score)
        return fail("Prepared musical inputs belong to another score");
    const Measure* predecessor = nullptr;
    const bool refresh = prefix && prefix->trialOnly;
    if (prefix) {
        if (!spacingCandidateSignaturesReady4(system, reason, true)) return false;
        const auto& measures = system->measures();
        if (measures.size() < 2 || prefix->measures.size() != measures.size() - (refresh ? 0 : 1)
            || !prefix->afterGraces.empty() || !prefix->mergedGraces.empty())
            return fail("Candidate courtesy has no valid prefix seed");
        const Measure* penultimate = toMeasure(measures[measures.size() - 2]);
        if (refresh) {
            if (penultimate->trailer() || prefix->measures.back().measure != measures.back()
                || !system->lastMeasure()->trailer())
                return fail("Accepted courtesy trial has not been finalized");
        } else {
            if (!penultimate->trailer())
                return fail("Candidate prefix seed has no pending penultimate courtesy");
            predecessor = penultimate;
        }
    }
    for (int st = 0; st < score->nstaves(); ++st) {
        if (!score->staff(st)->show() || !system->staff(st)->show())
            return fail("Hidden staves require a later adapter");
    }
    CrossStaffGeometry4 cross;
    if (!prepareCrossStaff4(system, cross, reason)) return false;
    SpacingGraceContext4 graces;
    std::vector<GraceAfterGeometry4> afters;
    std::vector<GraceMergedGeometry4> merged;
    // Grace packing/projection and the final system constraints are all
    // read-only. Borrow the same input owner and query-local memo throughout;
    // no standalone grace query silently drops the prepared attachment view.
    if (options.slurPolicy != SpacingSlurPolicy4::COMPATIBILITY
        && options.slurPolicy != SpacingSlurPolicy4::PREPARED) return fail("Invalid slur packing policy");
    if (options.slurPolicy == SpacingSlurPolicy4::PREPARED && !preparation)
        return fail("Prepared slur policy requires an attachment owner");
    SpacingContextCache4 contextCache = preparation ? SpacingContextCache4(*preparation, options.slurPolicy)
                                                  : SpacingContextCache4(input);
    if (!prepareGraces4(system, graces, afters, merged, reason, preparedMerged, contextCache)) return false;
    SpacingPlan4 plan;
    for (const auto& after : afters) plan.afterGraces.push_back(after.plan);
    for (const auto& joint : merged) plan.mergedGraces.push_back(joint.plan);
    std::vector<Geometry4> geometry;
    const SpacingPaddingTable4 table(score);
    bool hasManualSpace = false;
    bool hasRightAligned = false;
    for (MeasureBase* mb : system->measures()) {
        if (!mb->isMeasure() || toMeasure(mb)->isMMRest())
            return fail("Frames and MMRest measures are outside the common plan");
        Measure* measure = toMeasure(mb);
        plan.measures.push_back(SpacingMeasurePlan4(measure));
        for (Segment& segment : measure->segments()) {
            SpacingSegmentPlan4 entry(&segment);
            entry.ignored = !active4(&segment);
            const qreal leading = segment.extraLeadingSpace().val();
            if (!std::isfinite(leading))
                return fail("Invalid manual leading space");
            if (leading != 0) {
                for (int st = 0; st < score->nstaves(); ++st)
                    if (!score->staff(st)->isPitchedStaff(segment.tick()))
                        return fail("Manual leading space requires pitched staves");
                // The onset is a shared segment. Apply its edit
                // once for the system, regardless of how many staves play.
                if (!manualInteriorLeadingSpace4(&segment)
                    && !manualBarStartLeadingSpace4(segment, system->firstMeasure(), plan.segments, predecessor))
                    return fail("Manual leading space requires an interior onset or a supported bar start");
                hasManualSpace = true;
            }
            if (entry.ignored && segment.isChordRestType())
                return fail("Inactive rhythmic segments require a later adapter");
            Geometry4 g;
            if (!entry.ignored) {
                hasRightAligned = hasRightAligned || rightAligned4(&segment);
                if (courtesyType4(&segment) && (!segment.trailer()
                    || (measure != system->lastMeasure() && measure != predecessor)
                    || segment.rtick() != measure->ticks()))
                    return fail("Courtesy signatures require a terminal measure-end trailer");
                const Segment* next = segment.next();
                const bool carriesManualSpace = next && next->extraLeadingSpace().val() != 0
                    && manualInteriorLeadingSpace4(next);
                if (!eligibleType4(&segment) || (segment.trailer() && !courtesyType4(&segment))
                    || (segment.spacingWidthOffset4() != 0 && !cross.pairs.count(&segment)
                        && !carriesRightAlignmentOffset4(&segment) && !carriesManualSpace))
                    return fail("Unadapted segment type, trailer, manual space or cross-beam offset");
                if (segment.isBreathType() && (!segment.ticks().isZero() || segment.rtick() <= Fraction(0, 1)
                                               || segment.rtick() > measure->ticks()))
                    return fail("Breath spacing requires a zero-duration internal or end boundary");
                if (segment.isStartRepeatBarLineType()
                    && (!measure->repeatStart() || !segment.rtick().isZero() || !segment.ticks().isZero()))
                    return fail("Start-repeat spacing requires a zero-duration boundary");
                if (segment.isStartRepeatBarLineType() && !ordinaryRepeatBoundary4(measure, true))
                    return fail("Start-repeat spacing requires supported surrounding rests and chords");
                for (const Element* annotation : segment.annotations()) {
                    if (!annotation)
                        continue;
                    // Fermatas are absent from the horizontal staff shape.
                    // Native layout places them on their anchors after spacing.
                    if (!annotation->isSystemText() && !annotation->isStaffText() && !annotation->isRehearsalMark()
                        && !annotation->isTempoText() && !annotation->isDynamic() && !annotation->isFermata()
                        && !spacingPedalSymbol4(annotation)) {
                        if (!reason) return false;
                        QString name = annotation->_name();
                        if (annotation->isSymbol())
                            name += QStringLiteral(" (%1)").arg(QString::fromLatin1(
                                Sym::id2name(static_cast<const Symbol*>(annotation)->sym())));
                        return fail(QStringLiteral("Annotations require a later adapter: %1 [measure %2]")
                                    .arg(name).arg(measure->no() + 1));
                    }
                }
                for (const Element* item : segment.elist()) {
                    if (item && segment.isStartRepeatBarLineType()) {
                        if (!item->isBarLine()) return fail("Unexpected start-repeat item");
                        const BarLine* barline = toBarLine(item);
                        if (barline->barLineType() != BarLineType::START_REPEAT
                            || !barline->staff() || !barline->staff()->isPitchedStaff(barline->tick())
                            || !barline->visible() || !barline->addToSkyline() || !barline->el()->empty()
                            || !(barline->mag() > 0) || !std::isfinite(barline->mag())
                            || !(barline->layoutRect().width() > 0) || !(barline->layoutRect().height() > 0))
                            return fail("Unadapted start-repeat glyph, staff, decoration or dimensions");
                    }
                    if (item && segment.isBreathType()) {
                        if (!item->isBreath()) return fail("Unexpected breath-segment item");
                        const Breath* breath = toBreath(item);
                        const bool known = std::any_of(Breath::breathList.begin(), Breath::breathList.end(),
                            [breath](const BreathType& type) { return type.id == breath->symId(); });
                        if (!known || !breath->staff() || !breath->staff()->isPitchedStaff(breath->tick())
                            || !breath->visible() || !breath->addToSkyline()
                            || !(breath->mag() > 0) || !std::isfinite(breath->mag())
                            || !(breath->bbox().width() > 0) || !(breath->bbox().height() > 0))
                            return fail("Unadapted breath glyph, staff, visibility or dimensions");
                    }
                    if (item && segment.isClefType()) {
                        if (!item->isClef()) return fail("Unexpected clef-segment item");
                        const Clef* clef = toClef(item);
                        const ClefType type = clef->clefType();
                        if (type == ClefType::INVALID || type == ClefType::MAX
                            || ClefInfo::staffGroup(type) != StaffGroup::STANDARD
                            || !clef->visible() || !clef->addToSkyline()
                            || !(clef->mag() > 0) || !std::isfinite(clef->mag()))
                            return fail("Unadapted clef visibility, staff family or magnitude");
                    }
                    if (item && item->isRest()) {
                        const Rest* rest = toRest(item);
                        if (rest->isGap())
                            return fail("Gap rests require a later adapter");
                        if (rest->isFullMeasureRest() && !ordinaryMeasureRest4(rest))
                            return fail("Decorated, hidden or moved measure rests require a later adapter");
                    }
                    if (!item || !item->isChord())
                        continue;
                    const Chord* chord = toChord(item);
                    if (chord->tremolo())
                        return fail("Tremolo spacing requires a later adapter");
                    if ((chord->staffMove() != 0 || (chord->beam() && chord->beam()->cross()))
                        && !cross.chords.count(chord))
                        return fail("Cross-staff geometry requires its later stem constraints");
                    if (!chord->graceNotes().empty() && !graces.contains(chord) && !graces.containsAfter(chord))
                        return fail("Unadapted grace-note rhythm");
                    for (const Note* note : chord->notes()) {
                        SpacingPairPolicy4 policy;
                        if ((!note->visible() && !hiddenUnison4(note, chord))
                            || !spacingPairPolicy4(table, note, note, note->spatium(), policy, &graces, &contextCache))
                            return fail(reason ? QStringLiteral("Note policy: ") + notePolicyDiagnostic4(note, chord, contextCache)
                                               + QStringLiteral(" [measure %1, beat %2, tick %3, staff %4, voice %5]")
                                                   .arg(measure->no() + 1).arg(note->barbeat().second).arg(segment.tick().ticks())
                                                   .arg(note->staffIdx() + 1).arg(note->track() % VOICES + 1)
                                               : QString());
                        for (const Tie* tie : {note->tieFor(), note->tieBack()}) {
                            if (!tie) continue;
                            const auto& measures = system->measures();
                            const Note* other = tie->startNote() == note ? tie->endNote() : tie->startNote();
                            plan.hasOrdinaryTies = true;
                            if (std::find(measures.begin(), measures.end(), other->chord()->measure()) == measures.end()) {
                                const qreal sp = score->spatium();
                                const qreal grip = 0.25 * std::max(note->spatium(), sp);
                                const qreal minimum = score->styleP(Sid::minTieLength) * note->mag();
                                if (tie->startNote() == note)
                                    g.outgoingTieRoom = std::max(g.outgoingTieRoom, minimum + grip + 0.25 * sp);
                                else
                                    g.incomingTieRoom = std::max(g.incomingTieRoom, minimum + grip + sp);
                                continue;
                            }
                            if (tie->startNote() == note)
                                g.outgoingTie = true;
                            else {
                                const Segment* start = other->chord()->segment();
                                if (!g.earliestTieStart || start->tick() < g.earliestTieStart->tick())
                                    g.earliestTieStart = start;
                            }
                        }
                    }
                }
                QString geometryReason;
                if (!cacheGeometry4(&segment, table, g, cross, graces, afters, merged, contextCache,
                                    reason ? &geometryReason : nullptr))
                    return fail(reason ? QStringLiteral("Geometry: ") + geometryReason : QString());
                if (segment.isChordRestType()) {
                    if (!(segment.ticks() > Fraction(0, 1)))
                        return fail("Chord/rest duration is not ready");
                    entry.stretch = segmentSpacingStretch4(&segment);
                    if (!(entry.stretch > 0) || !std::isfinite(entry.stretch))
                        return fail("Invalid rhythmic stretch");
                }
            }
            const auto correction = cross.pairs.find(&segment);
            if (correction != cross.pairs.end()) entry.widthOffset = correction->second.widthOffset;
            plan.segments.push_back(entry);
            geometry.push_back(g);
        }
    }
    if (hasManualSpace && (hasRightAligned || !plan.afterGraces.empty() || !plan.mergedGraces.empty()
        || !graces.mains.empty() || !cross.chords.empty()))
        return fail("Manual leading space with grace, cross-staff or right-aligned geometry");
    if (plan.hasOrdinaryTies && (hasManualSpace || prefix))
        return fail("Tie clearance requires a full plan without manual leading space");
    // Match the native split-tie anchors: the last header glyph plus
    // one space, and the first active segment after the last rhythmic
    // onset minus a quarter space. Keep unfamiliar edges on fallback.
    if (plan.hasOrdinaryTies) {
        const size_t count = plan.segments.size();
        size_t firstRhythm = count, lastRhythm = count, header = count, boundary = count, previous = count;
        for (size_t i = 0; i < count; ++i) {
            const auto& entry = plan.segments[i];
            if (entry.ignored) continue;
            if (entry.segment->isChordRestType()) {
                if (firstRhythm == count) {
                    firstRhythm = i;
                    header = previous;
                }
                lastRhythm = i;
                boundary = count;
            }
            else if (lastRhythm != count && boundary == count)
                boundary = i;
            previous = i;
        }
        for (size_t i = 0; i < count; ++i) {
            Geometry4& g = geometry[i];
            if (g.incomingTieRoom > 0) {
                if (i != firstRhythm || header == count || !headerType4(plan.segments[header].segment))
                    return fail("Incoming split tie needs a prepared system header");
                g.incomingTieHeader = plan.segments[header].segment;
                g.earliestTieStart = g.incomingTieHeader;
            }
            if (g.outgoingTieRoom > 0) {
                if (boundary == count || !plan.segments[boundary].segment->isEndBarLineType()
                    || plan.segments[i].segment->measure() != plan.segments[boundary].segment->measure())
                    return fail("Outgoing split tie needs a prepared system end barline");
                g.outgoingTieBoundary = plan.segments[boundary].segment;
                if (!geometry[boundary].earliestTieStart)
                    geometry[boundary].earliestTieStart = plan.segments[i].segment;
            }
        }
    }
    const qreal manualSpatium = hasManualSpace ? system->spatium() : 0;
    const auto rhythmicAdvance = [&options](const SpacingSegmentPlan4& entry) {
        return 3.5 * entry.segment->spatium() * entry.stretch * options.stretchReduction;
    };
    qreal cursor = system->leftMargin();
    bool placed = false;
    size_t firstNew = 0;
    size_t collisionFirst = 0;
    size_t firstMutableMeasure = 0;
    if (prefix) {
        // Studio updates only the predecessor/new-measure pair. Earlier
        // measures retain both their positions and their occupied widths.
        const size_t frozenMeasures = prefix->measures.size() - (refresh ? 1 : 0);
        firstMutableMeasure = frozenMeasures - 1;
        const Segment* lastActive = nullptr;
        bool removedCourtesy = false;
        for (size_t mi = 0; mi < frozenMeasures; ++mi) {
            auto& measure = plan.measures[mi];
            const auto& savedMeasure = prefix->measures[mi];
            const bool finalizedPredecessor = refresh && mi == firstMutableMeasure;
            if (mi == firstMutableMeasure) collisionFirst = firstNew;
            if (measure.measure != savedMeasure.measure || !std::isfinite(savedMeasure.x)
                || !std::isfinite(savedMeasure.width) || savedMeasure.width <= 0
                || qAbs(savedMeasure.x - cursor) > 1e-6)
                return fail("Candidate prefix measure identity or extent changed");
            measure = savedMeasure;
            placed = false;
            for (; firstNew < plan.segments.size()
                && plan.segments[firstNew].segment->measure() == measure.measure; ++firstNew) {
                auto& current = plan.segments[firstNew];
                if (firstNew >= prefix->segments.size())
                    return fail("Candidate prefix segment list changed");
                const auto& saved = prefix->segments[firstNew];
                if (saved.segment != current.segment)
                    return fail("Candidate prefix segment identity changed");
                if (saved.ignored != current.ignored) {
                    if (finalizedPredecessor && current.ignored && courtesyType4(current.segment)
                        && current.segment->trailer() && current.segment->rtick() == measure.measure->ticks()
                        && lastActive && lastActive->measure() == measure.measure && lastActive->isEndBarLineType()) {
                        if (!removedCourtesy && (!std::isfinite(saved.x) || qAbs(cursor - saved.x) > 1e-6))
                            return fail("Accepted trial's former courtesy edge changed");
                        removedCourtesy = true;
                        continue;
                    }
                    return fail("Candidate prefix segment identity or visibility changed");
                }
                if (current.ignored) continue;
                const qreal expectedOffset = hasManualSpace && current.segment->isChordRestType()
                    ? manualSpringOffset4(current.segment, rhythmicAdvance(current), manualSpatium) : 0;
                if (!std::isfinite(saved.x) || !std::isfinite(saved.width) || saved.width < 0
                    || saved.widthOffset != expectedOffset || saved.width < saved.widthOffset
                    || saved.stretch != current.stretch
                    || saved.x < cursor - 1e-6 || (placed && qAbs(saved.x - cursor) > 1e-6))
                    return fail("Candidate prefix segment positions or advances are invalid");
                current.x = saved.x;
                current.width = saved.width;
                current.widthOffset = saved.widthOffset;
                cursor = saved.x + saved.width;
                lastActive = current.segment;
                placed = true;
            }
            // Studio loads the surviving end barline's saved advance even
            // when the courtesy that used to follow it has been disabled.
            // Reconstruct this measure's width below with the new barline.
            if (!placed || !std::isfinite(cursor)
                || (!finalizedPredecessor && qAbs(cursor - (measure.x + measure.width)) > 1e-6))
                return fail("Candidate prefix measure has no matching segment edge");
        }
        if (refresh) {
            if (!removedCourtesy || !lastActive || !lastActive->isEndBarLineType()
                || lastActive->measure() != plan.measures[firstMutableMeasure].measure
                || firstNew == plan.segments.size() || firstNew >= prefix->segments.size()
                || prefix->segments[firstNew].segment != plan.segments[firstNew].segment)
                return fail("Accepted trial has no matching finalized courtesy boundary");
        } else {
            if (firstNew != prefix->segments.size() || firstNew == plan.segments.size()
                || !lastActive || !courtesyType4(lastActive) || !std::isfinite(prefix->width)
                || qAbs(cursor - prefix->width) > 1e-6)
                return fail("Candidate prefix has no matching terminal courtesy edge");
            plan.trialOnly = true;
        }
    }
    for (size_t i = firstNew; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored) {
            current.x = cursor;
            continue;
        }
        qreal positionedX = cursor;
        if (!placed) {
            qreal offset = 0;
            if (current.segment->isChordRestType()) {
                offset = std::max(qreal(0), -geometry[i].leftEdge);
                offset += score->styleP(geometry[i].accidentals ? Sid::barAccidentalDistance : Sid::barNoteDistance);
                // Studio default for its style absent from Evolution.
                offset = std::max(offset, 1.25 * current.segment->spatium());
            }
            else if (current.segment->isHeaderClefType() || current.segment->isClefType())
                offset = score->styleP(Sid::clefLeftMargin);
            else if (current.segment->isKeySigType())
                offset = score->styleP(Sid::keysigLeftMargin);
            else if (current.segment->isTimeSigType())
                offset = score->styleP(Sid::timesigLeftMargin);
            cursor += offset * options.squeezeFactor;
            positionedX = cursor;
            placed = true;
        }
        else {
            qreal x = -std::numeric_limits<qreal>::max();
            // A tie or beam neighbor can lie beyond the ordinary lookback.
            const Segment* requiredPrevious = geometry[i].earliestTieStart;
            for (const auto& pair : cross.pairs) {
                if (pair.second.minimumStemDistance > 0 && pair.second.second->segment() == current.segment
                    && (!requiredPrevious || pair.second.first->tick() < requiredPrevious->tick()))
                    requiredPrevious = pair.second.first->segment();
            }
            int crCount = 0;
            for (size_t j = i; j > collisionFirst;) {
                --j;
                SpacingSegmentPlan4& previous = plan.segments[j];
                if (previous.ignored)
                    continue;
                if (previous.segment->isChordRestType())
                    ++crCount;
                qreal distance = 0;
                if (!distance4(table, previous, geometry[j], current, geometry[i], plan.measures.front().measure,
                               options.squeezeFactor, distance, cross, graces, contextCache))
                    return fail("Unsupported segment distance");
                x = std::max(x, previous.x + distance);
                if (x > cursor) {
                    if (crCount > 0) {
                        const qreal increase = (x - cursor) / crCount;
                        qreal movement = increase;
                        for (size_t k = j + 1; k < i; ++k) {
                            if (plan.segments[k].ignored)
                                continue;
                            plan.segments[k].x += movement;
                            if (plan.segments[k].segment->isChordRestType())
                                movement += increase;
                        }
                    }
                    cursor = x;
                }
                const Fraction previousEnd = previous.segment->tick() + previous.segment->ticks();
                if (previousEnd < current.segment->tick()
                    && (!requiredPrevious || previous.segment->tick() <= requiredPrevious->tick())) {
                    if (!current.segment->isChordRestType()
                        || cursor + geometry[i].leftEdge > previous.x + geometry[j].minRight
                        || (previous.segment->measure() != current.segment->measure()
                            && previous.segment->measure() != current.segment->measure()->prevMeasure()))
                        break;
                }
            }
            // Studio spaces right-aligned marks from collision geometry,
            // independently of the rhythmic cursor. Reserving the entire
            // cursor advance here would widen the surrounding note interval.
            positionedX = rightAligned4(current.segment) ? x : cursor;
        }
        // Studio moves the onset after the collision walk, leaving the
        // rhythmic cursor alone. The previous spring carries the edit so
        // justification stretches only its rhythmic portion.
        current.x = positionedX + current.segment->extraLeadingSpace().val() * manualSpatium;
        if (current.segment->isChordRestType()) {
            if (current.segment->measure() == plan.measures.front().measure && current.segment->rtick().isZero()) {
                const qreal minimum = system->leftMargin() + 1.25 * current.segment->spatium();
                cursor = std::max(cursor, minimum);
                current.x = std::max(current.x, minimum);
            }
            qreal advance = rhythmicAdvance(current) + current.widthOffset;
            const auto correction = cross.pairs.find(current.segment);
            if (correction != cross.pairs.end() && correction->second.minimumWidth > 0)
                advance = std::max(advance, correction->second.minimumWidth);
            if (advance < 0) return fail("Cross-staff advance needs stronger compression support");
            if (hasManualSpace)
                current.widthOffset += manualSpringOffset4(current.segment, advance, manualSpatium);
            cursor += advance;
        }
    }
    if (!placed)
        return fail("No active segments");
    if (!moveRightAligned4(plan, geometry, table, options.squeezeFactor, cross, graces, contextCache))
        return fail("Unadapted right-aligned segment geometry");
    plan.measures.front().x = system->leftMargin();
    std::map<Measure*, size_t> measureIndex;
    for (size_t i = 0; i < plan.measures.size(); ++i)
        measureIndex[plan.measures[i].measure] = i;
    for (size_t i = collisionFirst; i < plan.segments.size(); ++i) {
        SpacingSegmentPlan4& current = plan.segments[i];
        if (current.ignored)
            continue;
        size_t next = i + 1;
        while (next < plan.segments.size() && plan.segments[next].ignored)
            ++next;
        const size_t mi = measureIndex[current.segment->measure()];
        if (next == plan.segments.size()) {
            current.width = geometry[i].minRight;
            if (current.segment->trailer())
                current.width += score->styleP(Sid::systemTrailerRightMargin);
            plan.measures[mi].width = current.x + current.width - plan.measures[mi].x;
        }
        else {
            SpacingSegmentPlan4& following = plan.segments[next];
            const bool sameMeasure = current.segment->measure() == following.segment->measure();
            const bool leadingHeader = leadingNonRhythmic4(following.segment) && !current.segment->isType(SegmentType::BarLineType);
            current.width = sameMeasure || following.segment->isStartRepeatBarLineType() || leadingHeader
                ? following.x - current.x : geometry[i].minRight;
            if (!sameMeasure) {
                plan.measures[mi].width = current.x + current.width - plan.measures[mi].x;
                plan.measures[measureIndex[following.segment->measure()]].x = current.x + current.width;
                if (following.segment->extraLeadingSpace().val() != 0 && following.segment->rtick().isZero()
                    && following.x < current.x + current.width)
                    return fail("Manual leading space moves the onset before its barline");
            }
        }
        if (current.width < 0 || !std::isfinite(current.width)
            || (hasManualSpace && current.width < current.widthOffset))
            return fail("Invalid reconstructed segment width");
    }
    if (options.minimumMeasureWidths) {
        const qreal available = system->width() - system->leftMargin();
        for (size_t mi = firstMutableMeasure; mi < plan.measures.size(); ++mi) {
            SpacingMeasurePlan4& measure = plan.measures[mi];
            qreal minimum = score->styleP(Sid::minMeasureWidth);
            const qreal cap = available > 0 ? available : minimum;
            minimum = std::min(minimum, cap);
            if (measure.measure->ticks() < measure.measure->timesig())
                minimum *= (measure.measure->ticks() / measure.measure->timesig()).toDouble();
            for (size_t i = 0; i < plan.segments.size(); ++i) {
                const SpacingSegmentPlan4& firstCR = plan.segments[i];
                if (firstCR.ignored || firstCR.segment->measure() != measure.measure || !firstCR.segment->isChordRestType())
                    continue;
                if (firstCR.segment != measure.measure->firstEnabled()) {
                    const qreal start = firstCR.x - measure.x - geometry[i].minLeft
                        - score->styleP(geometry[i].accidentals ? Sid::barAccidentalDistance : Sid::barNoteDistance);
                    minimum = std::min(minimum + start, cap);
                }
                break;
            }
            if (minimum > measure.width) {
                const qreal extra = minimum - measure.width;
                if (!stretchMeasure4(plan, measure, extra))
                    return fail("Minimum-width springs cannot be solved");
                const qreal delta = reflowMeasure4(plan, measure);
                if (qAbs(delta - extra) > 1e-6 * measure.measure->spatium())
                    return fail("Minimum-width reconstruction lost space");
                for (size_t j = mi + 1; j < plan.measures.size(); ++j)
                    moveMeasure4(plan, plan.measures[j], delta);
            }
        }
    }
    for (const SpacingMeasurePlan4& measure : plan.measures) {
        if (!std::isfinite(measure.x) || !std::isfinite(measure.width) || measure.width <= 0)
            return fail("Invalid reconstructed measure width");
    }
    plan.width = plan.measures.back().x + plan.measures.back().width;
    result = plan;
    if (reason)
        reason->clear();
    return true;
}

bool buildSpacingPlan4(System* system, SpacingPlan4& result, QString* reason, const SpacingPlanOptions4& options,
                       const std::vector<GraceMergedGeometry4>* preparedMerged, const SpacingInput4* input)
{
    return buildSpacingPlanImpl4(system, result, reason, options, preparedMerged, input, nullptr);
}

bool buildSpacingPlan4(System* system, SpacingPlan4& result, const SpacingAttachmentPreparation4& preparation,
                       QString* reason, const SpacingPlanOptions4& options,
                       const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    return buildSpacingPlanImpl4(system, result, reason, options, preparedMerged, &preparation.input(), &preparation);
}

static bool squeezeSpacingPlanImpl4(System* system, qreal targetWidth, SpacingPlan4& result, QString* reason,
                                   const std::vector<GraceMergedGeometry4>* preparedMerged,
                                   const SpacingAttachmentPreparation4* preparation, SpacingSlurPolicy4 policy,
                                   bool retainedOverfullGroup = false)
{
    if (!std::isfinite(targetWidth) || targetWidth <= 0) {
        if (reason)
            *reason = "Invalid squeeze target";
        return false;
    }
    SpacingPlan4 candidate;
    const SpacingInput4* input = preparation ? &preparation->input() : nullptr;
    SpacingPlanOptions4 natural;
    natural.slurPolicy = policy;
    if (!buildSpacingPlanImpl4(system, candidate, reason, natural, preparedMerged, input, preparation))
        return false;
    if (candidate.width <= targetWidth) {
        result = candidate;
        return true;
    }
    // Studio 4.7.5 squeezeSystemToFit recomputes from native geometry each
    // pass, with minimum-measure widths overridden. For a retained group,
    // also try the two tighter padding levels, with nonnegative advances.
    const bool tightGroup = retainedOverfullGroup && !candidate.hasOrdinaryTies && system->score()->nstaves() == 1
        && candidate.afterGraces.empty() && candidate.mergedGraces.empty()
        && std::all_of(candidate.segments.begin(), candidate.segments.end(), [](const SpacingSegmentPlan4& s) {
            if (s.ignored) return true;
            if (s.widthOffset != 0 || rightAligned4(s.segment)) return false;
            if (!s.segment->isChordRestType())
                return headerType4(s.segment) || s.segment->isEndBarLineType() || s.segment->isBeginBarLineType();
            for (const Element* item : s.segment->elist())
                if (item && !item->isChord()) return false;
            return true;
        });
    SpacingPlanOptions4 options;
    options.slurPolicy = policy;
    options.minimumMeasureWidths = false;
    options.stretchReduction = 1 - 0.33;
    options.squeezeFactor = 1 - 0.2;
    for (int pass = 0; pass < (tightGroup ? 5 : 3); ++pass) {
        if (pass >= 3) {
            options.stretchReduction = 0;
            options.squeezeFactor = pass == 3 ? 0.2 : 0;
        }
        if (!buildSpacingPlanImpl4(system, candidate, reason, options, preparedMerged, input, preparation))
            return false;
        if (candidate.width <= targetWidth) {
            result = candidate;
            return true;
        }
        options.stretchReduction -= 0.33;
        options.squeezeFactor -= 0.2;
    }
    if (tightGroup) {
        // Last resort in Studio: reduce only rhythmic advances and rebuild
        // segment and measure positions together, even if notes overlap.
        for (int percent = 95; percent > 0; percent -= 5) {
            for (auto& s : candidate.segments)
                if (!s.ignored && s.segment->isChordRestType())
                    s.width *= percent / 100.0;
            qreal edge = system->leftMargin();
            for (auto& m : candidate.measures) {
                moveMeasure4(candidate, m, edge - m.x);
                reflowMeasure4(candidate, m);
                edge += m.width;
            }
            candidate.width = edge;
            if (edge <= targetWidth) {
                result = std::move(candidate);
                return true;
            }
        }
    }
    if (reason)
        *reason = "Target needs stronger compression than the adapted squeeze passes";
    return false;
}

bool squeezeSpacingPlan4(System* system, qreal targetWidth, SpacingPlan4& result, QString* reason,
                         const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    return squeezeSpacingPlanImpl4(system, targetWidth, result, reason, preparedMerged, nullptr, SpacingSlurPolicy4::COMPATIBILITY);
}

bool squeezeSpacingPlan4(System* system, qreal targetWidth, SpacingPlan4& result,
                         const SpacingAttachmentPreparation4& preparation, QString* reason,
                         const std::vector<GraceMergedGeometry4>* preparedMerged, SpacingSlurPolicy4 policy)
{
    return squeezeSpacingPlanImpl4(system, targetWidth, result, reason, preparedMerged, &preparation, policy);
}

bool spacingSystemJustified4(const Measure* last, const MeasureBase* next,
                             qreal naturalWidth, qreal availableWidth, qreal fillLimit)
{
    if (last && last->sectionBreak()) next = nullptr;
    while (next) {
        if (next->isMeasure() || next->isHBox()) return true;
        if (next->sectionBreak()) break;
        next = next->next();
    }
    return !(availableWidth > 0 && naturalWidth / availableWidth < fillLimit);
}

bool selectSystemSpacingPlan4(System* system, const SpacingSystemRequest4& request,
                              const SpacingAttachmentPreparation4& attachments, SpacingPlan4& result,
                              QString* reason, const std::vector<GraceMergedGeometry4>* preparedMerged)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || system->measures().empty()) return fail("Missing spacing system");
    if (request.pass != SpacingPass4::CANDIDATE && request.pass != SpacingPass4::FINAL)
        return fail("Invalid spacing system pass");
    if (!(request.availableWidth > 0) || !std::isfinite(request.availableWidth))
        return fail("Invalid spacing system width");
    if (request.pass == SpacingPass4::FINAL && !std::isfinite(request.fillLimit))
        return fail("Invalid spacing system fill limit");
    if (request.pass == SpacingPass4::FINAL && request.previousPrefix)
        return fail("Final spacing cannot use a pending courtesy seed");
    SpacingPlan4 plan;
    if (request.pass == SpacingPass4::CANDIDATE) {
        SpacingPlanOptions4 options;
        options.slurPolicy = request.slurPolicy;
        if (!buildSpacingPlanImpl4(system, plan, reason, options, preparedMerged, &attachments.input(),
                                  &attachments, request.previousPrefix)) return false;
    }
    else {
        if (!squeezeSpacingPlanImpl4(system, request.availableWidth, plan, reason, preparedMerged,
                                    &attachments, request.slurPolicy, request.retainedOverfullGroup)) return false;
        if (spacingSystemJustified4(system->lastMeasure(), request.next, plan.width, request.availableWidth,
                                    request.fillLimit) && !justifySpacingPlan4(plan, request.availableWidth))
            return fail("Final plan justification rejected");
    }
    result = std::move(plan);
    if (reason) reason->clear();
    return true;
}

bool justifySpacingPlan4(SpacingPlan4& target, qreal targetWidth)
{
    if (target.measures.empty() || !std::isfinite(targetWidth) || targetWidth < target.width)
        return false;
    if (targetWidth == target.width)
        return true;
    SpacingPlan4 plan = target;
    std::vector<SpacingSpring4> springs;
    for (const SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.stretch > 0)
            springs.push_back({ segment.width - segment.widthOffset, segment.stretch });
    }
    const qreal force = spacing4SpringForce(springs, targetWidth - plan.width);
    if (!(force > 0))
        return false;
    for (SpacingSegmentPlan4& segment : plan.segments) {
        if (!segment.ignored && segment.stretch > 0)
            segment.width = std::max(segment.width - segment.widthOffset, force * segment.stretch) + segment.widthOffset;
    }
    qreal movement = 0;
    for (SpacingMeasurePlan4& measure : plan.measures) {
        moveMeasure4(plan, measure, movement);
        movement += reflowMeasure4(plan, measure);
    }
    plan.width = plan.measures.back().x + plan.measures.back().width;
    if (qAbs(plan.width - targetWidth) > 1e-6)
        return false;
    target = plan;
    return true;
}

bool spacingPlanPublicationValid4(const System* system, const SpacingPlan4& plan, QString* reason)
{
    const auto fail = [reason](const char* text) { if (reason) *reason = text; return false; };
    if (!system || !system->score() || plan.measures.empty() || !(plan.width > 0) || !std::isfinite(plan.width))
        return fail("Invalid publication system or width");
    if (plan.trialOnly) return fail("Pending courtesy trial cannot be published");
    std::map<const Segment*, qreal> segmentX;
    std::set<const Chord*> mains;
    std::vector<const Rest*> measureRests;
    size_t mi = 0, si = 0;
    qreal edge = system->leftMargin();
    for (const MeasureBase* mb : system->measures()) {
        if (!mb || !mb->isMeasure() || mi >= plan.measures.size()) return fail("Changed publication measure domain");
        const Measure* measure = toMeasure(mb);
        const auto& m = plan.measures[mi++];
        if (m.measure != measure || measure->system() != system || !std::isfinite(m.x)
            || !(m.width > 0) || !std::isfinite(m.width) || !std::isfinite(edge)
            || qAbs(m.x - edge) > 1e-6) return fail("Invalid publication measure placement");
        edge = m.x + m.width;
        for (const Segment& segment : measure->segments()) {
            if (si >= plan.segments.size()) return fail("Incomplete publication segment domain");
            const auto& s = plan.segments[si++];
            if (s.segment != &segment || !segmentX.emplace(&segment, s.x - m.x).second || s.ignored != !active4(&segment)
                || !std::isfinite(s.x) || !std::isfinite(s.width) || s.width < 0
                || !std::isfinite(s.x - m.x)
                || !std::isfinite(s.stretch) || s.stretch < 0 || !std::isfinite(s.widthOffset)
                || (!s.ignored && segment.isChordRestType() && !(s.stretch > 0)))
                return fail("Invalid publication segment placement");
            if (!s.ignored) for (const Element* item : segment.elist()) {
                if (item && item->isChord()) mains.insert(toChord(item));
                if (item && item->isRest() && toRest(item)->isFullMeasureRest()) {
                    if (!ordinaryMeasureRest4(toRest(item))) return fail("Changed publication measure-rest context");
                    measureRests.push_back(toRest(item));
                }
            }
        }
    }
    if (mi != plan.measures.size() || si != plan.segments.size() || !std::isfinite(edge)
        || qAbs(edge - plan.width) > 1e-6) return fail("Incomplete publication plan extent");
    // Measure::layoutElementsAfterSpacing centers these bodies after segment
    // publication. Validate that exact conversion now, before any native write.
    // Admitted rests start at tick zero: preceding shapes are fixed headers,
    // and the following non-rhythmic boundary is a barline/courtesy signature.
    for (const Rest* rest : measureRests) {
        const Segment* segment = rest->segment();
        qreal x1 = 0, x2 = 0;
        for (const auto& m : plan.measures) if (m.measure == rest->measure()) x2 = m.width;
        const Segment* previous = segment->prevActive();
        while (previous && previous->allElementsInvisible()) previous = previous->prevActive();
        for (const Segment* s = previous; s; s = s->prevActive()) {
            const qreal right = segmentX.at(s) + s->minRight();
            if (!std::isfinite(right)) return fail("Invalid publication rest header extent");
            x1 = std::max(x1, right);
        }
        for (const Segment* s = segment->nextActive(); s; s = s->nextActive()) {
            if (!s->isChordRestType() && s->element(rest->staffIdx() * VOICES)) {
                x2 = segmentX.at(s) - s->minLeft();
                break;
            }
        }
        const qreal x = (x2 - x1 - rest->width()) * .5 + x1 - segmentX.at(segment) - rest->bbox().x();
        if (!std::isfinite(x2) || !std::isfinite(x) || !std::isfinite(x + rest->offset().x()))
            return fail("Invalid publication centered-rest position");
    }
    std::set<const Chord*> owners, members;
    const auto positions = [&](const std::vector<GraceAfterPosition4>& written, const QVector<Chord*>& expected,
                               const Segment* target) {
        const auto targetX = segmentX.find(target);
        if (targetX == segmentX.end()) return false;
        if (written.size() != static_cast<size_t>(expected.size())) return false;
        for (size_t i = 0; i < written.size(); ++i) {
            const Chord* chord = expected[int(i)];
            if (!chord || written[i].chord != chord || !std::isfinite(written[i].x)
                || !chord->parent() || !chord->parent()->isChord()
                || !mains.count(toChord(chord->parent())) || !std::isfinite(toChord(chord->parent())->x())
                || !members.insert(chord).second) return false;
            const Chord* parent = toChord(chord->parent());
            const auto parentX = segmentX.find(parent->segment());
            if (parentX == segmentX.end()
                || !std::isfinite(targetX->second - parentX->second - parent->x() + written[i].x)) return false;
        }
        return true;
    };
    for (const auto& group : plan.afterGraces) {
        if (!mains.count(group.owner) || !segmentX.count(group.target) || !owners.insert(group.owner).second
            || !positions(group.positions, group.owner->graceNotesAfter(), group.target))
            return fail("Invalid publication after-grace ownership");
    }
    for (const auto& group : plan.mergedGraces) {
        if (!mains.count(group.owner) || !mains.count(group.target) || group.owner == group.target
            || !owners.insert(group.owner).second) return fail("Invalid publication merged-grace ownership");
        auto expected = group.owner->graceNotesAfter();
        for (Chord* chord : group.target->graceNotesBefore()) expected.push_back(chord);
        if (!positions(group.positions, expected, group.target->segment())) return fail("Invalid publication merged-grace members");
    }
    for (const Chord* main : mains)
        if (!main->graceNotesAfter().empty() && !owners.count(main)) return fail("Missing publication after-grace group");
    if (reason) reason->clear();
    return true;
}

bool publishSpacingPlanSegments4(System* system, const SpacingPlan4& plan, QString* reason)
{
    if (!spacingPlanPublicationValid4(system, plan, reason)) return false;
    std::map<const Measure*, qreal> origins;
    for (const auto& m : plan.measures) origins[m.measure] = m.x;
    // No fallible validation/query follows the first write. All segment and
    // late grace destinations have already passed complete-domain preflight.
    for (const auto& s : plan.segments) {
        s.segment->rxpos() = s.x - origins.at(s.segment->measure());
        s.segment->setWidth(s.width);
        s.segment->setStretch(s.stretch);
        s.segment->setSpacingWidthOffset4(s.widthOffset);
    }
    return true;
}

bool spacingPlanMatches4(const SpacingPlan4& plan, QString* reason)
{
    const auto equal = [reason](qreal actual, qreal expected, const char* field) {
        if (std::isfinite(actual) && std::isfinite(expected) && qAbs(actual - expected) <= 1e-6)
            return true;
        if (reason) *reason = QString("Changed horizontal %1").arg(field);
        return false;
    };
    if (plan.measures.empty() || !plan.measures.front().measure
        || !plan.measures.front().measure->system()) return false;
    const System* system = plan.measures.front().measure->system();
    if (!equal(system->width(), plan.width, "system width")) return false;
    for (const auto& m : plan.measures) {
        if (!m.measure || m.measure->system() != system
            || !equal(m.measure->x(), m.x, "measure x")
            || !equal(m.measure->width(), m.width, "measure width")) return false;
    }
    for (const auto& s : plan.segments) {
        // Inactive headers can retain cache-only positions after earlier
        // break choices. The fresh plan excludes their geometry as well.
        if (s.ignored) continue;
        if (!s.segment || !s.segment->measure() || s.segment->measure()->system() != system
            || !equal(s.segment->measure()->x() + s.segment->x(), s.x, "segment x")
            || !equal(s.segment->width(), s.width, "segment width")
            || !equal(s.segment->stretch(), s.stretch, "segment stretch")
            || !equal(s.segment->spacingWidthOffset4(), s.widthOffset, "segment width offset")) return false;
    }
    const auto graceMatches = [&equal](const GraceAfterPosition4& g, const Segment* target) {
        if (!g.chord || !g.chord->parent() || !g.chord->parent()->isChord() || !target) return false;
        const Chord* parent = toChord(g.chord->parent());
        if (!parent->segment()) return false;
        return equal(g.chord->x() + parent->x() + parent->segment()->x() - target->x(), g.x, "grace x");
    };
    for (const auto& after : plan.afterGraces)
        for (const auto& g : after.positions)
            if (!graceMatches(g, after.target)) return false;
    for (const auto& merged : plan.mergedGraces) {
        if (!merged.target) return false;
        for (const auto& g : merged.positions)
            if (!graceMatches(g, merged.target->segment())) return false;
    }
    return true;
}

}
