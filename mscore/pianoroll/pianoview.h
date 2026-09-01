//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __PIANOVIEW_H__
#define __PIANOVIEW_H__

#include "pianorolledittool.h"
#include "preferences.h"

#include "libmscore/pos.h"

#include <QSet>

namespace Ms {

class Score;
class Staff;
class Chord;
class ChordRest;
class Segment;
class Measure;
class Note;
class NoteEvent;
class PianoView;
class NoteTweakerDialog;

enum class NoteSelectType : char {
      REPLACE = 0,
      XOR,
      ADD,
      SUBTRACT,
      FIRST
      };

enum class DragStyle : char {
      NONE = 0,
      CANCELLED,
      SELECTION_RECT,
      NOTE_POSITION,
      NOTE_LENGTH_START,
      NOTE_LENGTH_END,
      DRAW_NOTE,
      ERASE,
      CUT,
      TIE,
      EVENT_ONTIME,
      EVENT_MOVE,
      EVENT_LENGTH,
      MOVE_VIEWPORT
      };

enum class PianoRollCursorMode : char {
      SELECT,
      SELECTION_RECT,
      ADD,
      ERASE,
      CUT,
      TIE,
      EVENT_ADJUST,
      RESIZE,
      MOVE
      };

struct BarPattern {
      QString name;
      char isWhiteKey[12];  //Set to 1 for white keys, 0 for black
      };

//---------------------------------------------------------
//   PianoItem
//---------------------------------------------------------

class PianoItem {
      Note* _note;
      PianoView* _pianoView;
      
      QRect boundingRectTicks(NoteEvent* evt);
      QRect boundingRectPixels(NoteEvent* evt);
      bool intersectsBlock(int startTick, int endTick, int highPitch, int lowPitch, NoteEvent* evt);
      
   public:
      const static int NOTE_BLOCK_CORNER_RADIUS = 3;

      PianoItem(Note*, PianoView*);
      ~PianoItem() {}
      Note* note() { return _note; }
      bool intersects(int startTick, int endTick, int highPitch, int lowPitch);
      
      QRect boundingRect();
      
      NoteEvent* getTweakNoteEvent();
      };

//---------------------------------------------------------
//   PianoView
//---------------------------------------------------------

class PianoView : public QGraphicsView {
      Q_OBJECT

public:
      static const BarPattern barPatterns[];
      void setScope(PianoRollScope scope);
      void setColoring(Coloring);
      void setUseNoteColors(bool);

      PianoRollScope getScope() { return _scope; }
      void centerSelectionTimeInView();
      void ensureSelectionVisible(bool force = false);

      bool selectionRectAllowed() const;

      Fraction levelPreviewTickOffset() const;
      Fraction levelPreviewEventTickDelta() const;

      bool levelPreviewMovesNotes() const;
      bool levelPreviewMovesEvents() const;
      bool levelEventPreview(const NoteEvent* event, int& ontime, int& len) const;

      Fraction levelPreviewLengthOffset() const;
      bool levelPreviewResizesNotes() const;

      void setLevelInteractionNotes(const QSet<const Note*>& notes);
      void clearLevelInteractionNotes();
      bool levelInteractionHighlighted(const Note* note) const;

private:
      Staff* _staff { nullptr };
      Chord* _chord { nullptr };

      bool _playbackActive { false };

      VerticalPitchLayout _verticalPitchLayout;
      PianoRollScope _scope;
      Coloring _coloring;
      bool _useNoteColors { false };

      PianoRollOrientation _orientation;
      
      Pos _trackingPos;  //Track mouse position
      Pos* _locator { nullptr };
      int _ticks;
      TType _timeType;
      int _noteHeight;
      qreal _xZoom;
      int _tuplet;  //Tuplet divisions
      int _subdiv;  //Beat subdivisions
      int _barPattern;

      bool _mouseDown;
      bool _dragStarted;
      QString _dragNoteCache;
      QPointF _mouseDownPos;
      QPointF _lastMousePos;
      QPointF _lastCutDragPos;
      int _cutDragUndoStartIdx { -1 };

      struct TieDragTarget {
            Fraction tick;
            int track;
            int pitch;

