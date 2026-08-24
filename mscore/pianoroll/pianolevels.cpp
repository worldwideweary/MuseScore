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

#include "pianolevels.h"

#include "pianoruler.h"
#include "pianokeyboard.h"
#include "pianoview.h"
#include "pianolevelsfilter.h"
#include "preferences.h"

#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/noteevent.h"
#include "libmscore/part.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/staff.h"

namespace Ms {

//---------------------------------------------------------
//   PianoLevels
//---------------------------------------------------------

PianoLevels::PianoLevels(QWidget *parent)
    : QWidget(parent)
       {
       setMouseTracking(true);
       _score     = nullptr;
       _xpos      = 0;
       _xZoom     = X_ZOOM_INITIAL;
       _locator   = nullptr;
       _staff     = nullptr;
       _tuplet    = 1;
       _subdiv    = 0;
       _levelsIndex = 2; // Velocity (relative)
       minBeatGap = 20;
       vMargin    = 10;
       levelLen   = 20;
       mouseDown  = false;
       dragging   = false;
       _scope     = PianoRollScope::PART;
       }

//---------------------------------------------------------
//   ~PianoLevels
//---------------------------------------------------------

PianoLevels::~PianoLevels()
      {
      clearNoteData();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianoLevels::setColoring(Coloring c)
      {
      if (_coloring == c)
            return;

      _coloring = c;
      update();
      }

//---------------------------------------------------------
//   setPianoView
//---------------------------------------------------------

void PianoLevels::setPianoView(PianoView* v)
      {
      _pianoView = v;
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoLevels::setOrientation(PianoRollOrientation o)
      {
      if (_orientation == o)
            return;
      _orientation = o;
      update();
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void PianoLevels::setScore(Score* s, Pos* lc)
      {
      _score = s;
      _locator = lc;
      if (_score)
            _cursor.setContext(_score->tempomap(), _score->sigmap());
      setEnabled(_score != 0);
      }

//---------------------------------------------------------
//   setXpos
//---------------------------------------------------------

void PianoLevels::setXpos(int val)
      {
      _xpos = val;
      update();
      }


//---------------------------------------------------------
//   pixelXToTick
//---------------------------------------------------------

int PianoLevels::pixelXToTick(int pixX) {
      return static_cast<int>((pixX + _xpos) / _xZoom) - MAP_OFFSET;
      }


//---------------------------------------------------------
//   tickToPixelX
//---------------------------------------------------------

int PianoLevels::tickToPixelX(int tick) {
      return static_cast<int>(tick + MAP_OFFSET) * _xZoom - _xpos;
      }


//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PianoLevels::paintEvent(QPaintEvent* e)
      {
      QPainter p(this);

      QColor colPianoBg;
      QColor noteDeselected;
      QColor noteSelected;

      QColor colGridLine;
      QColor colText;

      switch (preferences.effectiveGlobalStyle()) {
            case MuseScoreEffectiveStyleType::DARK_FUSION:
                  colPianoBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_BASE_COLOR));
                  noteDeselected = preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_UNSEL_COLOR);
                  noteSelected = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_SEL_COLOR));

