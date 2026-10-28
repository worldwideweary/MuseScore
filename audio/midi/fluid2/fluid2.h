// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_H
#define MS_FLUID2_H

#include <QList>
#include <QStringList>
#include <memory>
#include <vector>
#include "audio/midi/synthesizer.h"

namespace Ms {

// Experimental adapter. This is distinct from the bundled FluidS backend.
class Fluid2 final : public Synthesizer {
      struct Impl;
      std::unique_ptr<Impl> d;
   public:
      explicit Fluid2(bool isolatedByDefault = false);
      ~Fluid2() override;
      const char* name() const override { return "Fluid"; }
      void init(float) override;
      void process(unsigned, float*, float*, float*) override;
      void processWithAux(unsigned, float*, float*, float*, float* auxiliary) override;
      bool setSourceSend(int source, float gain) override;
      bool supportsSourceSends() const override;
      float sourceSend(int source) const override;
      void play(const PlayEvent&) override;
      void playWithSource(const PlayEvent&, int sourceChannel) override;
      bool loadSoundFonts(const QStringList&) override;
      bool loadDefaultSoundFonts() override;
      bool addSoundFont(const QString&) override;
      bool removeSoundFont(const QString&) override;
      std::vector<SoundFontInfo> soundFontsInfo() const override;
      const QList<MidiPatch*>& getPatchInfo() const override;
      SynthesizerGroup state() const override;
      bool setState(const SynthesizerGroup&) override;
      void allSoundsOff(int) override;
      void allNotesOff(int) override;
      void setMasterTuning(double) override;
      double masterTuning() const override;
      SynthesizerGui* gui() override;
      };
}
#endif