            bool operator==(const TieDragTarget& other) const
                  {
                  return tick == other.tick
                        && track == other.track
                        && pitch == other.pitch;
                  }
            };

      QVector<TieDragTarget> _tieDragTargets;
      QPointF _lastTieDragPos;
      int _tieDragUndoStartIdx { -1 };

      int _drumPaintUndoStartIdx { -1 };
      QPointF _lastDrumPaintPos;
      QSet<int> _drumPaintedTicks;

      QPointF _mouseDownScreenPos;
      QPointF _lastMouseScreenPos;
      QPointF _viewportFocus;
      QPointF _popupMenuPos;
      DragStyle _dragStyle;
      int _dragStartPitch;
      Fraction _dragStartTick;
      Fraction _dragEndTick;
      int _dragNoteLengthMargin = 4;
      bool _inProgressUndoEvent;
      bool _selectionHandledOnPress { false };

      Fraction _levelPreviewLengthOffset;
      Fraction _levelPreviewTickOffset;
      Fraction _levelPreviewEventTickDelta;
      bool _levelPreviewActive { false };

      //The length of the note we are using for editng purposes, expressed as a fraction of the measure.
      // Note length will be (2^_editNoteLength) of a measure
      Fraction _editNoteLength = Fraction(1, 4);
      int _editNoteDots = 0;
      int _editNoteVoice = 0;
      PianoRollEditTool _editNoteTool = PianoRollEditTool::EVENT_ADJUST;
      Qt::KeyboardModifiers _cursorModifiers { Qt::NoModifier };
      QCursor _addNoteCursor;
      QCursor _eraseNoteCursor;
      QCursor _scissorsNoteCursor;
      QCursor _tieNoteCursor;
      bool _automaticVoiceAssignment { true }; // testing = true

      QList<PianoItem*> _noteList;
      quint8 _pitchHighlight[128];

      float _noteRectRoundedRadius = 3;

      int _lastLocatorPixel[3] { -1, -1, -1 };
      qreal _playbackLocatorTick { 0.0 };
      bool _playbackLocatorTickValid { false };

      struct LevelEventPreview {
            int ontime;
            int len;
            };

      QSet<const Note*> _levelInteractionNotes;

      QHash<const NoteEvent*, LevelEventPreview> _levelEventPreviews;

      QHash<const Note*, QSet<int>> _playbackNoteEvents;

      virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
      void drawNoteBlock(QPainter* p, PianoItem* block);
      QRect boundingRect(const Note* note, bool applyEvents);
      QRect boundingRect(const Note* note, const NoteEvent* evt, bool applyEvents);

      bool useOnsetDiamond(const Staff* staff,
                           const Fraction& tick) const;
      bool useOnsetDiamond(const Note* note) const;

      QRect onsetDiamondRect(const Note* note,
                             const NoteEvent* event = nullptr,
                             bool applyEvents = false) const;

      QVector<Fraction> onsetPaintTicks(const QPointF& from,
                                       const QPointF& to) const;
      Fraction gridLengthAt(const Fraction& tick) const;

      Fraction clampTickToScore(const Fraction& tick) const;

      void setSelectedNoteColor();
      void resetSelectedNoteColor();

      void addChord(Chord* _chord, int voice);
      QVector<Note*> getSegmentNotes(Segment* seg, int track);
      void updateBoundingSize();
      void clearNoteData();
      void clearNoteSelection();

      void selectItem(PianoItem* item, NoteSelectType selType);
      void selectNotes(int startTick, int endTick, int lowPitch, int highPitch, NoteSelectType selType);
      void showPopupMenu(const QPoint& pos);
      bool cutChordRest(ChordRest* targetCr,
                        int track,
                        Fraction cutTick,
                        ChordRest*& cr0,
                        ChordRest*& cr1,
                        bool preserveOriginalDuration = false);
      bool noteRangeContainsChord(const Fraction& startTick, const Fraction& duration, int track) const;

      bool voiceRangeIsFree(const Fraction& startTick,
                            const Fraction& duration,
                            int track) const;

      bool voiceHasMatchingChord(const Fraction& startTick,
                                 const Fraction& duration,
                                 int track) const;