                  colGridLine = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_GRIDLINE_COLOR));
                  colText = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_TEXT_COLOR));
                  break;
            default:
                  colPianoBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_BASE_COLOR));
                  noteDeselected = preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_UNSEL_COLOR);
                  noteSelected = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_SEL_COLOR));

                  colGridLine = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_GRIDLINE_COLOR));
                  colText = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_TEXT_COLOR));
                  break;
            }

      const QPen penLineMajor = QPen(colGridLine, 2.0, Qt::SolidLine);
      const QPen penLineMinor = QPen(colGridLine, 1.0, Qt::SolidLine);
      const QPen penLineSub = QPen(colGridLine, 1.0, Qt::DotLine);

      const QRect& r = e->rect();

      p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

      p.setBrush(colPianoBg);
      p.drawRect(0, 0, width(), height());

      if (!_score)
            return;

      const int timeStart = _orientation == PianoRollOrientation::HORIZONTAL
            ? r.x()
            : r.y();

      const int timeEnd = _orientation == PianoRollOrientation::HORIZONTAL
            ? r.x() + r.width()
            : r.y() + r.height();

      Pos pos1(_score->tempomap(),
               _score->sigmap(),
               qMax(pixelToTick(timeStart), 0),
               TType::TICKS);

      Pos pos2(_score->tempomap(),
               _score->sigmap(),
               qMax(pixelToTick(timeEnd), 0),
               TType::TICKS);

      //draw vert lines
      int bar1, bar2, beat, tick;

      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      //Estimate bar width since changing time signatures can make this inconsistent.
      // Assuming 480 ticks per beat, 4 beats per bar
      qreal pixPerBar = DIVISION * 4 * _xZoom;
      qreal pixPerBeat = DIVISION * _xZoom;

      int barSkip = ceil(minBeatGap / pixPerBar);
      barSkip = (int)pow(2, ceil(log(barSkip)/log(2)));

      int beatSkip = ceil(minBeatGap / pixPerBeat);
      beatSkip = (int)pow(2, ceil(log(beatSkip)/log(2)));

      //Round down to first bar to be a multiple of barSkip
      bar1 = (bar1 / barSkip) * barSkip;

      for (int bar = bar1; bar <= bar2; bar += barSkip) {
            Pos stick(_score->tempomap(), _score->sigmap(), bar, 0, 0);

            SigEvent sig = stick.timesig();
            int z = sig.timesig().numerator();
            for (int beat1 = 0; beat1 < z; beat1 += beatSkip) {
                  Pos beatPos(_score->tempomap(), _score->sigmap(), bar, beat1, 0);
                  int tp = tickToPixel(beatPos.time(TType::TICKS));
                  if (tp < 0)
                        continue;

                  if (beat1 == 0) {
                        p.setPen(penLineMajor);
                        }
                  else {
                        p.setPen(penLineMinor);
                        }

                  if (_orientation == PianoRollOrientation::HORIZONTAL)
                        p.drawLine(tp, 0, tp, height());
                  else
                        p.drawLine(0, tp, width(), tp);

                  int subbeats = _tuplet * (1 << _subdiv);

                  for (int sub = 1; sub < subbeats; ++sub) {
                        Pos subBeatPos(_score->tempomap(), _score->sigmap(), bar, beat1, sub * DIVISION / subbeats);
                        tp = tickToPixel(subBeatPos.time(TType::TICKS));

                        p.setPen(penLineSub);

                        if (_orientation == PianoRollOrientation::HORIZONTAL)
                              p.drawLine(tp, 0, tp, height());
                        else
                              p.drawLine(0, tp, width(), tp);
                        }
                  }
            }


      //draw horiz lines
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      int div = filter->divisionGap();
      int minGuide = (int)floor(filter->minRange() / (qreal)div);
      int maxGuide = (int)ceil(filter->maxRange() / (qreal)div);

      QFont f("FreeSans", 9);
      p.setFont(f);

      for (int i = minGuide; i <= maxGuide; ++i) {
            p.setPen(i == 0 || i == minGuide || i == maxGuide ? penLineMajor : penLineMinor);

            int vp = valToPixel(i * div);

            if (_orientation == PianoRollOrientation::HORIZONTAL)
                  p.drawLine(0, vp, width(), vp);
            else
                  p.drawLine(vp, 0, vp, height());

            //labels
            if (_orientation == PianoRollOrientation::HORIZONTAL) {
                  QRectF textRect(2, vp - 12, width() - 2, 12);
                  p.setPen(QPen(colText));
                  p.drawText(textRect,
                             Qt::AlignLeft | Qt::AlignBottom,
                             QString::number(i * div));
                  }
            else {
                  const QString text = QString::number(i * div);
                  const QFontMetrics fm(p.font());
                  const int textWidth = fm.width(text);

                  //
                  // Center the label under its value guide while keeping
                  // the first/last labels inside the widget.
                  //
                  const int x = qBound(
                        2,
                        vp - textWidth / 2,
                        width() - textWidth - 2);

                  QRectF textRect(
                        x,
                        height() - 14,
                        textWidth,
                        12);

                  p.setPen(QPen(colText));
                  p.drawText(textRect,
                             Qt::AlignHCenter | Qt::AlignBottom,
                             text);
                  }
            }


      //Note lines
      p.setBrush(Qt::NoBrush);
      int pix0 = valToPixel(0);

      for (int pass = 0; pass < 2; ++pass) {
            for (int i = 0; i < noteList.size(); ++i) {
                  Note* note = noteList[i];

                  //
                  // Draw unselected notes first, selected notes second,
                  // so overlapping notes from another staff cannot obscure
                  // the selection indication.
                  //
                  const bool selected = note->selected();

                  if ((pass == 0 && selected)
                      || (pass == 1 && !selected))
                        continue;

                  const bool interactionHighlighted =
                        _pianoView && _pianoView->levelInteractionHighlighted(note);

                  const QColor interactionColor =
                        darkTheme()
                              ? preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_DRAG_COLOR)
                              : preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_DRAG_COLOR);

                  noteDeselected = pianoRollNoteColor(note, _coloring, false);

                  if (filter->isPerEvent()) {

                        for (NoteEvent& ne : note->playEvents()) {
                              Fraction previewNoteTick = note->chord()->tick();
                              Fraction previewNoteLen = note->chord()->ticks();

                              if (_pianoView
                                  && note->selected()
                                  && _pianoView->levelPreviewResizesNotes()) {
                                    previewNoteTick += _pianoView->levelPreviewTickOffset();
                                    previewNoteLen += _pianoView->levelPreviewLengthOffset();
                                    }

                              int previewTick;

                              if (_pianoView
                                  && note->selected()
                                  && _pianoView->levelPreviewResizesNotes()) {
                                    Fraction eventTick =
                                          previewNoteTick
                                          + previewNoteLen * ne.ontime() / 1000;

                                    previewTick = eventTick.ticks();
                                    }
                              else {
                                    previewTick = noteStartTick(note, &ne);

                                    if (_pianoView && note->selected()) {
                                          if (_pianoView->levelPreviewMovesNotes())
                                                previewTick += _pianoView->levelPreviewTickOffset().ticks();
                                          else if (_pianoView->levelPreviewMovesEvents())
                                                previewTick += _pianoView->levelPreviewEventTickDelta().ticks();\
                                          }
                                    }

                              int tp = tickToPixel(previewTick);
                              int val = filter->value(note->staff(), note, &ne);

                              int previewOntime = ne.ontime();
                              int previewLen = ne.len();

                              bool previewValueAvailable = false;
                              if (_pianoView) {
                                    if (_pianoView->levelEventPreview(&ne, previewOntime, previewLen)) {
                                          previewValueAvailable = true;
                                          }

                                    if (note->selected() && _pianoView->levelPreviewResizesNotes()) {
                                          previewValueAvailable = true;
                                          }
                                    }

                              if (previewValueAvailable) {
                                    int previewVal;

                                    if (filter->previewValue(
                                        note,
                                        previewOntime,
                                        previewLen,
                                        previewNoteLen,
                                        previewVal)) {
                                          val = previewVal;
                                          }
                                    }

                              int vp = valToPixel(val);

                              const QColor levelColor =
                                    interactionHighlighted
                                          ? interactionColor
                                          : (selected ? noteSelected : noteDeselected);

                              drawLevelBar(p, tp, vp, pix0, levelColor);
                              }
                        }
                  else {
                        int previewTick = noteStartTick(note, nullptr);

                        if (_pianoView
                            && note->selected()
                            && _pianoView->levelPreviewMovesNotes()) {
                              previewTick += _pianoView->levelPreviewTickOffset().ticks();
                              }

                        int tp = tickToPixel(previewTick);
                        int val = filter->value(note->staff(), note, nullptr);
                        int vp = valToPixel(val);

                        const QColor levelColor =
                              interactionHighlighted
                                    ? interactionColor
                                    : (selected ? noteSelected : noteDeselected);

                        drawLevelBar(p, tp, vp, pix0, levelColor);
                        }
                  }
            }

      if (_playbackLocatorValid) {
            const int tp = tickToPixel(qRound(_playbackLocatorTick));

            p.setPen(QPen(Qt::red, 1));

            if (_orientation == PianoRollOrientation::HORIZONTAL)
                  p.drawLine(tp, 0, tp, height());
            else
                  p.drawLine(0, tp, width(), tp);
            }
      }


