//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __MSYNTHESIZER_H__
#define __MSYNTHESIZER_H__

#include <atomic>
#include <memory>
#include "auxiliaryreverbsettings.h"
#include "effects/effect.h"
#include "libmscore/synthesizerstate.h"

namespace Ms {

struct MidiPatch;
class NPlayEvent;
class Synthesizer;
class Effect;
class Xml;

//---------------------------------------------------------
//   MasterSynthesizer
//    hosts several synthesizers
//---------------------------------------------------------

class MasterSynthesizer : public QObject {
      Q_OBJECT

      float _gain             { defaultGain };
      float _boost            { 10.0  };     // +20dB
      double _masterTuning    { 440.0 };

      int _dynamicsMethod     { 1 };      // Default dynamics method
      int _ccToUse            { 1 };      // CC2

      static constexpr double MUTE = 0.00;            // for gain to decibels conversion
      static constexpr double MAX = 10.00;            // for gain to decibels conversion
      static constexpr double N = 20.0;               // for gain to decibels conversion

   public:
      static const int MAX_BUFFERSIZE = 8192;
      static const int MAX_EFFECTS = 2;
      static constexpr float defaultGain = 0.1f;  // -20dB

   private:
      std::atomic<bool> lock1      { false };
      std::atomic<bool> lock2      { true  };
      std::vector<Synthesizer*> _synthesizer;
      std::vector<Effect*> _effectList[MAX_EFFECTS];
      Effect* _effect[MAX_EFFECTS]  { nullptr, nullptr };

      float _sampleRate;

      float effect1Buffer[MAX_BUFFERSIZE];
      float effect2Buffer[MAX_BUFFERSIZE];
      // Allocators and the out-of-line constructor must agree on this layout
      // even when a consumer (such as testutils) lacks the backend define.
      struct AuxiliaryState;
      std::unique_ptr<AuxiliaryState> _auxiliary;
      int indexOfEffect(int ab, const QString& name);
      float convertGainToDecibels(float gain) const;

   public slots:
      void sfChanged() { emit soundFontChanged(); }
      void setGain(float f);

   signals:
      void soundFontChanged();
      void gainChanged(float);

   public:
      MasterSynthesizer();
      ~MasterSynthesizer();
      void registerSynthesizer(Synthesizer*);

      void init();

      float sampleRate()            { return _sampleRate; }
      void setSampleRate(float val);

      void process(unsigned, float*);
      void play(const NPlayEvent&, unsigned, int sourceChannel = -1, float sourceSend = -1.0f);
      bool setSourceSend(unsigned syntiIdx, int source, float gain);
      bool supportsSourceSends(const QString& synthName) const;
      bool auxiliaryReverbAvailable() const;
      AuxiliaryReverbSettings::Values auxiliaryReverbValues() const;
      bool setAuxiliaryReverbParameter(int id, double value);
      void resetAuxiliaryReverb();

      void setMasterTuning(double val);
      double masterTuning() const      { return _masterTuning; }

      int index(const QString&) const;
      QString name(unsigned) const;

      QList<MidiPatch*> getPatchInfo() const;
      MidiPatch* getPatchInfo(QString synti, int bank, int program);

      SynthesizerState state() const;
      bool setState(const SynthesizerState&);

      Synthesizer* synthesizer(const QString& name);
      const std::vector<Effect*>& effectList(int ab) const { return _effectList[ab]; }
      const std::vector<Synthesizer*> synthesizer() const { return _synthesizer; }
      bool hasSoundFontsLoaded() const;
      bool ensureSoundFonts();
      void registerEffect(int ab, Effect*);

      void reset();
      void clearAuxiliary();
      void allSoundsOff(int channel);
      void allNotesOff(int channel);

      void setEffect(int ab, int idx);
      Effect* effect(int ab);
      int indexOfEffect(int ab);

      float gain() const     { return _gain; }
      float gainAsDecibels() const;
      float defaultGainAsDecibels;
      float minGainAsDecibels = -80;
      float maxGainAsDecibels = 0;

      void setGainAsDecibels(float decibelValue);
      float boost() const    { return _boost; }
      void setBoost(float v) { _boost = v; }

      int dynamicsMethod() const          { return _dynamicsMethod; }
      void setDynamicsMethod(int val)     { _dynamicsMethod = val; }
      int ccToUseIndex() const            { return _ccToUse; }    // NOTE: this doesn't return a CC number, but returns an index instead
      void setCcToUseIndex(int val)       { _ccToUse = val; }

      bool storeState();
      };

}
#endif

