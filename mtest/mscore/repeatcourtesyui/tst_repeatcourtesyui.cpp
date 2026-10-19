// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual Style dialog bindings and their undo/cancel behavior.
#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "mscore/editstyle.h"
#include "mscore/icons.h"
#include "mscore/shortcut.h"
#include "mscore/inspector/scaleSelect.h"
#include "libmscore/staff.h"
#include "libmscore/measure.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"

using namespace Ms;

class TestRepeatCourtesyUi : public QObject, public MTest {
      Q_OBJECT
   private slots:
      void initTestCase() {
            initMuseScoreResources();
            initMTest();
            iconPath = ":/data/icons/";
            genIcons();
            QVERIFY(!icons[int(Icons::reset_ICON)]->pixmap(16).isNull());
            Shortcut::init();
            }
      void timeSignatureDrawingControlsAndCancel() {
            std::unique_ptr<MasterScore> s(new MasterScore(MScore::baseStyle()));
            QCOMPARE(s->loadMsc(root + "/libmscore/repeatcourtesy/studio-changes-inside-repeats.mscx", true), Score::FileError::FILE_NO_ERROR);
            s->doLayout(); QCOMPARE(s->nstaves(), 3);
            std::vector<QVariant> initial;
            for (int i = int(Sid::timeSigPlacement); i <= int(Sid::timeSigAcrossY); ++i) initial.push_back(s->styleV(Sid(i)));
            const bool initialStaff = s->staff(2)->timeSigSystemObject();
            EditStyle dialog(s.get(), nullptr); dialog.show();
            QWidget* page = dialog.findChild<QWidget*>("PageTimeSignatures"); QVERIFY(page);
            auto stack = qobject_cast<QStackedWidget*>(page->parentWidget()); QVERIFY(stack);
            dialog.setPage(stack->indexOf(page));
            QTRY_VERIFY(page->isVisible()); QTRY_VERIFY(!page->visibleRegion().isEmpty());
            auto placement = dialog.findChild<QComboBox*>("timeSigPlacement"); QVERIFY(placement);
            auto placementReset = dialog.findChild<QToolButton*>("resetTimeSigPlacement"); QVERIFY(placementReset);
            auto placementLayout = dialog.findChild<QGridLayout*>("timeSigPlacementForm"); QVERIFY(placementLayout);
            QVERIFY(placementLayout->itemAtPosition(0, 2));
            QCOMPARE(placementLayout->itemAtPosition(0, 2)->widget(), placementReset);
            QVERIFY(!placement->geometry().intersects(placementReset->geometry()));
            const char* modes[] = {"Normal", "Above", "Across"};
            const Sid first[] = {Sid::timeSigNormalStyle, Sid::timeSigAboveStyle, Sid::timeSigAcrossStyle};
            for (int mode = 0; mode < 3; ++mode) {
                  placement->setCurrentIndex(mode); QCOMPARE(s->styleI(Sid::timeSigPlacement), mode);
                  QString prefix = QString("timeSig") + modes[mode];
                  auto numeral = dialog.findChild<QComboBox*>(prefix + "Style"); QVERIFY(numeral);
                  auto scale = dialog.findChild<ScaleSelect*>(prefix + "Scale"); QVERIFY(scale);
                  auto lock = dialog.findChild<QCheckBox*>(prefix + "ScaleLock"); QVERIFY(lock);
                  auto gap = dialog.findChild<QDoubleSpinBox*>(prefix + "NumDist"); QVERIFY(gap);
                  auto y = dialog.findChild<QDoubleSpinBox*>(prefix + "Y"); QVERIFY(y);
                  QTRY_VERIFY(numeral->isVisible());
                  auto form = dialog.findChild<QGridLayout*>(prefix + "Form"); QVERIFY(form);
                  for (const char* field : {"Style", "Scale", "NumDist", "Y"}) {
                        auto reset = dialog.findChild<QToolButton*>("resetTimeSig" + QString(modes[mode]) + field); QVERIFY(reset);
                        auto control = dialog.findChild<QWidget*>(prefix + field); QVERIFY(control);
                        QVERIFY(form->indexOf(reset) >= 0);
                        QTRY_VERIFY(reset->isVisible());
                        QVERIFY(!control->geometry().intersects(reset->geometry()));
                        }
                  numeral->setCurrentIndex(2); QCOMPARE(s->styleI(first[mode]), 2);
                  if (lock->isChecked()) lock->click();
                  QVERIFY(!s->styleB(Sid(int(first[mode]) + 2)));
                  scale->xVal->setValue(1.25 + mode); scale->yVal->setValue(2.25 + mode);
                  QCOMPARE(s->styleV(Sid(int(first[mode]) + 1)).toSizeF(), QSizeF(1.25 + mode, 2.25 + mode));
                  lock->click(); QVERIFY(s->styleB(Sid(int(first[mode]) + 2)));
                  QCOMPARE(s->styleV(Sid(int(first[mode]) + 1)).toSizeF(), QSizeF(1.25 + mode, 1.25 + mode));
                  lock->click();
                  gap->setValue(0.35 + mode * 0.1);
                  QVERIFY(qAbs(s->styleS(Sid(int(first[mode]) + 3)).val() - gap->value()) < 0.001);
                  QVERIFY(y->isEnabled()); y->setValue(0.75);
                  QCOMPARE(s->styleS(Sid(int(first[mode]) + 4)).val(), 0.75);
                  auto reset = dialog.findChild<QPushButton*>(prefix + "ResetAppearance"); QVERIFY(reset && reset->isEnabled());
                  reset->click();
                  for (int i = 0; i < 5; ++i) QVERIFY(s->style().isDefault(Sid(int(first[mode]) + i)));
                  // Leave non-default values so Cancel must unwind all field types.
                  numeral->setCurrentIndex(2); if (lock->isChecked()) lock->click();
                  scale->xVal->setValue(1.4 + mode); scale->yVal->setValue(1.8 + mode);
                  gap->setValue(0.45); y->setValue(0.65);
                  if (qEnvironmentVariableIsSet("TIMESIG_STYLE_PREVIEW_DIR")) {
                        QDir directory(QString::fromLocal8Bit(qgetenv("TIMESIG_STYLE_PREVIEW_DIR")));
                        QVERIFY(directory.mkpath(".")); QApplication::processEvents();
                        QVERIFY(dialog.grab().save(directory.filePath(QString("time-signatures-") + modes[mode] + ".png")));
                        }
                  }
            placement->setCurrentIndex(1);
            auto center = dialog.findChild<QCheckBox*>("timeSigCenterOnBarline"); QVERIFY(center);
            auto centeredMargin = dialog.findChild<QComboBox*>("timeSigVSMarginCentered"); QVERIFY(centeredMargin);
            auto leftMargin = dialog.findChild<QComboBox*>("timeSigVSMarginNonCentered"); QVERIFY(leftMargin);
            QVERIFY(center->isChecked()); centeredMargin->setCurrentIndex(1);
            QCOMPARE(s->styleI(Sid::timeSigVSMarginCentered), 1);
            center->click(); QVERIFY(!s->styleB(Sid::timeSigCenterOnBarline));
            QVERIFY(!centeredMargin->isEnabled()); QVERIFY(leftMargin->isEnabled());
            leftMargin->setCurrentIndex(2); QCOMPARE(s->styleI(Sid::timeSigVSMarginNonCentered), 2);
            placement->setCurrentIndex(2);
            auto across = dialog.findChild<QCheckBox*>("timeSigCenterAcrossStaveGroup"); QVERIFY(across);
            QVERIFY(!across->isChecked()); across->click(); QVERIFY(s->styleB(Sid::timeSigCenterAcrossStaveGroup));
            QVERIFY(!dialog.findChild<QDoubleSpinBox*>("timeSigAcrossY")->isEnabled());
            auto staves = dialog.findChild<QListWidget*>("timeSigDisplayStaves"); QVERIFY(staves);
            QCOMPARE(staves->count(), 2); staves->item(1)->setCheckState(Qt::Checked);
            QVERIFY(s->staff(2)->timeSigSystemObject());
            auto top = dialog.findChild<QPushButton*>("timeSigUseTopStaff"); QVERIFY(top); top->click();
            QVERIFY(!s->staff(2)->timeSigSystemObject()); staves->item(1)->setCheckState(Qt::Checked);
            s->doLayout();
            Segment* meter = s->firstMeasure()->findSegmentR(SegmentType::TimeSig, Fraction(0,1)); QVERIFY(meter);
            QVERIFY(!meter->element(0)->bbox().isEmpty()); QVERIFY(meter->element(VOICES)->bbox().isEmpty());
            QVERIFY(!meter->element(2 * VOICES)->bbox().isEmpty());
            dialog.reject(); s->doLayout();
            for (int i = int(Sid::timeSigPlacement); i <= int(Sid::timeSigAcrossY); ++i)
                  QCOMPARE(s->styleV(Sid(i)), initial[size_t(i - int(Sid::timeSigPlacement))]);
            QCOMPARE(s->staff(2)->timeSigSystemObject(), initialStaff);
            }
      void styleBindingsAndCancel() {
            std::unique_ptr<MasterScore> s(readScore("libmscore/repeatcourtesy/matching.mscx")); QVERIFY(s);
            EditStyle dialog(s.get(), nullptr);
            dialog.show();
            QWidget* page = dialog.findChild<QWidget*>("PageClefs"); QVERIFY(page);
            auto stack = qobject_cast<QStackedWidget*>(page->parentWidget()); QVERIFY(stack);
            dialog.setPage(stack->indexOf(page));
            // The small-screen scroll wrapper is inserted during showEvent.
            // Wait for its splitter layout before interacting or capturing it.
            QTRY_VERIFY(page->isVisible());
            QTRY_VERIFY(page->visibleRegion().boundingRect().width() > 0);
            QTRY_VERIFY(page->visibleRegion().boundingRect().height() > 0);
            for (int index = int(Sid::changesBeforeBarlineRepeats); index <= int(Sid::smallParens); ++index) {
                  const Sid sid = Sid(index);
                  auto box = dialog.findChild<QCheckBox*>(MStyle::valueName(sid)); QVERIFY(box);
                  QVERIFY(box->isVisible());
                  const bool initial = s->styleB(sid);
                  QCOMPARE(box->isChecked(), initial);
                  box->click(); QCOMPARE(s->styleB(sid), !initial);
                  box->click(); QCOMPARE(s->styleB(sid), initial);
                  }
            auto before = dialog.findChild<QCheckBox*>("changesBeforeBarlineRepeats"); QVERIFY(before);
            QVERIFY(!before->isChecked()); before->click(); s->doLayout();
            Measure* end = s->firstMeasure()->nextMeasure()->nextMeasure();
            Segment* change = end->findSegmentR(SegmentType::TimeSig, end->ticks());
            QVERIFY(change && change->element(0));
            if (qEnvironmentVariableIsSet("COURTESY_STYLE_PREVIEW"))
                  QVERIFY(dialog.grab().save(QString::fromLocal8Bit(qgetenv("COURTESY_STYLE_PREVIEW"))));
            dialog.reject(); s->doLayout();
            QVERIFY(!s->styleB(Sid::changesBeforeBarlineRepeats));
            Measure* continuation = s->firstMeasure()->nextMeasure()->nextMeasure()->nextMeasure();
            change = continuation->findSegmentR(SegmentType::TimeSig, Fraction(0,1));
            QVERIFY(change && change->element(0));
            }
      };
QTEST_MAIN(TestRepeatCourtesyUi)
#include "tst_repeatcourtesyui.moc"
