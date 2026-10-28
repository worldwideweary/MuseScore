// Copyright (C) 2026 MuseScore contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef MS_LYRICSCOMPAT_H
#define MS_LYRICSCOMPAT_H

class QByteArray;
namespace Ms {
class MasterScore;
void importStudioRepeatLyrics(MasterScore*, const QByteArray&);
}
#endif
