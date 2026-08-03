//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include "system.h"
#include "page.h"
#include "box.h"

namespace Ms {

class Segment;
class Page;

//---------------------------------------------------------
//   VerticalStretchData
//    helper class for spreading staves over a page
//---------------------------------------------------------

class VerticalGapData {
   private:
      bool  _fixedHeight          { false };
      bool  _fixedSpacer          { false };
      qreal _factor               { 1.0   };
      qreal _normalisedSpacing    { 0.0   };
      qreal _maxActualSpacing     { 0.0   };
      qreal _addedNormalisedSpace { 0.0   };
      qreal _fillSpacing          { 0.0   };
      qreal _lastStep             { 0.0   };
      void  updateFactor(qreal factor);

   public:
      System*   system   { nullptr };
      SysStaff* sysStaff { nullptr };
      Staff*    staff    { nullptr };

      VerticalGapData(bool first, System* sys, Staff* st, SysStaff* sst, Spacer* nextSpacer, qreal y);

      void addSpaceBetweenSections();
      void addSpaceAroundVBox(bool above);
      void addSpaceAroundNormalBracket();
      void addSpaceAroundCurlyBracket();
      void insideCurlyBracket();

      qreal factor() const;
      qreal spacing() const;
      qreal actualAddedSpace() const;

      qreal addSpacing(qreal step);
      bool isFixedHeight() const;
      void undoLastAddSpacing();
      qreal addFillSpacing(qreal step, qreal maxFill);
      };

//---------------------------------------------------------
//   VerticalStretchDataList
//    helper class for spreading staves over a page
//---------------------------------------------------------

class VerticalGapDataList : public QList<VerticalGapData*> {
   public:
      void deleteAll();
      qreal sumStretchFactor() const;
      qreal smallest(qreal limit=-1.0) const;
      };

//---------------------------------------------------------
//   LayoutContext
//    temp values used during layout
//---------------------------------------------------------

struct LayoutContext {
      Score* score             { 0    };
      bool startWithLongNames  { true };
      bool firstSystem         { true };
      bool firstSystemIndent   { true };
      Page* page               { 0 };
      int curPage              { 0 };      // index in Score->page()s
      Fraction tick            { 0, 1 };

      QList<System*> systemList;          // reusable systems
      std::set<Spanner*> processedSpanners;

      System* prevSystem       { 0 };     // used during page layout
      System* curSystem        { 0 };

      MeasureBase* systemOldMeasure;
      MeasureBase* pageOldMeasure;
      bool rangeDone           { false };

      MeasureBase* prevMeasure { 0 };
      MeasureBase* curMeasure  { 0 };
      MeasureBase* nextMeasure { 0 };
      int measureNo            { 0 };
      Fraction startTick;
      Fraction endTick;

      // Vertical/Text Box chaining:
      struct DeferredGroup {
            qreal startY = 0.0;
            qreal startDistance = 0.0;
            unsigned remainingBoxes = 0;
            unsigned pageSystemCount = 0;
            bool waitingForTerminator = false;
            QVector<System*> systems;
            Page* startPage = nullptr;
            System* restartSystem = nullptr;
            System* previousSystem = nullptr;
            MeasureBase* restartPrevMeasure = nullptr;
            MeasureBase* restartCurMeasure  = nullptr;
            MeasureBase* restartNextMeasure = nullptr;
            Fraction restartTick;
            int restartMeasureNo;
            bool restartRangeDone;

            void saveState(LayoutContext* lc, MeasureBase* startingVBox, qreal y, qreal distance) {
                  restartSystem      = lc->curSystem;
                  previousSystem     = lc->prevSystem;
                  restartPrevMeasure = lc->prevMeasure;
                  restartCurMeasure  = lc->curMeasure;
                  restartNextMeasure = lc->nextMeasure;
                  restartTick        = lc->tick;
                  restartMeasureNo   = lc->measureNo;
                  restartRangeDone   = lc->rangeDone;
                  pageSystemCount    = lc->page->systems().size();
                  startPage          = lc->page;
                  startY             = y;
                  startDistance      = distance;

                  const MeasureBase* mb = startingVBox;
                  remainingBoxes = 1;
                  while (true) {
                        mb = mb->next();
                        if (!mb || !mb->isVBoxBase())
                              break;
                        const Box* const nextBox = toBox(mb);
                        if (!nextBox->bindToNextSystem())
                              break;
                        ++remainingBoxes;
                        }
                  }

            qreal restoreState(LayoutContext* lc) {
                  lc->prevSystem  = previousSystem;
                  lc->curSystem   = restartSystem;
                  lc->prevMeasure = restartPrevMeasure;
                  lc->curMeasure  = restartCurMeasure;
                  lc->nextMeasure = restartNextMeasure;
                  lc->tick        = restartTick;
                  lc->measureNo   = restartMeasureNo;
                  lc->rangeDone   = restartRangeDone;

                  return startY;
                  }

            qreal commitToPage(LayoutContext* lc) {
                  qreal yy = startY;
                  const System* prev = previousSystem;

                  for (System* s : systems) {
                        qreal dist = prev ? prev->minDistance(s) : startDistance;
                        yy += dist;
                        s->setPos(lc->page->lm(), yy);
                        s->restoreLayout2();
                        lc->page->appendSystem(s);
                        yy += s->height();
                        prev = s;
                        }

                  lc->curSystem = systems.last();
                  lc->rangeDone = false;
                  return yy; // return total height
                  }

            void clear() {
                  startY = 0.0;
                  startDistance = 0.0;
                  remainingBoxes = 0;
                  restartTick = {};
                  waitingForTerminator = false;
                  previousSystem = nullptr;
                  restartSystem = nullptr;
                  restartCurMeasure = nullptr;
                  restartNextMeasure = nullptr;
                  restartPrevMeasure = nullptr;
                  pageSystemCount = 0;
                  restartMeasureNo = 0;
                  restartRangeDone = false;
                  systems.clear();
                  startPage = nullptr;
                  }

            bool active() const {
                  return remainingBoxes || waitingForTerminator || !systems.empty();
                  }
            qreal extent() const;
            };

      DeferredGroup deferredGroup;
      bool deferredGroupBreak = false;

      LayoutContext(Score* s);
      LayoutContext(const LayoutContext&) = delete;
      LayoutContext& operator=(const LayoutContext&) = delete;
      ~LayoutContext();

      void layoutLinear();

      void layout();
      int adjustMeasureNo(MeasureBase*);
      void getNextPage();
      void collectPage();

   private:
      static void layoutPage(Page* page, qreal restHeight, qreal footerPadding);
      static void checkDivider(bool left, System* s, qreal yOffset, bool remove = false);
      static void distributeStaves(Page* page, qreal footerPadding);

      static void layoutCrossStaffElements(Page* page);
      static void layoutCrossStaffSlurs(System* system);
      static void layoutArticAndFingeringOnCrossStaffBeams(System* system);
      };

//---------------------------------------------------------
//   VerticalAlignRange
//---------------------------------------------------------

enum class VerticalAlignRange : char {
      SEGMENT, MEASURE, SYSTEM
      };

extern bool isTopBeam(ChordRest* cr);
extern bool notTopBeam(ChordRest* cr);
extern bool isTopTuplet(ChordRest* cr);
extern bool notTopTuplet(ChordRest* cr);

}     // namespace Ms
#endif

