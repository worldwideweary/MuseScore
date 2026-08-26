//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "pianoroll.h"
#include "shortcut.h"
#include "config.h"
#include "pianokeyboard.h"
#include "pianoruler.h"
#include "pianolevels.h"
#include "pianolevelschooser.h"
#include "pianoview.h"
#include "musescore.h"
#include "seq.h"
#include "scoreview.h"
#include "preferences.h"
#include "waveview.h"
#include "notetweakerdialog.h"
#include "libmscore/accidental.h"
#include "libmscore/staff.h"
#include "libmscore/measure.h"
#include "libmscore/note.h"
#include "libmscore/repeatlist.h"
#include "libmscore/score.h"
#include "libmscore/tempo.h"
#include "libmscore/undo.h"
#include "libmscore/part.h"
#include "libmscore/instrument.h"
#include "awl/pitchlabel.h"
#include "awl/pitchedit.h"
#include "awl/poslabel.h"


namespace Ms {

//---------------------------------------------------------
//   PianorollEditor
//---------------------------------------------------------

PianorollEditor::PianorollEditor(QWidget* parent)
   : QWidget(parent)
      {
      setObjectName("Pianoroll");
      setWindowTitle(QString("MuseScore"));

      waveView = 0;
      _score   = 0;
      staff    = 0;

      _scope = PianoRollScope::PART;
      _orientation = PianoRollOrientation::HORIZONTAL;

      const QSize toolbarIconSize(
            preferences.getInt(PREF_UI_THEME_ICONWIDTH),
            preferences.getInt(PREF_UI_THEME_ICONHEIGHT));

      QActionGroup* ag = Shortcut::getActionGroupForWidget(MsWidget::PIANO_ROLL_EDITOR);
      ag->setParent(this);
      addActions(ag->actions());
      connect(ag, SIGNAL(triggered(QAction*)), this, SLOT(handleAction(QAction*)));

      noteTweakerDlg = new NoteTweakerDialog(this);


      QWidget* mainWidget = new QWidget;
      tbMain = new QToolBar("Toolbar Main", this);
      tbMain->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbMain->setIconSize(toolbarIconSize);

      if (qApp->layoutDirection() == Qt::LayoutDirection::LeftToRight) {
            tbMain->addAction(getAction("undo"));
            tbMain->addAction(getAction("redo"));
            }
      else {
            tbMain->addAction(getAction("redo"));
            tbMain->addAction(getAction("undo"));
            }
      tbMain->addSeparator();
#ifdef HAS_MIDI
      tbMain->addAction(getAction("midi-on"));
#endif
      tbMain->addSeparator();

      tbMain->addAction(getAction("rewind"));
      tbMain->addAction(getAction("play"));
      tbMain->addSeparator();

      tbMain->addAction(getAction("loop"));
      tbMain->addSeparator();
      tbMain->addAction(getAction("repeat"));
      QAction* followAction = getAction("follow");
      followAction->setChecked(preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG));
      tbMain->addAction(followAction);
      tbMain->addSeparator();
      tbMain->addAction(getAction("metronome"));

      showWave = new QAction(tr("Wave"), tbMain);
      showWave->setToolTip(tr("Show wave display"));
      showWave->setCheckable(true);
      showWave->setChecked(false);
      connect(showWave, SIGNAL(toggled(bool)), SLOT(showWaveView(bool)));
      // Wave view is currently defunct:
      // tbMain->addAction(showWave);

      tbMain->addSeparator();

      staffBox = new QComboBox;
      staffBox->setToolTip(tr("Editable staff"));
      tbMain->addWidget(staffBox);

