//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2019 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include <memory>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/system.h"
#include "libmscore/undo.h"
#include "libmscore/box.h"
#include "libmscore/xml.h"
#include "libmscore/style.h"
#include "libmscore/textframe.h"

#define DIR QString("libmscore/box/")

using namespace Ms;

//---------------------------------------------------------
//   TestBox
//---------------------------------------------------------

class TestBox : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase();
      void undoRemoveVBox();
      void legacyFrameClearance();
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestBox::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   undoRemoveVBox
///   read a file with a vbox. Delete it, and undo. Check that the VBox still exists.
//---------------------------------------------------------

void TestBox::undoRemoveVBox()
      {
      QString readFile(DIR + "undoRemoveVBox.mscx");
      QString writeFile1("undoRemoveVBox1-test.mscx");
      QString reference1(DIR  + "undoRemoveVBox1-ref.mscx");
      QString writeFile2("undoRemoveVBox2-test.mscx");
      QString reference2(DIR  + "undoRemoveVBox2-ref.mscx");

      MasterScore* score = readScore(readFile);
      score->doLayout();

      System* s = score->systems()[0];
      VBox* box = toVBox(s->measure(0));

      score->startCmd();
      score->select(box);
      score->cmdDeleteSelection();
      score->endCmd();
      QVERIFY(saveCompareScore(score, writeFile1, reference1));

      // undo
      score->undoStack()->undo(nullptr);
      QVERIFY(saveCompareScore(score, writeFile2, reference2));

      delete score;
      }

// Missing clearances mean zero only for frames imported from pre-4.6 files.
void TestBox::legacyFrameClearance()
      {
      MasterScore imported(score->style());
      imported.setStyleValue(Sid::paddingToNotationAbove, QVariant::fromValue(Spatium(2.0)));
      imported.setStyleValue(Sid::paddingToNotationBelow, QVariant::fromValue(Spatium(2.0)));
      auto makeFrame = [&imported](ElementType type) -> Box* {
            switch (type) {
                  case ElementType::VBOX: return new VBox(&imported);
                  case ElementType::TBOX: return new TBox(&imported);
                  case ElementType::FBOX: return new FBox(&imported);
                  default: return nullptr;
                  }
            };
      for (ElementType type : { ElementType::VBOX, ElementType::TBOX, ElementType::FBOX }) {
            imported.setMscVersion(302);
            std::unique_ptr<Box> fresh(makeFrame(type));
            QVERIFY(fresh);
            QCOMPARE(fresh->paddingToNotationAbove().val(), 2.0);
            QCOMPARE(fresh->paddingToNotationBelow().val(), 2.0);
            const QByteArray tag = type == ElementType::VBOX ? "VBox" : (type == ElementType::TBOX ? "TBox" : "FBox");
            const QList<QByteArray> values {
                  QByteArray(),
                  "<paddingToNotationAbove>2</paddingToNotationAbove><paddingToNotationBelow>2</paddingToNotationBelow>",
                  "<paddingToNotationAbove>2.5</paddingToNotationAbove><paddingToNotationBelow>3.5</paddingToNotationBelow>",
                  "<paddingToNotationAbove>2.5</paddingToNotationAbove>",
                  "<paddingToNotationBelow>3.5</paddingToNotationBelow>",
                  "<paddingToNotationAbove>0</paddingToNotationAbove><paddingToNotationBelow>0</paddingToNotationBelow>"
                  };
            const double above[] = { 0.0, 2.0, 2.5, 2.5, 0.0, 0.0 };
            const double below[] = { 0.0, 2.0, 3.5, 0.0, 3.5, 0.0 };
            for (int version : { 206, 302, 450, 460, 475 }) {
                  imported.setMscVersion(version);
                  for (int i = 0; i < values.size(); ++i) {
                        const QByteArray data = "<" + tag + ">" + values[i] + "</" + tag + ">";
                        XmlReader xml(data);
                        QVERIFY(xml.readNextStartElement());
                        std::unique_ptr<Box> frame(makeFrame(type));
                        frame->read(xml);
                        QVERIFY(!xml.hasError());
                        // A later style refresh must not replace legacy zeros.
                        frame->styleChanged();

                        qDebug()
                              << "frame type" << int(type)
                              << "version" << version
                              << "case" << i;

                        const bool missingAbove = i == 0 || i == 4;
                        const bool missingBelow = i == 0 || i == 3;
                        const double expectedAbove = version >= 460 && missingAbove ? 2.0 : above[i];
                        const double expectedBelow = version >= 460 && missingBelow ? 2.0 : below[i];
                        QCOMPARE(frame->paddingToNotationAbove().val(), expectedAbove);
                        QCOMPARE(frame->paddingToNotationBelow().val(), expectedBelow);
                        if (version < 460) {
                              QVERIFY(frame->propertyFlags(Pid::PADDING_TO_NOTATION_ABOVE) == PropertyFlags::UNSTYLED);
                              QVERIFY(frame->propertyFlags(Pid::PADDING_TO_NOTATION_BELOW) == PropertyFlags::UNSTYLED);
                              }
                        std::unique_ptr<Element> copy(writeReadElement(frame.get()));
                        QVERIFY(copy);
                        QCOMPARE(copy->getProperty(Pid::PADDING_TO_NOTATION_ABOVE).value<Spatium>().val(), expectedAbove);
                        QCOMPARE(copy->getProperty(Pid::PADDING_TO_NOTATION_BELOW).value<Spatium>().val(), expectedBelow);
                        }
                  }
            // Reset restores the style default; creating another frame in an
            // old score must not inherit the imported frame's zero values.
            imported.setMscVersion(302);
            std::unique_ptr<Box> legacy(makeFrame(type));
            XmlReader missing("<" + tag + "/>");
            QVERIFY(missing.readNextStartElement());
            legacy->read(missing);
            legacy->resetProperty(Pid::PADDING_TO_NOTATION_ABOVE);
            QCOMPARE(legacy->paddingToNotationAbove().val(), 2.0);
            QCOMPARE(legacy->paddingToNotationBelow().val(), 0.0);
            std::unique_ptr<Box> another(makeFrame(type));
            QCOMPARE(another->paddingToNotationAbove().val(), 2.0);
            QCOMPARE(another->paddingToNotationBelow().val(), 2.0);
            imported.setPaletteMode(true);
            std::unique_ptr<Box> palette(makeFrame(type));
            XmlReader paletteXml("<" + tag + "/>");
            QVERIFY(paletteXml.readNextStartElement());
            palette->read(paletteXml);
            QCOMPARE(palette->paddingToNotationAbove().val(), 2.0);
            QCOMPARE(palette->paddingToNotationBelow().val(), 2.0);
            imported.setPaletteMode(false);
            }
      }

QTEST_MAIN(TestBox)
#include "tst_box.moc"
