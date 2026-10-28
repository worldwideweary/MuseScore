// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SOUNDFONTPATHS_H
#define MS_SOUNDFONTPATHS_H
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <functional>

namespace Ms { namespace SoundFontPaths {
inline QFileInfoList discover(const QString& shared, const QStringList& user,
                             const QStringList& extensions, const QString& buildFolder = QString())
      {
      QFileInfoList result;
      QSet<QString> directories;
      std::function<void(QString)> visit = [&](QString path) {
            if (path.isEmpty()) return; // An empty preference must not scan cwd.
            if (path == "~" || path.startsWith("~/")) path = QDir::homePath() + path.mid(1);
            const QFileInfo folder(path);
            const QString canonical = folder.canonicalFilePath();
            if (!folder.isDir() || canonical.isEmpty() || directories.contains(canonical)) return;
            directories.insert(canonical);
            for (const QFileInfo& file : QDir(path).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
                  if (file.isDir()) {
                        if (!file.isHidden()) visit(file.absoluteFilePath());
                        continue;
                        }
                  const QString suffix = file.suffix().toLower(), identity = file.canonicalFilePath();
                  // Keep alias filenames available for older saved basename
                  // references. The active modern list deduplicates identities.
                  if ((suffix == "sf" || suffix == "sf2" || suffix == "sf3") && !identity.isEmpty()) result.append(file);
                  }
            };
      if (!shared.isEmpty()) visit(QDir(shared).filePath("sound"));
      for (const QString& folder : user) visit(folder);
      for (const QString& folder : extensions) visit(folder);
      visit(buildFolder);
      return result;
      }

inline QString resolve(const QString& requested, const QFileInfoList& available)
      {
      if (requested.isEmpty()) return QString();
      if (QFileInfo(requested).isFile()) return QFileInfo(requested).canonicalFilePath();
      const QString name = QFileInfo(requested).fileName();
      for (const QFileInfo& file : available)
            if (file.fileName() == name && file.isFile()) return file.canonicalFilePath();
      // The source tree ships this placeholder; installation renames it to
      // MuseScore_General.sf3 when the downloaded General font is absent.
      if (name == "MuseScore_General.sf3") {
            for (const QFileInfo& file : available)
                  if (file.fileName() == "FluidR3Mono_GM.sf3" && file.isFile()) return file.canonicalFilePath();
            }
      return QString();
      }
}}
#endif
