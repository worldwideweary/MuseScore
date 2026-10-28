// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef MS_PLAYBACK_TIMING_H
#define MS_PLAYBACK_TIMING_H

#include <QDebug>
#include <QElapsedTimer>

namespace Ms {

// Optional wall-clock diagnostics. No timer or logging on the normal path.
// Nested scopes distinguish file/layout work from playback preparation.
class PlaybackTiming {
      const char* scope;
      QElapsedTimer clock;
      qint64 previous = 0;

   public:
      explicit PlaybackTiming(const char* name) : scope(name) {
            if (qEnvironmentVariableIntValue("MSCORE_AUDIO_PREP_TIMING") != 0)
                  clock.start();
            }
      bool enabled() const { return clock.isValid(); }
      void phase(const char* name) {
            if (!enabled()) return;
            const qint64 now = clock.elapsed();
            qDebug("Audio preparation: %s / %s = %lld ms (elapsed %lld ms)",
                  scope, name, static_cast<long long>(now - previous), static_cast<long long>(now));
            previous = now;
            }
      ~PlaybackTiming() {
            if (enabled())
                  qDebug("Audio preparation: %s total = %lld ms", scope, static_cast<long long>(clock.elapsed()));
            }
      };

} // namespace Ms
#endif