      connect(staffBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this](int index) {
                    if (!_score)
                          return;

                    const int staffIdx = staffBox->itemData(index).toInt();

                    if (staffIdx >= 0 && staffIdx < _score->nstaves())
                          setStaff(_score->staff(staffIdx));

                    restoreScoreViewFocus();
                    });

      // Option: Orientation Horizontal/Vertical
      tbMain->addSeparator();
      QComboBox* orientationBox = new QComboBox;
      orientationBox->setToolTip(tr("View orientation"));
      orientationBox->addItem(tr("Horizontal"), int(PianoRollOrientation::HORIZONTAL));
      orientationBox->addItem(tr("Vertical"),   int(PianoRollOrientation::VERTICAL));

      int orientationIndex = orientationBox->findData(int(_orientation));
      if (orientationIndex != -1)
            orientationBox->setCurrentIndex(orientationIndex);

      tbMain->addWidget(orientationBox);

      connect(orientationBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this, orientationBox](int index) {
                    setOrientation(
                          PianoRollOrientation(
                                orientationBox->itemData(index).toInt()));
                    });

      tbMain->addSeparator();

      // Option: Scope
      QComboBox* scopeBox = new QComboBox;
      scopeBox->addItem(tr("Staff"), int(PianoRollScope::STAFF));
      scopeBox->setToolTip(tr("Displayed scope"));
      scopeBox->addItem(tr("Part"),  int(PianoRollScope::PART));
      scopeBox->addItem(tr("Score"),  int(PianoRollScope::SCORE));

      int scopeIndex = scopeBox->findData(int(_scope));
      if (scopeIndex != -1)
            scopeBox->setCurrentIndex(scopeIndex);

      tbMain->addWidget(scopeBox);

      connect(scopeBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this, scopeBox](int index) {
                    setScope(PianoRollScope(scopeBox->itemData(index).toInt()));
                    });

      tbMain->addSeparator();

      // Option: Show levels editor
      QAction* showLevelsAction = new QAction(tr("Levels"), this);
      showLevelsAction->setCheckable(true);
      showLevelsAction->setChecked(_showPianoLevels);

      connect(showLevelsAction, &QAction::toggled,
              this, &PianorollEditor::setPianoLevelsVisible);

      tbMain->addAction(showLevelsAction);

      tbMain->addSeparator();

      // Option: Voice coloring / Unselect preference coloring:
      QComboBox* coloringBox = new QComboBox;
      coloringBox->setToolTip(tr("Coloring scheme"));
      coloringBox->addItem(tr("Voicing"),   int(Coloring::VOICING));
      coloringBox->addItem(tr("Singular"),  int(Coloring::STAFF));
      coloringBox->addItem(tr("Instrument"), int(Coloring::INSTRUMENT));

      tbMain->addWidget(coloringBox);

      auto applyColoring = [this, coloringBox](int index) {
            Coloring c = static_cast<Coloring>(coloringBox->itemData(index).toInt());
            setColoring(c);
            };

      connect(coloringBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              applyColoring);
      // Call applyColoring after other constructions later:

      QAction* useNoteColorsAction = new QAction(
            *icons[int(Icons::noteheadColor_ICON)],
            QString(),
            this);

      _useNoteColors = preferences.getBool(PREF_UI_PIANOROLL_USE_NOTE_COLORS);
      useNoteColorsAction->setCheckable(true);
      useNoteColorsAction->setChecked(_useNoteColors);
      useNoteColorsAction->setToolTip(
            tr("Honor user-defined note colors"));

      connect(useNoteColorsAction,
              &QAction::toggled,
              this,
              [this](bool checked) {
                    setUseNoteColors(checked);
                    restoreScoreViewFocus();
                    });

      tbMain->addAction(useNoteColorsAction);

      // Option: Show pitch names
      QAction* showPitchNamesAction = new QAction(
            QIcon(":/data/icons/note-show-pitch.svg"),
            tr("Show pitch names"),
            this);

      showPitchNamesAction->setCheckable(true);
      showPitchNamesAction->setChecked(
            preferences.getBool(PREF_UI_PIANOROLL_SHOW_PITCH_TEXT));

      showPitchNamesAction->setToolTip(tr("Show pitch names"));
      showPitchNamesAction->setStatusTip(tr("Show pitch names"));

      connect(showPitchNamesAction,
              &QAction::toggled,
              this,
              [this](bool checked) {
                    preferences.setPreference(
                          PREF_UI_PIANOROLL_SHOW_PITCH_TEXT,
                          checked);

                    if (pianoView)
                          pianoView->viewport()->update();

                    restoreScoreViewFocus();
                    });

      tbMain->addAction(showPitchNamesAction);



      // --------------------------------------------------
      // toolbars


      //----

      tbTool = new QToolBar("Action Buttons", this);
      QButtonGroup* bngrpActionBns = new QButtonGroup();
      tbTool->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbTool->setIconSize(toolbarIconSize);

      struct ToolIconData
      {
            QString _icon;
            QString _tooltip;
            PianoRollEditTool _tool;
            bool _selected;
            };
      ToolIconData _iconDataTool[] = {
            { QStringLiteral(":/data/icons/preEdit-select.svg"), tr("Select Notes"), PianoRollEditTool::SELECT, false },
            { QStringLiteral(":/data/icons/preEdit-insertNote.svg"), tr("Add Note"), PianoRollEditTool::ADD, false },
            //{ QStringLiteral(":/data/icons/preEdit-appendChord.svg"), tr("Append Note to Chord"), PianoRollEditTool::APPEND_NOTE, false },
            { QStringLiteral(":/data/icons/preEdit-cutNote.svg"), tr("Cut Chord"), PianoRollEditTool::CUT, false },
            { QStringLiteral(":/data/icons/preEdit-eraseNote.svg"), tr("Erase Note"), PianoRollEditTool::ERASE, false },
            { QStringLiteral(":/data/icons/preEdit-changeLength.svg"), tr("Change Playback Length"), PianoRollEditTool::EVENT_ADJUST, true },
            { QStringLiteral(":/data/icons/preEdit-tie.svg"), tr("Toggle Tie"), PianoRollEditTool::TIE, false },
            { "", "", PianoRollEditTool::LAST, false },
            };

      for (ToolIconData* p = _iconDataTool; p->_tool != PianoRollEditTool::LAST; ++p) {
            QToolButton* bn = new QToolButton();
            QIcon icon;
            icon.addFile(p->_icon, QSize(), QIcon::Normal, QIcon::Off);
            bn->setIcon(icon);
            bn->setIconSize(toolbarIconSize);
            bn->setCheckable(true);
            bn->setToolTip(p->_tooltip);
            PianoRollEditTool tool = p->_tool;
            connect(bn, &QToolButton::clicked, this, [=, this]() {this->setEditNoteTool(tool); });

            if (p->_selected)
                  bn->setChecked(true);
            bngrpActionBns->addButton(bn);
            tbTool->addWidget(bn);
            }

      //----

      struct LenIconData
      {
            QString _icon;
            int _measureFrac;  //Note length is 2^n of a measure
            bool _selected;
      };

      LenIconData _iconData[] = {
            { QStringLiteral(":/data/icons/note-longa.svg"), 2, false },
            { QStringLiteral(":/data/icons/note-breve.svg"), 1, false },
            { QStringLiteral(":/data/icons/note-1.svg"), 0, true },
            { QStringLiteral(":/data/icons/note-2.svg"), -1, false },
            { QStringLiteral(":/data/icons/note-4.svg"), -2, false },
            { QStringLiteral(":/data/icons/note-8.svg"), -3, false },
            { QStringLiteral(":/data/icons/note-16.svg"), -4, false },
            { QStringLiteral(":/data/icons/note-32.svg"), -5, false },
            { QStringLiteral(":/data/icons/note-64.svg"), -6, false },
            { QStringLiteral(":/data/icons/note-128.svg"), -7, false },
            { QStringLiteral(":/data/icons/note-256.svg"), -8, false },
            { QStringLiteral(":/data/icons/note-512.svg"), -9, false },
            { QStringLiteral(":/data/icons/note-1024.svg"), -10, false },
            { "", 0, false },
            };

      tbNoteLen = new QToolBar("Toolbar Note Length", this);
      bngrpNoteLen = new QButtonGroup(this);
      tbNoteLen->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbNoteLen->setIconSize(toolbarIconSize);

      for (LenIconData* p = _iconData; !p->_icon.isEmpty(); ++p) {
            QToolButton* bnLen = new QToolButton();
            QIcon icon;
            icon.addFile(p->_icon, QSize(), QIcon::Normal, QIcon::Off);
            bnLen->setIcon(icon);
            bnLen->setIconSize(toolbarIconSize);
            bnLen->setCheckable(true);

            int length = p->_measureFrac;
            bnLen->setProperty("measureFrac", length);

            connect(bnLen,
                    &QToolButton::clicked,
                    this,
                    [=, this]() {
                          setEditNoteLength(length);
                          });

            if (p->_selected)
                  bnLen->setChecked(true);
            bngrpNoteLen->addButton(bnLen);
            tbNoteLen->addWidget(bnLen);
            }

      //----

      tbDots = new QToolBar("Toolbar Dots", this);
      bngrpNoteDot = new QButtonGroup(this);
      tbDots->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbDots->setIconSize(toolbarIconSize);

      struct DotIconData
      {
            QString _icon;
            int _len;
            bool _selected;
      };
      DotIconData _iconDotData[] = {
            { QStringLiteral(":/data/icons/note-dot.svg"), 1, false },
            { QStringLiteral(":/data/icons/note-double-dot.svg"), 2, false },
            { QStringLiteral(":/data/icons/note-dot3.svg"), 3, false },
            { QStringLiteral(":/data/icons/note-dot4.svg"), 4, false },
            { "", -1, false },
            };


      for (DotIconData* p = _iconDotData; p->_len != -1; ++p) {
            QToolButton* bn = new QToolButton();
            QIcon icon;
            icon.addFile(p->_icon, QSize(), QIcon::Normal, QIcon::Off);
            bn->setIcon(icon);
            bn->setIconSize(toolbarIconSize);
            bn->setCheckable(true);

            int length = p->_len;
            bn->setProperty("dots", length);

            connect(bn,
                    &QToolButton::clicked,
                    this,
                    [=, this]() {
                          setEditNoteDots(length, bn);
                          });

            if (p->_selected)
                  bn->setChecked(true);
            bngrpNoteDot->addButton(bn);
            tbDots->addWidget(bn);
            }


      //----

      tbVoices = new QToolBar("Toolbar Voices", this);
      QButtonGroup* bngrpVoices = new QButtonGroup();
      tbVoices->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbVoices->setIconSize(toolbarIconSize);
      //bngrpNoteLen = new QButtonGroup();

      struct VoiceIconData
      {
            QString _icon;
            QString _tooltip;
            int _voice;
            bool _selected;
      };
      VoiceIconData _iconDataVoice[] = {
            { QStringLiteral(":/data/icons/voice-1.svg"), tr("Voice 1"), 0, true },
            { QStringLiteral(":/data/icons/voice-2.svg"), tr("Voice 2"), 1, false },
            { QStringLiteral(":/data/icons/voice-3.svg"), tr("Voice 3"), 2, false },
            { QStringLiteral(":/data/icons/voice-4.svg"), tr("Voice 4"), 3, false },
            { "", "", -1, false },
            };

      for (VoiceIconData* p = _iconDataVoice; p->_voice != -1; ++p) {
            QToolButton* bn = new QToolButton();
            QIcon icon;
            icon.addFile(p->_icon, QSize(), QIcon::Normal, QIcon::Off);
            bn->setIcon(icon);
            bn->setIconSize(toolbarIconSize);
            bn->setCheckable(true);
            bn->setToolTip(p->_tooltip);
            int voice = p->_voice;
            connect(bn, &QToolButton::clicked, this, [=, this](){this->setEditNoteVoice(voice);});

            if (p->_selected)
                  bn->setChecked(true);
            bngrpVoices->addButton(bn);
            tbVoices->addWidget(bn);
            }

      // --------------------------------------------------
      // empty area for spacing

      tbTweak = new QToolBar("Toolbar Tweak", this);
      tbTweak->setIconSize(toolbarIconSize);


      tbTweak->addWidget(new QLabel(tr("Cursor:")));
      pos = new Awl::PosLabel;
      pos->setFrameStyle(static_cast<int>(QFrame::NoFrame) | static_cast<int>(QFrame::Plain));

      tbTweak->addWidget(pos);
      Awl::PitchLabel* pl = new Awl::PitchLabel();
      pl->setFrameStyle(static_cast<int>(QFrame::NoFrame) | static_cast<int>(QFrame::Plain));
      tbTweak->addWidget(pl);

      tbTweak->addSeparator();

      tbTweak->addWidget(new QLabel(tr("Subdiv.:")));
      subdiv = new QSpinBox;
      subdiv->setToolTip(tr("Subdivide the beat this many times"));
      subdiv->setMinimum(0);
      subdiv->setValue(0);
      tbTweak->addWidget(subdiv);

      tbTweak->addWidget(new QLabel(tr("Tuplet:")));
      tuplet = new QSpinBox;
      tuplet->setToolTip(tr("Edit notes aligned to tuplets of this many beats"));
      tuplet->setMinimum(1);
      tuplet->setValue(1);
      tbTweak->addWidget(tuplet);

      tbTweak->addWidget(new QLabel(tr("Stripe Pattern:")));
      barPattern = new QComboBox;
      barPattern->setToolTip(tr("White stripes show the tones of this chord."));
      for (int i = 0; !PianoView::barPatterns[i].name.isEmpty(); ++i) {
            barPattern->addItem(qApp->translate("BarPattern", PianoView::barPatterns[i].name.toUtf8().data()), i);
            }
      tbTweak->addWidget(barPattern);




      // Option: Keyboard aligned grid
      keyboardAlignedGridSeparator = tbTweak->addSeparator();

      keyboardAlignedGrid =
            new QCheckBox(tr("Keyboard-aligned grid"));

      keyboardAlignedGrid->setToolTip(
            tr("Align the vertical piano-roll pitch lanes with the keyboard"));
      keyboardAlignedGrid->setChecked(
            preferences.getBool(
                  PREF_UI_PIANOROLL_VERTICAL_KEYBOARD_ALIGNED_GRID));

      keyboardAlignedGridAction =
            tbTweak->addWidget(keyboardAlignedGrid);

      connect(keyboardAlignedGrid, &QCheckBox::toggled,
            this, [this](bool checked) {
                  preferences.setPreference(
                        PREF_UI_PIANOROLL_VERTICAL_KEYBOARD_ALIGNED_GRID,
                        checked);

                  pianoView->setVerticalPitchLayout(
                        checked
                              ? VerticalPitchLayout::KEYBOARD_ALIGNED
                              : VerticalPitchLayout::CHROMATIC);
                  restoreScoreViewFocus();
                  });

      tbTweak->addSeparator();

      // Option: Velocity type
      tbTweak->addWidget(new QLabel(tr("Velocity:")));
      veloType = new QComboBox;
      veloType->addItem(tr("Offset"), int(Note::ValueType::OFFSET_VAL));
      veloType->addItem(tr("User"),   int (Note::ValueType::USER_VAL));
      tbTweak->addWidget(veloType);

      velocity = new QSpinBox;
      velocity->setRange(-127, 127);
      velocity->setReadOnly(true);

      velocity->setPrefix("+");
      const int velocityWidth = velocity->sizeHint().width();
      velocity->setPrefix("");
      velocity->setMinimumWidth(velocityWidth);

      tbTweak->addWidget(velocity);

      tbTweak->addWidget(new QLabel(tr("Pitch:")));
      pitch = new Awl::PitchEdit;
      pitch->setReadOnly(false);
      tbTweak->addWidget(pitch);

      tbTweak->addWidget(new QLabel(tr("OnTime:")));
      tbTweak->addWidget((onTime = new QSpinBox));
      onTime->setRange(-60000, +60000);
      onTime->setSingleStep(50);

      tbTweak->addWidget(new QLabel(tr("Len:")));
      tbTweak->addWidget((tickLen = new QSpinBox));
      tickLen->setRange(-2000, 60000);
      tickLen->setSingleStep(50);


      // --------------------------------------------------
      // empty area for spacing

      topLeftSpacer = new QWidget;
      topLeftSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      topLeftSpacer->setFixedWidth(PIANO_KEYBOARD_WIDTH);
      topLeftSpacer->setFixedHeight(pianoRulerHeight);

      ruler = new PianoRuler;
      ruler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      ruler->setFixedHeight(pianoRulerHeight);

      pianoKbd = new PianoKeyboard;
      pianoKbd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      pianoKbd->setFixedWidth(PIANO_KEYBOARD_WIDTH);
      pianoKbd->setOrientation(PianoOrientation::HORIZONTAL);
      pianoKbd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      pianoKbd->setFixedHeight(PIANO_KEYBOARD_WIDTH);
      pianoKbd->setMaximumWidth(QWIDGETSIZE_MAX);
      pianoKbd->setMinimumWidth(0);



      pianoView = new PianoView;
      pianoView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      pianoView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      ruler->setPianoView(pianoView);

      hsb = new QScrollBar(Qt::Horizontal);
      connect(pianoView->horizontalScrollBar(), SIGNAL(rangeChanged(int,int)),
            SLOT(rangeChanged(int,int)));

      QWidget* noteAreaWidget = new QWidget;

      noteAreaLayout = new QGridLayout;
      noteAreaLayout->setContentsMargins(0, 0, 0, 0);
      noteAreaLayout->setSpacing(0);

      // TEMP change
      // noteAreaLayout->addWidget(topLeftSpacer,  0, 0, 1, 1);
      // noteAreaLayout->addWidget(ruler,          0, 1, 1, 1);
      // noteAreaLayout->addWidget(pianoKbd,       1, 0, 1, 1);
      // noteAreaLayout->addWidget(pianoView,      1, 1, 1, 1);
      // noteAreaLayout->addWidget(hsb,            2, 1, 1, 1);

      noteAreaLayout->addWidget(pianoView, 0, 0);
      noteAreaLayout->addWidget(pianoKbd,  1, 0);

      noteAreaWidget->setLayout(noteAreaLayout);
      updateOrientationLayout();

      // levels area
      pianoLevelsChooser = new PianoLevelsChooser;
      pianoLevelsChooser->setPianoView(pianoView);
      pianoLevelsChooser->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      pianoLevelsChooser->setFixedWidth(PIANO_KEYBOARD_WIDTH);

      pianoLevels = new PianoLevels;
      pianoLevels->setPianoView(pianoView);
      pianoLevels->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      pianoLevels->setOrientation(_orientation);

      levelsAreaWidget = new QWidget;
      QHBoxLayout* levelsAreaLayout = new QHBoxLayout;
      levelsAreaLayout->setContentsMargins(0, 0, 0, 0);
      levelsAreaLayout->setSpacing(0);
      levelsAreaLayout->addWidget(pianoLevelsChooser);
      levelsAreaLayout->addWidget(pianoLevels);
      levelsAreaWidget->setLayout(levelsAreaLayout);

      // layout
      QSplitter* editAreaSplitter = new QSplitter(Qt::Vertical);
      editAreaSplitter->addWidget(noteAreaWidget);
      editAreaSplitter->addWidget(levelsAreaWidget);
      editAreaSplitter->setFrameShape(QFrame::NoFrame);

      editAreaSplitter->setSizes(QList<int>({300, 100}));

      split = new QSplitter(Qt::Vertical);
      split->setFrameShape(QFrame::NoFrame);

      QGridLayout* layout = new QGridLayout;
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(0);
      layout->setColumnMinimumWidth(0, PIANO_KEYBOARD_WIDTH);
      layout->addWidget(editAreaSplitter, 1, 0, 1, 1);

      mainWidget->setLayout(layout);

      QVBoxLayout* mainLayout = new QVBoxLayout(this);
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->setSpacing(0);

      QWidget* toolbarArea = new QWidget(this);
      toolbarArea->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Fixed);

      QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarArea);
      toolbarLayout->setContentsMargins(0, 0, 0, 0);
      toolbarLayout->setSpacing(0);

      toolbarLayout->addWidget(tbMain);
      toolbarLayout->addWidget(tbTool);
      toolbarLayout->addWidget(tbNoteLen);
      toolbarLayout->addWidget(tbDots);
      toolbarLayout->addWidget(tbVoices);
      toolbarLayout->addStretch(1);

      mainLayout->addWidget(toolbarArea);
      mainLayout->addWidget(tbTweak);
      mainLayout->addWidget(mainWidget);

      // Re-enable right-click menu for enable/disable toolbars now with dockable widget
      const QList<QToolBar*> toolbars {
            tbMain,
            tbNoteLen,
            tbDots,
            tbTool,
            tbVoices,
            tbTweak
            };


      for (QToolBar* toolbar : toolbars) {
            toolbar->setContextMenuPolicy(Qt::CustomContextMenu);

            connect(toolbar,
                    &QToolBar::customContextMenuRequested,
                    this,
                    [toolbar, toolbars](const QPoint& pos) {
                          QMenu menu;

                          for (QToolBar* tb : toolbars)
                                menu.addAction(tb->toggleViewAction());

                          menu.exec(toolbar->mapToGlobal(pos));
                          });
            }

      // And right-sided area
      toolbarArea->setContextMenuPolicy(Qt::CustomContextMenu);

      connect(toolbarArea,
              &QWidget::customContextMenuRequested,
              this,
              [toolbarArea, toolbars](const QPoint& pos) {
                    QMenu menu;

                    for (QToolBar* tb : toolbars)
                          menu.addAction(tb->toggleViewAction());

                    menu.exec(toolbarArea->mapToGlobal(pos));
                    });

      _playbackFollowTimer = new QTimer(this);
      _playbackFollowTimer->setTimerType(Qt::PreciseTimer);
      _playbackFollowTimer->setInterval(8);

      connect(_playbackFollowTimer, &QTimer::timeout,
              this, [this]() {
                    if (!_playbackFollowActive)
                          return;

                    //
                    // Stop visual following as soon as playback or Follow Song
                    // is no longer active.
                    //
                    if (!seq
                        || !seq->isPlaying()
                        || !preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG)) {
                          stopPlaybackFollow();
                          return;
                          }

                    //
                    // One real sequencer interval is needed before we know
                    // playback velocity.
                    //
                    if (!_playbackFollowVelocityValid)
                          return;

                    const qreal elapsed =
                          _playbackFollowElapsed.nsecsElapsed()
                          / 1000000000.0;

                    const qreal predictedTick =
                          _playbackFollowBaseTick
                          + elapsed * _playbackFollowTicksPerSecond;

                    pianoView->setPlaybackLocatorTick(predictedTick);

                    if (_showPianoLevels && pianoLevels)
                          pianoLevels->setPlaybackLocatorTick(predictedTick);

                    //
                    // Begin at the playhead's existing screen position and smoothly
                    // converge to normal centered playback following.
                    //
                    const qreal settleTime = 0.75;
                    const qreal settle =
                          qBound<qreal>(0.0, elapsed / settleTime, 1.0);

                    const qreal horizontalOffset =
                          _playbackFollowHorizontalOffset * (1.0 - settle);

                    pianoView->ensureVisible(predictedTick, horizontalOffset);

                    ruler->setPlaybackLocatorTick(predictedTick);
                    });



      connect(pianoView, &PianoView::onTimeDragged, this, [this](int value) {
            _previewOnTime = value;

            setOnTime(value);

            pianoLevelsChooser->setEventPreviewValues(
                  _previewOnTime,
                  _previewLen);

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();
            });

      connect(pianoView, &PianoView::tickLenDragged, this, [this](int value) {
            _previewLen = value;

            setTickLen(value);

            pianoLevelsChooser->setEventPreviewValues(
                  _previewOnTime,
                  _previewLen);

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();
            });

      connect(pianoView, &PianoView::noteEventsChanged, this, [this]() {
            pianoLevels->update();
            updateSelection();
            });

      connect(pianoView,
              &PianoView::editNoteLengthChanged,
              this,
              &PianorollEditor::updateNoteLengthControls);

      velocity->installEventFilter(this);
      pitch->installEventFilter(this);
      onTime->installEventFilter(this);
      tickLen->installEventFilter(this);
      subdiv->installEventFilter(this);
      tuplet->installEventFilter(this);

      applyColoring(coloringBox->currentIndex());
      pianoView->setUseNoteColors(_useNoteColors);
      pianoKbd->setUseNoteColors(_useNoteColors);
      pianoLevels->setUseNoteColors(_useNoteColors);

      connect(pianoView->horizontalScrollBar(), SIGNAL(valueChanged(int)), hsb,      SLOT(setValue(int)));

      connect(pianoView,          SIGNAL(xZoomChanged(qreal)),            ruler,       SLOT(setXZoom(qreal)));
      connect(pianoView,          SIGNAL(xZoomChanged(qreal)),            pianoLevels, SLOT(setXZoom(qreal)));
      connect(pianoView,          SIGNAL(noteHeightChanged(int)),         pianoKbd,    SLOT(setNoteHeight(int)));
      connect(pianoView,          SIGNAL(pitchChanged(int)),              pl,          SLOT(setPitch(int)));
      connect(pianoView,          SIGNAL(pitchChanged(int)),              pianoKbd,    SLOT(setPitch(int)));
      connect(pianoKbd,           SIGNAL(pitchChanged(int)),              pl,          SLOT(setPitch(int)));
      connect(pianoView,          &PianoView::trackingPosChanged, pos, &Awl::PosLabel::setValue);
      connect(pianoView,          &PianoView::trackingPosChanged, ruler, &PianoRuler::setPos);
      connect(pianoView,          &PianoView::trackingPosChanged, pianoLevels, &PianoLevels::setPos);
      connect(ruler,              &PianoRuler::posChanged, pos, &Awl::PosLabel::setValue);
      connect(pianoLevels,        &PianoLevels::posChanged, pos, &Awl::PosLabel::setValue);
      connect(tuplet,             SIGNAL(valueChanged(int)),              pianoView,   SLOT(setTuplet(int)));
      connect(tuplet,             SIGNAL(valueChanged(int)),              pianoLevels, SLOT(setTuplet(int)));

      connect(barPattern,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this](int index) {
                    pianoView->setBarPattern(index);
                    restoreScoreViewFocus();
                    });

      connect(pitch,
            &Awl::PitchEdit::returnPressed,
            this,
            [this]() {
                  applyPitchEdit();
                  restoreScoreViewFocus();
                  });

      connect(pianoView->horizontalScrollBar(),
              &QScrollBar::valueChanged,
              this,
              [this](int value) {
                    if (_orientation == PianoRollOrientation::VERTICAL)
                          pianoKbd->setYpos(value);
                    });

      connect(pianoView->verticalScrollBar(),
              &QScrollBar::valueChanged,
              this,
              [this](int value) {
                    if (_orientation == PianoRollOrientation::HORIZONTAL) {
                          pianoKbd->setYpos(value);
                          }
                    else if (_orientation == PianoRollOrientation::VERTICAL) {
                          if (_showPianoLevels && pianoLevels)
                                pianoLevels->update();
                          }
                    });

      connect(subdiv,             SIGNAL(valueChanged(int)),              pianoView,   SLOT(setSubdiv(int)));
      connect(subdiv,             SIGNAL(valueChanged(int)),              pianoLevels, SLOT(setSubdiv(int)));
      connect(pianoLevelsChooser, SIGNAL(levelsIndexChanged(int)),        pianoLevels, SLOT(setLevelsIndex(int)));
      connect(pianoKbd,           SIGNAL(pitchHighlightToggled(int)),     pianoView,   SLOT(togglePitchHighlight(int)));

      connect(hsb,                SIGNAL(valueChanged(int)),   SLOT(setXpos(int)));
      connect(ruler,              &PianoRuler::locatorMoved,  this, &PianorollEditor::moveLocator);
      connect(pianoLevels,        &PianoLevels::locatorMoved, this, &PianorollEditor::moveLocator);
      connect(veloType,           SIGNAL(activated(int)),                SLOT(veloTypeChanged(int)));
      connect(velocity,           SIGNAL(valueChanged(int)),             SLOT(velocityChanged(int)));
      connect(onTime,             SIGNAL(valueChanged(int)),             SLOT(onTimeChanged(int)));
      connect(tickLen,            SIGNAL(valueChanged(int)),             SLOT(tickLenChanged(int)));
      connect(pianoView,          SIGNAL(selectionChanged()),            SLOT(selectionChanged()));
      connect(pianoView,          SIGNAL(showNoteTweakerRequest()),      SLOT(showNoteTweaker()));
      connect(pianoKbd,           SIGNAL(keyPressed(int)),               SLOT(keyPressed(int)));
      connect(pianoKbd,           SIGNAL(keyReleased(int)),              SLOT(keyReleased(int)));
      connect(pianoLevels,        SIGNAL(noteLevelsChanged()),           SLOT(selectionChanged()));
      connect(noteTweakerDlg,     SIGNAL(notesChanged()),                SLOT(selectionChanged()));
      connect(pianoLevelsChooser, SIGNAL(notesChanged()),                SLOT(selectionChanged()));

      // readSettings();

      actions.append(getAction("tie"));
      actions.append(getAction("play"));
      actions.append(getAction("delete"));
      actions.append(getAction("pitch-up"));
      actions.append(getAction("pitch-down"));
      actions.append(getAction("pitch-up-octave"));
      actions.append(getAction("pitch-down-octave"));

