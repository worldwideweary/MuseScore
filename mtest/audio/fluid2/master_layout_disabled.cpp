// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifdef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#undef MSCORE_EXPERIMENTAL_FLUIDSYNTH2
#endif
#ifdef MS_MASTER_LAYOUT_REAL_QT
#include <QObject>
#include <QString>
#include <QList>
#endif
#include "audio/midi/msynthesizer.h"
#include "master_layout.h"

std::size_t masterSizeDisabled() { return sizeof(Ms::MasterSynthesizer); }
std::size_t masterAlignmentDisabled() { return alignof(Ms::MasterSynthesizer); }
