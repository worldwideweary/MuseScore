//=============================================================================
//  MuseScore
//  Copyright (C) 2026 MuseScore contributors
//  SPDX-License-Identifier: GPL-2.0-only
//=============================================================================
#ifndef MS_GUITARBEND_H
#define MS_GUITARBEND_H

#include "line.h"

namespace Ms {

// Numeric values are part of the Studio 4.x file format.
enum class GuitarBendType : char {
      BEND, PRE_BEND, GRACE_NOTE_BEND, SLIGHT_BEND, DIVE, PRE_DIVE, DIP, SCOOP
      };

class GuitarBend final : public SLine {
      GuitarBendType _bendType = GuitarBendType::BEND;
      Direction _direction = Direction::AUTO;
      int _hold = 0;
      qreal _startTime = 0.0;
      qreal _endTime = 1.0;
      qreal _targetTime = 0.25;
      int _amount = -2;
      bool _play = true;
      QString _holdXml;

   protected:
      QPointF linePos(Grip, System**) const override;

   public:
      explicit GuitarBend(Score*);
      GuitarBend* clone() const override { return new GuitarBend(*this); }
      ElementType type() const override { return ElementType::GUITAR_BEND; }
      GuitarBendType bendType() const { return _bendType; }
      void setBendType(GuitarBendType t) { _bendType = t; }
      Note* startNote() const;
      Note* endNote() const;
      static const Note* frettingNote(const Note*);
      static int frettingPitch(const Note*);
      static int tabFretVisibility(const Note*);
      static void updateTabNote(Note*);
      static bool hideTabFret(const Note*);
      int bendAmount() const;
      const GuitarBend* overlappingBendOrDive() const;
      int totalBendAmount() const;
      GuitarBend* precedingBend() const;
      const GuitarBend* followingPreBend() const;
      const Note* holdEndNote() const;
      int diveLevelIndex() const;
      QString bendText() const;
      bool isDive() const { return int(_bendType) >= int(GuitarBendType::DIVE); }
      bool singleNote() const;
      LineSegment* createLineSegment() override;
      void layout() override;
      void scanElements(void*, void (*)(void*, Element*), bool all = true) override;
      void read(XmlReader&) override;
      void write(XmlWriter&) const override;
      QVariant getProperty(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      Sid getPropertyStyle(Pid) const override;
      };

class GuitarBendSegment final : public LineSegment {
      QPainterPath _path;
      QPainterPath _holdPath;
      QPolygonF _arrow;
      QString _text;
      QRectF _textRect;
      QPointF _textPos;
      QPointF _vertexOff;
      QPointF _drawStart;
      QPointF _drawEnd;
      QPointF _vertex;
      QPointF _scoopPos;

   protected:
      void startEditDrag(EditData&) override;
      void editDrag(EditData&) override;

   public:
      GuitarBendSegment(Spanner* s, Score* score) : LineSegment(s, score, ElementFlag::MOVABLE) {}
      GuitarBendSegment* clone() const override { return new GuitarBendSegment(*this); }
      ElementType type() const override { return ElementType::GUITAR_BEND_SEGMENT; }
      GuitarBend* guitarBend() const { return toGuitarBend(spanner()); }
      void layout() override;
      void draw(QPainter*) const override;
      Shape shape() const override;
      Element* propertyDelegate(Pid) override;
      int gripsCount() const override;
      std::vector<QPointF> gripsPositions(const EditData& = EditData()) const override;
      QVariant getProperty(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      void reset() override;
      void spatiumChanged(qreal, qreal) override;
      void localSpatiumChanged(qreal, qreal) override;
      const QPainterPath& path() const { return _path; }
      const QPolygonF& arrow() const { return _arrow; }
      const QPainterPath& holdPath() const { return _holdPath; }
      const QRectF& textRect() const { return _textRect; }
      QFont labelFont(qreal pixelRatio = 1.0) const;
      QPen linePen() const;
      };

} // namespace Ms
#endif