//      QMenu* popup = new QMenu(this);
//      popup->setSeparatorsCollapsible(false);
//      QAction* a = popup->addSeparator();
//      popup->addAction(getAction("cut"));
//      popup->addAction(getAction("copy"));
//      popup->addAction(getAction("paste"));
//      popup->addAction(getAction("swap"));
//      popup->addAction(getAction("delete"));

      addActions(actions);
      for (auto*& action : actions)
            connect(action, &QAction::triggered, this, [this, action](bool){ cmd(action); });

      setXpos(0);
      }


//---------------------------------------------------------
//   ~PianorollEditor
//---------------------------------------------------------

PianorollEditor::~PianorollEditor()
      {
      if (_score)
            _score->removeViewer(this);
      for (auto*& action : actions)
            action->disconnect(this);
      }

//---------------------------------------------------------
//   setEditNoteLength
//---------------------------------------------------------

void PianorollEditor::setEditNoteLength(int len)
      {
      pianoView->setEditNoteLength(
            Fraction::fromTicks(pow(2, len + 2) * DIVISION));

      if (tbDots)
            tbDots->setEnabled(true);
      }

//---------------------------------------------------------
//   setEditNoteVoice
//---------------------------------------------------------

void PianorollEditor:: setEditNoteVoice(int voice)
      {
            pianoView->setEditNoteVoice(voice);
      }

