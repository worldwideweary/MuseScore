//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include <QAction>
#include <QFile>
#include <QTemporaryDir>

#include "libmscore/accidental.h"
#include "libmscore/articulation.h"
#include "libmscore/chord.h"
#include "libmscore/chordbracket.h"
#include "libmscore/element.h"
#include "libmscore/guitarbend.h"
#include "libmscore/guitartechnique.h"
#include "libmscore/ledgerline.h"
#include "libmscore/measure.h"
#include "libmscore/note.h"
#include "libmscore/page.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/staff.h"
#include "libmscore/stafftext.h"
#include "libmscore/stafftype.h"
#include "libmscore/symbol.h"
#include "libmscore/system.h"
#include "libmscore/tie.h"
#include "libmscore/xml.h"
#include "libmscore/undo.h"

#include "audio/midi/event.h"

#include "mtest/testutils.h"

using namespace Ms;

// Native 3.x note-anchored spanners are owned through their notes;
// Studio 5's reader additionally registers them in the score map.
static QSet<Spanner*> allTestSpanners(MasterScore& score)
      {
      QSet<Spanner*> result;
      for (const auto& entry : score.spanner())
            result.insert(entry.second);
      score.scanElements(&result, [](void* context, Element* element) {
            if (element->isNote()) {
                  for (Spanner* spanner : toNote(element)->spannerFor())
                        static_cast<QSet<Spanner*>*>(context)->insert(spanner);
                  }
            });
      return result;
      }

//---------------------------------------------------------
//   TestElement
//---------------------------------------------------------

class TestElement : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase() { initMTest(); }
      void testIds();
      void testChordBracketRoundTrip();
      void testChordBracketClone();
      void testChordBracketHandles();
      void testStudioTabFont();
      void testGuitarBendProperties();
      void testGuitarBendStudioImport();
      void testGuitarBendCreation();
      void testWhammyBarDefaults();
      void testGuitarBendGeometryAndEdits();
      void testGuitarBendFretRules();
      void testGuitarDiveAndHoldGeometry();
      void testGuitarTapping();
      void testStudioGhostAndDeadFrets();
      void testGuitarSkylineClearance();
      void testGuitarBendMidiCurve();
      void testPolyphonicGuitarBendMidi();
      void testOverlappingGuitarBendMidi();
      void testWideGuitarBendMidi();
      void testGuitarBendReportedPassage();
      void testStudio5GuitarImport();
      void testStudio5SpannerMap();
      void testStudio5NoteSpanners();
      void testNoteParenthesesClearance();
      void testGuitarTabFretSizeAndAnchors();
      void testGuitarBendLabelStyle();
      void testGuitarPreDivePen();
      void testGuitarTechniqueClearance();
      void testTabParenthesesMask();
      void testTappingTextStyle();
      void testFretParenthesisFontAndGraceSize();
      void testGuitarAngularStaffLineClearance();
      void testGuitarConnectedReleaseLevels();
      void testStudioParenthesisGroups();
      void testChordParenthesisSpan();
      void testLeftHandTappingDisplay();
      void testHopoPlacementAndFlip();
      void testOverlappingBendDiveAmounts();
      void testConnectedDiveAndHeldBar();
      void testReleaseLabelPreservesFretAlignment();
      void testChordBendStagger();
      void testAccidentalInsideNoteParentheses();
      void testBendClearsChordParentheses();
      void testBendConditionalClearance();
      void testTabParenthesesUseRelativeSlightBendShape();
      void testHiddenBendArrivalFretting();
      void testGuitarBendWidthStyles();
      };

//---------------------------------------------------------
//   testIds
//---------------------------------------------------------

void TestElement::testIds()
      {
      ElementType ids[] = {
            ElementType::VOLTA,
            ElementType::OTTAVA,
            ElementType::TEXTLINE,
            ElementType::TRILL,
            ElementType::PEDAL,
            ElementType::HAIRPIN,
            ElementType::CLEF,
            ElementType::KEYSIG,
            ElementType::TIMESIG,
            ElementType::BAR_LINE,
            ElementType::ARPEGGIO,
            ElementType::CHORD_BRACKET,
            ElementType::BREATH,
            ElementType::GLISSANDO,
            ElementType::GUITAR_BEND,
            ElementType::HAMMER_ON_PULL_OFF,
            ElementType::WHAMMY_BAR,
//            ElementType::BRACKET,
            ElementType::ARTICULATION,
            ElementType::CHORDLINE,
            ElementType::ACCIDENTAL,
            ElementType::DYNAMIC,
            ElementType::TEXT,
            ElementType::INSTRUMENT_NAME,
            ElementType::STAFF_TEXT,
            ElementType::REHEARSAL_MARK,
            ElementType::INSTRUMENT_CHANGE,
            ElementType::NOTEHEAD,
            ElementType::NOTEDOT,
            ElementType::TREMOLO,
            ElementType::LAYOUT_BREAK,
            ElementType::MARKER,
            ElementType::JUMP,
            ElementType::REPEAT_MEASURE,
            ElementType::ICON,
            ElementType::NOTE,
            ElementType::SYMBOL,
            ElementType::FSYMBOL,
            ElementType::CHORD,
            ElementType::REST,
            ElementType::SPACER,
            ElementType::STAFF_STATE,
            ElementType::TEMPO_TEXT,
            ElementType::HARMONY,
            ElementType::FRET_DIAGRAM,
            ElementType::BEND,
            ElementType::TREMOLOBAR,
            ElementType::LYRICS,
            ElementType::FIGURED_BASS,
            ElementType::STEM,
            ElementType::SLUR,
            ElementType::FINGERING,
            ElementType::HBOX,
            ElementType::VBOX,
            ElementType::TBOX,
            ElementType::FBOX,
            ElementType::MEASURE,
            ElementType::TAB_DURATION_SYMBOL,
            ElementType::OSSIA
            };

      for (ElementType t : ids) {
            Element* e = Element::create(t, score);
            Element* ee = writeReadElement(e);
            QCOMPARE(e->type(), ee->type());
            delete e;
            delete ee;
            }
      }

void TestElement::testChordBracketRoundTrip()
      {
      for (Direction hooks : { Direction::AUTO, Direction::UP, Direction::DOWN }) {
            for (bool right : { false, true }) {
                  for (int span : { 1, 2, 5 }) {
                        ChordBracket bracket(score);
                        bracket.setProperty(Pid::BRACKET_HOOK_LEN, Spatium(1.2));
                        bracket.setProperty(Pid::BRACKET_HOOK_POS, QVariant::fromValue(hooks));
                        bracket.setProperty(Pid::BRACKET_RIGHT_SIDE, right);
                        bracket.setSpan(span);
                        bracket.setUserLen1(.75 * bracket.spatium());
                        bracket.setUserLen2(-.25 * bracket.spatium());
                        Element* element = writeReadElement(&bracket);
                        QVERIFY(element && element->isChordBracket());
                        ChordBracket* copy = toChordBracket(element);
                        QCOMPARE(copy->hookLength().val(), 1.2);
                        QCOMPARE(int(copy->hookPos()), int(hooks));
                        QCOMPARE(copy->rightSide(), right);
                        QCOMPARE(copy->span(), span);
                        QCOMPARE(copy->userLen1(), bracket.userLen1());
                        QCOMPARE(copy->userLen2(), bracket.userLen2());
                        QVERIFY(!copy->playArpeggio());
                        delete copy;
                        }
                  }
            }
      }

void TestElement::testChordBracketClone()
      {
      Chord original(score);
      original.setTrack(0);
      Note* note = new Note(score);
      note->setPitch(60);
      original.add(note);
      original.add(new Arpeggio(score));
      ChordBracket* bracket = new ChordBracket(score);
      bracket->setProperty(Pid::BRACKET_HOOK_POS, QVariant::fromValue(Direction::UP));
      original.add(bracket);
      original.add(new ChordBracket(score));

      Chord* copy = original.clone();
      QVERIFY(copy->arpeggio());
      QCOMPARE(static_cast<int>(copy->el().size()), 2);
      for (Element* child : copy->el()) {
            QVERIFY(child->isChordBracket());
            QCOMPARE(child->parent(), static_cast<Element*>(copy));
            }
      QVERIFY(copy->el().front() != bracket);
      QCOMPARE(int(toChordBracket(copy->el().front())->hookPos()), int(Direction::UP));
      delete copy;
      }

void TestElement::testChordBracketHandles()
      {
      ChordBracket bracket(score);
      bracket.layout();
      auto before = bracket.gripsPositions();
      EditData drag;
      drag.curGrip = Grip::START;
      drag.delta = QPointF(0, .25 * bracket.spatium());
      bracket.editDrag(drag);
      auto after = bracket.gripsPositions();
      QCOMPARE(after[0].y(), before[0].y() + drag.delta.y());
      QCOMPARE(after[1].y(), before[1].y());
      before = after;
      drag.curGrip = Grip::END;
      bracket.editDrag(drag);
      after = bracket.gripsPositions();
      QCOMPARE(after[0].y(), before[0].y());
      QCOMPARE(after[1].y(), before[1].y() + drag.delta.y());
      }

// Studio text-style fret fonts resolve to explicit 3.x staff settings.
void TestElement::testStudioTabFont()
      {
      const QByteArray xml("<StaffType group=\"tablature\"><fretUseTextStyle>1</fretUseTextStyle>"
            "<fretTextStyle>tab_fret_number</fretTextStyle></StaffType>");
      XmlReader reader(xml);
      QVERIFY(reader.readNextStartElement());
      StaffType st;
      st.read(reader, score);
      QCOMPARE(st.fretFontSize(), 9.0);

      const QByteArray explicitXml("<StaffType group=\"tablature\"><fretFontSize>12</fretFontSize>"
            "<fretTextStyle>tab_fret_number</fretTextStyle><fretUseTextStyle>1</fretUseTextStyle></StaffType>");
      XmlReader explicitReader(explicitXml);
      QVERIFY(explicitReader.readNextStartElement());
      st.read(explicitReader, score);
      QCOMPARE(st.fretFontSize(), 12.0);
      }

void TestElement::testGuitarBendProperties()
      {
      for (int kind = 0; kind < 8; ++kind) {
            GuitarBend bend(score);
            bend.setBendType(GuitarBendType(kind));
            QCOMPARE(bend.lineWidth(), bend.propertyDefault(Pid::LINE_WIDTH).toDouble());
            bend.setProperty(Pid::BEND_START_TIME_FACTOR, 0.125);
            bend.setProperty(Pid::BEND_END_TIME_FACTOR, 0.75);
            bend.setProperty(Pid::BEND_TARGET_TIME_FACTOR, 0.375);
            bend.setProperty(Pid::GUITAR_BEND_AMOUNT, -7);
            bend.setProperty(Pid::BEND_SHOW_HOLD_LINE, 2);
            auto* copy = toGuitarBend(writeReadElement(&bend));
            QCOMPARE(int(copy->bendType()), kind);
            QCOMPARE(copy->getProperty(Pid::BEND_START_TIME_FACTOR).toDouble(), 0.125);
            QCOMPARE(copy->getProperty(Pid::BEND_END_TIME_FACTOR).toDouble(), 0.75);
            QCOMPARE(copy->getProperty(Pid::BEND_SHOW_HOLD_LINE).toInt(), 2);
            if (kind == int(GuitarBendType::DIP)) {
                  QCOMPARE(copy->getProperty(Pid::GUITAR_BEND_AMOUNT).toInt(), -7);
                  QCOMPARE(copy->getProperty(Pid::BEND_TARGET_TIME_FACTOR).toDouble(), 0.375);
                  }
            delete copy;
            }
      }

