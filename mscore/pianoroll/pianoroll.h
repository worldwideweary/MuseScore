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

#ifndef __PIANOROLL_H__
#define __PIANOROLL_H__

namespace Awl {
      class PitchEdit;
      class PosLabel;
      };

#include "libmscore/mscoreview.h"
#include "libmscore/pos.h"
#include "libmscore/score.h"
#include "libmscore/select.h"
#include "pianorolledittool.h"

#include <QElapsedTimer>
#include <QTimer>

namespace Ms {

class Score;
class Staff;
class PianoView;
class PianoKeyboard;
class PianoLevels;
class PianoLevelsChooser;
class NoteTweakerDialog;
class Note;
class PianoRuler;
class Seq;
class WaveView;

//---------------------------------------------------------
//   PianorollEditor
//---------------------------------------------------------

class PianorollEditor : public QWidget, public MuseScoreView {
      Q_OBJECT

      PianoView* pianoView { nullptr };
      PianoKeyboard* pianoKbd { nullptr };
      PianoLevels* pianoLevels { nullptr };
      PianoLevelsChooser* pianoLevelsChooser { nullptr };
      QWidget* levelsAreaWidget { nullptr };
      bool _showPianoLevels { true };
      QScrollBar* hsb { nullptr };        // horizontal scroll bar for pianoView
      QGridLayout* noteAreaLayout { nullptr };
      QWidget* topLeftSpacer { nullptr };
      Score* _score { nullptr };
      Staff* staff { nullptr };
      QLabel* partLabel { nullptr };
      Awl::PitchEdit* pitch { nullptr };
      QSpinBox* velocity { nullptr };
      QSpinBox* onTime { nullptr };
      QSpinBox* tickLen { nullptr };
      Pos locator[3];
      QComboBox* barPattern { nullptr };
      QComboBox* veloType { nullptr };
      QSpinBox* subdiv { nullptr };
      QSpinBox* tuplet { nullptr };
      Awl::PosLabel* pos { nullptr };
      PianoRuler* ruler { nullptr };
      QAction* showWave { nullptr };
      WaveView* waveView { nullptr };
      QSplitter* split { nullptr };
      QList<QAction*> actions;
      PianoRollScope _scope;
      PianoRollOrientation _orientation;
      Coloring _coloring;

      int _horizontalPitchScrollPos { 0 };
      int _verticalPitchScrollPos { 0 };
      bool _horizontalPitchScrollValid { false };
      bool _verticalPitchScrollValid { false };

      int _previewOnTime { 0 };
      int _previewLen { 1000 };

      QTimer* _playbackFollowTimer { nullptr };
      QElapsedTimer _playbackFollowElapsed;
      qreal _playbackFollowBaseTick { 0.0 };
      unsigned _playbackFollowLastSampleTick { 0 };
      bool _playbackFollowActive { false };
      qreal _playbackFollowTicksPerSecond { 0.0 };
      bool _playbackFollowVelocityValid { false };
      qreal _playbackFollowHorizontalOffset { 0.0 };

      bool updateScheduled = false;
      NoteTweakerDialog* noteTweakerDlg { nullptr };

      void updateVelocity(Note* note);
      void updateSelection();
      void readSettings();
      void doUpdate();
      void applyPitchEdit();

      void stopPlaybackFollow();

   private slots:
      void selectionChanged();
      void veloTypeChanged(int);
      void velocityChanged(int);
      void keyPressed(int);
      void keyReleased(int);
      void moveLocator(int, const Pos&);
      void cmd(QAction*);
      void rangeChanged(int min, int max);
      void setXpos(int x);
      void showWaveView(bool);
      void posChanged(POS pos, unsigned tick);
      void tickLenChanged(int);
      void onTimeChanged(int val);
      void playlistChanged();

   public slots:
      void changeSelection(SelState);
      void handleAction(QAction*);
      void showNoteTweaker();
      void setOnTime(int);
      void setTickLen(int);
      void setPianoLevelsVisible(bool visible);

   public:
      PianorollEditor(QWidget* parent = 0);
      virtual ~PianorollEditor();

      bool eventFilter(QObject* obj, QEvent* event) override;

      void restoreScoreViewFocus();

      void setStaff(Staff* staff);
      void focusOnPosition(Position* p);
      void heartBeat(Seq*);

      void setEditNoteLength(int);
      void setEditNoteVoice(int);
      void setEditNoteTool(PianoRollEditTool);
      void setEditNoteDots(int, QToolButton*);

      virtual void dataChanged(const QRectF&) override;
      virtual void updateAll() override;
      virtual void removeScore() override;
      virtual void changeEditElement(Element*) override;
      virtual QCursor cursor() const override;
      virtual void setCursor(const QCursor&) override;
      const QTransform& matrix() const;
      virtual Element* elementNear(QPointF) override;
      virtual void drawBackground(QPainter* /*p*/, const QRectF& /*r*/) const override {}
      virtual void drawBackgroundOffset(QPainter*, const QRectF&, const QRectF&, const Element*) const override {}

      void clearPlaybackPitches();

      void setLocator(POS posi, int tick) { locator[int(posi)].setTick(tick); }

      void updateOrientationLayout();
      void setOrientation(PianoRollOrientation);
      void setScope(PianoRollScope scope);
      void setColoring(Coloring);

      void writeSettings();
      virtual const QRect geometry() const override { return QWidget::geometry(); }

      void zoom(int amount = 1, bool horiz = true);
      };


} // namespace Ms
#endif