//---------------------------------------------------------
//   setEditNoteDots
//---------------------------------------------------------

void PianorollEditor::setEditNoteDots(int value, QToolButton* bn)
      {
      if (pianoView->editNoteDots() == value) {
            bn->group()->setExclusive(false);
            bn->setChecked(false);
            bn->group()->setExclusive(true);
            pianoView->setEditNoteDots(0);
            }
      else
            pianoView->setEditNoteDots(value);
      }

//---------------------------------------------------------
//   updateNoteLengthControls
//---------------------------------------------------------

void PianorollEditor::updateNoteLengthControls(const Fraction& duration)
      {
      if (!bngrpNoteLen || !bngrpNoteDot)
            return;

      QToolButton* lengthButton = nullptr;
      QToolButton* dotButton = nullptr;
      int matchedMeasureFrac = 0;
      int matchedDots = 0;

      for (QAbstractButton* button : bngrpNoteLen->buttons()) {
            QToolButton* toolButton = qobject_cast<QToolButton*>(button);
            if (!toolButton)
                  continue;

            const int measureFrac =
                  toolButton->property("measureFrac").toInt();

            const Fraction base =
                  Fraction::fromTicks(
                        pow(2, measureFrac + 2) * DIVISION);

            for (int dots = 0; dots <= 4; ++dots) {
                  Fraction value = base;

                  if (dots > 0) {
                        const int denominator = 1 << dots;
                        const int numerator =
                              (1 << (dots + 1)) - 1;

                        value *= Fraction(numerator, denominator);
                        }

                  if (value == duration) {
                        lengthButton = toolButton;
                        matchedMeasureFrac = measureFrac;
                        matchedDots = dots;
                        break;
                        }
                  }

            if (lengthButton)
                  break;
            }

      //
      // Temporarily disable exclusivity so a custom duration can leave
      // all duration and dot buttons unchecked.
      //
      bngrpNoteLen->setExclusive(false);
      for (QAbstractButton* button : bngrpNoteLen->buttons())
            button->setChecked(false);
      bngrpNoteLen->setExclusive(true);

      bngrpNoteDot->setExclusive(false);
      for (QAbstractButton* button : bngrpNoteDot->buttons())
            button->setChecked(false);
      bngrpNoteDot->setExclusive(true);

      if (!lengthButton) {
            //
            // Not representable by the duration/dot controls.
            // Keep the exact dragged value as a custom duration.
            //
            pianoView->setEditNoteLength(duration);
            pianoView->setEditNoteDots(0);
            tbDots->setEnabled(false);
            return;
            }

      lengthButton->setChecked(true);

      if (matchedDots > 0) {
            for (QAbstractButton* button : bngrpNoteDot->buttons()) {
                  if (button->property("dots").toInt() == matchedDots) {
                        dotButton = qobject_cast<QToolButton*>(button);
                        break;
                        }
                  }

            if (dotButton)
                  dotButton->setChecked(true);
            }

      //
      // Store the decoded base duration and dots in PianoView.
      //
      setEditNoteLength(matchedMeasureFrac);
      pianoView->setEditNoteDots(matchedDots);
      }