void TestElement::testGuitarBendStudioImport()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QVERIFY(imported.staff(1)->isLinked(imported.staff(0)));
      QCOMPARE(imported.staff(1)->staffType(Fraction(0, 1))->fretFontSize(), 9.0);
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      QCOMPARE(bends.size(), 16);
      for (GuitarBend* b : bends) {
            QCOMPARE(b->linkList().size(), 2);
            QVERIFY(b->startNote());
            QCOMPARE(b->startNote()->linkList().size(), 2);
            QVERIFY(b->endNote());
            QVERIFY(b->startNote()->spannerFor().contains(b));
            QVERIFY(b->endNote()->spannerBack().contains(b));
            QVERIFY(!b->spannerSegments().empty());
            if (!b->singleNote())
                  QCOMPARE(GuitarBend::frettingPitch(b->endNote()), GuitarBend::frettingPitch(b->startNote()));
            }
      // Studio's named parentheses must survive import, not be parsed as
      // a false boolean. Both standard-staff pre types use a grace anchor.
      for (GuitarBend* b : bends) {
            if (b->staffIdx() != 0 || (b->bendType() != GuitarBendType::PRE_BEND
                  && b->bendType() != GuitarBendType::PRE_DIVE))
                  continue;
            bool left = false;
            bool right = false;
            for (Element* e : b->startNote()->el()) {
                  if (e->type() != ElementType::SYMBOL)
                        continue;
                  const SymId id = static_cast<Symbol*>(e)->sym();
                  left |= id == SymId::noteheadParenthesisLeft;
                  right |= id == SymId::noteheadParenthesisRight;
                  }
            QVERIFY(left);
            QVERIFY(right);
            }
      GuitarBend* edited = *bends.begin();
      const qreal oldEndTime = edited->getProperty(Pid::BEND_END_TIME_FACTOR).toDouble();
      imported.startCmd();
      edited->undoChangeProperty(Pid::BEND_END_TIME_FACTOR, 0.625);
      imported.endCmd();
      for (const ScoreElement* linked : edited->linkList())
            QCOMPARE(linked->getProperty(Pid::BEND_END_TIME_FACTOR).toDouble(), 0.625);
      imported.startCmd();
      edited->undoChangeProperty(Pid::BEND_END_TIME_FACTOR, oldEndTime);
      imported.endCmd();
      QVERIFY(!imported.pages().isEmpty());
      const QSizeF pageSize = imported.pages().first()->bbox().size();
      const qreal scale = 1500.0 / pageSize.width();
      QImage preview(QSize(1500, qCeil(pageSize.height() * scale)), QImage::Format_ARGB32_Premultiplied);
      preview.fill(Qt::white);
      const qreal previousPixelRatio = MScore::pixelRatio;
      MScore::pixelRatio = DPI / preview.logicalDpiX();
      {
            QPainter painter(&preview);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.scale(scale, scale);
            imported.print(&painter, 0);
      }
      MScore::pixelRatio = previousPixelRatio;
      QVERIFY(preview.save("guitar-import-preview.png"));
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      bool hasBend = false;
      bool hasReset = false;
      for (const auto& event : events) {
            if (event.second.type() != ME_PITCHBEND)
                  continue;
            const int value = event.second.dataA() + 128 * event.second.dataB();
            hasBend |= value > 8192;
            hasReset |= value == 8192;
            }
      QVERIFY(hasBend);
      QVERIFY(hasReset);
      QVERIFY(saveScore(&imported, "guitar-roundtrip.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-roundtrip.mscx"));
      QVERIFY(reread);
      bends.clear();
      reread->scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      QCOMPARE(bends.size(), 16);
      }

void TestElement::testGuitarBendCreation()
      {
      for (int kind = 0; kind < 8; ++kind) {
            MasterScore imported(score->style());
            QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
                  Score::FileError::FILE_NO_ERROR);
            imported.doLayout();
            Measure* measure = imported.firstMeasure();
            for (int i = 0; i < 3; ++i)
                  measure = measure->nextMeasure();
            QVERIFY(measure);
            auto* chord = toChord(measure->first(SegmentType::ChordRest)->element(0));
            QCOMPARE(chord->linkList().size(), 2);
            Note* note = chord->upNote();
            auto* bend = new GuitarBend(&imported);
            bend->setBendType(GuitarBendType(kind));
            EditData data(nullptr);
            data.dropElement = bend;
            QVERIFY(note->acceptDrop(data));
            imported.startCmd();
            Element* result = note->drop(data);
            imported.endCmd();
            QVERIFY(result && result->isGuitarBend());
            QCOMPARE(bend->linkList().size(), 2);
            for (ScoreElement* linked : bend->linkList()) {
                  const auto* b = toGuitarBend(linked);
                  QCOMPARE(int(b->bendType()), kind);
                  QVERIFY(b->startNote());
                  QVERIFY(b->endNote());
                  if (b->singleNote())
                        QCOMPARE(b->startNote(), b->endNote());
                  else
                        QVERIFY(b->startNote() != b->endNote());
                  }
            }
      }

void TestElement::testGuitarBendGeometryAndEdits()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* edited = nullptr;
      for (GuitarBend* b : bends) {
            auto* seg = static_cast<GuitarBendSegment*>(b->frontSegment());
            const bool tab = b->staffIdx() == 1;
            const auto kind = b->bendType();
            QVERIFY(seg->isMovable());
            QCOMPARE(seg->normalModeEditBehavior(), Element::EditBehavior::Edit);
            imported.select(b);
            QCOMPARE(imported.selection().element(), static_cast<Element*>(seg));
            if (tab && !b->isDive()) {
                  QCOMPARE(seg->arrow().size(), 3);
                  const QPointF arrowBase = (seg->arrow()[1] + seg->arrow()[2]) * 0.5;
                  QCOMPARE(seg->path().currentPosition(), arrowBase);
                  QVERIFY(seg->arrow()[0] != arrowBase);
                  }
            if (!tab && kind == GuitarBendType::SLIGHT_BEND) {
                  QCOMPARE(seg->path().elementCount(), 4);
                  QCOMPARE(int(seg->path().elementAt(1).type), int(QPainterPath::CurveToElement));
                  }
            if (!tab && kind == GuitarBendType::PRE_BEND) {
                  QCOMPARE(seg->path().elementCount(), 3);
                  QVERIFY(seg->path().currentPosition().x() > seg->path().elementAt(0).x);
                  }
            if (kind == GuitarBendType::DIP || kind == GuitarBendType::SCOOP)
                  QCOMPARE(seg->gripsCount(), 0);
            else
                  QCOMPARE(seg->gripsCount(), b->isDive() ? 3 : 4);
            if (tab && kind == GuitarBendType::BEND && !edited)
                  edited = b;
            }
      QVERIFY(edited);
      auto* seg = static_cast<GuitarBendSegment*>(edited->frontSegment());
      const auto before = seg->gripsPositions();
      const QPointF delta(0.3 * seg->spatium(), -0.4 * seg->spatium());
      EditData data;
      data.curGrip = Grip::APERTURE;
      data.evtDelta = delta;
      Element* editor = seg;
      // Run the same property capture/finalization used by a GUI grip drag.
      auto* editState = new ElementEditData;
      editState->e = editor;
      data.addData(editState);
      imported.startCmd();
      editor->startEditDrag(data);
      editor->editDrag(data);
      editor->endEditDrag(data);
      imported.endCmd();
      QCOMPARE(seg->getProperty(Pid::BEND_VERTEX_OFF).toPointF(), delta);
      imported.undoStack()->undo(&data);
      QCOMPARE(seg->getProperty(Pid::BEND_VERTEX_OFF).toPointF(), QPointF());
      imported.undoStack()->redo(&data);
      QCOMPARE(seg->getProperty(Pid::BEND_VERTEX_OFF).toPointF(), delta);
      seg->layout();
      auto grips = seg->gripsPositions();
      QCOMPARE(grips[int(Grip::START)], before[int(Grip::START)]);
      QCOMPARE(grips[int(Grip::END)], before[int(Grip::END)]);
      QVERIFY(QLineF(grips[int(Grip::APERTURE)], before[int(Grip::APERTURE)] + delta).length() < 0.000001);
      std::unique_ptr<GuitarBend> vertexOnly(toGuitarBend(writeReadElement(edited)));
      QVERIFY(vertexOnly && !vertexOnly->segmentsEmpty());
      // writeReadElement() reads into MTest::score, whose spatium can differ
      // from the imported score. Serialized offsets use staff-space units.
      const QPointF deltaSp = delta / imported.spatium();
      QVERIFY(QLineF(vertexOnly->frontSegment()->getProperty(Pid::BEND_VERTEX_OFF).toPointF()
            / vertexOnly->score()->spatium(), deltaSp).length() < 0.000001);
      seg->setProperty(Pid::BEND_VERTEX_OFF, QPointF());
      data.curGrip = Grip::END;
      editor->editDrag(data);
      seg->layout();
      grips = seg->gripsPositions();
      QVERIFY(QLineF(grips[int(Grip::END)], before[int(Grip::END)] + delta).length() < 0.000001);
      std::unique_ptr<GuitarBend> endOnly(toGuitarBend(writeReadElement(edited)));
      QVERIFY(endOnly && !endOnly->segmentsEmpty());
      QVERIFY(QLineF(endOnly->frontSegment()->userOff2() / endOnly->score()->spatium(), deltaSp).length() < 0.000001);
      seg->setProperty(Pid::BEND_VERTEX_OFF, delta);
      data.clearData();
      QVERIFY(saveScore(&imported, "guitar-edited-roundtrip.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-edited-roundtrip.mscx"));
      QVERIFY(reread);
      reread->doLayout();
      QSet<GuitarBend*> copies;
      reread->scanElements(&copies, [](void* context, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(context)->insert(toGuitarBend(e));
            });
      int editedCount = 0;
      for (GuitarBend* b : copies) {
            const auto* copy = static_cast<GuitarBendSegment*>(b->frontSegment());
            if (copy->getProperty(Pid::BEND_VERTEX_OFF).toPointF().isNull())
                  continue;
            ++editedCount;
            QVERIFY(QLineF(copy->getProperty(Pid::BEND_VERTEX_OFF).toPointF(), delta).length() < 0.000001);
            QVERIFY(QLineF(copy->userOff2(), delta).length() < 0.000001);
            }
      QCOMPARE(editedCount, 1);
      }

void TestElement::testGuitarBendFretRules()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* pre = nullptr;
      for (GuitarBend* b : bends) {
            if (b->staffIdx() != 1 || b->singleNote())
                  continue;
            const Note* origin = GuitarBend::frettingNote(b->endNote());
            QCOMPARE(b->endNote()->string(), origin->string());
            QCOMPARE(b->endNote()->fret(), origin->fret());
            if (b->bendType() == GuitarBendType::PRE_BEND)
                  pre = b;
            }
      QVERIFY(pre);
      QVERIFY(!pre->startNote()->visible());
      QVERIFY(pre->endNote()->visible());
      // A grace-note bend uses a visible grace fret and hidden arrival.
      // A stale imported visibility bit must not reverse those semantics.
      pre->setBendType(GuitarBendType::GRACE_NOTE_BEND);
      pre->startNote()->setVisible(false);
      GuitarBend::updateTabNote(pre->startNote());
      GuitarBend::updateTabNote(pre->endNote());
      QVERIFY(pre->startNote()->visible());
      QVERIFY(!pre->endNote()->visible());
      imported.startCmd();
      pre->startNote()->undoChangeProperty(Pid::VISIBLE, false);
      imported.endCmd();
      QVERIFY(pre->startNote()->overrideBendVisibilityRules());
      GuitarBend::updateTabNote(pre->startNote());
      QVERIFY(!pre->startNote()->visible());
      QVERIFY(saveScore(&imported, "guitar-visibility-override.mscx"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("guitar-visibility-override.mscx"));
      QVERIFY(copy);
      int overrides = 0;
      copy->scanElements(&overrides, [](void* data, Element* e) {
            if (e->isNote() && toNote(e)->overrideBendVisibilityRules())
                  ++*static_cast<int*>(data);
            });
      QCOMPARE(overrides, pre->startNote()->linkList().size());
      imported.undoStack()->undo(&ed);
      QVERIFY(!pre->startNote()->overrideBendVisibilityRules());
      GuitarBend::updateTabNote(pre->startNote());
      QVERIFY(pre->startNote()->visible());
      }