//---------------------------------------------------------
//   noteStartTick
//---------------------------------------------------------

int PianoLevels::noteStartTick(Note* note, NoteEvent* evt)
      {
      Chord* chord = note->chord();
      int ticks = chord->ticks().ticks();

      return note->chord()->tick().ticks() + (evt ? evt->ontime() * ticks / 1000 : 0);
      }


//---------------------------------------------------------
//   valToPixelY
//---------------------------------------------------------

int PianoLevels::valToPixelY(int value) {
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      int range = filter->maxRange() - filter->minRange();
      qreal frac = (value - filter->minRange()) / (qreal)range;

      return static_cast<int>(height() - vMargin * 2) * (1 - frac) + vMargin;
      }


//---------------------------------------------------------
//   pixelYToVal
//---------------------------------------------------------

int PianoLevels::pixelYToVal(int pix) {
      qreal frac = 1 - (pix - vMargin) / (qreal)(height() - vMargin * 2);

      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];
      int range = filter->maxRange() - filter->minRange();
      return static_cast<int>(frac * range + filter->minRange());
      }

//---------------------------------------------------------
//   pickNoteEvent
//---------------------------------------------------------

bool PianoLevels::pickNoteEvent(int x, int y, bool selectedOnly, Note*& pickedNote, NoteEvent*& pickedNoteEvent)
      {
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];
      const int pix0 = valToPixel(0);

      for (int i = 0; i < noteList.size(); ++i) {
            Note* note = noteList[i];
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& e : note->playEvents()) {
                        const int noteTick =
                              tickToPixel(noteStartTick(note, &e));

                        const int noteVal =
                              valToPixel(filter->value(note->staff(), note, &e));

                        const int noteX =
                              _orientation == PianoRollOrientation::HORIZONTAL
                              ? noteTick
                              : noteVal;

                        const int noteY =
                              _orientation == PianoRollOrientation::HORIZONTAL
                              ? noteVal
                              : noteTick;

                        bool hit;

                        if (_orientation == PianoRollOrientation::HORIZONTAL) {
                              const int left = noteX - 2;
                              const int right = noteX + levelLen + 2;

                              const int top = qMin(pix0, noteY) - 2;
                              const int bottom = qMax(pix0, noteY) + 2;

                              hit =
                                    x >= left
                                    && x <= right
                                    && y >= top
                                    && y <= bottom;
                              }
                        else {
                              const int left = qMin(pix0, noteX) - 2;
                              const int right = qMax(pix0, noteX) + 2;

                              const int top = noteY - 2;
                              const int bottom = noteY + levelLen + 2;

                              hit =
                                    x >= left
                                    && x <= right
                                    && y >= top
                                    && y <= bottom;
                              }

                        if (hit) {
                              pickedNote = note;
                              pickedNoteEvent = &e;
                              return true;
                              }
                        }
                  }
            else {
                  const int noteTick = tickToPixel(noteStartTick(note, nullptr));
                  const int noteVal = valToPixel(
                        filter->value(note->staff(), note, nullptr));

                  int noteX;
                  int noteY;

                  if (_orientation == PianoRollOrientation::HORIZONTAL) {
                        noteX = noteTick;
                        noteY = noteVal;
                        }
                  else {
                        noteX = noteVal;
                        noteY = noteTick;
                        }

                  bool hit = false;

                  if (_orientation == PianoRollOrientation::HORIZONTAL) {
                        const int left = noteX - 2;
                        const int right = noteX + levelLen + 2;

                        const int top = qMin(pix0, noteY) - 2;
                        const int bottom = qMax(pix0, noteY) + 2;

                        hit =
                              x >= left
                              && x <= right
                              && y >= top
                              && y <= bottom;
                        }
                  else if (_orientation == PianoRollOrientation::VERTICAL) {
                        const int left = qMin(pix0, noteX) - 2;
                        const int right = qMax(pix0, noteX) + 2;

                        const int top = noteY - 2;
                        const int bottom = noteY + levelLen + 2;

                        hit =
                              x >= left
                              && x <= right
                              && y >= top
                              && y <= bottom;
                        }

                  if (hit) {
                        pickedNote = note;
                        pickedNoteEvent = nullptr;
                        return true;
                        }
                  }
            }

      pickedNote = nullptr;
      pickedNoteEvent = nullptr;
      return false;
      }