//---------------------------------------------------------
//   setEditNoteTool
//---------------------------------------------------------

void PianorollEditor::setEditNoteTool(PianoRollEditTool value)
      {
      pianoView->setEditNoteTool(value);

      pianoLevelsChooser->setPlaybackEditingEnabled(
            value == PianoRollEditTool::EVENT_ADJUST);
      }

//---------------------------------------------------------
//   handleAction
//---------------------------------------------------------

void PianorollEditor::handleAction(QAction* a)
      {
      QString cmd(a->data().toString());

      if (cmd == "zoom-in-horiz-pre")
            zoom(1, true);
      else if (cmd == "zoom-out-horiz-pre")
            zoom(-1, true);
      else if (cmd == "zoom-in-vert-pre")
            zoom(1, false);
      else if (cmd == "zoom-out-vert-pre")
            zoom(-1, false);
      }


//---------------------------------------------------------
//   showNoteTweaker
//---------------------------------------------------------

void PianorollEditor::showNoteTweaker()
      {
      noteTweakerDlg->show();
      }

//---------------------------------------------------------
//   setOnTime
//---------------------------------------------------------

void PianorollEditor::setOnTime(int v)
      {
      QSignalBlocker blocker(onTime);
      onTime->setValue(v);;
      }

//---------------------------------------------------------
//   setTickLen
//---------------------------------------------------------

void PianorollEditor::setTickLen(int v)
      {
      QSignalBlocker blocker(tickLen);
      tickLen->setValue(v);
      }

//---------------------------------------------------------
//   setPianoLevelsVisible
//---------------------------------------------------------

void PianorollEditor::setPianoLevelsVisible(bool visible)
      {
      if (_showPianoLevels == visible)
            return;

      _showPianoLevels = visible;

      if (!levelsAreaWidget)
            return;

      levelsAreaWidget->setVisible(visible);

      if (pianoLevels) {
            if (visible) {
                  //
                  // Bring the view back into sync when it becomes visible.
                  //
                  pianoLevels->setXpos(
                        pianoView->horizontalScrollBar()->value());

                  pianoLevels->update();
                  }
            else {
                  pianoLevels->clearPlaybackLocatorTick();
                  }
            }
      }

//---------------------------------------------------------
//   focusOnPosition
//---------------------------------------------------------