      int automaticVoiceForNote(const Fraction& startTick,
                                const Fraction& duration,
                                int pitch,
                                int staffIdx,
                                int preferredVoice) const;

      int insertionVoiceForNote(const Fraction& startTick,
                                const Fraction& duration,
                                int pitch,
                                int staffIdx,
                                int preferredVoice) const;

      QVector<Note*> addNote(Fraction startTick, Fraction duration, int pitch, int track);
      void handleSelectionClick();
      void insertNote(int modifiers);
      Fraction roundToNearestBeat(int tick, bool down = true) const;
      Fraction noteEditLength() const;
      void changeChordLength(const QPointF& pos);
      void eraseNote(const QPointF& pos);
      void appendNoteToChord(const QPointF& pos);
      void cutChord(const QPointF& pos);
      bool cutChordAt(const Fraction& tick, int track);
      bool removeTiesAtBoundary(const Fraction& tick, int track);
      bool cutChordDragSegment(const QPointF& from, const QPointF& to);
      void toggleTie(const QPointF& pos);
      bool toggleTie(Note*);
      Note* tieNoteAt(const QPointF& pos);
      bool toggleTieDragSegment(const QPointF& from, const QPointF& to);
      void compactMeasures(const QMap<Measure*, QSet<int>>& changedTracks);

      bool rangeTouchesTuplet(const Fraction& startTick,
                              const Fraction& duration,
                              int track) const;

      bool pasteWouldTouchTuplet(const QString& copiedNotes,
                                 Fraction pasteStartTick,
                                 Fraction lengthOffset,
                                 bool xIsOffset) const;

      void dragSelectionNoteGroup();
      void finishNoteGroupDrag(QMouseEvent* event);
      void finishNoteEventAdjustDrag();
      void updateTrackingPos(const QPoint& viewportPos);
      bool toolCanDragNotes() const {
            return _editNoteTool == PianoRollEditTool::SELECT || _editNoteTool == PianoRollEditTool::ADD ||
                  _editNoteTool == PianoRollEditTool::APPEND_NOTE || _editNoteTool == PianoRollEditTool::CUT ||
                  _editNoteTool == PianoRollEditTool::TIE;
            }
      bool calculateNoteDragOffsets(Fraction& pasteTickOffset,
                                    Fraction& pasteLengthOffset,
                                    int& pitchOffset) const;

      bool paintOnsetDragSegment(const QPointF& from,
                                 const QPointF& to);

      void drawPitchText(QPainter* p, const QRectF& bounds, const QString& name, const QColor& noteColor);
      QString pitchNameForMidi(int) const;

      QAction* getAction(const char* id);
      PianoRollCursorMode effectiveCursorMode() const;
      void updateCursor();

    protected:
      bool eventFilter(QObject* watched, QEvent* event) override;
      void wheelEvent(QWheelEvent* event) override;
      void keyReleaseEvent(QKeyEvent* event) override;
      void mousePressEvent(QMouseEvent* event) override;
      void mouseReleaseEvent(QMouseEvent* event) override;
      void mouseMoveEvent(QMouseEvent* event) override;
      void leaveEvent(QEvent*) override;
      void contextMenuEvent(QContextMenuEvent *event) override;

   signals:
      void xZoomChanged(qreal);
      void tupletChanged(int);
      void subdivChanged(int);
      void barPatternChanged(int);
      void noteHeightChanged(int);
      void pitchChanged(int);
      void trackingPosChanged(const Pos&);
      void selectionChanged();
      void onTimeDragged(int);
      void tickLenDragged(int);
      void showNoteTweakerRequest();
      void noteEventsChanged();
      void editNoteLengthChanged(const Fraction& duration);

   public slots:
      void moveLocator(int);
      void updateNotes();
      void setXZoom(int);
      void setTuplet(int);
      void setSubdiv(int);
      void setBarPattern(int);
      void togglePitchHighlight(int pitch);
      void showNoteTweaker();
      void setNotesToVoice(int voice);

      QString serializeSelectedNotes();
      QVector<Note*> pasteNotes(const QString& copiedNotes, Fraction pasteStartTick, Fraction lengthOffset, int pitchOffset, bool xIsOffset = false);
      void drawDraggedNotes(QPainter* painter);
      void drawDraggedNote(QPainter* painter, Fraction startTick, Fraction frac, int pitch, int track, QColor color, const QString& pitchName = QString());

