// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
// Legacy audio headers use declarations supplied by this common header.
// Include it textually: this target deliberately does not reuse ms_pch.
#include "build/pch/pch.h"
#include "fluid2.h"
#include "synthbanks.h"
#include "fluid2gui.h"

#include <fluidsynth.h>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <array>
#include <cmath>

#include "audio/midi/event.h"
#include "audio/midi/midipatch.h"
#include "audio/midi/synthesizergui.h"
#include "audio/midi/fluid/fluid.h"
#include "audio/midi/fluid/soundfontpaths.h"

namespace Ms {
struct Fluid2::Impl {
      QMutex mutex;
      Fluid2SynthBanks engine;
      QStringList paths;
      QList<MidiPatch*> patches;
      double tuning = 440.0;
      bool ready = false;
      bool isolated = false;
      bool isolatedByDefault = false;
      unsigned groups = 0;
      std::array<int, 256> midiBanks {};
      std::array<int, 256> bankMsbs {};
      std::array<int, 256> programs {};

      ~Impl() { clear(); }
      void clear()
            {
            qDeleteAll(patches);
            patches.clear();
            engine.clear();
            }
      void select(int ch) { engine.select(ch, midiBanks[ch], programs[ch]); }
      void tune() { engine.tune(tuning); }
      static QByteArray encodedPath(const QString& file)
            {
#ifdef Q_OS_WIN
            // FluidSynth >= 2.2 expects UTF-8 on Windows, independent of the
            // local ANSI code page. Other platforms use native file bytes.
            return file.toUtf8();
#else
            return QFile::encodeName(file);
#endif
            }
      static std::vector<std::string> encodedPaths(const QStringList& files)
            {
            std::vector<std::string> result;
            result.reserve(files.size());
            for (const QString& file : files) result.push_back(encodedPath(file).toStdString());
            return result;
            }
      void refreshPatches()
            {
            qDeleteAll(patches);
            patches.clear();
            for (unsigned priority = 0; priority < engine.fontCount(0); ++priority) {
                  const int id = engine.fontId(0, priority);
                  fluid_sfont_t* sf = fluid_synth_get_sfont_by_id(engine.synth(0), id);
                  fluid_sfont_iteration_start(sf);
                  while (fluid_preset_t* preset = fluid_sfont_iteration_next(sf)) {
                        const int bank = fluid_preset_get_banknum(preset), program = fluid_preset_get_num(preset);
                        MidiPatch* patch = new MidiPatch;
                        patch->bank = bank + engine.fontBankOffset(0, priority);
                        patch->prog = program;
                        patch->sfid = int(priority);
                        patch->drum = bank == 128;
                        patch->synti = "Fluid";
                        patch->name = QString::fromUtf8(fluid_preset_get_name(preset));
                        patches.append(patch);
                        }
                  }
            }
      bool load(const QStringList& files)
            {
            if (!engine.loadSoundFonts(encodedPaths(files))) return false;
            paths = files;
            for (int ch = 0; ch < 256; ++ch) select(ch);
            refreshPatches();
            return true;
            }
      bool rebuild(float rate, const QStringList& files)
            {
            if (!engine.reinitialize(rate, groups, isolated, encodedPaths(files), tuning)) return false;
            paths = files;
            refreshPatches();
            ready = true;
            return true;
            }
      };

Fluid2::Fluid2(bool isolatedByDefault) : d(new Impl) { d->isolatedByDefault = isolatedByDefault; init(44100.0f); }
Fluid2::~Fluid2() = default;

void Fluid2::init(float rate)
      {
      if (!std::isfinite(rate) || rate <= 0) return;
      QMutexLocker lock(&d->mutex);
      if (d->ready && sampleRate() == rate) return;
      Synthesizer::init(rate);
      // Returning to the retained engine's rate can recover without reopening
      // a font which has moved or disappeared from disk.
      if (d->engine.bankCount() && d->engine.sampleRate() == rate) {
            d->ready = true;
            return;
            }
      d->isolated = qEnvironmentVariableIsSet("MSCORE_FLUID2_ISOLATED_CHANNELS")
            ? qgetenv("MSCORE_FLUID2_ISOLATED_CHANNELS") == "1" : d->isolatedByDefault;
      d->groups = 0;
      const QByteArray requestedGroups = qgetenv("MSCORE_FLUID2_AUDIO_GROUPS");
      if (!requestedGroups.isEmpty() && !d->isolated) {
            bool ok = false;
            d->groups = requestedGroups.toUInt(&ok);
            if (!ok || !Fluid2AudioBuffer::validGroupCount(d->groups)) {
                  qWarning("Fluid2: invalid audio groups; using the default stereo renderer");
                  d->groups = 0;
                  }
            }
      if (!d->rebuild(rate, d->paths)) {
            // Keep accepted font metadata and controls for retry, but never
            // render the retained engine at the host's different sample rate.
            d->ready = false;
            d->engine.allSoundsOff(-1);
            for (unsigned bank = 0; bank < d->engine.bankCount(); ++bank) {
                  fluid_synth_t* synth = d->engine.synth(bank);
                  fluid_synth_process(synth, 2 * fluid_synth_get_internal_bufsize(synth), 0, nullptr, 0, nullptr);
                  }
            qWarning("Fluid2: sample-rate/font reload failed; audio muted until reload or sample-rate recovery");
            }
      }

bool Fluid2::loadSoundFonts(const QStringList& files)
      {
      QStringList selected = files;
      const QString forced = QString::fromLocal8Bit(qgetenv("MSCORE_FLUID2_SOUNDFONT"));
      if (!forced.isEmpty()) selected = QStringList(forced);
      QStringList paths;
      for (QString path : selected) {
            if (path.isEmpty()) continue;
            path = SoundFontPaths::resolve(path, QFileInfo(path).isFile() ? QFileInfoList() : FluidS::Fluid::sfFiles());
            if (path.isEmpty()) return false;
            if (!paths.contains(path)) paths.append(path);
            }
      QMutexLocker lock(&d->mutex);
      return d->ready ? d->load(paths) : d->rebuild(sampleRate(), paths);
      }

bool Fluid2::loadDefaultSoundFonts()
      {
      return loadSoundFonts({"MuseScore_General.sf3"});
      }

bool Fluid2::addSoundFont(const QString& file)
      {
      if (!qgetenv("MSCORE_FLUID2_SOUNDFONT").isEmpty() || file.isEmpty()) return false;
      QStringList files;
      for (const SoundFontInfo& info : soundFontsInfo()) files.append(info.fileName);
      files.prepend(file);
      return loadSoundFonts(files);
      }

bool Fluid2::removeSoundFont(const QString& file)
      {
      if (!qgetenv("MSCORE_FLUID2_SOUNDFONT").isEmpty() || file.isEmpty()) return false;
      QStringList files;
      for (const SoundFontInfo& info : soundFontsInfo()) files.append(info.fileName);
      const QString canonical = QFileInfo(file).canonicalFilePath();
      int found = -1;
      for (int i = 0; i < files.size(); ++i) {
            if (files[i] == file || (!canonical.isEmpty() && files[i] == canonical)) { found = i; break; }
            }
      // Retain legacy basename removal only when it identifies one file.
      if (found < 0 && QFileInfo(file).fileName() == file) {
            for (int i = 0; i < files.size(); ++i) {
                  if (QFileInfo(files[i]).fileName() != file) continue;
                  if (found >= 0) return false;
                  found = i;
                  }
            }
      if (found < 0) return false;
      files.removeAt(found);
      return loadSoundFonts(files);
      }

void Fluid2::process(unsigned frames, float* output, float* effectA, float* effectB)
      {
      processWithAux(frames, output, effectA, effectB, nullptr);
      }

void Fluid2::processWithAux(unsigned frames, float* output, float*, float*, float* auxiliary)
      {
      if (!output || !d->mutex.tryLock()) return;
      // Legacy/raw-group modes keep dry playback when a host requests an aux.
      // They cannot provide independent sends and leave that buffer untouched.
      if (d->ready) d->engine.process(frames, output, d->engine.isolated() ? auxiliary : nullptr);
      d->mutex.unlock();
      }

bool Fluid2::setSourceSend(int source, float gain)
      {
      QMutexLocker lock(&d->mutex);
      return d->engine.isolated() && d->engine.setSourceSend(source, gain);
      }

bool Fluid2::supportsSourceSends() const
      {
      QMutexLocker lock(&d->mutex);
      return d->engine.isolated();
      }

float Fluid2::sourceSend(int source) const
      {
      QMutexLocker lock(&d->mutex);
      return d->engine.sourceSend(source);
      }

void Fluid2::play(const PlayEvent& event)
      {
      playWithSource(event, event.channel());
      }

void Fluid2::playWithSource(const PlayEvent& event, int sourceChannel)
      {
      QMutexLocker lock(&d->mutex);
      if (!d->ready) return;
      const int logical = event.channel(), a = event.dataA(), b = event.dataB();
      const Fluid2SynthBanks::Channel target = d->engine.channel(logical);
      if (!target.synth) return;
      if (d->engine.groupCount()) d->engine.setSourceChannel(logical, sourceChannel);
      // Only the internal bank/channel address changes. The score event and
      // external MIDI channel remain intact, including isolated bend lanes.
      fluid_synth_t* synth = target.synth;
      const int ch = target.channel;
      switch (event.type()) {
            case ME_NOTEON:
                  if (b) d->engine.noteOn(logical, a, b, event.tuning());
                  else fluid_synth_noteoff(synth, ch, a);
                  break;
            case ME_NOTEOFF: fluid_synth_noteoff(synth, ch, a); break;
            case ME_PITCHBEND: fluid_synth_pitch_bend(synth, ch, a + 128 * b); break;
            case ME_AFTERTOUCH: fluid_synth_channel_pressure(synth, ch, a); break;
            case ME_POLYAFTER: fluid_synth_key_pressure(synth, ch, a, b); break;
            case ME_PROGRAM: d->programs[logical] = a; d->select(logical); break;
            case ME_CONTROLLER:
                  if (a == CTRL_PROGRAM) { d->programs[logical] = b; d->select(logical); }
                  else if (a == CTRL_HBANK) {
                        d->bankMsbs[logical] = b & 127;
                        d->midiBanks[logical] = d->bankMsbs[logical] << 7;
                        }
                  else if (a == CTRL_LBANK) d->midiBanks[logical] = (d->bankMsbs[logical] << 7) | (b & 127);
                  else if (a == CTRL_PRESS) fluid_synth_channel_pressure(synth, ch, b);
                  else if (a == 121) {
                        if (d->engine.resetControllers(logical)) d->bankMsbs[logical] = 0;
                        }
                  else if (a >= 0 && a < 128) {
                        if (d->engine.controlChange(logical, a, b) && a == CTRL_ALL_SOUNDS_OFF)
                              d->engine.clearSourceChannel(logical);
                        }
                  break;
            default: break;
            }
      }

void Fluid2::allSoundsOff(int ch)
      {
      QMutexLocker lock(&d->mutex);
      d->engine.allSoundsOff(ch);
      }
void Fluid2::allNotesOff(int ch)
      {
      QMutexLocker lock(&d->mutex);
      // Preserve ownership while release tails still belong to the source.
      d->engine.allNotesOff(ch);
      }

void Fluid2::setMasterTuning(double hz)
      {
      if (!std::isfinite(hz) || hz <= 0) return;
      QMutexLocker lock(&d->mutex);
      d->tuning = hz;
      d->tune();
      }
double Fluid2::masterTuning() const
      {
      QMutexLocker lock(&d->mutex);
      return d->tuning;
      }
const QList<MidiPatch*>& Fluid2::getPatchInfo() const { return d->patches; }
std::vector<SoundFontInfo> Fluid2::soundFontsInfo() const
      {
      QMutexLocker lock(&d->mutex);
      std::vector<SoundFontInfo> result;
      for (const QString& file : d->paths) result.emplace_back(file, QFileInfo(file).completeBaseName());
      return result;
      }
SynthesizerGroup Fluid2::state() const
      {
      QMutexLocker lock(&d->mutex);
      SynthesizerGroup group;
      group.setName(name());
      for (const QString& file : d->paths) group.push_back(IdValue(0, file));
      return group;
      }
bool Fluid2::setState(const SynthesizerGroup& group)
      {
      QStringList fonts;
      for (const IdValue& value : group) if (value.id == 0) fonts.append(value.data);
      return loadSoundFonts(fonts);
      }
SynthesizerGui* Fluid2::gui()
      {
      if (!_gui) _gui = new Fluid2Gui(this);
      return _gui;
      }
}