void PianorollEditor::focusOnPosition(Position* p)
      {
      if (!p || !p->segment)
            return;

      // move view so that view is centered on this element
      pianoView->ensureVisible(p->segment->tick().ticks());
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool PianorollEditor::eventFilter(QObject* obj, QEvent* event)
      {
      if (event->type() == QEvent::ShortcutOverride) {
            QKeyEvent* const ke = static_cast<QKeyEvent*>(event);

            const bool spinBoxHasFocus =
                  qobject_cast<QAbstractSpinBox*>(obj);

            const bool verticalArrowPress =
                  ke->key() == Qt::Key_Up
                  || ke->key() == Qt::Key_Down;

            if (spinBoxHasFocus && verticalArrowPress) {
                  event->accept();
                  return true;
                  }

            const bool pitchEnterPress =
                  obj == pitch
                  && (ke->key() == Qt::Key_Return
                      || ke->key() == Qt::Key_Enter);

            if (pitchEnterPress) {
                  event->accept();
                  return true;
                  }
            }

      return QWidget::eventFilter(obj, event);
      }

//---------------------------------------------------------
//   restoreScoreViewFocus
//---------------------------------------------------------

void PianorollEditor::restoreScoreViewFocus()
      {
      ScoreView* scoreView = mscore->currentScoreView();
      if (scoreView)
            scoreView->setFocus();
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianorollEditor::setStaff(Staff* st)
      {
      if (staff == st)
            return;

      if ((st && st->score() != _score) || (!st && _score)) {
            if (_score) {
                  _score->removeViewer(this);
                  disconnect(_score, SIGNAL(posChanged(POS,unsigned)),
                             this, SLOT(posChanged(POS,unsigned)));
                  disconnect(_score, SIGNAL(playlistChanged()),
                             this, SLOT(playlistChanged()));
                  disconnect(_score->masterScore(), &Score::partColorChanged,
                             this, &PianorollEditor::redraw);
                  }
            _score = st ? st->score() : nullptr;
            if (_score) {
                  _score->addViewer(this);
                  setLocator(POS::CURRENT, _score->pos(POS::CURRENT).ticks());
                  setLocator(POS::LEFT,    _score->pos(POS::LEFT).ticks());
                  setLocator(POS::RIGHT,   _score->pos(POS::RIGHT).ticks());
                  connect(_score, &Score::posChanged,
                          this, &PianorollEditor::posChanged);
                  connect(_score, SIGNAL(playlistChanged()),
                          SLOT(playlistChanged()));
                  connect(_score->masterScore(), &Score::partColorChanged,
                          this, &PianorollEditor::redraw);
                  }
            updateStaffBox();
            }
      staff = st;

      if (staffBox) {
            QSignalBlocker blocker(staffBox);

            const int index =
                  staff ? staffBox->findData(staff->idx()) : -1;

            staffBox->setCurrentIndex(index);
            }

      if (staff) {
            setWindowTitle(tr("<%1> Staff: %2").arg(_score->masterScore()->fileInfo()->completeBaseName()).arg(st->idx()));
            TempoMap* tl = _score->tempomap();
            TimeSigMap*  sl = _score->sigmap();
            for (int i = 0; i < 3; ++i)
                  locator[i].setContext(tl, sl);
            pos->setContext(tl, sl);
            }
      else
            setWindowTitle(tr("Piano roll editor"));

      ruler->setScore(_score, locator);
      pianoView->setStaff(staff, locator);
      pianoLevels->setScore(_score, locator);
      pianoLevels->setStaff(staff, locator);

      pianoView->setScope(_scope);
      pianoLevels->setScope(_scope);

      pianoLevelsChooser->setStaff(staff);
      pianoKbd->setStaff(staff);
      noteTweakerDlg->setStaff(staff);

      updateSelection();
      setEnabled(st);
      }

//---------------------------------------------------------
//   updateToolbarIconSize
//---------------------------------------------------------

void PianorollEditor::updateToolbarIconSize()
      {
      const QSize iconSize(
            preferences.getInt(PREF_UI_THEME_ICONWIDTH),
            preferences.getInt(PREF_UI_THEME_ICONHEIGHT));

      const QList<QToolBar*> toolbars {
            tbMain,
            tbTool,
            tbNoteLen,
            tbDots,
            tbVoices,
            tbTweak
            };

      for (QToolBar* toolbar : toolbars) {
            if (!toolbar)
                  continue;

            toolbar->setIconSize(iconSize);

            const QList<QToolButton*> buttons =
                  toolbar->findChildren<QToolButton*>();

            for (QToolButton* button : buttons)
                  button->setIconSize(iconSize);
            }
      }

//---------------------------------------------------------
//   updateOrientationLayout
//---------------------------------------------------------

void PianorollEditor::updateOrientationLayout()
      {
      while (QLayoutItem* item = noteAreaLayout->takeAt(0)) {
            // Removes the layout item only. The widgets themselves
            // remain alive and owned by their existing parents.
            delete item;
            }

      if (pianoLevels)
            pianoLevels->setOrientation(_orientation);

      const bool vertical =
            _orientation == PianoRollOrientation::VERTICAL;

      if (keyboardAlignedGridSeparator)
            keyboardAlignedGridSeparator->setVisible(vertical);

      if (keyboardAlignedGridAction)
            keyboardAlignedGridAction->setVisible(vertical);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            //
            // Widgets used only by horizontal orientation
            //
            topLeftSpacer->show();
            ruler->show();
            hsb->show();

            ruler->setOrientation(PianoRollOrientation::HORIZONTAL);

            ruler->setSizePolicy(
                  QSizePolicy::Expanding,
                  QSizePolicy::Fixed);

            ruler->setMinimumWidth(0);
            ruler->setMaximumWidth(QWIDGETSIZE_MAX);
            ruler->setFixedHeight(pianoRulerHeight);

            //
            // Release constraints left by vertical mode.
            //
            pianoKbd->setMinimumHeight(0);
            pianoKbd->setMaximumHeight(QWIDGETSIZE_MAX);

            pianoView->setOrientation(PianoRollOrientation::HORIZONTAL);
            pianoKbd->setOrientation(PianoOrientation::VERTICAL);
            pianoKbd->setSizePolicy(
                  QSizePolicy::Fixed,
                  QSizePolicy::Expanding);
            pianoKbd->setFixedWidth(PIANO_KEYBOARD_WIDTH);

            pianoKbd->setYpos(pianoView->verticalScrollBar()->value());

            noteAreaLayout->addWidget(topLeftSpacer, 0, 0, 1, 1);
            noteAreaLayout->addWidget(ruler,         0, 1, 1, 1);
            noteAreaLayout->addWidget(pianoKbd,      1, 0, 1, 1);
            noteAreaLayout->addWidget(pianoView,     1, 1, 1, 1);
            noteAreaLayout->addWidget(hsb,           2, 1, 1, 1);
            }
      else { // VERTICAL
            //
            // Horizontal-only widgets are still children of
            // noteAreaWidget even after being removed from the layout.
            //
            topLeftSpacer->hide();
            ruler->show(); // Better not slow us down
            hsb->hide();

            ruler->setOrientation(PianoRollOrientation::VERTICAL);

            ruler->setSizePolicy(
                  QSizePolicy::Fixed,
                  QSizePolicy::Expanding);

            ruler->setMinimumHeight(0);
            ruler->setMaximumHeight(QWIDGETSIZE_MAX);
            ruler->setFixedWidth(pianoRulerHeight);

            //
            // Release constraints left by horizontal mode.
            //
            pianoKbd->setMinimumWidth(0);
            pianoKbd->setMaximumWidth(QWIDGETSIZE_MAX);

            pianoView->setOrientation(PianoRollOrientation::VERTICAL);
            pianoKbd->setOrientation(PianoOrientation::HORIZONTAL);
            pianoKbd->setSizePolicy(
                  QSizePolicy::Expanding,
                  QSizePolicy::Fixed);
            pianoKbd->setFixedHeight(PIANO_KEYBOARD_WIDTH);

            pianoKbd->setYpos(pianoView->horizontalScrollBar()->value());

            noteAreaLayout->addWidget(ruler,     0, 0);
            noteAreaLayout->addWidget(pianoView, 0, 1);
            noteAreaLayout->addWidget(pianoKbd,  1, 1);
            }
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianorollEditor::setOrientation(PianoRollOrientation orientation)
      {
      if (_orientation == orientation)
            return;

      const int referenceTick = pianoView->viewportReferenceTick();

      //
      // Remember the pitch viewport belonging to the orientation
      // we are leaving.
      //

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            _horizontalPitchScrollPos =
                  pianoView->verticalScrollBar()->value();

            _horizontalPitchScrollValid = true;
            }
      else {
            _verticalPitchScrollPos =
                  pianoView->horizontalScrollBar()->value();

            _verticalPitchScrollValid = true;
            }

      _orientation = orientation;
      updateOrientationLayout();

      //
      // Pitch viewport belongs independently to each orientation.
      //

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            if (_horizontalPitchScrollValid) {
                  pianoView->verticalScrollBar()->setValue(
                        _horizontalPitchScrollPos);
                  }
            }
      else {
            if (_verticalPitchScrollValid) {
                  pianoView->horizontalScrollBar()->setValue(
                        _verticalPitchScrollPos);
                  }
            }

      const bool hasSelection =
            !pianoView->getSelectedItems().isEmpty();

      if (hasSelection)
            pianoView->centerSelectionTimeInView();
      else
            pianoView->positionViewportAtTick(referenceTick);

      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setScope
//---------------------------------------------------------

void PianorollEditor::setScope(PianoRollScope scope)
      {
      if (_scope == scope)
            return;

      _scope = scope;
      pianoView->setScope(scope);
      pianoLevels->setScope(scope);
      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianorollEditor::setColoring(Coloring c)
      {
      _coloring = c;

      if (pianoView)
            pianoView->setColoring(c);

      if (pianoKbd)
            pianoKbd->setColoring(c);

      if (pianoLevels)
            pianoLevels->setColoring(c);

      update();
      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------

void PianorollEditor::setUseNoteColors(bool value)
      {
      if (_useNoteColors == value)
            return;

      _useNoteColors = value;

      preferences.setPreference(
            PREF_UI_PIANOROLL_USE_NOTE_COLORS,
            value);

      pianoView->setUseNoteColors(value);
      pianoKbd->setUseNoteColors(value);
      pianoLevels->setUseNoteColors(value);

      redraw();
      }

//---------------------------------------------------------
//   clearPlaybackPitches
//---------------------------------------------------------

void PianorollEditor::clearPlaybackPitches()
      {
      if (pianoKbd)
            pianoKbd->setPlaybackNotes(QHash<int, const Note*>());

      if (pianoView) {
            pianoView->setPlaybackActive(false);
            pianoView->clearPlaybackNoteEvents();
            }
      }

//---------------------------------------------------------
//   writeSettings
//---------------------------------------------------------

void PianorollEditor::writeSettings()
      {
      MuseScore::saveGeometry(this);
      }

//---------------------------------------------------------
//   readSettings
//---------------------------------------------------------

void PianorollEditor::readSettings()
      {
      resize(QSize(800, 600)); // ensure default size if no geometry in settings
      MuseScore::restoreGeometry(this);
      }

//---------------------------------------------------------
//   setXpos
//---------------------------------------------------------

void PianorollEditor::setXpos(int x)
      {
      pianoView->horizontalScrollBar()->setValue(x);
      ruler->setXpos(x);

      if (_showPianoLevels)
            pianoLevels->setXpos(x);

      if (waveView && showWave->isChecked())
            waveView->setXpos(x);

      }

//---------------------------------------------------------
//   rangeChanged
//---------------------------------------------------------

void PianorollEditor::rangeChanged(int min, int max)
      {
      hsb->setRange(min, max);
      }

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void PianorollEditor::updateSelection()
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      bool enabled = false;

      //
      // Pitch can represent either a common selected pitch
      // or a mixed selection.
      //
      if (!items.empty()) {
            Note* first = items[0]->note();

            const int firstPitch = first->pitch();
            const int firstTpc = first->concertPitch()
                  ? first->tpc1()
                  : first->tpc2();

            bool mixed = false;

            for (int i = 1; i < items.size(); ++i) {
                  Note* note = items[i]->note();

                  const int noteTpc = note->concertPitch()
                        ? note->tpc1()
                        : note->tpc2();

                  if (note->pitch() != firstPitch || noteTpc != firstTpc) {
                        mixed = true;
                        break;
                        }
                  }

            if (mixed)
                  pitch->setMixedPitch();
            else
                  pitch->setPitch(firstPitch, firstTpc);
            }

      //
      // These fields still only have an unambiguous value
      // when exactly one PianoItem is selected.
      //
      if (items.size() == 1) {
            PianoItem* item = items[0];
            Note* note = item->note();

            NoteEvent* event = item->getTweakNoteEvent();
            if (event) {
                  QSignalBlocker onTimeBlocker(onTime);
                  QSignalBlocker tickLenBlocker(tickLen);

                  onTime->setValue(event->ontime());
                  tickLen->setValue(event->len());

                  _previewOnTime = event->ontime();
                  _previewLen = event->len();
                  }

            updateVelocity(note);
            }

      // if all selected notes don't have the same veloType,
      // velocity field should be disabled
      bool sameVeloType = true;
      if (!items.empty()) {
            enabled = true;

            Note::ValueType vt = items[0]->note()->veloType();
            for (int i = 1; i < items.size(); i++) {
                  if (items[i]->note()->veloType() != vt) {
                        sameVeloType = false;
                        break;
                        }
                  }
            }

      velocity->setEnabled(enabled && sameVeloType);
      pitch->setEnabled(enabled);
      veloType->setEnabled(enabled);
      onTime->setEnabled(enabled);
      tickLen->setEnabled(enabled);
      pianoLevelsChooser->updateSetboxValue();
      }

//---------------------------------------------------------
//   selectionChanged
//    called if selection in PianoView changed
//---------------------------------------------------------

void PianorollEditor::selectionChanged()
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (items.size() == 1) {
            Note* note = items[0]->note();
            _score->select(note, SelectType::SINGLE, 0);
            }
      else if (items.size() == 0)
            _score->select(0, SelectType::SINGLE, 0);
      else {
            _score->deselectAll();
            for (PianoItem*& item : items) {
                  Note* note = item->note();
                  if (!note->selected())
                        _score->select(note, SelectType::ADD, 0);
                  }
            }

      if (!_score->selection().isNone())
            _score->selection().setSource(SelectionSource::PIANO_ROLL);


      pianoView->scene()->update();
      pianoLevels->update();
      updateSelection();
      }

//---------------------------------------------------------
//   changeSelection
//---------------------------------------------------------

void PianorollEditor::changeSelection(SelState)
      {
      if (!_score || !pianoView)
            return;

      //
      // PianoItem uses the actual score Note::selected() state,
      // so no separate selection transfer is required.
      //
      pianoView->scene()->update();
      pianoLevels->update();

      updateSelection();

      //
      // Selection by mouse should not disturb an already useful view.
      // Keyboard/navigation selection that goes off-screen should follow.
      //
      pianoView->ensureSelectionVisible();
      }

//---------------------------------------------------------
//   veloTypeChanged
//---------------------------------------------------------

void PianorollEditor::veloTypeChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      const Note::ValueType vt = Note::ValueType(val);
      switch (vt) {
            case Note::ValueType::USER_VAL:
                  velocity->setRange(0, 127);
                  velocity->setPrefix("");
                  break;

            case Note::ValueType::OFFSET_VAL:
                  velocity->setRange(-127, 127);
                  velocity->setPrefix(
                        velocity->value() > 0 ? "+" : "");
                  break;
            }

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            if (Note::ValueType(val) == note->veloType())
                  continue;

            int newVelocity = note->veloOffset();
            int dynamicsVel = staff->velocities().val(note->tick());

            switch (Note::ValueType(val)) {
                  case Note::ValueType::USER_VAL:
                        // relative offset -> absolute velocity
                        newVelocity = qBound(0, dynamicsVel + newVelocity, 127);
                        break;

                  case Note::ValueType::OFFSET_VAL:
                  // absolute velocity -> relative offset
                        newVelocity = qBound(-127, newVelocity - dynamicsVel, 127);
                        break;
                  }

            _score->undo(new ChangeVelocity(note, Note::ValueType(val), newVelocity));
            updateVelocity(note);
            }
      _score->endCmd();

      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   updateStaffBox