//---------------------------------------------------------
//   pickNearestLevelInTimeBand
//---------------------------------------------------------

bool PianoLevels::pickNearestLevelInTimeBand(int timePixel,
                                             int valuePixel,
                                             int timeRadius,
                                             bool selectedOnly,
                                             Note*& pickedNote,
                                             NoteEvent*& pickedEvent)
      {
      pickedNote = nullptr;
      pickedEvent = nullptr;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      int bestDistance = std::numeric_limits<int>::max();

      for (Note* note : noteList) {
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& event : note->playEvents()) {
                        const int tp =
                              tickToPixel(noteStartTick(note, &event));

                        if (std::abs(tp - timePixel) > timeRadius)
                              continue;

                        const int value =
                              filter->value(
                                    note->staff(),
                                    note,
                                    &event);

                        const int vp = valToPixel(value);

                        const int dt = tp - timePixel;
                        const int dv = vp - valuePixel;

                        const int distance =
                              dt * dt + dv * dv;

                        if (distance < bestDistance) {
                              bestDistance = distance;
                              pickedNote = note;
                              pickedEvent = &event;
                              }
                        }
                  }
            else {
                  const int tp =
                        tickToPixel(noteStartTick(note, nullptr));

                  if (std::abs(tp - timePixel) > timeRadius)
                        continue;

                  const int value =
                        filter->value(
                              note->staff(),
                              note,
                              nullptr);

                  const int vp = valToPixel(value);

                  const int dt = tp - timePixel;
                  const int dv = vp - valuePixel;

                  const int distance =
                        dt * dt + dv * dv;

                  if (distance < bestDistance) {
                        bestDistance = distance;
                        pickedNote = note;
                        pickedEvent = nullptr;
                        }
                  }
            }

      return pickedNote != nullptr;
      }

//---------------------------------------------------------
//   captureLevelDragTargets
//---------------------------------------------------------

