// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2GUI_H
#define MS_FLUID2GUI_H
#include <QCoreApplication>
#include <QWidget>
#include <QStringList>
#include "audio/midi/synthesizergui.h"

class QListWidget;
class QPushButton;
namespace Ms {
// The base supplies Qt signals/slots. No additional meta-object is needed.
class Fluid2Gui final : public SynthesizerGui {
      Q_DECLARE_TR_FUNCTIONS(Fluid2Gui)
      QListWidget* fonts;
      QPushButton* add;
      QPushButton* remove;
      QPushButton* top;
      QPushButton* up;
      QPushButton* down;
      QPushButton* bottom;
      bool busy = false;
      QStringList paths() const;
      void refresh(int row);
      void updateButtons();
      void apply(const QStringList&, int row);
      void move(int target);
   public:
      explicit Fluid2Gui(Synthesizer*);
      void synthesizerChanged() override;
      };
}
#endif