//---------------------------------------------------------

void PianorollEditor::updateStaffBox()
      {
      if (!staffBox)
            return;

      QSignalBlocker blocker(staffBox);

      staffBox->clear();

      if (!_score) {
            staffBox->setEnabled(false);
            return;
            }

      for (Staff* st : _score->staves()) {
            if (!st)
                  continue;

            QString label = st->partName();

            Part* part = st->part();
            if (part && part->nstaves() > 1) {
                  const int staffIndex = part->staves()->indexOf(st);

                  if (staffIndex >= 0)
                        label += tr(": Staff %1").arg(staffIndex + 1);
                  }

            staffBox->addItem(label, st->idx());
            }

      staffBox->setEnabled(staffBox->count() > 0);
      }

//---------------------------------------------------------
//   updateVelocity
//---------------------------------------------------------

void PianorollEditor::updateVelocity(Note* note)
      {
      const Note::ValueType vt = note->veloType();
      const int value = note->veloOffset();

      QSignalBlocker typeBlocker(veloType);
      QSignalBlocker velocityBlocker(velocity);

      veloType->setCurrentIndex(int(vt));

      velocity->setReadOnly(false);
      velocity->setSuffix("");

      switch (vt) {
            case Note::ValueType::USER_VAL:
                  velocity->setRange(0, 127);
                  velocity->setPrefix("");
                  break;

            case Note::ValueType::OFFSET_VAL:
                  velocity->setRange(-127, 127);
                  velocity->setPrefix(value > 0 ? "+" : "");
                  break;
            }

      velocity->setValue(value);

      pianoLevels->update();
      }

//---------------------------------------------------------
//   velocityChanged
//---------------------------------------------------------

void PianorollEditor::velocityChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      const Note::ValueType currentType =
            Note::ValueType(veloType->currentIndex());

      if (currentType == Note::ValueType::OFFSET_VAL && val > 0)
            velocity->setPrefix("+");
      else
            velocity->setPrefix("");

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            Note::ValueType vt = note->veloType();

            if (val == note->veloOffset())
                  continue;

            _score->undo(new ChangeVelocity(note, vt, val));
            }
      _score->endCmd();

      pianoLevels->update();
      }

//---------------------------------------------------------
//   keyPressed
//---------------------------------------------------------

