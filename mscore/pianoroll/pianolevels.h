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

#ifndef __PIANOLEVELS_H__
#define __PIANOLEVELS_H__

#include <QWidget>

#include "pianorolledittool.h"
#include "pianoroll/pianoview.h"

#include "libmscore/pos.h"

namespace Ms {

class Score;
class Staff;
class Chord;
class Note;
class NoteEvent;
class PianoItem;
enum class PianoRollOrientation;



//---------------------------------------------------------
//   PianoLevels
//---------------------------------------------------------

class PianoLevels : public QWidget
{
      Q_OBJECT

      enum DragStyle {
            LERP, OFFSET
            };

      PianoView* _pianoView { nullptr };
      PianoRollOrientation _orientation { PianoRollOrientation::HORIZONTAL };
      Coloring _coloring;
      Score* _score { nullptr };
      int _xpos;
      qreal _xZoom;
      Pos _cursor;
      Pos* _locator { nullptr };
      Staff* _staff { nullptr };
      int _tuplet;
      int _subdiv;
      int _levelsIndex;
      int vMargin;
      int levelLen;
      int pickRadius = 4;

      qreal _playbackLocatorTick { 0.0 };
      bool _playbackLocatorValid { false };
      bool _editCommandActive { false };

      PianoRollScope _scope;

      bool mouseDown;
      QPointF mouseDownPos;
      QPointF lastMousePos;
      int dragging = false;
      DragStyle dragStyle = DragStyle::OFFSET;
      Note* singleNoteDrag { nullptr };
      NoteEvent* singleNoteEventDrag { nullptr };
      QSet<const Note*> _levelInteractionNotes;

      struct LevelDragTarget {
            Note* note { nullptr };
            NoteEvent* event { nullptr };
            int startValue { 0 };
            };

      QVector<LevelDragTarget> _levelDragTargets;
      int _levelDragAnchorValue { 0 };

      int minBeatGap;

      QList<Note*> noteList;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*);

      int pixelXToTick(int pixX);
      int tickToPixelX(int tick);
      int valToPixelY(int value);
      int pixelYToVal(int value);
      int noteStartTick(Note* note, NoteEvent* evt);
      void moveLocator(QMouseEvent*);
      void addChord(Chord* chord, int voice);
      void clearNoteData();

      bool pickNoteEvent(int x, int y, bool selectedOnly,
                         Note*& pickedNote, NoteEvent*& pickedNoteEvent);

      bool pickNearestLevelInTimeBand(int timePixel,
                                      int valuePixel,
                                      int timeRadius,
                                      bool selectedOnly,
                                      Note*& pickedNote,
                                      NoteEvent*& pickedEvent);

      void captureLevelDragTargets(Note* anchorNote,
                                   NoteEvent* anchorEvent,
                                   bool selectedOnly);

      void adjustCapturedLevels(int value);
      void adjustLevelLerp(int tick0, int value0, int tick1, int value1, bool selectedOnly = true);
      void adjustLevel(Note* note, NoteEvent* noteEvt, int value);
      void drawLevelBar(QPainter& painter,
                        int timePixel,
                        int valuePixel,
                        int zeroPixel,
                        const QColor& color);

      int pixelToTick(int pixel) const;
      int tickToPixel(int tick) const;

      int valToPixel(int value) const;
      int pixelToVal(int pixel) const;

      bool hasSelectedNotes() const;

signals:
      void posChanged(const Pos&);
      void tupletChanged(int);
      void subdivChanged(int);
      void levelsIndexChanged(int);
      void locatorMoved(int idx, const Pos&);
      void noteLevelsChanged();

public slots:
      void setXpos(int);
      void setTuplet(int);
      void setSubdiv(int);
      void setXZoom(qreal);
      void setPos(const Pos&);
      void setLevelsIndex(int index);

public:
      PianoLevels(QWidget *parent = 0);
      ~PianoLevels();

      void setColoring(Coloring);
      void setPianoView(PianoView*);
      void setOrientation(PianoRollOrientation);
      void setScore(Score*, Pos* locator);
      Staff* staff() { return _staff; }
      void setStaff(Staff*, Pos* locator);
      void updateNotes();
      int tuplet() const { return _tuplet; }
      int subdiv() const { return _subdiv; }

      void setPlaybackLocatorTick(qreal tick);
      void clearPlaybackLocatorTick();

      void setScope(PianoRollScope scope);

      int mouseTimePixel(const QPointF& pos) const;
      int mouseValuePixel(const QPointF& pos) const;

      int xpos() const { return _xpos; }
      qreal xZoom() const { return _xZoom; }
};

}
#endif // __PIANOLEVELS_H__
