// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#define MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#endif
#ifdef MS_MASTER_LAYOUT_REAL_QT
#include <QObject>
#include <QString>
#include <QList>
#endif
#include "audio/midi/msynthesizer.h"
#include "master_layout.h"

std::size_t masterSizeEnabled() { return sizeof(Ms::MasterSynthesizer); }
std::size_t masterAlignmentEnabled() { return alignof(Ms::MasterSynthesizer); }
