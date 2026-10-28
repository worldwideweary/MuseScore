//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "msynthesizer.h"
#include "synthesizer.h"
#include "synthesizergui.h"

#include "libmscore/xml.h"

#include "midi/event.h"
#include "midi/midipatch.h"

#include "mscore/preferences.h"
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#include "auxiliarybus.h"
#include "effects/zita1/zita.h"
#include <QByteArray>
#endif

namespace Ms {

extern QString dataPath;

struct MasterSynthesizer::AuxiliaryState {
      AuxiliaryReverbSettings settings;
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      std::unique_ptr<ZitaReverb> reverb;
      std::atomic<bool> sourceSendsEnabled { false };
      AuxiliaryBus<MAX_BUFFERSIZE / 2> bus;
      float returnLevel = 1.0f;  // render-thread copy, never written by the GUI
#endif
      };

//---------------------------------------------------------
//   MasterSynthesizer
//---------------------------------------------------------

MasterSynthesizer::MasterSynthesizer()
   : QObject(0)
      {
      defaultGainAsDecibels = convertGainToDecibels(defaultGain);
      _auxiliary.reset(new AuxiliaryState);
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      if (!qgetenv("MSCORE_FLUID2_REVERB_SEND").isEmpty())
            qWarning("Fluid2: MSCORE_FLUID2_REVERB_SEND is retired; use the mixer's Audio reverb send control");
#endif
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MasterSynthesizer::init()
      {
      SynthesizerState state;
      QString s(dataPath + "/synthesizer.xml");
      QFile f(s);
      if (!f.open(QIODevice::ReadOnly)) {
            // qDebug("cannot read synthesizer settings <%s>", qPrintable(s));
            setState(defaultState);
            ensureSoundFonts();
            return;
            }
      XmlReader e(&f);
      while (e.readNextStartElement()) {
            if (e.name() == "Synthesizer")
                  state.read(e);
            else
                  e.unknown();
            }
      if (!setState(state)) {
            f.remove();
            setState(defaultState);
            }
      ensureSoundFonts();
      }

//---------------------------------------------------------
//   MasterSynthesizer
//---------------------------------------------------------

MasterSynthesizer::~MasterSynthesizer()
      {
      for (Synthesizer* s : _synthesizer)
            delete s;
      for (int i = 0; i < MAX_EFFECTS; ++i) {
            for (Effect* e : _effectList[i])
                  delete e;
            // delete _effect[i];   // _effect takes from _effectList
            }
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void MasterSynthesizer::reset()
      {
      for (Synthesizer* s : _synthesizer)
            s->reset();
      clearAuxiliary();
      }

void MasterSynthesizer::clearAuxiliary()
      {
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      // GUI calls only request a reset; the audio thread clears the delay lines.
      _auxiliary->bus.requestReset();
#endif
      }

//---------------------------------------------------------
//   play
//---------------------------------------------------------

void MasterSynthesizer::play(const NPlayEvent& event, unsigned syntiIdx, int sourceChannel, float sourceSend)
      {
      if (syntiIdx >= _synthesizer.size())
            return;
      const int source = sourceChannel >= 0 ? sourceChannel : event.channel();
      if (sourceSend >= 0.0f)
            setSourceSend(syntiIdx, source, sourceSend);
      _synthesizer[syntiIdx]->setActive(true);
      _synthesizer[syntiIdx]->playWithSource(event, source);
      }

bool MasterSynthesizer::setSourceSend(unsigned syntiIdx, int source, float gain)
      {
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      return _auxiliary->sourceSendsEnabled.load() && syntiIdx < _synthesizer.size()
            && _synthesizer[syntiIdx]->setSourceSend(source, gain);
#else
      Q_UNUSED(syntiIdx);
      Q_UNUSED(source);
      Q_UNUSED(gain);
      return false;
#endif
      }

bool MasterSynthesizer::supportsSourceSends(const QString& synthName) const
      {
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      if (_auxiliary->sourceSendsEnabled.load()) {
            for (const Synthesizer* s : _synthesizer) {
                  if (synthName == s->name())
                        return s->supportsSourceSends();
                  }
            }
#else
      Q_UNUSED(synthName);
#endif
      return false;
      }

bool MasterSynthesizer::auxiliaryReverbAvailable() const
      {
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      return _auxiliary->sourceSendsEnabled.load();
#else
      return false;
#endif
      }

AuxiliaryReverbSettings::Values MasterSynthesizer::auxiliaryReverbValues() const
      {
      return _auxiliary->settings.snapshot();
      }

bool MasterSynthesizer::setAuxiliaryReverbParameter(int id, double value)
      {
      return _auxiliary->settings.set(id, value);
      }

void MasterSynthesizer::resetAuxiliaryReverb()
      {
      _auxiliary->settings.replace(AuxiliaryReverbSettings::defaults());
      clearAuxiliary();
      }

//---------------------------------------------------------
//   synthNameToIndex
//---------------------------------------------------------

int MasterSynthesizer::index(const QString& name) const
      {
      int idx = 0;
      for (Synthesizer* s : _synthesizer) {
            if (s->name() == name)
                  return idx;
            ++idx;
            }
      qDebug("MasterSynthesizer::index for <%s> not found", qPrintable(name));
      return 0;
      }

//---------------------------------------------------------
//   synthIndexToName
//---------------------------------------------------------

QString MasterSynthesizer::name(unsigned idx) const
      {
      if (idx >= _synthesizer.size()) {
            qDebug("MasterSynthesizer::name() bad index %d", idx);
            return QString();
            }
      return QString(_synthesizer[idx]->name());
      }

//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------

QList<MidiPatch*> MasterSynthesizer::getPatchInfo() const
      {
      QList<MidiPatch*> pl;
      for (Synthesizer* s : _synthesizer)
            pl += s->getPatchInfo();
      return pl;
      }

//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------

MidiPatch* MasterSynthesizer::getPatchInfo(QString synti, int bank, int program)
      {
      for (Synthesizer* s : _synthesizer) {
            for (MidiPatch* p: s->getPatchInfo()) {
                  if (p->synti == synti && p->bank == bank && p->prog == program)
                        return p;
                  }
            }
      return nullptr;
      }

//---------------------------------------------------------
//   allSoundsOff
//---------------------------------------------------------

void MasterSynthesizer::allSoundsOff(int channel)
      {
      for (Synthesizer* s : _synthesizer)
            s->allSoundsOff(channel);
      if (channel == -1)
            clearAuxiliary();
      }

//---------------------------------------------------------
//   allNotesOff
//---------------------------------------------------------

void MasterSynthesizer::allNotesOff(int channel)
      {
      for (Synthesizer* s : _synthesizer)
            s->allNotesOff(channel);
      }

//---------------------------------------------------------
//   synth
//---------------------------------------------------------

Synthesizer* MasterSynthesizer::synthesizer(const QString& name)
      {
      for (Synthesizer* s : _synthesizer) {
            if (s->name() == name)
                  return s;
            }
      return 0;
      }

//---------------------------------------------------------
//   hasSoundFontsLoaded
///   Checks whether any of the synthesizers in use has
///   at least one sound font loaded. \p false value may
///   indicate some errors in the used synthesizer state
///   as such configuration will produce no sound.
//---------------------------------------------------------

bool MasterSynthesizer::hasSoundFontsLoaded() const
      {
      for (const Synthesizer* s : _synthesizer) {
            if (!s->soundFontsInfo().empty())
                  return true;
            }
      return false;
      }

bool MasterSynthesizer::ensureSoundFonts()
      {
      // A loaded Zerberus/SFZ setup is already usable. Never replace its
      // deliberate empty Fluid list merely because Fluid has no font.
      if (hasSoundFontsLoaded()) return true;
      for (Synthesizer* synth : _synthesizer)
            if (synth->loadDefaultSoundFonts()) return true;
      return false;
      }

//---------------------------------------------------------
//   registerSynthesizer
//    ownership of s moves to MasterSynthesizer
//---------------------------------------------------------

void MasterSynthesizer::registerSynthesizer(Synthesizer* s)
      {
      _synthesizer.push_back(s);
      }

//---------------------------------------------------------
//   registerEffect
//---------------------------------------------------------

void MasterSynthesizer::registerEffect(int ab, Effect* e)
      {
      _effectList[ab].push_back(e);
      }

//---------------------------------------------------------
//   setEffect
//---------------------------------------------------------

void MasterSynthesizer::setEffect(int ab, int idx)
      {
      if (idx < 0 || idx >= int(_effectList[ab].size())) {
            qDebug("MasterSynthesizer::setEffect: bad idx %d %d", ab, idx);
            return;
            }
      lock2 = true;
      while(lock1)
#if (!defined (_MSCVER) && !defined (_MSC_VER))
         sleep(1);
#else
         Sleep(1000);      // MS-equivalent function, time in ms instead of seconds.
#endif
      _effect[ab] = _effectList[ab][idx];
      lock2 = false;
      }

//---------------------------------------------------------
//   effect
//---------------------------------------------------------

Effect* MasterSynthesizer::effect(int idx)
      {
      return _effect[idx];
      }

//---------------------------------------------------------
//   setSampleRate
//---------------------------------------------------------

void MasterSynthesizer::setSampleRate(float val)
      {
      // Use the same processing exclusion as setEffect before replacing DSP.
      lock2 = true;
      while (lock1)
#if (!defined (_MSCVER) && !defined (_MSC_VER))
            sleep(1);
#else
            Sleep(1000);
#endif
      _sampleRate = val;
      for (Synthesizer* s : _synthesizer) {
            s->init(_sampleRate);
            connect(s->gui(), SIGNAL(sfChanged()), SLOT(sfChanged()));
            }
      for (Effect* e : _effectList[0])
            e->init(_sampleRate);
      for (Effect* e : _effectList[1])
            e->init(_sampleRate);
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      _auxiliary->sourceSendsEnabled = false;
      _auxiliary->reverb.reset();
      for (const Synthesizer* s : _synthesizer) {
            if (s->supportsSourceSends()) {
                  _auxiliary->reverb.reset(new ZitaReverb);
                  _auxiliary->reverb->init(_sampleRate);
                  _auxiliary->reverb->set_opmix(1.0f);  // no second dry signal
                  _auxiliary->settings.requestApply();
                  _auxiliary->sourceSendsEnabled = true;
                  break;
                  }
            }
      clearAuxiliary();
#endif
      lock2 = false;
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void MasterSynthesizer::process(unsigned n, float* p)
      {
      // Reject before taking the process guard, including the oversize case.
      if (!p || !n || n > MAX_BUFFERSIZE / 2)
            return;
      if (lock2)
            return;
      lock1 = true;
      if (lock2) {
            lock1 = false;
            return;
            }
      auto render = [this, n, p](float* auxiliary) {
            for (Synthesizer* s : _synthesizer) {
                  if (s->active()) {
                        if (auxiliary)
                              s->processWithAux(n, p, effect1Buffer, effect2Buffer, auxiliary);
                        else
                              s->process(n, p, effect1Buffer, effect2Buffer);
                        }
                  }
            };
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
      if (_auxiliary->reverb) {
            AuxiliaryReverbSettings::Values values;
            if (_auxiliary->settings.consume(values)) {
                  applyAuxiliaryReverbSettings(values, *_auxiliary->reverb);
                  _auxiliary->returnLevel = values[AuxiliaryReverbSettings::ReturnLevel];
                  }
            _auxiliary->bus.process(n, p, render,
                  [this]() { _auxiliary->reverb->clear(); },
                  [this](unsigned frames, float* input, float* wet) {
                        _auxiliary->reverb->process(int(frames), input, wet);
                        for (unsigned i = 0; i < frames * 2; ++i)
                              wet[i] *= _auxiliary->returnLevel;
                        });
            }
      else
#endif
            render(nullptr);

      if (_effect[0] && _effect[1]) {
            memset(effect1Buffer, 0, n * sizeof(float) * 2);
            _effect[0]->process(n, p, effect1Buffer);
            _effect[1]->process(n, effect1Buffer, p);
            }
      else if (_effect[0] || _effect[1]) {
            memcpy(effect1Buffer, p, n * sizeof(float) * 2);
            if (_effect[0])
                  _effect[0]->process(n, effect1Buffer, p);
            else
                  _effect[1]->process(n, effect1Buffer, p);
            }
      float g = _gain * _boost;
      for (unsigned i = 0; i < n * 2; ++i)
            *p++ *= g;
      lock1 = false;
      }

//---------------------------------------------------------
//   indexOfEffect
//---------------------------------------------------------

int MasterSynthesizer::indexOfEffect(int ab, const QString& name)
      {
      int idx = 0;
      for (Effect* e : _effectList[ab]) {
            if (e && e->name() == name)
                  return idx;
            ++idx;
            }
      qDebug("indexOfEffect %d %s not found", ab, qPrintable(name));
      return -1;
      }

int MasterSynthesizer::indexOfEffect(int ab)
      {
      if (!_effect[ab])
            return 0;
      return indexOfEffect(ab, _effect[ab]->name());
      }

//---------------------------------------------------------
//   setState
//---------------------------------------------------------

bool MasterSynthesizer::setState(const SynthesizerState& ss)
      {
      bool result = true;
      auto auxiliaryValues = AuxiliaryReverbSettings::defaults();
      for (const SynthesizerGroup& g : ss) {
            // Effects without saved parameters return an empty default group.
            if (g.name().isEmpty() && g.empty())
                  continue;
            if (g.name() == AuxiliaryReverbSettings::stateGroup()) {
                  for (const IdValue& v : g) {
                        bool ok = false;
                        const double value = v.data.toDouble(&ok);
                        if (ok) AuxiliaryReverbSettings::update(auxiliaryValues, v.id, value);
                        }
                  }
            else if (g.name() == "master") {
                  for (const IdValue& v : g) {
                        switch (v.id) {
                              case 0:
                                    setEffect(0, indexOfEffect(0, v.data));
                                    break;
                              case 1:
                                    setEffect(1, indexOfEffect(1, v.data));
                                    break;
                              case 2: {
                                    float f = preferences.getDouble(PREF_APP_PLAYBACK_DEFAULT_MASTER_VOLUME);
                                    setGain(f);
                                    }
                                    break;
                              case 3:
                                    setMasterTuning(v.data.toDouble());
                                    break;
                              case 4:
                                    setDynamicsMethod(v.data.toInt());
                                    break;
                              case 5:
                                    setCcToUseIndex(v.data.toInt());
                                    break;
                              default:
                                    qDebug("MasterSynthesizer::setState: unknown master id <%d>", v.id);
                              }
                        }
                  }
            else {
                  Synthesizer* s = synthesizer(g.name());
                  if (s) {
                        bool r = s->setState(g);
                        result = result && r;
                        }
                  else {
                        if (effect(0) && effect(0)->name() == g.name())
                              effect(0)->setState(g);
                        else if (effect(1) && effect(1)->name() == g.name())
                              effect(1)->setState(g);
                        else
                              qDebug("MasterSynthesizer::setState: unknown <%s>", qPrintable(g.name()));
                        }
                  }
            }
      _auxiliary->settings.replace(auxiliaryValues);
      clearAuxiliary();
      return result;
      }

//---------------------------------------------------------
//   state
//---------------------------------------------------------

SynthesizerState MasterSynthesizer::state() const
      {
      SynthesizerState ss;
      SynthesizerGroup g;
      g.setName("master");
      g.push_back(IdValue(0, QString("%1").arg(_effect[0] ? _effect[0]->name() : "NoEffect")));
      g.push_back(IdValue(1, QString("%1").arg(_effect[1] ? _effect[1]->name() : "NoEffect")));
      g.push_back(IdValue(2, QString("%1").arg(gain())));
      g.push_back(IdValue(3, QString("%1").arg(masterTuning())));
      g.push_back(IdValue(4, QString("%1").arg(dynamicsMethod())));
      g.push_back(IdValue(5, QString("%1").arg(ccToUseIndex())));
      ss.push_back(g);
      for (Synthesizer* s : _synthesizer)
            ss.push_back(s->state());
      if (_effect[0])
            ss.push_back(_effect[0]->state());
      if (_effect[1])
            ss.push_back(_effect[1]->state());
      SynthesizerGroup auxiliary;
      auxiliary.setName(AuxiliaryReverbSettings::stateGroup());
      const auto values = auxiliaryReverbValues();
      for (int id = 0; id < AuxiliaryReverbSettings::Count; ++id)
            auxiliary.push_back(IdValue(id, QString::number(values[id], 'g', 9)));
      ss.push_back(auxiliary);
      return ss;
      }

//---------------------------------------------------------
//   storeState
//---------------------------------------------------------

bool MasterSynthesizer::storeState()
      {
      QString s(dataPath + "/synthesizer.xml");
      QFile f(s);
      if (!f.open(QIODevice::WriteOnly)) {
            qDebug("cannot write synthesizer settings <%s>", qPrintable(s));
            return false;
            }
      XmlWriter xml(0, &f);
      xml.header();
      // force the write, since the msynth state is created when state() is called and so will
      // automatically have _isDefault = true, when in fact we need to write the state here, default or not
      state().write(xml, true);
      return true;
      }

//---------------------------------------------------------
//   setGain
//---------------------------------------------------------

void MasterSynthesizer::setGain(float f)
      {
      if (_gain != f) {
            _gain = f;
            emit gainChanged(_gain);
            }
      preferences.setPreference(PREF_APP_PLAYBACK_DEFAULT_MASTER_VOLUME, f);
      }


//---------------------------------------------------------
//   setGainAsDecibels
//---------------------------------------------------------

void MasterSynthesizer::setGainAsDecibels(float decibelValue)
      {
      if (decibelValue == minGainAsDecibels)
            setGain(MUTE);
      else
            setGain(pow(10, ((decibelValue + N) / N )));
      }

//---------------------------------------------------------
//   convertGainToDecibels
//---------------------------------------------------------

float MasterSynthesizer::convertGainToDecibels(float gain) const
      {
      if (gain == MUTE)
            return minGainAsDecibels; // return a usable value instead of -∞
      return ((N * std::log10(gain)) - N);
      }

//---------------------------------------------------------
//   gainAsDecibels
//---------------------------------------------------------

float MasterSynthesizer::gainAsDecibels() const
      {
      return convertGainToDecibels(_gain);
      }

//---------------------------------------------------------
//   setMasterTuning
//---------------------------------------------------------

void MasterSynthesizer::setMasterTuning(double val)
      {
      _masterTuning = val;
      for (Synthesizer* s : _synthesizer)
            s->setMasterTuning(_masterTuning);
      }
} // namespace Ms