void PianoLevels::captureLevelDragTargets(Note* anchorNote,
                                          NoteEvent* anchorEvent,
                                          bool selectedOnly)
      {
      _levelDragTargets.clear();

      if (!anchorNote)
            return;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int anchorTick =
            noteStartTick(anchorNote,
                          filter->isPerEvent()
                                ? anchorEvent
                                : nullptr);

      _levelDragAnchorValue =
            filter->value(
                  anchorNote->staff(),
                  anchorNote,
                  filter->isPerEvent()
                        ? anchorEvent
                        : nullptr);

      //
      // With no score selection, capture only the explicitly
      // chosen node.  A selection is what turns simultaneous
      // nodes into a group.
      //
      if (!selectedOnly) {
            LevelDragTarget target;

            target.note = anchorNote;
            target.event =
                  filter->isPerEvent()
                        ? anchorEvent
                        : nullptr;
            target.startValue = _levelDragAnchorValue;

            _levelDragTargets.append(target);
            return;
            }

      //
      // Capture every selected level at the anchor's effective
      // time, preserving each one's original value.
      //
      for (Note* note : noteList) {
            if (!note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& event : note->playEvents()) {
                        if (noteStartTick(note, &event) != anchorTick)
                              continue;

                        LevelDragTarget target;

                        target.note = note;
                        target.event = &event;
                        target.startValue =
                              filter->value(
                                    note->staff(),
                                    note,
                                    &event);

                        _levelDragTargets.append(target);
                        }
                  }
            else {
                  if (noteStartTick(note, nullptr) != anchorTick)
                        continue;

                  LevelDragTarget target;

                  target.note = note;
                  target.event = nullptr;
                  target.startValue =
                        filter->value(
                              note->staff(),
                              note,
                              nullptr);

                  _levelDragTargets.append(target);
                  }
            }
      }

//---------------------------------------------------------
//   adjustCapturedLevels
//---------------------------------------------------------

void PianoLevels::adjustCapturedLevels(int value)
      {
      if (_levelDragTargets.isEmpty())
            return;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int delta =
            value - _levelDragAnchorValue;

      for (const LevelDragTarget& target : _levelDragTargets) {
            if (!target.note)
                  continue;

            filter->setValue(
                  target.note->staff(),
                  target.note,
                  target.event,
                  target.startValue + delta);

            _levelInteractionNotes.insert(target.note);
            }

      if (_pianoView)
            _pianoView->setLevelInteractionNotes(
                  _levelInteractionNotes);

      update();
      emit noteLevelsChanged();
      }

//---------------------------------------------------------
//   adjustLevelLerp
//---------------------------------------------------------

void PianoLevels::adjustLevel(Note* note, NoteEvent* noteEvt, int value)
      {
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      filter->setValue(note->staff(), note, noteEvt, value);

      _levelInteractionNotes.insert(note);

      if (_pianoView)
            _pianoView->setLevelInteractionNotes(_levelInteractionNotes);

      update();
      emit noteLevelsChanged();
      }

//---------------------------------------------------------
//   drawLevelBar
//---------------------------------------------------------

void PianoLevels::drawLevelBar(QPainter& p,
                               int tp,
                               int vp,
                               int pix0,
                               const QColor& color)
      {
      QColor fillColor = color;
      fillColor.setAlphaF(0.35);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const int top = qMin(pix0, vp);
            const int bottom = qMax(pix0, vp);

            QRect barRect(
                  tp,
                  top,
                  levelLen,
                  qMax(1, bottom - top));

            p.setBrush(fillColor);
            p.setPen(QPen(color, 1));
            p.drawRect(barRect);

            p.setBrush(color);
            p.setPen(QPen(color, 2));
            p.drawEllipse(tp - 4, vp - 4, 9, 9);
            }
      else if (_orientation == PianoRollOrientation::VERTICAL) {
            const int left = qMin(pix0, vp);
            const int right = qMax(pix0, vp);

            QRect barRect(
                  left,
                  tp,
                  qMax(1, right - left),
                  levelLen);

            p.setBrush(fillColor);
            p.setPen(QPen(color, 1));
            p.drawRect(barRect);

            p.setBrush(color);
            p.setPen(QPen(color, 2));
            p.drawEllipse(vp - 4, tp - 4, 9, 9);
            }
      }

//---------------------------------------------------------
//   pixelToTick
//---------------------------------------------------------

int PianoLevels::pixelToTick(int pixel) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>((pixel + _xpos) / _xZoom) - MAP_OFFSET;

      if (_pianoView && height() > 0 && _pianoView->viewport()->height() > 0) {
            const qreal pianoY =
                  qreal(pixel)
                  * _pianoView->viewport()->height()
                  / height();

            const QPointF scenePos =
                  _pianoView->mapToScene(
                        QPoint(0, qRound(pianoY)));

            return _pianoView->pixelYToTick(
                  qRound(scenePos.y()));
            }

      return 0;
      }

//---------------------------------------------------------
//   tickToPixel
//---------------------------------------------------------

