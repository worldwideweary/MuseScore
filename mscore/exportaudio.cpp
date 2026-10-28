//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2009 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "config.h"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1 
#endif

#ifdef HAS_AUDIOFILE
#include <sndfile.h>
#endif

#include "libmscore/score.h"
#include "libmscore/note.h"
#include "libmscore/part.h"
#include "libmscore/mscore.h"
#include "audio/midi/msynthesizer.h"
#include "musescore.h"
#include "exportaudio.h"
#include "preferences.h"
#include <QScopedValueRollback>

namespace Ms {

///
/// \brief Function to synthesize audio and output it into a generic QIODevice
/// \param score The score to output
/// \param device The output device
/// \param updateProgress An optional callback function that will be notified with the progress in range [0, 1]
/// \return True on success, false otherwise.
///
/// If the callback function is non zero an returns false the export will be canceled.
///
bool MuseScore::saveAudio(Score* score, QIODevice *device, std::function<bool(float)> updateProgress)
      {
      if (MScore::noGui)
            return renderAudio(score, device, nullptr, synthesizerFactory, updateProgress);
      const SynthesizerState state = synthesizerState();
      return renderAudio(score, device, &state, synthesizerFactory, updateProgress);
      }


#ifdef HAS_AUDIOFILE


//---------------------------------------------------------
//   saveAudio
//---------------------------------------------------------

bool MuseScore::saveAudio(Score* score, const QString& name)
      {
      bool wasCanceled = false;
      return saveAudio(score, name, wasCanceled);
      }

bool MuseScore::saveAudio(Score* score, const QString& name, bool& wasCanceled)
      {
      wasCanceled = false;
      if (!score)
            return false;
      // QIODevice - SoundFile wrapper class
      class SoundFileDevice : public QIODevice {
      private:
            SF_INFO info;
            SNDFILE *sf = nullptr;
            bool failed = false;
            bool opened = false;
            const QString filename;
      public:
            SoundFileDevice(int sampleRate, int format, const QString& name)
                  : filename(name) {
                  memset(&info, 0, sizeof(info));
                  info.channels   = 2;
                  info.samplerate = sampleRate;
                  info.format     = format;
                  }
            ~SoundFileDevice() {
                  if (sf) {
                        sf_close(sf);
                        sf = nullptr;
                        }
                  }

            virtual qint64 readData(char *dta, qint64 maxlen) override final {
                  Q_UNUSED(dta);
                  qDebug() << "Error: No write supported!";
                  return maxlen;
                  }

            virtual qint64 writeData(const char *dta, qint64 len) override final {
                  const sf_count_t frames = len / sizeof(float) / 2;
                  const sf_count_t written = sf_writef_float(sf, reinterpret_cast<const float*>(dta), frames);
                  if (written != frames) {
                        failed = true;
                        setErrorString(QString::fromLocal8Bit(sf_strerror(sf)));
                        }
                  return written < 0 ? -1 : written * 2 * qint64(sizeof(float));
                  }

            bool succeeded() const { return !failed; }
            bool openedFile() const { return opened; }

            bool open(QIODevice::OpenMode mode) override {
                  if ((mode & QIODevice::WriteOnly) == 0) {
                        return false;
                        }

#ifdef Q_OS_WIN
                  #define SF_FILENAME_LEN	1024
                  QByteArray path = filename.toUtf8();
                  wchar_t wpath[SF_FILENAME_LEN];
                  int dwRet = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.constData(), -1, wpath, SF_FILENAME_LEN);
                  if (dwRet == 0) {
                        qCritical() << Q_FUNC_INFO << "filed get path: " << GetLastError() << "\n";  
                        return false; 
                        }
                  sf = sf_wchar_open(wpath, SFM_WRITE, &info);
#else  // Q_OS_WIN
                  sf = sf_open(qPrintable(filename), SFM_WRITE, &info);
#endif // Q_OS_WIN

                  if (sf == nullptr) {
                        qDebug("open soundfile failed: %s", sf_strerror(sf));
                        return false;
                        } 

                  opened = true;

                  return QIODevice::open(mode);
                  }
            void close() override {
                  if (sf && sf_close(sf)) {
                        failed = true;
                        qDebug("close soundfile failed");
                        }
     
                  sf = nullptr;
                  QIODevice::close();
                  }
            };
      int format;
      int PCMRate;
      switch (preferences.getInt(PREF_EXPORT_AUDIO_PCMRATE)) {
            case 32: PCMRate = SF_FORMAT_PCM_32; break;
            case 24: PCMRate = SF_FORMAT_PCM_24; break;
            case 16: PCMRate = SF_FORMAT_PCM_16; break;
            case 8:  PCMRate = SF_FORMAT_PCM_S8; break;
            default: PCMRate = SF_FORMAT_PCM_16; break;
            }

      if (name.endsWith(".wav"))
            format = SF_FORMAT_WAV | PCMRate;
      else if (name.endsWith(".ogg"))
            format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
      else if (name.endsWith(".flac"))
            format = SF_FORMAT_FLAC | PCMRate;
      else {
            qDebug("unknown audio file type <%s>", qPrintable(name));
            return false;
            }

      EventMap events;
      score->renderMidi(&events, synthesizerState());
      if(events.size() == 0)
            return false;

      // The shared QIODevice exporter below owns the rendering synthesizer.
      // Do not load a second, unused font stack in this format wrapper.
      int sampleRate = preferences.getInt(PREF_EXPORT_AUDIO_SAMPLERATE);

      QScopedValueRollback<int> restoreSampleRate(MScore::sampleRate, sampleRate);


      SoundFileDevice device(sampleRate, format, name);

      // dummy callback function that will be used if there is no gui
      std::function<bool(float)> progressCallback = [](float) {return true;};

      QProgressDialog progress(this);
      progress.setWindowFlags(Qt::WindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowTitleHint));
      progress.setWindowModality(Qt::ApplicationModal);
      //progress.setCancelButton(0);
      progress.setCancelButtonText(tr("Cancel"));
      progress.setLabelText(tr("Exporting…"));
      if (!MScore::noGui) {
            // callback function that will update the progress bar
            // it will return false and thus cancel the export if the user
            // cancels the progress dialog.
            progressCallback = [&progress](float v) -> bool {
                  if (progress.wasCanceled())
                        return false;
                  progress.setValue(v * 1000);
                  qApp->processEvents();
                  return true;
                  };

            progress.show();
            }

      // The range is set arbitrarily to 1000 as steps.
      // The callback will return float numbers between 0 and 1
      // which will be scaled into integer 0 to 1000 numbers
      // which allows a smooth transition.
      progress.setRange(0, 1000);

      // Save the audio to the SoundFile device
      bool result = saveAudio(score, &device, progressCallback);

      wasCanceled = progress.wasCanceled();
      progress.close();

      result = result && !wasCanceled && device.succeeded();
      if ((wasCanceled || !result) && device.openedFile())
            QFile::remove(name);

      return result;
      }

#endif // HAS_AUDIOFILE
}