void TestElement::testGuitarDiveAndHoldGeometry()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* firstBend = nullptr;
      GuitarBend* pre = nullptr;
      for (GuitarBend* b : bends) {
            auto* seg = static_cast<GuitarBendSegment*>(b->frontSegment());
            if (b->bendType() == GuitarBendType::SCOOP) {
                  QVERIFY(seg->path().isEmpty());
                  QVERIFY(seg->bbox().width() > 0.0);
                  }
            if (b->staffIdx() == 0 && b->bendType() == GuitarBendType::DIP) {
                  const QPointF previous = b->startNote()->pos();
                  b->startNote()->rypos() -= 8.0 * b->spatium();
                  seg->layout();
                  QVERIFY(seg->path().boundingRect().bottom() < b->startNote()->bbox().top());
                  b->startNote()->setPos(previous);
                  }
            if (b->staffIdx() != 1)
                  continue;
            if (b->bendType() == GuitarBendType::DIVE) {
                  const int oldPitch = b->endNote()->pitch();
                  b->endNote()->setPitch(b->startNote()->pitch() - 12);
                  b->layout();
                  const StaffType* tab = b->startNote()->staff()->staffTypeForElement(b->startNote());
                  const qreal staffBottom = seg->system()->staffYpage(1)
                        + (tab->lines() - 1) * tab->lineDistance().val() * seg->spatium();
                  QVERIFY(seg->pagePos().y() + seg->path().currentPosition().y() > staffBottom);
                  QVERIFY(seg->textRect().top() > seg->path().currentPosition().y());
                  b->endNote()->setPitch(oldPitch);
                  }
            if (b->bendType() == GuitarBendType::BEND && b->tick() == Fraction(0, 1))
                  firstBend = b;
            if (b->bendType() == GuitarBendType::PRE_BEND)
                  pre = b;
            }
      QVERIFY(firstBend && pre);
      // Reproduce a held bend followed by a matching pre-bend without
      // needing a tie between the two notational representations.
      pre->startNote()->setPitch(firstBend->startNote()->pitch());
      pre->startNote()->setString(firstBend->startNote()->string());
      pre->startNote()->setFret(firstBend->startNote()->fret());
      pre->endNote()->setPitch(firstBend->endNote()->pitch());
      imported.doLayout();
      QCOMPARE(firstBend->followingPreBend(), static_cast<const GuitarBend*>(pre));
      firstBend->layout();
      const auto* segment = static_cast<const GuitarBendSegment*>(firstBend->frontSegment());
      QVERIFY(!segment->holdPath().isEmpty());
      }

void TestElement::testGuitarTapping()
      {
      Articulation tapping(score);
      tapping.setTappingHand(2);
      QCOMPARE(tapping.symId(), SymId::pluckedLeftHandPizzicato);
      std::unique_ptr<Articulation> copy(static_cast<Articulation*>(writeReadElement(&tapping)));
      QVERIFY(copy);
      QCOMPARE(copy->tappingHand(), 2);
      QCOMPARE(copy->symId(), SymId::pluckedLeftHandPizzicato);
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      Segment* segment = imported.firstSegment(SegmentType::ChordRest);
      QVERIFY(segment);
      auto* tap = new Articulation(&imported);
      tap->setTappingHand(2);
      tap->setTrack(0);
      tap->setParent(segment->element(0));
      imported.startCmd();
      imported.undoAddElement(tap);
      imported.endCmd();
      imported.doLayout();
      QCOMPARE(tap->linkList().size(), 2);
      for (ScoreElement* linked : tap->linkList()) {
            const auto* articulation = toArticulation(linked);
            QCOMPARE(articulation->tappingHand(), 2);
            QVERIFY(articulation->bbox().width() > 0.0);
            }
      QVERIFY(saveScore(&imported, "guitar-tapping-roundtrip.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-tapping-roundtrip.mscx"));
      QVERIFY(reread);
      int taps = 0;
      reread->scanElements(&taps, [](void* data, Element* e) {
            if (e->isArticulation() && toArticulation(e)->tappingHand() == 2)
                  ++*static_cast<int*>(data);
            });
      QCOMPARE(taps, 2);
      }

void TestElement::testStudioGhostAndDeadFrets()
      {
      MasterScore modern(score->style());
      modern.setMscVersion(460);
      Note ghost(&modern);
      XmlReader ghostXml(QByteArray("<Note><pitch>60</pitch><tpc>14</tpc><ghost>1</ghost></Note>"));
      QVERIFY(ghostXml.readNextStartElement());
      ghost.read(ghostXml);
      QVERIFY(ghost.studioGhost());
      QVERIFY(!ghost.ghost()); // 3.x's flag is the dead-fret x
      std::unique_ptr<Note> copy(static_cast<Note*>(writeReadElement(&ghost)));
      QVERIFY(copy && copy->studioGhost());
      QVERIFY(!copy->ghost());
      Note dead(&modern);
      XmlReader deadXml(QByteArray("<Note><pitch>60</pitch><tpc>14</tpc><dead>1</dead></Note>"));
      QVERIFY(deadXml.readNextStartElement());
      dead.read(deadXml);
      QVERIFY(dead.ghost());
      QVERIFY(!dead.studioGhost());
      MasterScore legacy(score->style());
      legacy.setMscVersion(302);
      Note oldDead(&legacy);
      XmlReader oldXml(QByteArray("<Note><pitch>60</pitch><tpc>14</tpc><ghost>1</ghost></Note>"));
      QVERIFY(oldXml.readNextStartElement());
      oldDead.read(oldXml);
      QVERIFY(oldDead.ghost());
      QVERIFY(!oldDead.studioGhost());
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      Note* tab = toChord(imported.firstSegment(SegmentType::ChordRest)->element(4))->upNote();
      tab->setStudioGhost(true);
      tab->layout();
      QVERIFY(tab->fretString().startsWith('('));
      QVERIFY(tab->fretString().endsWith(')'));
      QVERIFY(!tab->ghost());
      }

void TestElement::testGuitarSkylineClearance()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      struct Items { StaffText* text = nullptr; GuitarBend* dip = nullptr; } items;
      imported.scanElements(&items, [](void* data, Element* e) {
            auto* items = static_cast<Items*>(data);
            if (e->staffIdx() != 0)
                  return;
            if (e->isStaffText() && toStaffText(e)->plainText() == "Dip")
                  items->text = toStaffText(e);
            if (e->isGuitarBend() && toGuitarBend(e)->bendType() == GuitarBendType::DIP
                  && e->tick() == Fraction(5, 1))
                  items->dip = toGuitarBend(e);
            });
      QVERIFY(items.text && items.dip);
      const auto* segment = items.dip->frontSegment();
      const QRectF text = items.text->bbox().translated(items.text->pagePos());
      const QRectF bend = segment->bbox().translated(segment->pagePos());
      QVERIFY(!text.intersects(bend));
      }

// The supplied Studio MIDI reaches one quarter of the pitch change at
// half duration: +1 of +4 semitones, and -3 of a -12-semitone dive.
void TestElement::testGuitarBendMidiCurve()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      int channel = -1;
      for (const auto& event : events) {
            if (event.first == 0 && event.second.type() == ME_NOTEON && event.second.velo() > 0) {
                  channel = event.second.channel();
                  break;
                  }
            }
      QVERIFY(channel >= 0);
      auto wheelAt = [&](int tick) {
            int activeChannel = channel;
            for (const auto& entry : events) {
                  if (entry.first > tick)
                        break;
                  const auto& event = entry.second;
                  if (event.type() == ME_NOTEON && event.velo() > 0
                        && event.note() && event.note()->track() == 0)
                        activeChannel = event.channel();
                  }
            int value = 8192;
            for (const auto& event : events) {
                  if (event.first > tick)
                        break;
                  if (event.second.channel() == activeChannel && event.second.type() == ME_PITCHBEND)
                        value = event.second.dataA() + 128 * event.second.dataB();
                  }
            return value;
            };
      QVERIFY(qAbs(wheelAt(240) - qRound(8192.0 + 8192.0 / 12.0)) <= 2);
      QVERIFY(qAbs(wheelAt(480) - qRound(8192.0 + 4.0 * 8192.0 / 12.0)) <= 2);
      QVERIFY(qAbs(wheelAt(8400) - 6144) <= 2);
      QVERIFY(qAbs(wheelAt(8640)) <= 2);
      }

void TestElement::testPolyphonicGuitarBendMidi()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Chord* from = toChord(first->element(0));
      Chord* to = toChord(next->element(0));
      QVERIFY(from && to && from->notes().size() >= 3 && to->notes().size() >= 3);
      for (int i = 0; i < 2; ++i) {
            Note* start = from->notes()[i];
            Note* end = to->notes()[i];
            end->setPitch(start->pitch() + 2 + i);
            auto* bend = new GuitarBend(&imported);
            bend->setStartElement(start);
            bend->setEndElement(end);
            bend->setTick(start->tick());
            bend->setTick2(end->tick());
            bend->setTrack(0);
            bend->setTrack2(0);
            start->add(bend);
            }
      imported.doLayout();
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      int channels[] = { -1, -1, -1 };
      int arrivals = 0;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() != ME_NOTEON || event.velo() <= 0)
                  continue;
            for (int i = 0; i < 3; ++i) {
                  if (event.note() == from->notes()[i])
                        channels[i] = event.channel();
                  }
            if (event.note() == to->notes()[0] || event.note() == to->notes()[1])
                  ++arrivals;
            }
      QVERIFY(channels[0] >= 0 && channels[1] >= 0 && channels[2] >= 0);
      QVERIFY(channels[0] != channels[1]);
      QVERIFY(channels[0] != channels[2] && channels[1] != channels[2]);
      QCOMPARE(arrivals, 0); // bent strings sustain through their chord arrivals
      for (const auto& entry : events) {
            if (entry.second.type() == ME_PITCHBEND)
                  QVERIFY(entry.second.channel() != channels[2]);
            }
      for (int i = 0; i < 2; ++i) {
            int wheel = 8192;
            for (const auto& entry : events) {
                  if (entry.first > to->tick().ticks())
                        break;
                  if (entry.second.channel() == channels[i] && entry.second.type() == ME_PITCHBEND)
                        wheel = entry.second.dataA() + 128 * entry.second.dataB();
                  }
            QVERIFY(qAbs(wheel - qRound(8192.0 + (2 + i) * 8192.0 / 12.0)) <= 2);
            }
      const size_t mappingCount = imported.midiMapping().size();
      EventMap again;
      imported.renderMidi(&again, synth);
      QCOMPARE(imported.midiMapping().size(), mappingCount); // reuse, not channel leakage
      imported.rebuildMidiMapping();
      for (const MidiMapping& mapping : imported.midiMapping())
            QCOMPARE(mapping.bendSource(), -1); // transient lanes never become instrument channels
      }

void TestElement::testOverlappingGuitarBendMidi()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Note* start = toChord(first->element(0))->notes().front();
      Note* end = toChord(next->element(0))->notes().front();
      end->setPitch(start->pitch() - 3); // +1 finger bend combined with -4 bar dive
      for (GuitarBendType type : { GuitarBendType::BEND, GuitarBendType::DIVE }) {
            auto* bend = new GuitarBend(&imported);
            bend->setBendType(type);
            bend->setStartElement(start);
            bend->setEndElement(end);
            bend->setTick(start->tick());
            bend->setTick2(end->tick());
            bend->setTrack(0);
            bend->setTrack2(0);
            if (type == GuitarBendType::BEND)
                  bend->setProperty(Pid::BEND_END_TIME_FACTOR, 0.5);
            else {
                  bend->setProperty(Pid::GUITAR_BEND_AMOUNT, -8);
                  bend->setProperty(Pid::BEND_START_TIME_FACTOR, 0.5);
                  }
            start->add(bend);
            }
      imported.doLayout();
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      int channel = -1;
      for (const auto& entry : events) {
            if (entry.second.type() == ME_NOTEON && entry.second.velo() > 0 && entry.second.note() == start)
                  channel = entry.second.channel();
            }
      QVERIFY(channel >= 0);
      auto wheelAt = [&](int tick) {
            int value = 8192;
            for (const auto& entry : events) {
                  if (entry.first > tick)
                        break;
                  if (entry.second.channel() == channel && entry.second.type() == ME_PITCHBEND)
                        value = entry.second.dataA() + 128 * entry.second.dataB();
                  }
            return value;
            };
      const int halfway = (start->tick().ticks() + end->tick().ticks()) / 2;
      QVERIFY(qAbs(wheelAt(halfway) - qRound(8192.0 + 8192.0 / 12.0)) <= 2);
      QVERIFY(qAbs(wheelAt(end->tick().ticks()) - 6144) <= 2);
      QSet<int> wheelTicks;
      for (const auto& entry : events) {
            if (entry.second.channel() == channel && entry.second.type() == ME_PITCHBEND) {
                  QVERIFY(!wheelTicks.contains(entry.first)); // one combined curve, no competing writes
                  wheelTicks.insert(entry.first);
                  }
            }
      }