int PianoLevels::tickToPixel(int tick) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>((tick + MAP_OFFSET) * _xZoom - _xpos);

      if (_pianoView && height() > 0 && _pianoView->viewport()->height() > 0) {
            const int sceneY =
                  _pianoView->tickToPixelY(tick);

            const int pianoY =
                  _pianoView->mapFromScene(
                        QPointF(0.0, sceneY)).y();

            return qRound(
                  qreal(pianoY)
                  * height()
                  / _pianoView->viewport()->height());
            }

      return 0;
      }

//---------------------------------------------------------
//   valToPixel
//---------------------------------------------------------

int PianoLevels::valToPixel(int value) const
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int range =
            filter->maxRange() - filter->minRange();

      const qreal frac =
            (value - filter->minRange()) / qreal(range);

      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>(height() - vMargin * 2)
                  * (1.0 - frac) + vMargin;

      return static_cast<int>(width() - vMargin * 2)
            * frac + vMargin;
      }

//---------------------------------------------------------
//   pixelToVal
//---------------------------------------------------------

int PianoLevels::pixelToVal(int pixel) const
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int range =
            filter->maxRange() - filter->minRange();

      qreal frac;

      if (_orientation == PianoRollOrientation::HORIZONTAL)
            frac = 1.0 -
                  (pixel - vMargin)
                  / qreal(height() - vMargin * 2);
      else
            frac =
                  (pixel - vMargin)
                  / qreal(width() - vMargin * 2);

      return static_cast<int>(
            frac * range + filter->minRange());
      }

//---------------------------------------------------------
//   hasSelectedNotes
//---------------------------------------------------------

bool PianoLevels::hasSelectedNotes() const
      {
      for (Note* note : noteList) {
            if (note->selected())
                  return true;
            }

      return false;
      }

//---------------------------------------------------------
//   adjustLevelLerp
//       For all points between tick0 and tick1, linearly interploate between value0 and value1 and
//       use it to set the value of the level.
//---------------------------------------------------------

void PianoLevels::adjustLevelLerp(int tick0, int value0, int tick1, int value1, bool selectedOnly)
      {
      if (tick1 < tick0) {
            std::swap(tick0, tick1);
            std::swap(value0, value1);
            }

      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];
      bool hitNote = false;

      for (int i = 0; i < noteList.size(); ++i) {
            Note* note = noteList[i];
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& e : note->playEvents()) {
                        int tick = noteStartTick(note, &e);
                        if (tick0 <= tick && tick <= tick1) {
                              int value = tick0 == tick1 ? value0
                                    : (value1 - value0) * (tick - tick0) / (tick1 - tick0) + value0;

                              filter->setValue(note->staff(), note, &e, value);

                              _levelInteractionNotes.insert(note);
                              hitNote = true;
                              }
                        }
                  }
            else {
                  int tick = noteStartTick(note, 0);
                  if (tick0 <= tick && tick <= tick1) {
                        int value = tick0 == tick1 ? value0
                              : (value1 - value0) * (tick - tick0) / (tick1 - tick0) + value0;
                        filter->setValue(note->staff(), note, nullptr, value);

                        _levelInteractionNotes.insert(note);
                        hitNote = true;
                        }
                  }
            }

      if (hitNote) {
            if (_pianoView)
                  _pianoView->setLevelInteractionNotes(_levelInteractionNotes);

            update();
            emit noteLevelsChanged();
            }
      }



