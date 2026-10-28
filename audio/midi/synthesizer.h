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

#ifndef __SYNTHESIZER_H__
#define __SYNTHESIZER_H__

#include "libmscore/synthesizerstate.h"

namespace Ms {

struct MidiPatch;
class PlayEvent;
class Synth;
class SynthesizerGui;

//---------------------------------------------------------
//   SoundFontInfo
//---------------------------------------------------------

struct SoundFontInfo {
      QString fileName;
      QString fontName;

      SoundFontInfo(QString _fileName) : fileName(_fileName), fontName(_fileName) {}
      SoundFontInfo(QString _fileName, QString _fontName) : fileName(_fileName), fontName(_fontName) {}
      };

//---------------------------------------------------------
//   Synthesizer
//---------------------------------------------------------

class Synthesizer {
      bool _active;

   protected:
      float _sampleRate { 44100.0f };
      SynthesizerGui* _gui { nullptr };

   public:
      Synthesizer() : _active(false) { _gui = 0; }
      virtual ~Synthesizer() {}
      virtual void init(float sr)    { _sampleRate = sr; }
      float sampleRate() const       { return _sampleRate; }

      virtual const char* name() const = 0;

      virtual void setMasterTuning(double) {}
      virtual double masterTuning() const { return 440.0; }

      virtual bool loadSoundFonts(const QStringList&) = 0;
      // Optional startup/default recovery; an explicit empty list still unloads.
      virtual bool loadDefaultSoundFonts() { return false; }
      virtual bool addSoundFont(const QString&)    { return false; }
      virtual bool removeSoundFont(const QString&) { return false; }

      virtual std::vector<SoundFontInfo> soundFontsInfo() const = 0;

      virtual void process(unsigned, float*, float*, float*) = 0;
      // Optional additive stereo auxiliary input for a host effect. Existing
      // backends retain their process method and contribute no auxiliary audio.
      virtual void processWithAux(unsigned frames, float* output, float* effectA, float* effectB, float* /*auxiliary*/)
            { process(frames, output, effectA, effectB); }
      virtual bool setSourceSend(int /*source*/, float /*gain*/) { return false; }
      virtual bool supportsSourceSends() const { return false; }
      virtual float sourceSend(int /*source*/) const { return 0.0f; }
      virtual void play(const PlayEvent&) = 0;
      // sourceChannel is the logical mixer channel; an isolated bend lane
      // retains its own MIDI channel but inherits its source's audio sends.
      virtual void playWithSource(const PlayEvent& event, int /*sourceChannel*/) { play(event); }

      virtual const QList<MidiPatch*>& getPatchInfo() const = 0;

      // get/set synthesizer state
      virtual SynthesizerGroup state() const = 0;
      virtual bool setState(const SynthesizerGroup&) = 0;
      virtual void setValue(int, double) {}
      virtual double value(int) const { return 0.0; }

      void reset()                    { _active = false; }
      bool active() const             { return _active; }
      void setActive(bool val = true) { _active = val;  }

      virtual void allSoundsOff(int /*channel*/) {}
      virtual void allNotesOff(int /*channel*/) {}

      virtual SynthesizerGui* gui()  { return _gui; }
      };

}
#endif