void TestElement::testWideGuitarBendMidi()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Note* start = toChord(first->element(0))->notes().front();
      Note* end = toChord(next->element(0))->notes().front();
      end->setPitch(start->pitch() - 18);
      auto* bend = new GuitarBend(&imported);
      bend->setStartElement(start);
      bend->setEndElement(end);
      bend->setTick(start->tick());
      bend->setTick2(end->tick());
      bend->setTrack(0);
      bend->setTrack2(0);
      start->add(bend);
      imported.doLayout();
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      int channel = -1;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() != ME_NOTEON || event.velo() <= 0)
                  continue;
            QVERIFY(event.note() != end); // sustain, do not reattack the low arrival
            if (event.note() == start) {
                  QCOMPARE(entry.first, start->tick().ticks());
                  QCOMPARE(event.pitch(), start->ppitch() - 6);
                  channel = event.channel();
                  }
            }
      QVERIFY(channel >= 0);
      int initial = -1;
      int arrival = -1;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() != ME_PITCHBEND || event.channel() != channel)
                  continue;
            const int value = event.dataA() + 128 * event.dataB();
            if (entry.first == start->tick().ticks())
                  initial = value;
            if (entry.first == end->tick().ticks())
                  arrival = value;
            }
      QCOMPARE(initial, 12288); // key shifted down 6, wheel shifted up 6
      QCOMPARE(arrival, 0);     // then release to -12, covering 18 semitones
      }

void TestElement::testGuitarBendReportedPassage()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitar-bend-fret-conflict475.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      EventMap events;
      SynthesizerState synth;
      imported.renderMidi(&events, synth);
      int releaseChannel = -1;
      int chordAttacks = 0;
      bool graceAttack = false;
      for (const auto& entry : events) {
            const NPlayEvent& event = entry.second;
            if (event.type() != ME_NOTEON || event.velo() <= 0)
                  continue;
            if (entry.first == 16320) {
                  QCOMPARE(event.pitch(), 61); // pre-bend setup uses the audible key
                  releaseChannel = event.channel();
                  }
            if (entry.first == 17280)
                  ++chordAttacks;
            QVERIFY(entry.first != 17520); // no delayed wide-bend fallback attack
            if (entry.first == 18120 && event.pitch() == 60)
                  graceAttack = true;
            }
      QVERIFY(releaseChannel >= 0);
      QCOMPARE(chordAttacks, 3);
      QVERIFY(graceAttack);
      int releaseWheel = 8192;
      for (const auto& entry : events) {
            if (entry.first > 16800)
                  break;
            if (entry.second.channel() == releaseChannel && entry.second.type() == ME_PITCHBEND)
                  releaseWheel = entry.second.dataA() + 128 * entry.second.dataB();
            }
      // C#4 - 8 semitones = the reported low F3, sustained through its arrival.
      QVERIFY(qAbs(releaseWheel - qRound(8192.0 - 8.0 * 8192.0 / 12.0)) <= 2);
      }

void TestElement::testStudio5GuitarImport()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates5-eid.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      auto check = [](MasterScore& current) {
            int bends = 0;
            int hopos = 0;
            int bars = 0;
            int rings = 0;
            for (const Spanner* spanner : allTestSpanners(current)) {
                  if (spanner->isGuitarBend()) {
                        ++bends;
                        const GuitarBend* bend = toGuitarBend(spanner);
                        QVERIFY(bend->startNote() && bend->endNote());
                        QVERIFY(bend->startNote()->spannerFor().contains(const_cast<GuitarBend*>(bend)));
                        QVERIFY(bend->endNote()->spannerBack().contains(const_cast<GuitarBend*>(bend)));
                        QCOMPARE(bend->tick(), bend->startNote()->tick());
                        QCOMPARE(bend->tick2(), bend->endNote()->tick());
                        }
                  else if (spanner->isHammerOnPullOff()) {
                        ++hopos;
                        QVERIFY(spanner->startElement() && spanner->startElement()->isChord());
                        QVERIFY(spanner->endElement() && spanner->endElement()->isChord());
                        }
                  else if (spanner->type() == ElementType::WHAMMY_BAR) {
                        ++bars;
                        QCOMPARE(spanner->tick(), Fraction(3, 1));
                        QCOMPARE(spanner->ticks(), Fraction(1, 2));
                        }
                  else if (spanner->type() == ElementType::LET_RING)
                        ++rings;
                  }
            QCOMPARE(bends, 34);
            QCOMPARE(hopos, 4);
            QCOMPARE(bars, 1);
            QCOMPARE(rings, 2);
            int parentheses = 0;
            current.scanElements(&parentheses, [](void* context, Element* element) {
                  if (!element->isSymbol())
                        return;
                  const SymId symbol = toSymbol(element)->sym();
                  if (symbol == SymId::noteheadParenthesisLeft || symbol == SymId::noteheadParenthesisRight)
                        ++*static_cast<int*>(context);
                  });
            QVERIFY(parentheses >= 10);
            };
      check(imported);
      imported.doLayout();
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("studio5-guitar-roundtrip.mscx");
      QVERIFY(saveScore(&imported, path));
      MasterScore restored(score->style());
      QCOMPARE(restored.loadMsc(path, true), Score::FileError::FILE_NO_ERROR);
      check(restored);
      restored.doLayout();
      }

void TestElement::testStudio5SpannerMap()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/studio5-spanner-map.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      auto check = [](MasterScore& current) {
            const QList<ElementType> types { ElementType::VIBRATO, ElementType::PALM_MUTE,
                  ElementType::HAIRPIN, ElementType::OTTAVA, ElementType::TRILL,
                  ElementType::TEXTLINE, ElementType::PEDAL, ElementType::SLUR, ElementType::VOLTA };
            QMap<ElementType, int> counts;
            for (const Spanner* spanner : allTestSpanners(current)) {
                  if (!types.contains(spanner->type()))
                        continue;
                  ++counts[spanner->type()];
                  QCOMPARE(spanner->track(), 0);
                  QCOMPARE(spanner->track2(), 0);
                  QCOMPARE(spanner->tick(), Fraction(0, 1));
                  if (spanner->type() == ElementType::VOLTA) {
                        QVERIFY(spanner->startElement() && spanner->startElement()->isMeasure());
                        QVERIFY(spanner->endElement() && spanner->endElement()->isMeasure());
                        QCOMPARE(spanner->tick2(), current.firstMeasure()->nextMeasure()->endTick());
                        }
                  else {
                        QCOMPARE(spanner->ticks(), Fraction(1, 4));
                        if (spanner->isSlur()) {
                              QVERIFY(spanner->startElement() && spanner->startElement()->isChord());
                              QVERIFY(spanner->endElement() && spanner->endElement()->isChord());
                              }
                        }
                  }
            for (ElementType type : types)
                  QCOMPARE(counts.value(type), 1);
            };
      // Fail with a useful assertion before layout indexes a system staff.
      for (const Spanner* spanner : allTestSpanners(imported)) {
            QVERIFY(spanner->track() >= 0);
            QVERIFY(spanner->staffIdx() < imported.nstaves());
            QVERIFY(spanner->track2() >= 0);
            QVERIFY(spanner->track2() / VOICES < imported.nstaves());
            }
      imported.doLayout();
      check(imported);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("studio5-lines-roundtrip.mscx");
      QVERIFY(saveScore(&imported, path));
      MasterScore restored(score->style());
      QCOMPARE(restored.loadMsc(path, true), Score::FileError::FILE_NO_ERROR);
      restored.doLayout();
      check(restored);
      }

void TestElement::testStudio5NoteSpanners()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/studio5-spanner-map.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      auto check = [](MasterScore& current) {
            QSet<Tie*> ties;
            current.scanElements(&ties, [](void* context, Element* element) {
                  if (element->isNote() && toNote(element)->tieFor())
                        static_cast<QSet<Tie*>*>(context)->insert(toNote(element)->tieFor());
                  });
            QCOMPARE(ties.size(), 1);
            Tie* tie = *ties.begin();
            QVERIFY(tie->startNote() && tie->endNote());
            QCOMPARE(tie->startNote()->pitch(), 72);
            QCOMPARE(tie->endNote()->pitch(), 72);
            QVERIFY(tie->startNote()->tieFor() == tie);
            QVERIFY(tie->endNote()->tieBack() == tie);
            int glissandi = 0;
            for (Spanner* spanner : allTestSpanners(current)) {
                  QVERIFY(!spanner->isTie()); // 3.x ties belong to their notes
                  if (!spanner->isGlissando())
                        continue;
                  ++glissandi;
                  QVERIFY(spanner->startElement() == tie->endNote());
                  QVERIFY(spanner->endElement() && spanner->endElement()->isNote());
                  QCOMPARE(toNote(spanner->endElement())->pitch(), 76);
                  QVERIFY(toNote(spanner->startElement())->spannerFor().contains(spanner));
                  QVERIFY(toNote(spanner->endElement())->spannerBack().contains(spanner));
                  QVERIFY(toNote(spanner->endElement())->chord()->endsGlissando());
                  }
            QCOMPARE(glissandi, 1);
            };
      imported.doLayout();
      check(imported);
      QTemporaryDir directory;
      QVERIFY(directory.isValid());
      const QString path = directory.filePath("studio5-note-lines-roundtrip.mscx");
      QVERIFY(saveScore(&imported, path));
      MasterScore restored(score->style());
      QCOMPARE(restored.loadMsc(path, true), Score::FileError::FILE_NO_ERROR);
      restored.doLayout();
      check(restored);
      }

void TestElement::testNoteParenthesesClearance()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QList<Note*> notes;
      imported.scanElements(&notes, [](void* data, Element* e) {
            if (e->isNote() && e->staffIdx() == 0)
                  static_cast<QList<Note*>*>(data)->append(toNote(e));
            });
      Note* target = nullptr;
      Symbol* left = nullptr;
      Symbol* right = nullptr;
      for (Note* note : notes) {
            if (!note->chord()->ledgerLines())
                  continue;
            for (Element* e : note->el()) {
                  if (e->isSymbol() && toSymbol(e)->sym() == SymId::noteheadParenthesisLeft)
                        left = toSymbol(e);
                  if (e->isSymbol() && toSymbol(e)->sym() == SymId::noteheadParenthesisRight)
                        right = toSymbol(e);
                  }
            if (left && right) {
                  target = note;
                  break;
                  }
            left = right = nullptr;
            }
      QVERIFY(target && left && right);
      if (!target->accidental()) {
            auto* accidental = new Accidental(&imported);
            accidental->setAccidentalType(AccidentalType::SHARP);
            target->add(accidental);
            }
      target->accidental()->layout();
      target->accidental()->setPos(-2.0 * target->spatium(), 0.0);
      target->layout2();
      const QRectF l = left->bbox().translated(left->pos());
      const QRectF r = right->bbox().translated(right->pos());
      const QRectF accidental = target->accidental()->bbox().translated(target->accidental()->pos());
      QVERIFY(l.right() < accidental.left());
      int checked = 0;
      for (LedgerLine* line = target->chord()->ledgerLines(); line; line = line->next()) {
            const QRectF ledger = line->bbox().translated(line->pos() - target->pos());
            if (!line->visible() || ledger.bottom() < l.top() || ledger.top() > l.bottom())
                  continue;
            QVERIFY(l.right() < ledger.left());
            QVERIFY(r.left() > ledger.right());
            ++checked;
            }
      QVERIFY(checked > 0);
      const QPointF leftPos = left->pos();
      const QPointF rightPos = right->pos();
      target->layout2();
      QCOMPARE(left->pos(), leftPos);
      QCOMPARE(right->pos(), rightPos);
      }

