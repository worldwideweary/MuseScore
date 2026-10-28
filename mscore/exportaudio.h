// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_EXPORTAUDIO_H
#define MS_EXPORTAUDIO_H

#include <functional>

class QIODevice;

namespace Ms {
class Score;
class SynthesizerState;
class MasterSynthesizer;
using AudioSynthesizerFactory = MasterSynthesizer* (*)();

// Common float-stereo renderer for file/MP3 export. A live state uses the
// current playback settings; nullptr uses the score's saved state instead.
// The caller owns the device. An output opened here is closed on every exit.
// Ownership of the factory's result passes to the renderer. The application
// and lightweight tests supply factories without sharing bootstrap globals.
bool renderAudio(Score*, QIODevice*, const SynthesizerState* liveState,
                 AudioSynthesizerFactory, std::function<bool(float)> updateProgress = nullptr);
}
#endif
