//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation.
//=============================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QTemporaryFile>

#include "libmscore/score.h"
#include "libmscore/style.h"
#include "mtest/testutils.h"

using namespace Ms;

class TestStyleDefaults : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase()
            {
            // The legacy styles live in the application's resource bundle,
            // not in the minimal mtest.qrc resource collection.
            Q_INIT_RESOURCE(musescore);
            initMTest();
            }
      void bundledLegacyResources_data();
      void bundledLegacyResources();
      void explicitFontOverride();
      void modernBuiltInDefaults();
      };

void TestStyleDefaults::bundledLegacyResources_data()
      {
      QTest::addColumn<int>("defaultsVersion");
      QTest::addColumn<QString>("resourcePath");
      QTest::newRow("v1.14") << 114 << QString(":/styles/legacy-style-defaults-v1.mss");
      QTest::newRow("v2.06") << 206 << QString(":/styles/legacy-style-defaults-v2.mss");
      QTest::newRow("v3.01") << 301 << QString(":/styles/legacy-style-defaults-v3.mss");
      }

void TestStyleDefaults::bundledLegacyResources()
      {
      QFETCH(int, defaultsVersion);
      QFETCH(QString, resourcePath);

      // Catch missing Qt resource registration independently of MStyle's
      // fallback (which would otherwise appear as the modern defaults).
      QVERIFY2(QFile::exists(resourcePath), qPrintable(resourcePath));

      const MStyle* legacy = MStyle::resolveStyleDefaults(defaultsVersion);
      QVERIFY(legacy != nullptr);
      QCOMPARE(legacy->value(Sid::musicalSymbolFont).toString(), QString("Emmentaler"));
      QCOMPARE(legacy->value(Sid::musicalTextFont).toString(), QString("MScore Text"));
      if (defaultsVersion == 301)
            QCOMPARE(legacy->value(Sid::enableIndentationOnFirstSystem).toBool(), false);
      }

void TestStyleDefaults::explicitFontOverride()
      {
      // A score or style file explicitly requesting Leland must not be
      // forced back to Emmentaler by historical defaults.
      MStyle style(*MStyle::resolveStyleDefaults(301));
      QTemporaryFile file;
      QVERIFY(file.open());
      const QByteArray xml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<museScore version=\"3.01\"><Style>"
            "<musicalSymbolFont>Leland</musicalSymbolFont>"
            "<musicalTextFont>Leland Text</musicalTextFont>"
            "</Style></museScore>";
      QCOMPARE(file.write(xml), qint64(xml.size()));
      QVERIFY(file.flush());
      QVERIFY(file.seek(0));
      QVERIFY(style.load(&file, true));
      QCOMPARE(style.value(Sid::musicalSymbolFont).toString(), QString("Leland"));
      QCOMPARE(style.value(Sid::musicalTextFont).toString(), QString("Leland Text"));
      }

void TestStyleDefaults::modernBuiltInDefaults()
      {
      MStyle modern;
      QCOMPARE(modern.value(Sid::musicalSymbolFont).toString(), QString("Leland"));
      QCOMPARE(modern.value(Sid::musicalTextFont).toString(), QString("Leland Text"));
      }

QTEST_MAIN(TestStyleDefaults)
#include "tst_styledefaults.moc"