void TestElement::testGuitarTabFretSizeAndAnchors()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.style().set(Sid::useCueSizeFretForGraceBends, false);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 1)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* grace = nullptr;
      GuitarBend* pre = nullptr;
      for (GuitarBend* bend : bends) {
            if (bend->bendType() == GuitarBendType::GRACE_NOTE_BEND)
                  grace = bend;
            if (bend->bendType() == GuitarBendType::PRE_BEND)
                  pre = bend;
            }
      QVERIFY(pre && grace);
      for (GuitarBend* bend : { pre, grace }) {
            const Note* source = bend->startNote();
            QCOMPARE(source->chord()->chordMag(), 1.0);
            const auto* segment = static_cast<const GuitarBendSegment*>(bend->frontSegment());
            const QPointF start = segment->pagePos() + segment->path().pointAtPercent(0.0);
            const Note* anchor = bend == pre ? bend->endNote() : source;
            const QRectF fret = anchor->bbox().translated(anchor->pagePos());
            if (bend == pre) {
                  QVERIFY(qAbs(start.x() - fret.center().x()) < 0.00001);
                  QVERIFY(qAbs(start.y() - (fret.top() - 0.25 * segment->spatium())) < 0.00001);
                  }
            else {
                  QVERIFY(qAbs(start.x() - (fret.right() + 0.25 * segment->spatium())) < 0.00001);
                  QVERIFY(qAbs(start.y() - (fret.top() + 0.5 * bend->lineWidth())) < 0.00001);
                  }
            }
      imported.style().set(Sid::useCueSizeFretForGraceBends, true);
      QCOMPARE(grace->startNote()->chord()->chordMag(), imported.styleD(Sid::graceNoteMag));
      QVERIFY(saveScore(&imported, "guitar-cue-fret-style.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-cue-fret-style.mscx"));
      QVERIFY(reread);
      QVERIFY(reread->styleB(Sid::useCueSizeFretForGraceBends));
      }

void TestElement::testGuitarBendLabelStyle()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 1)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* half = nullptr;
      for (GuitarBend* bend : bends) {
            if (!bend->isDive() && bend->totalBendAmount() == 2)
                  half = bend;
            }
      QVERIFY(half);
      auto* segment = static_cast<GuitarBendSegment*>(half->frontSegment());
      imported.style().set(Sid::bendFontFace, QString("Edwin"));
      imported.style().set(Sid::bendFontSize, 12.0);
      imported.style().set(Sid::bendFontStyle, int(FontStyle::Bold + FontStyle::Italic));
      imported.style().set(Sid::bendFontSpatiumDependent, true);
      segment->layout();
      const QFont font = segment->labelFont();
      QCOMPARE(font.family(), QString("Edwin"));
      QVERIFY(font.bold() && font.italic());
      QVERIFY(qAbs(font.pointSizeF() - 12.0 * segment->spatium() / SPATIUM20) < 0.00001);
      QVERIFY(!segment->arrow().isEmpty());
      const qreal gap = segment->arrow().front().y() - segment->textRect().bottom();
      QVERIFY(qAbs(gap - 0.2 * segment->spatium()) < 0.00001);
      const qreal largeHeight = segment->textRect().height();
      imported.style().set(Sid::bendFontSize, 8.0);
      segment->layout();
      QVERIFY(segment->textRect().height() < largeHeight);
      imported.style().set(Sid::bendFontSpatiumDependent, false);
      QCOMPARE(segment->labelFont().pointSizeF(), 8.0);
      imported.style().set(Sid::useFractionCharacters, true);
      QCOMPARE(half->bendText(), QString::fromUtf8("½"));
      imported.style().set(Sid::useFractionCharacters, false);
      QCOMPARE(half->bendText(), QString("1/2"));
      // Full-bend text uses the real connected pitch amount.
      for (GuitarBend* bend : bends) {
            if (!bend->isDive() && bend->totalBendAmount() == 4) {
                  imported.style().set(Sid::guitarBendUseFull, true);
                  QCOMPARE(bend->bendText(), QString("full"));
                  }
            }
      QVERIFY(saveScore(&imported, "guitar-label-style.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-label-style.mscx"));
      QVERIFY(reread);
      QVERIFY(!reread->styleB(Sid::useFractionCharacters));
      QVERIFY(reread->styleB(Sid::guitarBendUseFull));
      }

void TestElement::testGuitarPreDivePen()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 1)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      int checked = 0;
      for (GuitarBend* bend : bends) {
            const auto* segment = static_cast<const GuitarBendSegment*>(bend->frontSegment());
            const QPen pen = segment->linePen();
            QCOMPARE(pen.capStyle(), Qt::FlatCap);
            if (bend->bendType() != GuitarBendType::PRE_DIVE)
                  continue;
            const auto pattern = pen.dashPattern();
            QCOMPARE(pattern.size(), 2);
            const qreal length = segment->path().length() / pen.widthF();
            const int pairs = qMax(1, int(length / 6.0));
            QVERIFY(pattern[0] > 0.0 && pattern[1] > 0.0);
            QVERIFY(qAbs(pattern[0] * (pairs + 1) + pattern[1] * pairs - length) < 0.00001);
            ++checked;
            }
      QVERIFY(checked > 0);
      }

void TestElement::testGuitarTechniqueClearance()
      {
      // Check explicit style values independently of the compressed score and
      // its notation reader; include a custom value, not only Studio's default.
      for (qreal distance : {0.4, 0.75}) {
            MStyle parsed(score->style());
            XmlReader xml(QString("<Style><articulationMinDistance>%1</articulationMinDistance></Style>")
                  .arg(distance).toUtf8());
            QVERIFY(xml.readNextStartElement());
            parsed.load(xml, 470);
            QCOMPARE(parsed.value(Sid::articulationMinDistance).value<Spatium>().val(), distance);
            }
      // A compatibility reader must consume minEmptyMeasures exactly once:
      // a following property must still load, for default and custom values.
      for (int minimum : {1, 3}) {
            MStyle parsed(score->style());
            parsed.set(Sid::minEmptyMeasures, 2);
            XmlReader xml(QString("<Style><minEmptyMeasures>%1</minEmptyMeasures>"
                  "<articulationMinDistance>0.75</articulationMinDistance></Style>")
                  .arg(minimum).toUtf8());
            QVERIFY(xml.readNextStartElement());
            parsed.load(xml, 470);
            QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
            QCOMPARE(parsed.value(Sid::minEmptyMeasures).toInt(), minimum == 1 ? 2 : minimum);
            QCOMPARE(parsed.value(Sid::articulationMinDistance).value<Spatium>().val(), 0.75);
            }
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      QCOMPARE(imported.styleS(Sid::propertyDistanceHead).val(), 0.4);
      QCOMPARE(imported.styleS(Sid::propertyDistanceStem).val(), 0.4);
      QCOMPARE(imported.styleS(Sid::propertyDistance).val(), 0.4);
      QCOMPARE(imported.styleS(Sid::articulationMinDistance).val(), 0.4);
      imported.doLayout();
      QList<Articulation*> marks;
      imported.scanElements(&marks, [](void* data, Element* e) {
            if (e->isArticulation()) {
                  auto* a = toArticulation(e);
                  if (a->tappingHand() || a->symId() == SymId::stringsThumbPosition)
                        static_cast<QList<Articulation*>*>(data)->append(a);
                  }
            });
      int checked = 0;
      for (Articulation* a : marks) {
            if (!a->up())
                  continue;
            const Chord* chord = toChord(a->parent());
            const Note* note = chord->upNote();
            const qreal gap = note->pagePos().y() + note->bbox().top()
                  - (a->pagePos().y() + a->bbox().bottom());
            QVERIFY(gap >= 0.39 * a->spatium());
            QVERIFY(gap < 1.0 * a->spatium());
            ++checked;
            }
      QVERIFY(checked >= 4);
      QVERIFY(saveScore(&imported, "guitar-technique-spacing.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-technique-spacing.mscx"));
      QVERIFY(reread);
      QCOMPARE(reread->styleS(Sid::propertyDistanceHead).val(), 0.4);
      }

void TestElement::testTabParenthesesMask()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      auto* chord = toChord(imported.firstMeasure()->first(SegmentType::ChordRest)->element(VOICES));
      QVERIFY(chord);
      Note note(&imported);
      note.setParent(chord);
      note.setTrack(chord->track());
      note.setFret(10);
      note.setString(0);
      note.layout();
      const qreal bareWidth = note.bbox().width();
      note.setStudioGhost(true);
      note.layout();
      QCOMPARE(note.fretString(), QString("(10)"));
      QVERIFY(note.bbox().width() > bareWidth);
      const QRectF inlineMask = note.tabFretMaskRect();
      QVERIFY(inlineMask.left() < note.bbox().left());
      QVERIFY(inlineMask.right() > note.bbox().right());
      note.setStudioGhost(false);
      note.layout();
      const QRectF bareMask = note.tabFretMaskRect();
      for (SymId id : { SymId::noteheadParenthesisLeft, SymId::noteheadParenthesisRight }) {
            auto* symbol = new Symbol(&imported);
            symbol->setSym(id);
            note.add(symbol);
            }
      note.layout2();
      const QRectF attachedMask = note.tabFretMaskRect();
      for (const Element* e : note.el()) {
            const QRectF paren = e->bbox().translated(e->pos());
            QVERIFY(attachedMask.left() < paren.left());
            QVERIFY(attachedMask.right() > paren.right());
            }
      QCOMPARE(attachedMask.top(), inlineMask.top());
      QCOMPARE(attachedMask.bottom(), inlineMask.bottom());
      for (Element* child : note.el()) {
            Symbol* symbol = toSymbol(child);
            symbol->setFlexibleTabParenthesis(true);
            symbol->layoutTabParenthesis(note.bbox().adjusted(-note.spatium(), -note.spatium(),
                  note.spatium(), note.spatium()));
            QVERIFY(!symbol->tabParenthesisPath().isEmpty());
            }
      QCOMPARE(note.tabFretMaskRect(), bareMask);
      }

void TestElement::testTappingTextStyle()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QList<Articulation*> taps;
      imported.scanElements(&taps, [](void* data, Element* e) {
            if (e->isArticulation() && e->staffIdx() == 1 && toArticulation(e)->tappingHand() == 2)
                  static_cast<QList<Articulation*>*>(data)->append(toArticulation(e));
            });
      QVERIFY(!taps.isEmpty());
      auto* tap = taps.first();
      imported.style().set(Sid::hammerOnPullOffTappingFontFace, QString("Edwin"));
      imported.style().set(Sid::hammerOnPullOffTappingFontSize, 8.0);
      tap->layout();
      const qreal height = tap->bbox().height();
      imported.style().set(Sid::hammerOnPullOffTappingFontSize, 12.0);
      tap->layout();
      QVERIFY(tap->bbox().height() > height * 1.3);
      QVERIFY(saveScore(&imported, "guitar-tapping-text-style.mscx"));
      std::unique_ptr<MasterScore> reread(readCreatedScore("guitar-tapping-text-style.mscx"));
      QVERIFY(reread);
      QCOMPARE(reread->styleD(Sid::hammerOnPullOffTappingFontSize), 12.0);
      QCOMPARE(reread->styleSt(Sid::hammerOnPullOffTappingFontFace), QString("Edwin"));
      }