//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoLevels::mousePressEvent(QMouseEvent* e)
      {
      if (e->button() == Qt::LeftButton) {
            _levelInteractionNotes.clear();
            _levelDragTargets.clear();

            if (_pianoView)
                  _pianoView->clearLevelInteractionNotes();

            mouseDown = true;
            mouseDownPos = e->pos();
            lastMousePos = mouseDownPos;

            const Qt::KeyboardModifiers modifiers =
                  QGuiApplication::keyboardModifiers();
            const bool forceLerp =
                  modifiers & Qt::ControlModifier;

            const bool selectedOnly = hasSelectedNotes();

            const int timePixel =
                  mouseTimePixel(mouseDownPos);

            const int valuePixel =
                  mouseValuePixel(mouseDownPos);

            const int val =
                  pixelToVal(valuePixel);

            Note* anchorNote = nullptr;
            NoteEvent* anchorEvent = nullptr;

            //
            // First try the actual visible bar/node hit area.
            //
            const int lerpPickRadius =
                  qMax(4, levelLen / 2);

            if (forceLerp) {
                  dragStyle = DragStyle::LERP;
                  }
            else {
                  const bool barHit =
                        pickNoteEvent(mouseDownPos.x(),
                                      mouseDownPos.y(),
                                      selectedOnly,
                                      anchorNote,
                                      anchorEvent);

                  if (barHit) {
                        //
                        // More than one filled bar can overlap.  Use
                        // proximity to choose the actual anchor.
                        //
                        pickNearestLevelInTimeBand(
                              timePixel,
                              valuePixel,
                              qMax(lerpPickRadius, levelLen + 2),
                              selectedOnly,
                              anchorNote,
                              anchorEvent);

                        dragStyle = DragStyle::OFFSET;
                        }
                  else if (pickNearestLevelInTimeBand(
                        timePixel,
                        valuePixel,
                        lerpPickRadius,
                        selectedOnly,
                        anchorNote,
                        anchorEvent)) {
                        dragStyle = DragStyle::OFFSET;
                        }
                  else {
                        dragStyle = DragStyle::LERP;
                        }
                  }

            if (_score && !_editCommandActive) {
                  _score->startCmd();
                  _editCommandActive = true;
                  }

            if (dragStyle == DragStyle::OFFSET) {
                  singleNoteDrag = anchorNote;
                  singleNoteEventDrag = anchorEvent;

                  captureLevelDragTargets(
                        anchorNote,
                        anchorEvent,
                        selectedOnly);

                  //
                  // Jump the anchor to the press value immediately.
                  // All captured simultaneous selected levels follow
                  // by the same delta.
                  //
                  adjustCapturedLevels(val);
                  }
            else {
                  const int lerpPickRadius =
                        qMax(4, levelLen / 2);

                  const int tick0 =
                        pixelToTick(
                              timePixel - lerpPickRadius);

                  const int tick1 =
                        pixelToTick(
                              timePixel + lerpPickRadius);

                  adjustLevelLerp(
                        tick0,
                        val,
                        tick1,
                        val,
                        selectedOnly);
                  }

            update();
            }
      }


