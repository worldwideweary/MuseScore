// MuseScore chord presentation UI regression tests. GPL-2.0-or-later.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "mscore/editstyle.h"
#include "mscore/icons.h"
#include "mscore/musescore.h"
#include "libmscore/score.h"
#include "libmscore/harmony.h"
#include "libmscore/sym.h"
#include "libmscore/undo.h"

using namespace Ms;

class TestChordPresentationUi : public QObject {
      Q_OBJECT
   private slots:
      void initTestCase()
            {
            qputenv("QML_DISABLE_DISK_CACHE", "true");
            MScore::noGui = true;
            MScore::testMode = true;
            initMuseScoreResources();
            QStringList args;
            MuseScore::init(args);
            if (!icons[int(Icons::goNext_ICON)])
                  genIcons();
            }
      void presetsAndCancel()
            {
            MasterScore score(MScore::defaultStyle());
            EditStyle dialog(&score, nullptr);
            dialog.show();
            qApp->processEvents();
            // Opening the dialog must not convert a legacy score.
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 0);
            auto standard = dialog.findChild<QRadioButton*>("chordsStandard");
            auto jazz = dialog.findChild<QRadioButton*>("chordsJazz");
            auto legacy = dialog.findChild<QRadioButton*>("chordsLegacy");
            QVERIFY(standard && jazz && legacy);
            QVERIFY(legacy->isChecked());
            standard->click();
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 1);
            QCOMPARE(score.styleSt(Sid::chordSymPosition), QString("center"));
            QVERIFY(!dialog.findChild<QDoubleSpinBox*>("maxChordShiftAbove")->isEnabled());
            auto stack = dialog.findChild<QCheckBox*>("verticallyStackModifiers");
            QVERIFY(stack);
            stack->click();
            QVERIFY(score.styleB(Sid::verticallyStackModifiers));
            jazz->click();
            QCOMPARE(score.styleSt(Sid::chordSymbolAFontFace), QString("MuseJazz Text"));
            QVERIFY(score.styleB(Sid::harmonyParenUseSmuflSym));
            legacy->click();
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 0);
            QVERIFY(dialog.findChild<QDoubleSpinBox*>("maxChordShiftAbove")->isEnabled());
            QVERIFY(!score.styleB(Sid::verticallyStackModifiers));
            dialog.reject();
            QCOMPARE(score.styleSt(Sid::chordStyle), QString("std"));
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 0);
            }
      void musicFontSwitchingAndCancel()
            {
            MasterScore score(MScore::defaultStyle());
            const QString initial = score.styleSt(Sid::musicalSymbolFont);
            EditStyle dialog(&score, nullptr);
            dialog.show();
            qApp->processEvents();
            auto fonts = dialog.findChild<QComboBox*>("musicalSymbolFontComboBox");
            QVERIFY(fonts);
            for (const QString& family : { QString("Petaluma"), QString("MuseJazz"),
                        QString("Gonville"), QString("MuseJazz"), QString("Petaluma"), QString("Gonville") }) {
                  const int index = fonts->findData(family);
                  QVERIFY(index >= 0);
                  fonts->setCurrentIndex(index);
                  QCOMPARE(score.styleSt(Sid::musicalSymbolFont), family);
                  QCOMPARE(score.scoreFont()->name(), family);
                  }
            dialog.reject();
            QCOMPARE(score.styleSt(Sid::musicalSymbolFont), initial);
            QCOMPARE(score.scoreFont()->name(), initial);
            }
      void refreshDoesNotQueueEdits()
            {
            MasterScore score(MScore::defaultStyle());
            score.style().set(Sid::figuredBassFontSize, 13.25);
            const MStyle initial = score.style();
            EditStyle dialog(&score, nullptr);
            dialog.show();
            qApp->processEvents();
            auto countStyleCommands = [&]() {
                  int count = 0;
                  if (const UndoMacro* macro = score.undoStack()->current()) {
                        for (const UndoCommand* cmd : macro->commands())
                              if (!qstrcmp(cmd->name(), "ChangeStyleVal"))
                                    ++count;
                        }
                  return count;
                  };
            QCOMPARE(countStyleCommands(), 0);
            auto styles = dialog.findChild<QListWidget*>("textStyles");
            QVERIFY(styles);
            for (int row = 0; row < styles->count(); ++row) {
                  styles->setCurrentRow(row);
                  qApp->processEvents();
                  QCOMPARE(countStyleCommands(), 0);
                  }
            for (int i = int(Sid::NOSTYLE) + 1; i < int(Sid::STYLES); ++i)
                  QCOMPARE(score.styleV(Sid(i)), initial.value(Sid(i)));
            auto optimize = dialog.findChild<QCheckBox*>("optimizeStyleCheckbox");
            auto fonts = dialog.findChild<QComboBox*>("musicalSymbolFontComboBox");
            QVERIFY(optimize && fonts);
            optimize->setChecked(false);
            const int index = fonts->findData(QString("Petaluma"));
            QVERIFY(index >= 0);
            fonts->setCurrentIndex(index);
            QCOMPARE(countStyleCommands(), 1);
            // Even a redundant change notification must not create an undo item.
            QVERIFY(QMetaObject::invokeMethod(&dialog, "valueChanged", Qt::DirectConnection,
                  Q_ARG(int, int(Sid::musicalSymbolFont))));
            QCOMPARE(countStyleCommands(), 1);
            dialog.reject();
            for (int i = int(Sid::NOSTYLE) + 1; i < int(Sid::STYLES); ++i)
                  QCOMPARE(score.styleV(Sid(i)), initial.value(Sid(i)));
            }
      void slashGapAndCancel()
            {
            MasterScore score(MScore::defaultStyle());
            EditStyle dialog(&score, nullptr);
            dialog.show();
            qApp->processEvents();
            auto gap = dialog.findChild<QDoubleSpinBox*>("chordBassNoteGap");
            auto reset = dialog.findChild<QToolButton*>("resetchordBassNoteGap");
            QVERIFY(gap && reset);
            QCOMPARE(gap->value(), 10.0);
            gap->setValue(18.0);
            QCOMPARE(score.styleD(Sid::chordBassNoteGap), 0.18);
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 1);
            reset->click();
            QCOMPARE(gap->value(), 10.0);
            QCOMPARE(score.styleD(Sid::chordBassNoteGap), 0.1);
            gap->setValue(6.0);
            dialog.reject();
            QCOMPARE(score.styleD(Sid::chordBassNoteGap), 0.1);
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 0);
            }
      void firstPresentationEdit()
            {
            MasterScore score(MScore::defaultStyle());
            score.style().set(Sid::chordExtensionAdjust, -2.0);
            Harmony reference(&score);
            const qreal cap = QFontMetricsF(reference.font(), MScore::paintDevice()).capHeight();
            const qreal expectedOffset = -2.0 * .2 * score.spatium() * reference.magS() / cap;
            EditStyle dialog(&score, nullptr);
            dialog.show();
            qApp->processEvents();
            auto stack = dialog.findChild<QCheckBox*>("verticallyStackModifiers");
            QVERIFY(stack);
            stack->click();
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 1);
            QVERIFY(score.styleB(Sid::verticallyStackModifiers));
            QVERIFY(qAbs(score.styleD(Sid::chordExtensionAdjust) - expectedOffset) < 1e-6);
            const QString preview = QString::fromLocal8Bit(qgetenv("MSCORE_CHORD_STYLE_PREVIEW"));
            if (!preview.isEmpty()) {
                  QWidget* page = dialog.findChild<QWidget*>("scrollArea_chordSymbols_contents");
                  QVERIFY(page);
                  QVERIFY(page->grab().save(preview));
                  }
            dialog.reject();
            QCOMPARE(score.styleI(Sid::chordSymbolLayoutVersion), 0);
            QCOMPARE(score.styleD(Sid::chordExtensionAdjust), -2.0);
            }
      void cleanupTestCase()
            {
            qApp->processEvents();
            delete Ms::mscore;
            Ms::mscore = nullptr;
            }
      };

QTEST_MAIN(TestChordPresentationUi)
#include "tst_chordpresentationui.moc"