void TestElement::testFretParenthesisFontAndGraceSize()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      auto* chord = toChord(imported.firstMeasure()->first(SegmentType::ChordRest)->element(VOICES));
      Note note(&imported);
      note.setParent(chord);
      note.setTrack(chord->track());
      note.setFret(8);
      note.setString(0);
      note.layout();
      auto* left = new Symbol(&imported);
      left->setSym(SymId::noteheadParenthesisLeft);
      note.add(left);
      note.layout2();
      QVERIFY(left->isTabFretParenthesis());
      const StaffType* tab = note.staff()->staffTypeForElement(&note);
      QFont font(tab->fretFont());
      font.setPointSizeF(tab->fretFontSize() * note.magS());
      const QFontMetricsF fm(font, MScore::paintDevice());
      const QRectF expected = fm.tightBoundingRect("(").translated(0.0, tab->fretFontYOffset() * note.magS());
      QCOMPARE(left->bbox(), expected);
      chord->setSmall(true);
      note.layout();
      note.layout2();
      QCOMPARE(left->mag(), note.mag());
      font.setPointSizeF(tab->fretFontSize() * note.magS());
      const QFontMetricsF smallMetrics(font, MScore::paintDevice());
      QCOMPARE(left->bbox(), smallMetrics.tightBoundingRect("(").translated(0.0, tab->fretFontYOffset() * note.magS()));
      const QPointF pos = left->pos();
      note.layout2();
      QCOMPARE(left->pos(), pos);
      QList<Note*> notes;
      imported.scanElements(&notes, [](void* data, Element* e) {
            if (e->isNote() && e->staffIdx() == 0 && toNote(e)->chord()->isGrace())
                  static_cast<QList<Note*>*>(data)->append(toNote(e));
            });
      int checked = 0;
      for (Note* n : notes) {
            QRectF content = n->symBbox(n->noteHead());
            if (n->accidental() && n->accidental()->visible())
                  content |= n->accidental()->bbox().translated(n->accidental()->pos());
            const qreal maxHeight = content.height() + 0.5 * n->spatium() * n->mag();
            for (const Element* e : n->el()) {
                  if (e->isSymbol() && (toSymbol(e)->sym() == SymId::noteheadParenthesisLeft
                        || toSymbol(e)->sym() == SymId::noteheadParenthesisRight)) {
                        QVERIFY(e->height() <= maxHeight + 0.00001);
                        QVERIFY(e->mag() <= n->mag());
                        const qreal drawnHeight = e->symBbox(toSymbol(e)->sym()).height();
                        QVERIFY(qAbs(drawnHeight - e->height()) < 0.00001);
                        ++checked;
                        }
                  }
            }
      QVERIFY(checked > 0);
      }

void TestElement::testGuitarAngularStaffLineClearance()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates4x-expanded.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 0)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      int checked = 0;
      bool changed = false;
      for (GuitarBend* bend : bends) {
            if (bend->singleNote())
                  continue;
            auto* segment = static_cast<GuitarBendSegment*>(bend->frontSegment());
            const auto path = segment->path();
            QCOMPARE(path.elementCount(), 3);
            const Note* note = bend->startNote();
            const StaffType* type = note->staff()->staffTypeForElement(note);
            const qreal spacing = type->lineDistance().val() * segment->spatium();
            const qreal clearance = 0.5 * imported.styleP(Sid::staffLineWidth) + 0.15 * segment->spatium();
            const qreal staffTop = segment->system()->staffYpage(segment->staffIdx());
            for (int i = 0; i < path.elementCount(); ++i) {
                  const qreal y = segment->pagePos().y() + path.elementAt(i).y - staffTop;
                  const int line = qRound(y / spacing);
                  if (line >= 0 && line < type->lines()) {
                        QVERIFY(qAbs(y - line * spacing) >= clearance - 0.00001);
                        ++checked;
                        }
                  }
            segment->setAutoplace(false);
            segment->layout();
            changed |= segment->path() != path;
            segment->setAutoplace(true);
            segment->layout();
            }
      QVERIFY(checked > 0);
      QVERIFY(changed);
      }

void TestElement::testGuitarConnectedReleaseLevels()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates475.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 1)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* release = nullptr;
      GuitarBend* slight = nullptr;
      for (GuitarBend* b : bends) {
            if (b->startNote()->pitch() == 67 && b->endNote()->pitch() == 61)
                  release = b;
            if (b->bendType() == GuitarBendType::SLIGHT_BEND && b->startNote()->pitch() == 61)
                  slight = b;
            }
      QVERIFY(release && slight);
      QCOMPARE(release->totalBendAmount(), 2);
      QCOMPARE(slight->totalBendAmount(), 3);
      QCOMPARE(slight->bendText(), QString::fromUtf8("¾"));
      QCOMPARE(slight->precedingBend(), release);
      // A downward bend below the unbent pitch keeps its signed target.
      GuitarBend* below = nullptr;
      for (GuitarBend* b : bends)
            if (b->startNote()->pitch() == 61 && b->endNote()->pitch() == 53)
                  below = b;
      QVERIFY(below);
      QCOMPARE(below->totalBendAmount(), -14);
      QCOMPARE(below->bendText(), QString::fromUtf8("-3½"));
      QCOMPARE(release->bendText(), QString::fromUtf8("½"));
      const auto* partial = static_cast<const GuitarBendSegment*>(release->frontSegment());
      const auto* next = static_cast<const GuitarBendSegment*>(slight->frontSegment());
      const QPointF expected = partial->pagePos() + partial->arrow().front()
            - QPointF(0.0, 0.5 * release->lineWidth());
      const QPointF actual = next->pagePos() + next->path().pointAtPercent(0.0);
      QVERIFY(QLineF(expected, actual).length() < 0.00001);
      QVERIFY(partial->textRect().top() > partial->arrow().front().y());
      QVERIFY(next->arrow().front().y() < next->path().pointAtPercent(0.0).y());
      // Relayout in reverse dependency order must keep the same join.
      slight->layout();
      const QPointF again = next->pagePos() + next->path().pointAtPercent(0.0);
      QVERIFY(QLineF(actual, again).length() < 0.00001);
      auto* editable = static_cast<GuitarBendSegment*>(slight->frontSegment());
      const QPointF offset(0.3 * editable->spatium(), -0.2 * editable->spatium());
      editable->setOffset(offset);
      slight->layout();
      const QPointF moved = editable->pagePos() + editable->path().pointAtPercent(0.0);
      QVERIFY(QLineF(moved, actual + offset).length() < 0.00001);
      }

void TestElement::testStudioParenthesisGroups()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates475.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      auto check = [](MasterScore* s) {
            Measure* measure = s->firstMeasure();
            for (int i = 0; i < 8; ++i)
                  measure = measure->nextMeasure();
            int standard = 0;
            int tabVisible = 0;
            int tabHidden = 0;
            for (Segment* seg = measure->first(); seg; seg = seg->next()) {
                  if (!seg->isChordRestType())
                        continue;
                  for (int track : { 0, VOICES }) {
                        Element* element = seg->element(track);
                        if (!element || !element->isChord())
                              continue;
                        Chord* chord = toChord(element);
                        std::vector<Chord*> chords { chord };
                        for (Chord* grace : chord->graceNotes())
                              chords.push_back(grace);
                        for (const Chord* c : chords) {
                              for (const Note* note : c->notes()) {
                                    for (const Element* child : note->el()) {
                                          if (!child->isSymbol())
                                                continue;
                                          const Symbol* symbol = toSymbol(child);
                                          if (symbol->sym() != SymId::noteheadParenthesisLeft
                                                && symbol->sym() != SymId::noteheadParenthesisRight)
                                                continue;
                                          if (track == 0) {
                                                QVERIFY(c->isGrace());
                                                ++standard;
                                                }
                                          else {
                                                QVERIFY(symbol->flexibleTabParenthesis());
                                                QVERIFY(!note->fretString().contains('('));
                                                if (note->pitch() == 61) {
                                                      QVERIFY(!symbol->tabParenthesisPath().isEmpty());
                                                      QVERIFY(symbol->bbox().height() > 1.5 * note->bbox().height());
                                                      const QRectF bounds = symbol->bbox().translated(symbol->ipos());
                                                      QVERIFY(bounds.top() < note->bbox().top());
                                                      QVERIFY(bounds.bottom() >= note->bbox().bottom() - 0.00001);
                                                      }
                                                else
                                                      QVERIFY(symbol->tabParenthesisPath().isEmpty());
                                                if (symbol->visible())
                                                      ++tabVisible;
                                                else
                                                      ++tabHidden;
                                                }
                                          }
                                    }
                              }
                        }
                  }
            QCOMPARE(standard, 4);
            QCOMPARE(tabVisible, 2);
            QCOMPARE(tabHidden, 4);
            };
      imported.doLayout();
      check(&imported);
      QVERIFY(saveScore(&imported, "studio-parenthesis-groups.mscx"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("studio-parenthesis-groups.mscx"));
      QVERIFY(copy);
      copy->doLayout();
      check(copy.get());
      }

void TestElement::testChordParenthesisSpan()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      int chords = 0;
      for (Segment* seg = imported.firstMeasure()->first(); seg; seg = seg->next()) {
            if (!seg->isChordRestType())
                  continue;
            for (int track : { 0, VOICES }) {
                  auto* chord = toChord(seg->element(track));
                  QVERIFY(chord);
                  int pairs = 0;
                  for (Note* note : chord->notes()) {
                        for (Element* child : note->el()) {
                              if (!child->isSymbol())
                                    continue;
                              const Symbol* symbol = toSymbol(child);
                              if (symbol->parenthesisNotes().isEmpty())
                                    continue;
                              QVERIFY(!symbol->tabParenthesisPath().isEmpty());
                              QVERIFY(symbol->bbox().height() > note->bbox().height());
                              ++pairs;
                              }
                        }
                  QCOMPARE(pairs, chords < 3 ? 2 : 0);
                  }
            ++chords;
            }
      QCOMPARE(chords, 4);
      auto lastChord = [](MasterScore* s) {
            Chord* result = nullptr;
            for (Segment* seg = s->firstMeasure()->first(); seg; seg = seg->next())
                  if (seg->isChordRestType() && seg->element(0) && seg->element(0)->isChord())
                        result = toChord(seg->element(0));
            return result;
            };
      auto* chord = lastChord(&imported);
      QVERIFY(chord);
      imported.select(chord->notes().front());
      imported.select(chord->notes().back(), SelectType::ADD);
      QVERIFY(chord->notes().front()->accidental());
      imported.select(chord->notes().front()->accidental(), SelectType::ADD);
      imported.startCmd();
      QAction action(nullptr);
      action.setData(QStringLiteral("add-parentheses"));
      EditData ed;
      imported.cmd(&action, ed);
      imported.endCmd();
      imported.doLayout();
      int count = 0;
      for (Element* child : chord->notes().front()->el()) {
            if (child->isSymbol()) {
                  QCOMPARE(toSymbol(child)->parenthesisNotes().size(), 3);
                  ++count;
                  }
            }
      QCOMPARE(count, 2);
      QCOMPARE(int(chord->notes().front()->accidental()->bracket()), int(AccidentalBracket::NONE));
      QVERIFY(saveScore(&imported, "chord-parenthesis-span.mscx"));
      QFile xml("chord-parenthesis-span.mscx");
      QVERIFY(xml.open(QIODevice::ReadOnly));
      const QByteArray savedXml = xml.readAll();
      QVERIFY(savedXml.contains("<NoteParenGroup>"));
      QVERIFY(!savedXml.contains("<parenthesisNotes>"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("chord-parenthesis-span.mscx"));
      QVERIFY(copy);
      copy->doLayout();
      auto* saved = lastChord(copy.get());
      QVERIFY(saved);
      count = 0;
      for (Element* child : saved->notes().front()->el())
            if (child->isSymbol() && toSymbol(child)->parenthesisNotes().size() == 3)
                  ++count;
      QCOMPARE(count, 2);
      }

void TestElement::testLeftHandTappingDisplay()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      auto* chord = toChord(imported.firstMeasure()->first(SegmentType::ChordRest)->element(VOICES));
      QVERIFY(chord);
      auto* tap = new Articulation(&imported);
      tap->setTappingHand(1);
      tap->setParent(chord);
      tap->setTrack(chord->track());
      chord->add(tap);
      imported.doLayout();
      QVERIFY(!tap->tappingPath().isEmpty());
      QVERIFY(tap->tappingPath().elementCount() > 8); // upper and lower TAB half-slurs
      imported.style().set(Sid::lhTappingShowItemsTab, 1);
      imported.style().set(Sid::lhTappingSymbolTab, QString("circledT"));
      tap->layout();
      QVERIFY(tap->tappingPath().isEmpty());
      QVERIFY(tap->bbox().width() > 0.0);
      imported.style().set(Sid::lhTappingShowItemsTab, 2);
      tap->layout();
      QVERIFY(!tap->tappingPath().isEmpty());
      QVERIFY(saveScore(&imported, "left-hand-tapping-style.mscx"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("left-hand-tapping-style.mscx"));
      QVERIFY(copy);
      QCOMPARE(copy->styleI(Sid::lhTappingShowItemsTab), 2);
      QCOMPARE(copy->styleSt(Sid::lhTappingSymbolTab), QString("circledT"));
      }