//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoLevels::mouseReleaseEvent(QMouseEvent* e)
      {
      if (e->button() == Qt::LeftButton) {

            if (!dragging) {
                  // Handle click
                  lastMousePos = e->pos();
                  }

            if (_editCommandActive && _score) {
                  _score->endCmd();
                  _editCommandActive = false;
                  }

            _levelInteractionNotes.clear();
            _levelDragTargets.clear();

            if (_pianoView)
                  _pianoView->clearLevelInteractionNotes();

            mouseDown = false;
            dragging = false;
            update();
            }
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoLevels::mouseMoveEvent(QMouseEvent* e)
      {
      int modifiers = QGuiApplication::keyboardModifiers();
      bool bnShift = modifiers & Qt::ShiftModifier;

      if (mouseDown) {
            if (!dragging) {
                  int dx = e->x() - mouseDownPos.x();
                  int dy = e->y() - mouseDownPos.y();
                  if (dx * dx + dy * dy > pickRadius * pickRadius) {
                        // Start dragging
                        dragging = true;

                        if (_score && !_editCommandActive) {
                              _score->startCmd();
                              _editCommandActive = true;
                              }
                        }
                  }

            if (dragging) {
                  if (dragStyle == DragStyle::OFFSET) {
                        const int val = pixelToVal(mouseValuePixel(e->pos()));
                        adjustCapturedLevels(val);
                        }
                  else {
                        int tick0 =
                              pixelToTick(mouseTimePixel(lastMousePos));
                        int tick1 =
                              pixelToTick(mouseTimePixel(e->pos()));

                        int val0;
                        int val1;

                        if (bnShift) {
                              val0 = pixelToVal(mouseValuePixel(mouseDownPos));
                              val1 = val0;
                              }
                        else {
                              val0 = pixelToVal(mouseValuePixel(lastMousePos));
                              val1 = pixelToVal(mouseValuePixel(e->pos()));
                              }

                        adjustLevelLerp(tick0, val0, tick1, val1, hasSelectedNotes());
                        }

                  lastMousePos = e->pos();
                  update();
                  }

            }
      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianoLevels::moveLocator(QMouseEvent* e)
      {
      Pos pos(
            _score->tempomap(),
            _score->sigmap(),
            qMax(pixelToTick(mouseTimePixel(e->pos())), 0),
            TType::TICKS);

      if (e->buttons() & Qt::LeftButton)
            emit locatorMoved(0, pos);
      else if (e->buttons() & Qt::MiddleButton)
            emit locatorMoved(1, pos);
      else if (e->buttons() & Qt::RightButton)
            emit locatorMoved(2, pos);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoLevels::leaveEvent(QEvent*)
      {
      _cursor.setInvalid();
      emit posChanged(_cursor);
      update();
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PianoLevels::setPos(const Pos& pos)
      {
      if (_cursor != pos) {
            _cursor = pos;
            update();
            }
      }

//---------------------------------------------------------
//   setXZoom
//---------------------------------------------------------

void PianoLevels::setXZoom(qreal xZoom)
      {
      _xZoom = xZoom;
      update();
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoLevels::setStaff(Staff* s, Pos* l)
      {
      _locator = l;

      if (_staff == s)
            return;

      _staff    = s;
      updateNotes();
      }


//---------------------------------------------------------
//   addChord
//---------------------------------------------------------

void PianoLevels::addChord(Chord* chord, int voice)
      {
      for (Chord*& c : chord->graceNotes())
            addChord(c, voice);
      for (Note* note : chord->notes()) {
            if (note->tieBack())
                  continue;
            noteList.append(note);
            }
      }

//---------------------------------------------------------
//   updateNotes
//    Todo: factor out updateNotes between pianolevels and pianoview
//---------------------------------------------------------

void PianoLevels::updateNotes()
      {
      clearNoteData();

      if (!_staff)
            return;

      const Score* const score = _staff->score();

      SegmentType st = SegmentType::ChordRest;

      if (_scope == PianoRollScope::STAFF) {
            int staffIdx = _staff->idx();
            if (staffIdx == -1)
                  return;

            for (Segment* s = score->firstSegment(st); s; s = s->next1(st)) {
                  for (int voice = 0; voice < VOICES; ++voice) {
                        int track = voice + staffIdx * VOICES;
                        Element* e = s->element(track);
                        if (e && e->isChord())
                              addChord(toChord(e), voice);
                        }
                  }
            }
      else if (_scope == PianoRollScope::PART) {
            Part* part = _staff->part();
            if (!part || !part->staves())
                  return;

            const QList<Staff*>* staves = part->staves();

            for (Segment* s = score->firstSegment(st); s; s = s->next1(st)) {
                  for (Staff* staff : *staves) {
                        int staffIdx = staff->idx();
                        if (staffIdx == -1)
                              continue;

                        for (int voice = 0; voice < VOICES; ++voice) {
                              int track = voice + staffIdx * VOICES;
                              Element* e = s->element(track);
                              if (e && e->isChord())
                                    addChord(toChord(e), voice);
                              }
                        }
                  }
            }
      else if (_scope == PianoRollScope::SCORE) {
            for (Segment* s = score->firstSegment(st); s; s = s->next1(st)) {
                  for (Staff* staff : score->staves()) {
                        if (!staff)
                              continue;

                        int staffIdx = staff->idx();
                        if (staffIdx == -1)
                              continue;

                        for (int voice = 0; voice < VOICES; ++voice) {
                              int track = voice + staffIdx * VOICES;
                              Element* e = s->element(track);

                              if (e && e->isChord())
                                    addChord(toChord(e), voice);
                              }
                        }
                  }
            }

      update();
      }

//---------------------------------------------------------
//   setPlaybackLocatorTick
//---------------------------------------------------------

void PianoLevels::setPlaybackLocatorTick(qreal tick)
      {
      _playbackLocatorTick = tick;
      _playbackLocatorValid = true;
      update();
      }

//---------------------------------------------------------
//   clearPlaybackLocatorTick
//---------------------------------------------------------
void PianoLevels::clearPlaybackLocatorTick()
      {
      if (!_playbackLocatorValid)
            return;

      _playbackLocatorValid = false;
      update();
      }

//---------------------------------------------------------
//   setScope
//---------------------------------------------------------

void PianoLevels::setScope(PianoRollScope scope)
      {
      if (_scope == scope)
            return;

      _scope = scope;
      updateNotes();
      }

//---------------------------------------------------------
//   mouseTimePixel
//---------------------------------------------------------

int PianoLevels::mouseTimePixel(const QPointF& pos) const
      {
      return _orientation == PianoRollOrientation::HORIZONTAL
            ? qRound(pos.x())
            : qRound(pos.y());
      }

//---------------------------------------------------------
//   mouseValuePixel
//---------------------------------------------------------

int PianoLevels::mouseValuePixel(const QPointF& pos) const
      {
      return _orientation == PianoRollOrientation::HORIZONTAL
            ? qRound(pos.y())
            : qRound(pos.x());
      }

//---------------------------------------------------------
//   clearNoteData
//---------------------------------------------------------

void PianoLevels::clearNoteData()
      {
      noteList.clear();
      }

//---------------------------------------------------------
//   setTuplet
//---------------------------------------------------------

void PianoLevels::setTuplet(int value)
      {
      if (_tuplet != value) {
            _tuplet = value;
            update();
            emit tupletChanged(_tuplet);
            }
      }

//---------------------------------------------------------
//   setSubdiv
//---------------------------------------------------------

void PianoLevels::setSubdiv(int value)
      {
      if (_subdiv != value) {
            _subdiv = value;
            update();
            emit subdivChanged(_subdiv);
            }
      }

//---------------------------------------------------------
//   setLevelsIndex
//---------------------------------------------------------

void PianoLevels::setLevelsIndex(int index)
      {
      if (_levelsIndex != index) {
            _levelsIndex = index;
            update();
            emit levelsIndexChanged(_levelsIndex);
            }
      }

}
