#include "pianolevelschooser.h"
#include "pianolevelsfilter.h"
#include "pianoview.h"

#include "libmscore/score.h"

namespace Ms {

//---------------------------------------------------------
//   PianoLevelsChooser
//---------------------------------------------------------

PianoLevelsChooser::PianoLevelsChooser(QWidget *parent)
      : QWidget(parent)
      {
      setupUi(this);

      _levelsIndex = 0;

      for (int i = 0; PianoLevelsFilter::FILTER_LIST[i]; ++i) {
            QString name = PianoLevelsFilter::FILTER_LIST[i]->name();
            levelsCombo->addItem(name, i);
            levelsCombo->setItemData(i, PianoLevelsFilter::FILTER_LIST[i]->tooltip(), Qt::ToolTipRole);
            }

      connect(levelsCombo, SIGNAL(activated(int)), SLOT(setLevelsIndex(int)));
      connect(setEventsBn, SIGNAL(clicked(bool)), SLOT(setEventDataPressed()));
      }

//---------------------------------------------------------
//   setPianoView
//---------------------------------------------------------

void PianoLevelsChooser::setPianoView(PianoView* pianoView)
      {
      _pianoView = pianoView;
      }

//---------------------------------------------------------
//   setPlaybackEditingEnabled
//---------------------------------------------------------

void PianoLevelsChooser::setPlaybackEditingEnabled(bool enabled)
      {
      _playbackEditingEnabled = enabled;
      updateEditorEnabled();
      }

//---------------------------------------------------------
//   updateEditorEnabled
//---------------------------------------------------------

void PianoLevelsChooser::updateEditorEnabled()
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const bool enabled =
            !filter->isPerEvent() || _playbackEditingEnabled;

      eventValSpinBox->setEnabled(enabled);
      setEventsBn->setEnabled(enabled);
      }

//---------------------------------------------------------
//   updateSetboxValue
//---------------------------------------------------------

void PianoLevelsChooser::updateSetboxValue()
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      eventValSpinBox->setRange(
            filter->minRange(),
            filter->maxRange());

      QList<PianoItem*> items = _pianoView->getSelectedItems();

      if (items.size() == 1) {
            PianoItem* item = items[0];
            Note* note = item->note();

            NoteEvent* event = item->getTweakNoteEvent();

            if (filter->isPerEvent() && !event)
                  return;

            int value = filter->value(note->staff(), note, event);
            eventValSpinBox->setValue(value);
            }
      }

//---------------------------------------------------------
//   setLevelsIndex
//---------------------------------------------------------

void PianoLevelsChooser::setLevelsIndex(int index)
      {
      if (_levelsIndex != index) {
            _levelsIndex = index;
            updateSetboxValue();
            updateEditorEnabled();
            emit levelsIndexChanged(index);
            }
      }

//---------------------------------------------------------
//   setEventDataPressed
//---------------------------------------------------------

void PianoLevelsChooser::setEventDataPressed()
      {
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      int val = eventValSpinBox->value();
      QList<Note*> noteList = _staff->getNotes();

      Score* score = _staff->score();

      score->startCmd();

      for (Note*& note: noteList) {
            if (!note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& e : note->playEvents()) {
                        filter->setValue(_staff, note, &e, val);
                        }
                  }
                  else
                        filter->setValue(_staff, note, nullptr, val);

            }

      score->endCmd();

      emit notesChanged();
      }

}