void TestElement::testHopoPlacementAndFlip()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      QVERIFY(first && next);
      for (int track : { 0, VOICES }) {
            auto* start = toChord(first->element(track));
            auto* end = toChord(next->element(track));
            auto* hopo = new HammerOnPullOff(&imported);
            hopo->setTrack(track);
            hopo->setTrack2(track);
            hopo->setStartElement(start);
            hopo->setEndElement(end);
            hopo->setTick(start->tick());
            hopo->setTick2(end->tick());
            imported.addSpanner(hopo);
            imported.doLayout();
            if (track == VOICES)
                  QVERIFY(hopo->up()); // TAB AUTO goes over the fret chord
            hopo->setProperty(Pid::SLUR_DIRECTION, QVariant::fromValue(Direction::DOWN));
            imported.doLayout();
            QVERIFY(!hopo->segmentsEmpty());
            const SlurSegment* segment = hopo->frontSegment();
            QVERIFY(!segment->hopoTextRect().isEmpty());
            const QRectF text = segment->hopoTextRect().translated(segment->pagePos());
            QVERIFY(text.bottom() < start->upNote()->pagePos().y() + start->upNote()->bbox().top());
            QVERIFY(text.bottom() < end->upNote()->pagePos().y() + end->upNote()->bbox().top());
            const QPointF endpoint = segment->pagePos() + segment->ups(Grip::END).p;
            const Note* bottom = end->downNote();
            QVERIFY(endpoint.y() > bottom->pagePos().y() + bottom->bbox().bottom());
            }
      }

void TestElement::testOverlappingBendDiveAmounts()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Note* start = toChord(first->element(VOICES))->notes().front();
      Note* end = toChord(next->element(VOICES))->notes().front();
      end->setPitch(start->pitch() - 3); // combined interval is -1½ tones
      auto add = [&](GuitarBendType type) {
            auto* bend = new GuitarBend(&imported);
            bend->setBendType(type);
            bend->setStartElement(start);
            bend->setEndElement(end);
            bend->setTick(start->tick());
            bend->setTick2(end->tick());
            bend->setTrack(start->track());
            bend->setTrack2(end->track());
            start->add(bend);
            return bend;
            };
      GuitarBend* bend = add(GuitarBendType::BEND);
      GuitarBend* dive = add(GuitarBendType::DIVE);
      dive->setProperty(Pid::GUITAR_BEND_AMOUNT, -8); // bar contributes -2 tones
      QCOMPARE(bend->overlappingBendOrDive(), static_cast<const GuitarBend*>(dive));
      QCOMPARE(dive->bendAmount(), -8);
      QCOMPARE(bend->bendAmount(), 2); // bend contributes +½ tone
      QCOMPARE(bend->bendText(), QString::fromUtf8("½"));
      QCOMPARE(dive->bendText(), QString("-2"));
      imported.doLayout();
      const auto* segment = static_cast<const GuitarBendSegment*>(bend->frontSegment());
      QVERIFY(segment->arrow().front().y() < segment->path().pointAtPercent(0.0).y());
      QVERIFY(saveScore(&imported, "overlapping-bend-dive.mscx"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("overlapping-bend-dive.mscx"));
      QVERIFY(copy);
      QSet<GuitarBend*> loaded;
      copy->scanElements(&loaded, [](void* data, Element* element) {
            if (element->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(element));
            });
      QCOMPARE(loaded.size(), 2);
      for (GuitarBend* b : loaded)
            QCOMPARE(b->bendAmount(), b->isDive() ? -8 : 2);
      }

void TestElement::testConnectedDiveAndHeldBar()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      std::vector<Note*> notes;
      for (Segment* seg = imported.firstMeasure()->first(); seg; seg = seg->next())
            if (seg->isChordRestType())
                  notes.push_back(toChord(seg->element(VOICES))->upNote());
      QCOMPARE(int(notes.size()), 4);
      notes[1]->setPitch(notes[0]->pitch() - 1);
      notes[2]->setPitch(notes[1]->pitch() - 2);
      notes[3]->setPitch(notes[2]->pitch() + 1);
      auto add = [&](int index, GuitarBendType type) {
            auto* bend = new GuitarBend(&imported);
            bend->setBendType(type);
            bend->setStartElement(notes[index]);
            bend->setEndElement(notes[index + 1]);
            bend->setTick(notes[index]->tick());
            bend->setTick2(notes[index + 1]->tick());
            bend->setTrack(VOICES);
            bend->setTrack2(VOICES);
            notes[index]->add(bend);
            return bend;
            };
      auto* pre = add(0, GuitarBendType::PRE_DIVE);
      auto* dive = add(1, GuitarBendType::DIVE);
      add(2, GuitarBendType::BEND);
      imported.doLayout();
      QCOMPARE(pre->diveLevelIndex(), 0);
      QCOMPARE(dive->diveLevelIndex(), 1);
      QCOMPARE(dive->holdEndNote(), static_cast<const Note*>(notes[3]));
      const auto* p = static_cast<const GuitarBendSegment*>(pre->frontSegment());
      const auto* d = static_cast<const GuitarBendSegment*>(dive->frontSegment());
      const QPointF corner = p->path().pointAtPercent(1.0) + p->pagePos();
      const QPointF join = d->path().pointAtPercent(0.0) + d->pagePos();
      QVERIFY(QLineF(corner, join).length() < 0.00001);
      const Note* bottom = notes[1]->chord()->downNote();
      QVERIFY((p->path().pointAtPercent(0.0) + p->pagePos()).y()
            > bottom->pagePos().y() + bottom->bbox().bottom());
      QVERIFY(!d->holdPath().isEmpty());
      const QPointF heldFrom = d->holdPath().pointAtPercent(0.0);
      QVERIFY(QLineF(heldFrom, d->path().pointAtPercent(1.0)).length() < 0.00001);
      QVERIFY(d->path().pointAtPercent(1.0).y() + d->pagePos().y() > corner.y());
      }

void TestElement::testReleaseLabelPreservesFretAlignment()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Note* start = toChord(first->element(VOICES))->upNote();
      Note* end = toChord(next->element(VOICES))->upNote();
      end->setPitch(start->pitch() - 2);
      auto* bend = new GuitarBend(&imported);
      bend->setStartElement(start);
      bend->setEndElement(end);
      bend->setTick(start->tick());
      bend->setTick2(end->tick());
      bend->setTrack(VOICES);
      bend->setTrack2(VOICES);
      start->add(bend);
      imported.doLayout();
      auto* segment = static_cast<GuitarBendSegment*>(bend->frontSegment());
      const qreal target = end->pagePos().y() + end->bbox().center().y();
      const qreal current = segment->textRect().center().y() + segment->pagePos().y();
      segment->setUserOff2(QPointF(0.0, target - current));
      const QPointF manual = end->offset();
      const QRectF fretBefore = end->bbox();
      imported.doLayout();
      QCOMPARE(end->offset(), manual);
      QCOMPARE(end->bbox(), fretBefore);
      imported.doLayout();
      QCOMPARE(end->bbox(), fretBefore);
      }

void TestElement::testChordBendStagger()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Chord* from = toChord(first->element(VOICES));
      Chord* to = toChord(next->element(VOICES));
      QVector<GuitarBend*> bends;
      for (int i = 0; i < 3; ++i) {
            Note* start = from->notes().at(i);
            Note* end = to->notes().at(i);
            end->setPitch(start->pitch() - 2);
            auto* bend = new GuitarBend(&imported);
            bend->setStartElement(start);
            bend->setEndElement(end);
            bend->setTick(start->tick());
            bend->setTick2(end->tick());
            bend->setTrack(VOICES);
            bend->setTrack2(VOICES);
            start->add(bend);
            bends.append(bend);
            }
      imported.doLayout();
      auto* part = static_cast<GuitarBendSegment*>(bends.front()->frontSegment());
      const QPointF original = part->arrow().front() + part->pagePos();
      const QRectF fret = to->notes().front()->bbox();
      const QPointF fretOffset = to->notes().front()->offset();
      // The lowest pitch is on the lowest string; changing its amount
      // triggers the Studio stagger relative to the higher-string bend.
      Note* end = bends.front()->endNote();
      end->setPitch(end->pitch() - 2);
      imported.doLayout();
      const QPointF moved = part->arrow().front() + part->pagePos();
      QVERIFY(qAbs(moved.x() - original.x() - part->spatium()) < 0.00001);
      QVERIFY(qAbs(moved.y() - original.y() - part->spatium()) < 0.00001);
      QCOMPARE(end->bbox(), fret);
      QCOMPARE(end->offset(), fretOffset);
      imported.doLayout();
      QVERIFY(QLineF(moved, part->arrow().front() + part->pagePos()).length() < 0.00001);
      // Equal amounts return to the unstaggered endpoint without drift.
      end->setPitch(end->pitch() + 2);
      imported.doLayout();
      QVERIFY(QLineF(original, part->arrow().front() + part->pagePos()).length() < 0.00001);
      }

void TestElement::testAccidentalInsideNoteParentheses()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates475-parentheses.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QList<Accidental*> accidentals;
      imported.scanElements(&accidentals, [](void* data, Element* e) {
            if (e->isAccidental() && e->staffIdx() == 0
                  && toAccidental(e)->bracket() == AccidentalBracket::PARENTHESIS)
                  static_cast<QList<Accidental*>*>(data)->append(toAccidental(e));
            });
      QVERIFY(!accidentals.isEmpty());
      Accidental* accidental = accidentals.first();
      Note* note = accidental->note();
      QVERIFY(note);
      accidental->layout();
      QCOMPARE(accidental->bbox(), accidental->symBbox(accidental->symbol()));
      QCOMPARE(int(accidental->bracket()), int(AccidentalBracket::PARENTHESIS));
      // Removing the outer pair restores the accidental's stored pair.
      QList<Element*> parentheses;
      for (Element* child : note->el())
            if (child->isSymbol() && (toSymbol(child)->sym() == SymId::noteheadParenthesisLeft
                  || toSymbol(child)->sym() == SymId::noteheadParenthesisRight))
                  parentheses.append(child);
      QVERIFY(!parentheses.isEmpty());
      for (Element* child : parentheses) {
            note->remove(child);
            delete child;
            }
      accidental->layout();
      QVERIFY(accidental->bbox().width() > accidental->symBbox(accidental->symbol()).width());
      QCOMPARE(int(accidental->bracket()), int(AccidentalBracket::PARENTHESIS));
      }

