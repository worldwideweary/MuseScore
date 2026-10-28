// Optional Studio spacing: common-notation collision policy.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGCOLLISION4_H
#define MS_SPACINGCOLLISION4_H
#include <QtGlobal>
#include <QRectF>
#include <array>
#include <vector>
#include <utility>
#include <unordered_map>

namespace Ms {
class Element;
class Chord;
class Note;
class Score;
struct SpacingShape4;
class SpacingInput4;
class SpacingAttachmentPreparation4;
enum class ElementType : char;

// Same-measure, same-staff vertical comparison without system/page access.
// Result uses the slur side mask (above=1, below/equal=2); failure is atomic.
bool spacingLocalSlurSide4(const Note*, const Element*, unsigned& result);

// Migration is explicit: production remains COMPATIBILITY until mutable grace
// production/publication can use the same prepared policy as read-only plans.
enum class SpacingSlurPolicy4 { COMPATIBILITY, PREPARED };
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
    std::vector<std::pair<const Chord*, const Chord*>> mergedPairs; // Validated owner/target slur scope.
    bool contains(const Chord* chord) const;
    bool containsAfter(const Chord* chord) const;
};

// Stack-local memo for one read-only spacing query, including a complete plan.
// Discard before score edits, geometry preparation or later layout. Eligibility is
// reused, including native shape intersections; item checks and pair-specific
// padding are always evaluated. Geometry must remain unchanged for its lifetime.
class SpacingContextCache4 {
    const SpacingInput4* _input = nullptr;
    const SpacingAttachmentPreparation4* _preparation = nullptr;
    SpacingSlurPolicy4 _slurPolicy = SpacingSlurPolicy4::COMPATIBILITY;
    bool _initialized = false, _allowGrace = false, _hasContext = false;
    const Chord* _scopedMain = nullptr;
    SpacingGraceContext4 _context, _mergedContext;
    const Chord* _mergedOwner = nullptr;
    const Chord* _mergedTarget = nullptr;
    std::vector<std::pair<const Chord*, bool>> _chords;
    std::vector<std::pair<const Chord*, unsigned>> _slurKerning;
    std::unordered_map<const Element*, std::unordered_map<const Element*, bool>> _overlaps;
public:
    // Optional caller-owned musical view, prepared once outside inner loops.
    // It must describe the same unchanged score for the entire query lifetime.
    explicit SpacingContextCache4(const SpacingInput4* input = nullptr) : _input(input) {}
    // Borrows the complete pass owner. Query memoization remains local to
    // this cache; the owner can be reused by later read-only squeeze queries.
    explicit SpacingContextCache4(const SpacingAttachmentPreparation4&,
                                  SpacingSlurPolicy4 = SpacingSlurPolicy4::COMPATIBILITY);
    const SpacingInput4* input() const { return _input; }
    const SpacingAttachmentPreparation4* preparation() const { return _preparation; }
    SpacingSlurPolicy4 slurPolicy() const { return _slurPolicy; }
    // Exact-endpoint readiness, independent of curve eligibility/member counts.
    // Kerning bits alone do not certify readiness; admission checks this first.
    bool preparedSlursReady(const Chord*) const;
    // Validated, non-null items; preserves the ordered native shape query.
    bool shapesIntersect(const Element* first, const Element* second);
    // Endpoint-side restrictions, reused only within this unchanged query.
    unsigned forwardSlurKerning(const Chord*);
    const SpacingGraceContext4* mergedContext(const Chord* owner, const Chord* target);
    bool lookup(const Chord*, bool allowGrace, const SpacingGraceContext4*,
                const Chord* scopedMain, bool& result);
    void remember(const Chord* chord, bool result) { _chords.push_back({chord, result}); }
};

// False means this context is not covered: output is left untouched.
// shapeSpatium is the first shape's staff-scaled spatium; table padding uses
// score spatium and item magnification, as in Studio. They are not interchangeable.
bool spacingPairPolicy4(const SpacingPaddingTable4&, const Element* first, const Element* second,
                        qreal shapeSpatium, SpacingPairPolicy4& result, const SpacingGraceContext4* = nullptr,
                        SpacingContextCache4* = nullptr);
// Scoped grace-group query. Both items must belong to main or its before-graces.
// Does not broaden the ordinary collision projection/planner's admission rules.
bool spacingGracePairPolicy4(const SpacingPaddingTable4&, const Chord* main,
                             const Element* first, const Element* second,
                             qreal shapeSpatium, SpacingPairPolicy4& result, SpacingContextCache4* = nullptr);
// Read-only after-grace scope: owner's after-graces and the selected chord/rest/barline.
// Callers separately validate the target's layout eligibility.
// includeOwner also allows source-chord components for a publication clearance check.
// Does not admit after-graces into default/system spacing queries.
bool spacingAfterGracePairPolicy4(const SpacingPaddingTable4&, const Chord* owner, const Element* target,
                                  const Element* first, const Element* second,
                                  qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner = false,
                                  SpacingContextCache4* = nullptr);
// Explicit joint-query scope: incoming after-graces, target before-graces and target.
// Does not admit merged groups into ordinary or system queries.
bool spacingMergedGracePairPolicy4(const SpacingPaddingTable4&, const Chord* owner, const Chord* target,
                                   const Element* first, const Element* second,
                                   qreal shapeSpatium, SpacingPairPolicy4& result, bool includeOwner = false,
                                   SpacingContextCache4* = nullptr);
// Pure rectangular constraint; finite, normalized geometry/policy is required.
// Returns -max(qreal) when this pair imposes no horizontal constraint.
qreal spacingPairDistance4(const QRectF&, const QRectF&, const SpacingPairPolicy4&,
                           qreal shapeSpatium, qreal squeezeFactor);
bool spacingShapeDistance4(const SpacingPaddingTable4&, const SpacingShape4&, const SpacingShape4&,
                           qreal shapeSpatium, qreal squeezeFactor, qreal& result, const SpacingGraceContext4* = nullptr,
                           SpacingContextCache4* = nullptr);
// A separate, validated view for the new placement pass. Native snapshots keep
// all rectangles; only producer-tagged redundant legacy bounds are excluded.
// Proven shared noteheads expand to all owners so pair queries retain every
// voice's policy; unknown ownership or unsupported shared types reject the view.
bool spacingCollisionProjection4(const SpacingPaddingTable4&, const SpacingShape4&,
                                 qreal shapeSpatium, SpacingShape4& result, const SpacingGraceContext4* = nullptr,
                                 SpacingContextCache4* = nullptr);
}
#endif
