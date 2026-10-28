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

#ifndef __SLUR_H__
#define __SLUR_H__

#include "slurtie.h"

namespace Ms {

//---------------------------------------------------------
//   @@ SlurSegment
///    a single segment of slur; also used for Tie
//---------------------------------------------------------

class SlurSegment final : public SlurTieSegment {

   protected:
      qreal _extraHeight = 0.0;
      QString _hopoText;
      QRectF _hopoTextRect;
      QPointF _hopoTextPos;
      QFont hopoFont(qreal pixelRatio = 1.0) const;
      void layoutHopoText();
      void changeAnchor(EditData&, Element*) override;

   public:
      SlurSegment(Score* s) : SlurTieSegment(s) {}
      SlurSegment(const SlurSegment& ss) : SlurTieSegment(ss),
            _hopoText(ss._hopoText), _hopoTextRect(ss._hopoTextRect), _hopoTextPos(ss._hopoTextPos) {}

      SlurSegment* clone() const override  { return new SlurSegment(*this); }
      ElementType type() const override    { return ElementType::SLUR_SEGMENT; }
      int subtype() const override         { return static_cast<int>(spanner()->type()); }
      void draw(QPainter*) const override;

      void layoutSegment(const QPointF& p1, const QPointF& p2);
      const QRectF& hopoTextRect() const { return _hopoTextRect; }
      const QString& hopoText() const { return _hopoText; }

      bool isEdited() const;
      bool edit(EditData&) override;

      Slur* slur() const { return toSlur(spanner()); }

      void computeBezier(QPointF so = QPointF()) override;
      };

//---------------------------------------------------------
//   @@ Slur
//---------------------------------------------------------

class Slur : public SlurTie {

      void slurPosChord(SlurPos*);
      int _partialDirection { -1 }; // Studio: none=-1, incoming=0, outgoing=1, both=2


   public:
      Slur(Score* = 0);
      Slur(const Slur&);
      ~Slur() {}

      Slur* clone() const override        { return new Slur(*this); }
      ElementType type() const override { return ElementType::SLUR; }
      void write(XmlWriter& xml) const override;
      bool readProperties(XmlReader&) override;
      int partialDirection() const { return _partialDirection; }
      void setPartialDirection(int d) { _partialDirection = d; }
      bool isIncoming() const { return _partialDirection == 0 || _partialDirection == 2; }
      bool isOutgoing() const { return _partialDirection == 1 || _partialDirection == 2; }
      QVariant getProperty(Pid) const override;
      QVariant propertyDefault(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      void layout() override;
      SpannerSegment* layoutSystem(System*) override;
      void setTrack(int val) override;
      void slurPos(SlurPos*) override;

      SlurSegment* frontSegment()               { return toSlurSegment(Spanner::frontSegment()); }
      const SlurSegment* frontSegment() const   { return toSlurSegment(Spanner::frontSegment()); }
      SlurSegment* backSegment()                { return toSlurSegment(Spanner::backSegment());  }
      const SlurSegment* backSegment() const    { return toSlurSegment(Spanner::backSegment());  }
      SlurSegment* segmentAt(int n)             { return toSlurSegment(Spanner::segmentAt(n));   }
      const SlurSegment* segmentAt(int n) const { return toSlurSegment(Spanner::segmentAt(n));   }

      SlurTieSegment* newSlurTieSegment() override { return new SlurSegment(score()); }
      };

}     // namespace Ms
#endif