void TestElement::testBendClearsChordParentheses()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Chord* from = toChord(first->element(0));
      Chord* to = toChord(next->element(0));
      QVERIFY(from && to && from->notes().size() == 3 && to->notes().size() == 3);
      // Put the middle attachment within the staff, so ledger-line ink does
      // not already determine its horizontal clearance.
      for (Chord* chord : {from, to}) {
            chord->setStemDirection(Direction::DOWN);
            for (Note* note : chord->notes())
                  note->setPitch(note->pitch() - 12);
            }
      int pairs = 0;
      for (Note* owner : to->notes()) {
            for (Element* child : owner->el()) {
                  if (child->isSymbol() && !toSymbol(child)->parenthesisNotes().isEmpty()) {
                        toSymbol(child)->setParenthesisNotes({0, 1, 2});
                        ++pairs;
                        }
                  }
            }
      QCOMPARE(pairs, 2);
      auto* created = new GuitarBend(&imported);
      created->setStartElement(from->notes()[1]);
      created->setEndElement(to->notes()[1]);
      created->setTick(from->tick());
      created->setTick2(to->tick());
      created->setTrack(0);
      created->setTrack2(0);
      created->setProperty(Pid::DIRECTION, QVariant::fromValue(Direction::UP));
      from->notes()[1]->add(created);
      imported.doLayout();
      QSet<GuitarBend*> bends {created};
      bool checked = false;
      for (GuitarBend* bend : bends) {
            Note* end = bend->endNote();
            if (!end || bend->isDive() || end->chord()->notes().size() != 3
                  || end != end->chord()->notes()[1])
                  continue;
            QList<Symbol*> parentheses;
            for (Note* owner : end->chord()->notes())
                  for (Element* child : owner->el())
                        if (child->isSymbol() && child->visible() && toSymbol(child)->parenthesisNotes().size() == 3)
                              parentheses.append(toSymbol(child));
            if (parentheses.isEmpty())
                  continue;
            auto* part = static_cast<GuitarBendSegment*>(bend->backSegment());
            const QPointF cleared = part->path().pointAtPercent(1.0) + part->pagePos();
            for (Symbol* symbol : parentheses) {
                  const QPainterPath& path = symbol->tabParenthesisPath();
                  QVERIFY(path.elementCount() >= 7);
                  // A normal three-note group has a slim tapered midpoint.
                  const qreal midWidth = 0.75 * qAbs(path.elementAt(1).x - path.elementAt(4).x);
                  QVERIFY(midWidth < 0.18 * symbol->spatium());
                  // Layout must include the outline around the tapered fill.
                  QVERIFY(symbol->bbox().left() < path.boundingRect().left());
                  QVERIFY(symbol->bbox().right() > path.boundingRect().right());
                  QVERIFY(symbol->bbox().top() < path.boundingRect().top());
                  QVERIFY(symbol->bbox().bottom() > path.boundingRect().bottom());
                  symbol->setVisible(false);
                  }
            bend->layout();
            const QPointF plain = part->path().pointAtPercent(1.0) + part->pagePos();
            QVERIFY(plain.x() > cleared.x());
            for (Symbol* symbol : parentheses)
                  symbol->setVisible(true);
            bend->layout();
            QVERIFY(QLineF(cleared, part->path().pointAtPercent(1.0) + part->pagePos()).length() < 0.00001);
            checked = true;
            break;
            }
      QVERIFY(checked);
      }

void TestElement::testBendConditionalClearance()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/chord-parenthesis-groups.mscx", true),
            Score::FileError::FILE_NO_ERROR);
      Segment* first = imported.firstMeasure()->first(SegmentType::ChordRest);
      Segment* next = first->next(SegmentType::ChordRest);
      Chord* from = toChord(first->element(0));
      Chord* to = toChord(next->element(0));
      // Keep the tested upper-note attachment away from ledger lines.
      // Otherwise their left edge can dominate with or without parentheses.
      for (Chord* chord : {from, to}) {
            for (Note* note : chord->notes()) {
                  note->setPitch(note->pitch() - 12);
                  // This case isolates parentheses. The lower D-flat's
                  // accidental can otherwise be wider at attachment height.
                  if (note->accidental())
                        note->accidental()->setVisible(false);
                  }
            }
      from->setStemDirection(Direction::DOWN);
      to->setStemDirection(Direction::DOWN);
      auto* bend = new GuitarBend(&imported);
      bend->setStartElement(from->upNote());
      bend->setEndElement(to->upNote());
      bend->setTick(from->tick());
      bend->setTick2(to->tick());
      bend->setTrack(0);
      bend->setTrack2(0);
      bend->setProperty(Pid::DIRECTION, QVariant::fromValue(Direction::UP));
      from->upNote()->add(bend);
      imported.doLayout();
      QList<Symbol*> parentheses;
      for (Note* owner : to->notes())
            for (Element* child : owner->el())
                  if (child->isSymbol() && child->visible() && !toSymbol(child)->parenthesisNotes().isEmpty())
                        parentheses.append(toSymbol(child));
      QVERIFY(!parentheses.isEmpty());
      auto* part = static_cast<GuitarBendSegment*>(bend->backSegment());
      auto endpoint = [&]() { return part->path().pointAtPercent(1.0) + part->pagePos(); };
      const QPointF outside = endpoint();
      for (Symbol* symbol : parentheses)
            symbol->setVisible(false);
      bend->layout();
      QVERIFY(QLineF(outside, endpoint()).length() < 0.00001);
      // On the other side the same highest note is an inner attachment:
      // Studio's adjustEnd is now true, so its pair must affect the reach.
      for (Symbol* symbol : parentheses)
            symbol->setVisible(true);
      bend->setProperty(Pid::DIRECTION, QVariant::fromValue(Direction::DOWN));
      bend->layout();
      const QPointF cleared = endpoint();
      for (Symbol* symbol : parentheses)
            symbol->setVisible(false);
      bend->layout();
      QVERIFY2(endpoint().x() > cleared.x(), qPrintable(QString("Parenthesis reach: plain=(%1,%2), cleared=(%3,%4)")
            .arg(endpoint().x()).arg(endpoint().y()).arg(cleared.x()).arg(cleared.y())));
      for (Symbol* symbol : parentheses)
            symbol->setVisible(true);
      bend->layout();
      QVERIFY(QLineF(cleared, endpoint()).length() < 0.00001);
      }

void TestElement::testTabParenthesesUseRelativeSlightBendShape()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitarupdates475-parentheses.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend() && e->staffIdx() == 1)
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      bool checked = false;
      for (GuitarBend* bend : bends) {
            if (bend->bendType() != GuitarBendType::SLIGHT_BEND || !bend->startNote())
                  continue;
            Symbol* parenthesis = nullptr;
            for (Element* child : bend->startNote()->el())
                  if (child->isSymbol() && toSymbol(child)->flexibleTabParenthesis())
                        parenthesis = toSymbol(child);
            if (!parenthesis)
                  continue;
            const auto* segment = static_cast<const GuitarBendSegment*>(bend->frontSegment());
            const qreal parenTop = parenthesis->bbox().top() + parenthesis->pagePos().y();
            const qreal labelTop = segment->textRect().top() + segment->pagePos().y();
            const Note* note = bend->startNote();
            const qreal fretTop = note->bbox().top() + note->pagePos().y();
            // The pair reaches above the fret, but not to the annotation's
            // final position above the staff.
            QVERIFY(parenTop < fretTop);
            QVERIFY(parenTop > labelTop);
            checked = true;
            break;
            }
      QVERIFY(checked);
      }

void TestElement::testHiddenBendArrivalFretting()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitar-bend-fret-conflict475.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      auto check = [&]() {
            Measure* measure = imported.firstMeasure();
            for (int i = 1; i < 9 && measure; ++i)
                  measure = measure->nextMeasure();
            QVERIFY(measure);
            bool checked = false;
            for (Segment* segment = measure->first(SegmentType::ChordRest); segment;
                  segment = segment->next(SegmentType::ChordRest)) {
                  Element* element = segment->element(VOICES);
                  if (!element || !element->isChord() || toChord(element)->notes().size() != 3)
                        continue;
                  Note* ordinary = nullptr;
                  Note* arrival = nullptr;
                  for (Note* note : toChord(element)->notes()) {
                        if (note->pitch() == 61)
                              ordinary = note;
                        if (note->pitch() == 53)
                              arrival = note;
                        }
                  QVERIFY(ordinary && arrival);
                  QCOMPARE(ordinary->string(), 1);
                  QCOMPARE(ordinary->fret(), 2);
                  QVERIFY(!ordinary->fretConflict());
                  QVERIFY(GuitarBend::hideTabFret(arrival));
                  QCOMPARE(arrival->string(), 1);
                  QCOMPARE(arrival->fret(), 1);
                  QVERIFY(!arrival->fretConflict());
                  checked = true;
                  }
            QVERIFY(checked);
            };
      imported.doLayout();
      check();
      imported.doLayout();
      check();
      QVERIFY(saveScore(&imported, "guitar-bend-fret-conflict.mscx"));
      std::unique_ptr<MasterScore> copy(readCreatedScore("guitar-bend-fret-conflict.mscx"));
      QVERIFY(copy);
      copy->doLayout();
      // The ordinary C-sharp must also retain fret 2 after serialization.
      Measure* ninth = copy->firstMeasure();
      for (int i = 1; i < 9 && ninth; ++i)
            ninth = ninth->nextMeasure();
      QVERIFY(ninth);
      bool checked = false;
      for (Segment* segment = ninth->first(SegmentType::ChordRest); segment;
            segment = segment->next(SegmentType::ChordRest)) {
            Element* element = segment->element(VOICES);
            if (!element || !element->isChord() || toChord(element)->notes().size() != 3)
                  continue;
            for (const Note* note : toChord(element)->notes())
                  if (note->pitch() == 61) {
                        QCOMPARE(note->string(), 1);
                        QCOMPARE(note->fret(), 2);
                        checked = true;
                        }
            }
      QVERIFY(checked);
      }

void TestElement::testGuitarBendWidthStyles()
      {
      MasterScore imported(score->style());
      QCOMPARE(imported.loadMsc(root + "/libmscore/element/guitar-bend-fret-conflict475.mscz", true),
            Score::FileError::FILE_NO_ERROR);
      imported.doLayout();
      QSet<GuitarBend*> bends;
      imported.scanElements(&bends, [](void* data, Element* e) {
            if (e->isGuitarBend())
                  static_cast<QSet<GuitarBend*>*>(data)->insert(toGuitarBend(e));
            });
      GuitarBend* tabBend = nullptr;
      bool notation = false;
      bool dive = false;
      for (GuitarBend* bend : bends) {
            const bool tab = bend->staff()->isTabStaff(bend->tick());
            const qreal expected = imported.spatium() * (tab ? 0.15 : 0.13);
            QVERIFY(qAbs(bend->lineWidth() - expected) < 0.00001);
            QCOMPARE(bend->getProperty(Pid::LINE_WIDTH).toDouble(), bend->lineWidth());
            if (tab && !bend->isDive())
                  tabBend = bend;
            notation |= !tab;
            dive |= bend->isDive();
            }
      QVERIFY(tabBend && notation && dive);
      imported.style().set(Sid::guitarBendLineWidthTab, Spatium(0.18));
      imported.doLayout();
      QVERIFY(qAbs(tabBend->lineWidth() - 0.18 * imported.spatium()) < 0.00001);
      tabBend->setProperty(Pid::LINE_WIDTH, 0.22 * imported.spatium());
      tabBend->setPropertyFlags(Pid::LINE_WIDTH, PropertyFlags::UNSTYLED);
      imported.style().set(Sid::guitarBendLineWidthTab, Spatium(0.19));
      imported.doLayout();
      QVERIFY(qAbs(tabBend->lineWidth() - 0.22 * imported.spatium()) < 0.00001);
      tabBend->resetProperty(Pid::LINE_WIDTH);
      imported.doLayout();
      QVERIFY(qAbs(tabBend->lineWidth() - 0.19 * imported.spatium()) < 0.00001);
      QCOMPARE(int(tabBend->propertyFlags(Pid::LINE_WIDTH)), int(PropertyFlags::STYLED));
      }

void TestElement::testWhammyBarDefaults()
      {
      WhammyBar bar(score);
      QCOMPARE(int(bar.endHookType()), int(HookType::HOOK_90T));
      QCOMPARE(bar.endHookHeight().val(), 0.6);
      std::unique_ptr<WhammyBar> copy(static_cast<WhammyBar*>(writeReadElement(&bar)));
      QVERIFY(copy);
      QCOMPARE(int(copy->endHookType()), int(HookType::HOOK_90T));
      QCOMPARE(copy->endHookHeight().val(), 0.6);
      copy->setProperty(Pid::END_HOOK_HEIGHT, QVariant::fromValue(Spatium(1.1)));
      std::unique_ptr<WhammyBar> custom(static_cast<WhammyBar*>(writeReadElement(copy.get())));
      QVERIFY(custom);
      QCOMPARE(custom->endHookHeight().val(), 1.1);
      }

QTEST_MAIN(TestElement)

#include "tst_element.moc"