      void cutNotes();
      void copyNotes();
      void pasteNotesAtCursor();

   public:
      PianoView();
      ~PianoView();
      void setVerticalPitchLayout(VerticalPitchLayout layout);
      void setOrientation(PianoRollOrientation orientation);
      Staff* staff() { return _staff; }
      void setEditableStaff(Staff* st);
      void setStaff(Staff*, Pos* locator);
      bool playbackTickBeyondCenter(qreal tick) const;
      void ensureVisible(qreal tick, qreal horizontalOffset = 0.0);
      void ensurePlaybackTickVisible(qreal tick);
      int noteHeight() { return _noteHeight; }
      qreal xZoom() { return _xZoom; }
      int tuplet() { return _tuplet; }
      int subdiv() { return _subdiv; }
      int barPattern() { return _barPattern; }
      PianoRollEditTool editTool() const { return _editNoteTool; }
      QList<QGraphicsItem*> items() { return scene()->selectedItems(); }
      int editNoteDots() const { return _editNoteDots; }

      void deleteSelectedNotes();

      void setEditNoteLength(Fraction len) { _editNoteLength = len; }
      void setEditNoteVoice(int voice) { _editNoteVoice = voice; }
      void setEditNoteDots(int dot) { _editNoteDots = dot; }
      void setEditNoteTool(PianoRollEditTool);

      void setPlaybackActive(bool active) { _playbackActive = active; }


      void setAutomaticVoiceAssignment(bool value)
            { _automaticVoiceAssignment = value; }
      bool automaticVoiceAssignment() const
            { return _automaticVoiceAssignment; }

      void setPlaybackNoteEvents(const QHash<const Note*, QSet<int>>& events);
      void clearPlaybackNoteEvents();

      void setPlaybackLocatorTick(qreal tick);
      void clearPlaybackLocatorTick();

      Fraction snapTickToGrid(int tick, Direction) const;

// TODO: Any of these that can be private should be private:

      qreal tickToPixelXF(qreal tick) const;
      qreal tickToPixelYF(qreal tick) const;

      int pixelXToTick(int pixX) const;
      int tickToPixelX(int tick) const;

      int pixelYToTick(int y) const;
      int tickToPixelY(int tick) const;

      int pixelXToPitch(int pixX) const;
      int pitchToPixelX(int pitch) const;

      int pixelYToPitch(int pixY) const;
      int pitchToPixelY(int pitch) const;

      int scenePosToTick(const QPointF& pos) const;
      int scenePosToPitch(const QPointF& pos) const;

      int dragTickDelta(const QPointF& from, const QPointF& to) const;
      int dragPitchDelta(const QPointF& from, const QPointF& to) const;

      int viewportReferenceTick() const;
      void positionViewportAtTick(int tick);
      int viewportReferencePitch() const;
      void positionViewportAtPitch(int pitch);

      QRectF keyboardAlignedPitchLane(int midiPitch) const;

      QRectF verticalPitchRect(int midiPitch) const;

      PianoItem* pickNote(int tick, int pitch);
      PianoItem* pickNote(const QPointF& pos);

      QList<PianoItem*> getSelectedItems();
      QList<PianoItem*> getItems();
      
      void zoomView(int step, bool horizontal, int centerX, int centerY);

      bool isTool(PianoRollEditTool v) const
            { return v == _editNoteTool; }
      bool eventsAdjustTool() const
            { return _editNoteTool == PianoRollEditTool::EVENT_ADJUST; }
      bool selectTool() const
            { return _editNoteTool == PianoRollEditTool::SELECT; }
      bool addTool() const
            { return _editNoteTool == PianoRollEditTool::ADD; }
      bool eraseTool() const
            { return _editNoteTool == PianoRollEditTool::ERASE; }

      bool isHorizontal() const
            { return _orientation == PianoRollOrientation::HORIZONTAL; }
      bool isVertical() const
            { return _orientation == PianoRollOrientation::VERTICAL; }

      };


} // namespace Ms
#endif

