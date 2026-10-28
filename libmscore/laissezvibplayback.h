// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_LAISSEZVIBPLAYBACK_H
#define MS_LAISSEZVIBPLAYBACK_H

#include <algorithm>
#include <vector>

namespace Ms {
struct LaissezVibHold { int start; int end; };
struct LaissezVibSwitch { int tick; bool on; };

// Preserve each distinct capture time while releasing only after the longest
// overlapping request. Repeated pedal-on events capture newly sounding notes.
inline std::vector<LaissezVibSwitch> laissezVibSwitches(std::vector<LaissezVibHold> holds)
      {
      std::sort(holds.begin(), holds.end(), [](const LaissezVibHold& a, const LaissezVibHold& b) {
            return a.start < b.start || (a.start == b.start && a.end < b.end);
            });
      std::vector<LaissezVibSwitch> result;
      int end = -1, lastCapture = -1;
      for (const LaissezVibHold& hold : holds) {
            if (hold.start >= hold.end) continue;
            if (end >= 0 && hold.start > end) {
                  result.push_back({ end, false });
                  end = -1;
                  }
            if (hold.start != lastCapture) {
                  result.push_back({ hold.start, true });
                  lastCapture = hold.start;
                  }
            end = std::max(end, hold.end);
            }
      if (end >= 0) result.push_back({ end, false });
      return result;
      }
}
#endif
