// Optional rhythmic spacing adapted from Studio v4.7.5 HorizontalSpacing.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGLAYOUT4_H
#define MS_SPACINGLAYOUT4_H
#include <QtGlobal>
#include <cmath>
#include <algorithm>
#include <vector>

namespace Ms {
class Segment;
double segmentSpacingStretch4(const Segment*);
class Measure;

// This scope supplies Studio-only inputs to the existing Measure spring
// solver. An inactive Studio style supplies zero fixed offsets and no
// rhythmic-weight override; the native spring algorithm stays in Measure.
struct StudioMeasureSpring4 {
    qreal fixedOffset { 0 };
    qreal rhythmicWeight { 0 };
    bool useRhythmicWeight { false };
};

class StudioMeasureSpringScope4 {
    bool _enabled { false };
    bool _rhythmicEnabled { false };
public:
    explicit StudioMeasureSpringScope4(const Measure*);
    StudioMeasureSpring4 spring(const Segment*) const;
    qreal fixedOffset(const Segment*) const;
};

// The Studio width contribution is read before native collision clearance;
// its rhythmic minimum is applied afterwards. Native sizing stays separate.
struct StudioSegmentWidth4 {
    qreal offset { 0 };
    qreal minimum { 0 };
    bool enabled { false };
};

// The optional module owns the Studio-only duration refresh, the signed
// cross-beam offset and the post-collision rhythmic width floor. Ordinary
// collision measurement and segment placement stay in Measure.
bool studioUsesRhythmicMeasureWidths4(const Measure*);
class StudioMeasureWidthScope4 {
    bool _enabled { false };
public:
    explicit StudioMeasureWidthScope4(const Measure*);
    void prepareDurations(Measure*) const;
    StudioSegmentWidth4 propose(const Segment*, const Segment*) const;
    qreal finalize(Segment*, const StudioSegmentWidth4&, qreal measuredWidth,
                   qreal measureSpatium) const;
};

// Flexible widths exclude fixed cross-beam offsets. The returned common force
// follows Studio stretchSegmentsToWidth; collision floors remain unstretched
// until the common force exceeds their pre-tension.
struct SpacingSpring4 {
    double width;
    double stretch;
};

inline double spacing4SpringForce(std::vector<SpacingSpring4> springs, double extraWidth)
{
    if (springs.empty() || !(extraWidth > 0) || !std::isfinite(extraWidth))
        return 0;
    for (const SpacingSpring4& spring : springs) {
        if (!(spring.width >= 0) || !std::isfinite(spring.width)
            || !(spring.stretch > 0) || !std::isfinite(spring.stretch))
            return 0;
    }
    std::sort(springs.begin(), springs.end(), [](const SpacingSpring4& a, const SpacingSpring4& b) {
        return a.width / a.stretch < b.width / b.stretch;
    });
    double compliance = 0;
    double force = 0;
    for (std::size_t i = 0; i < springs.size(); ++i) {
        compliance += springs[i].stretch;
        extraWidth += springs[i].width;
        force = extraWidth / compliance;
        if (i + 1 == springs.size() || force < springs[i + 1].width / springs[i + 1].stretch)
            break;
    }
    return std::isfinite(force) && force > 0 ? force : 0;
}

// Durations are fractions of a whole note. A missing previous onset is zero.
// This is only the rhythmic weight, not collision padding or justification.
inline double spacing4DurationStretch(double slope, double ticks, double shortest,
                                     double previousTicks = 0, double previousShortest = 0)
{
    if (!(ticks > 0) || !std::isfinite(ticks))
        return 0;
    if (!(slope > 0) || !std::isfinite(slope))
        slope = 1.5;
    if (!(shortest > 0) || !std::isfinite(shortest))
        shortest = ticks;
    const bool adjacent = ticks == shortest;
    const bool previousAdjacent = previousTicks == previousShortest;
    double duration = ticks;
    double proportion = 1;
    if (!adjacent) {
        duration = previousShortest > 0 && !previousAdjacent && previousShortest < shortest
            ? previousShortest : shortest;
        proportion = ticks / duration;
    }
    const double result = std::pow(slope, std::log2(duration / 0.25)) * proportion;
    return std::isfinite(result) && result > 0 ? result : 1;
}
}
#endif