void PianorollEditor::keyPressed(int p)
      {
      seq->startNote(staff->part()->instrument()->channel(0)->channel(), p, 80, 0, 0.0);
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void PianorollEditor::keyReleased(int /*p*/)
      {
      seq->stopNotes();
      }

//---------------------------------------------------------
//   stopPlaybackFollow
//---------------------------------------------------------

void PianorollEditor::stopPlaybackFollow()
      {
      _playbackFollowActive = false;
      _playbackFollowVelocityValid = false;
      _playbackFollowTicksPerSecond = 0.0;
      _playbackFollowHorizontalOffset = 0.0;

      pianoView->clearPlaybackLocatorTick();
      ruler->clearPlaybackLocatorTick();
      if (pianoLevels)
            pianoLevels->clearPlaybackLocatorTick();

      if (_playbackFollowTimer->isActive())
            _playbackFollowTimer->stop();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void PianorollEditor::heartBeat(Seq* s)
      {
      unsigned tick = s->getCurTick();

      if (score()->masterScore())
            tick = score()->masterScore()->repeatList().utick2tick(tick);

      pianoView->setPlaybackActive(s->isPlaying());

      //
      // Keep the authoritative PRE playback position synchronized
      // with the sequencer.
      //
      if (locator[0].tick() != tick)
            posChanged(POS::CURRENT, tick);


      QHash<int, const Note*> playbackNotes;
      const auto& active = s->activePitches();
      for (auto it = active.constBegin(); it != active.constEnd(); ++it) {
            if (it.value().count > 0 && it.value().note)
                  playbackNotes.insert(it.key(), it.value().note);
            }
      pianoKbd->setPlaybackNotes(playbackNotes);


      QHash<const Note*, QSet<int>> playbackNoteEvents;
      const auto& activeNoteEvents = s->activeNoteEvents();
      for (const ActiveNoteEventInfo& info : activeNoteEvents) {
            if (info.owner && info.noteEventIndex >= 0)
                  playbackNoteEvents[info.owner].insert(info.noteEventIndex);
            }
      pianoView->setPlaybackNoteEvents(playbackNoteEvents);


      pianoView->updatePlaybackHighlights();

      //
      // Smooth viewport following is purely visual. It does not
      // replace the authoritative locator/playback position above.
      //
      if (!preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG)
          || !s->isPlaying()) {
            stopPlaybackFollow();
            return;
            }

      const TempoMap* tempoMap = _score->tempomap();

      const qreal newTicksPerSecond =
            DIVISION
            * tempoMap->tempo(tick)
            * tempoMap->relTempo();

      //
      // First playback sample establishes both the visual time
      // origin and the playback velocity.
      //
      if (!_playbackFollowActive) {
            _playbackFollowActive = true;
            _playbackFollowVelocityValid = true;

            _playbackFollowBaseTick = qreal(tick);
            _playbackFollowLastSampleTick = tick;
            _playbackFollowTicksPerSecond = newTicksPerSecond;

            _playbackFollowHorizontalOffset =
                  pianoView->playbackFollowHorizontalOffset(tick);

            _playbackFollowElapsed.restart();
            _playbackFollowTimer->start();

            return;
            }

      const qreal visualElapsed =
            _playbackFollowElapsed.nsecsElapsed()
            / 1000000000.0;

      const qreal predictedTick =
            _playbackFollowBaseTick
            + visualElapsed * _playbackFollowTicksPerSecond;

      //
      // Detect playback discontinuities.
      //
      // Backward movement is always a discontinuity.  For forward
      // movement, compare the authoritative sequencer position with
      // the position predicted by the smooth visual clock.  Ordinary
      // playback stays close to that prediction; seeks do not.
      //
      const qreal seekThreshold =
            qMax<qreal>(DIVISION / 8.0,
                        _playbackFollowTicksPerSecond * 0.10);

      const bool backwardJump =
            tick < _playbackFollowLastSampleTick;

      const bool forwardJump =
            qreal(tick) - predictedTick > seekThreshold;

      if (backwardJump || forwardJump) {
            _playbackFollowBaseTick = qreal(tick);
            _playbackFollowLastSampleTick = tick;
            _playbackFollowTicksPerSecond = newTicksPerSecond;
            _playbackFollowVelocityValid = true;

            _playbackFollowElapsed.restart();

            pianoView->ensureVisible(tick);
            return;
            }

      //
      // Repeated sequencer ticks contain no new playback-position
      // information.
      //
      if (tick == _playbackFollowLastSampleTick)
            return;

      //
      // If the tempo has changed, preserve the current visual
      // position exactly while changing the slope from this point.
      //
      if (!qFuzzyCompare(newTicksPerSecond,
                         _playbackFollowTicksPerSecond)) {
            _playbackFollowBaseTick = predictedTick;
            _playbackFollowElapsed.restart();

            _playbackFollowTicksPerSecond =
                  newTicksPerSecond;
            }

      _playbackFollowLastSampleTick = tick;
      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianorollEditor::moveLocator(int i, const Pos& p)
      {
      if (!locator[i].valid())
            return;

      const int tick = p.tick();

      //
      // Locator 0 is the current playback position.
      // During playback, use the sequencer's real seek path rather
      // than merely moving the score locator.
      //
      if (i == 0 && seq && seq->isPlaying()) {
            const int uTick =
                  score()->repeatList().tick2utick(tick);

            seq->seek(uTick, true);
            return;
            }

      score()->setPos(POS(i), Fraction::fromTicks(tick));
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void PianorollEditor::cmd(QAction* /*a*/)
      {
      //score()->startCmd();
      pianoView->setStaff(staff, locator);
      pianoLevels->setStaff(staff, locator);
      pianoLevelsChooser->setStaff(staff);
      pianoKbd->setStaff(staff);
      //score()->endCmd();
      }

//---------------------------------------------------------
//   dataChanged
//---------------------------------------------------------

void PianorollEditor::dataChanged(const QRectF&)
      {
      }

//---------------------------------------------------------
//   removeScore
//---------------------------------------------------------

void PianorollEditor::removeScore()
      {
      _score = nullptr;
      setStaff(nullptr);
      }

//---------------------------------------------------------
//   changeEditElement
//---------------------------------------------------------

void PianorollEditor::changeEditElement(Element*)
      {
      }

//---------------------------------------------------------
//   cursor
//---------------------------------------------------------

QCursor PianorollEditor::cursor() const
      {
      return QCursor();
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void PianorollEditor::setCursor(const QCursor&)
      {
      }

//---------------------------------------------------------
//   matrix
//---------------------------------------------------------

const QTransform& PianorollEditor::matrix() const
      {
      static QTransform t;
      return t;
      }

//---------------------------------------------------------
//   elementNear
//---------------------------------------------------------

Element* PianorollEditor::elementNear(QPointF)
      {
      return 0;
      }

//---------------------------------------------------------
//   updateAll
//---------------------------------------------------------

void PianorollEditor::updateAll()
      {
      if (updateScheduled)
            return;

      QTimer::singleShot(0, this, &PianorollEditor::doUpdate);
      updateScheduled = true;
      }

//---------------------------------------------------------
//   doUpdate
//---------------------------------------------------------

void PianorollEditor::doUpdate()
      {
      updateScheduled = false;

      if (staff && staff->idx() == -1) { // staff removed
            removeScore();
            return;
            }
      pianoView->updateNotes();
      pianoLevels->updateNotes();
      }

//---------------------------------------------------------
//   applyPitchEdit
//---------------------------------------------------------

void PianorollEditor::applyPitchEdit()
      {
      if (!_score)
            return;

      const int newPitch = pitch->value();
      const int tpc = pitch->typedTpc();

      if (!pitchIsValid(newPitch) || !tpcIsValid(tpc))
            return;

      std::list<Note*> notes = _score->selection().uniqueNotes();
      if (notes.empty())
            return;

      _score->startCmd();

      for (Note* note : notes) {
            int newTpc1;
            int newTpc2;

            //
            // The typed TPC describes the pitch spelling currently
            // presented to the user. Derive the corresponding other
            // TPC for concert/transposed representation.
            //
            if (note->concertPitch()) {
                  newTpc1 = tpc;
                  newTpc2 = note->transposeTpc(tpc);
                  }
            else {
                  newTpc2 = tpc;
                  newTpc1 = note->transposeTpc(tpc);
                  }

            if (note->pitch() == newPitch
                && note->tpc1() == newTpc1
                && note->tpc2() == newTpc2)
                  continue;

            if (note->accidental())
                  _score->undoRemoveElement(note->accidental());

            _score->undoChangePitch(
                  note,
                  newPitch,
                  newTpc1,
                  newTpc2);
            }

      _score->endCmd();

      //
      // Refresh PRE note geometry and the tweak controls immediately.
      //
      pianoView->updateNotes();
      pianoLevels->updateNotes();
      updateSelection();
      pianoView->ensureSelectionVisible();
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void PianorollEditor::redraw() const
      {
      if (pianoView)
            pianoView->viewport()->update();
      if (pianoKbd)
            pianoKbd->update();
      if (pianoLevels)
            pianoLevels->update();
      }

//---------------------------------------------------------
//   playlistChanged
//---------------------------------------------------------

void PianorollEditor::playlistChanged()
      {
      }

//---------------------------------------------------------
//   showWavView
//---------------------------------------------------------

void PianorollEditor::showWaveView(bool val)
      {
      if (val) {
            if (waveView == 0) {
                  waveView = new WaveView;
                  connect(pianoView, SIGNAL(magChanged(double,double)), waveView, SLOT(setMag(double,double)));
                  connect(pianoView, SIGNAL(posChanged(Pos&)), waveView, SLOT(setValue(Pos&)));
                  waveView->setAudio(_score->audio());
                  waveView->setScore(_score, locator);
                  split->addWidget(waveView);
                  waveView->setXpos(ruler->xpos());
                  }
            waveView->setVisible(true);
            }
      else {
            if (waveView)
                  waveView->setVisible(false);
            }
      }

//---------------------------------------------------------
//   posChanged
//    position in score has changed
//---------------------------------------------------------

void PianorollEditor::posChanged(POS p, unsigned tick)
      {
      if (locator[int(p)].tick() == unsigned(tick))
            return;

      setLocator(p, tick);

      if (p != POS::CURRENT) {
            pianoView->moveLocator(int(p));

            if (waveView)
                  waveView->moveLocator(int(p));

            ruler->update();

            if (_showPianoLevels)
                  pianoLevels->update();

            return;
            }

      //
      // The PianoView playback locator itself is still
      // horizontal-only.  Avoid the full scene update in
      // vertical mode.
      //
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            pianoView->moveLocator(int(p));

      //
      // PianoRuler is now visible in both orientations.
      //
      ruler->update();

      if (waveView)
            waveView->moveLocator(int(p));

      // No continuous pianoLevels->update() for POS::CURRENT.
      }

//---------------------------------------------------------
//   onTimeChanged
//---------------------------------------------------------

void PianorollEditor::onTimeChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            NoteEvent* event = item->getTweakNoteEvent();
            if (!event || event->ontime() == val)
                  continue;

            NoteEvent ne = *event;
            ne.setOntime(val);

            _score->undo(new ChangeNoteEvent(note, event, ne));
            }
      _score->endCmd();

      pianoView->updateNotes();
      pianoLevels->updateNotes();
      }

//---------------------------------------------------------
//   tickLenChanged
//---------------------------------------------------------

void PianorollEditor::tickLenChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            NoteEvent* event = item->getTweakNoteEvent();
            if (!event || event->len() == val)
                  continue;

            NoteEvent ne = *event;
            ne.setLen(val);

            _score->undo(new ChangeNoteEvent(note, event, ne));
            }
      _score->endCmd();

      pianoView->updateNotes();
      pianoLevels->updateNotes();
      }

//---------------------------------------------------------
//   zoom
//---------------------------------------------------------

void PianorollEditor::zoom(int amount, bool horiz)
      {
      int cx = pianoView->width() / 2;
      int cy = pianoView->height() / 2;

      pianoView->zoomView(amount, horiz, cx, cy);
      }

}
