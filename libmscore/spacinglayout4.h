// Optional rhythmic spacing adapted from Studio v4.7.5 HorizontalSpacing.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGLAYOUT4_H
#define MS_SPACINGLAYOUT4_H
#include <cmath>

namespace Ms {
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
