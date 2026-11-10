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

#ifndef __TIE_H__
#define __TIE_H__

#include "slurtie.h"

namespace Ms {

// Note-local attachment for partial ties and release marks. Outside placement
// still has inner chord ties, which must clear adjacent noteheads and dots.
QPointF tieNoteAttachment(const Note*, bool up, bool inside, bool start, bool adjust = true);

//---------------------------------------------------------
//   @@ TieSegment
///    a single segment of slur; also used for Tie
//---------------------------------------------------------

class TieSegment final : public SlurTieSegment {
      QPointF autoAdjustOffset;
      // Transient displacement of the two endpoints produced by automatic
      // staff-line and horizontal collision clearance. Never stored in scores.
      QPointF automaticGripOffset[2];
      void consolidateAutomaticGripOffsets();
      void preserveAutomaticShoulderHeight();
      qreal shoulderHeightMin = 0.3;
      qreal shoulderHeightMax = 2.0;

      void setAutoAdjust(const QPointF& offset);

   protected:
      void changeAnchor(EditData&, Element*) override;

   public:
      TieSegment(Score* s) : SlurTieSegment(s) { autoAdjustOffset = QPointF(); }
      TieSegment(const TieSegment& s) : SlurTieSegment(s) { autoAdjustOffset = QPointF(); }

      TieSegment* clone() const override   { return new TieSegment(*this); }
      ElementType type() const override    { return ElementType::TIE_SEGMENT; }
      int subtype() const override         { return static_cast<int>(spanner()->type()); }
      void draw(QPainter*) const override;

      void layoutSegment(const QPointF& p1, const QPointF& p2);
      void adjustX();
      void finalizeSegment();

      bool isEdited() const;
      void editDrag(EditData&) override;
      bool edit(EditData&) override;

      Tie* tie() const { return (Tie*)spanner(); }
      Element* propertyDelegate(Pid) override;

      void computeBezier(QPointF so = QPointF()) override;
      };

//---------------------------------------------------------
//   @@ Tie
//!    a Tie has a Note as startElement/endElement
//---------------------------------------------------------

class Tie final : public SlurTie {
      static Note* editStartNote;
      static Note* editEndNote;

   private:
      bool _isInside{ false };
      // Partial ties belong to a single note and never enter tieFor/tieBack.
      // Keeping those chains linear avoids cycles at backwards repeat jumps.
      int _partialDirection { -1 }; // -1 normal, 0 incoming, 1 outgoing

      // Same values as Studio: Auto, Inside, Outside.
      int _tiePlacement = 0;

   public:
      Tie(Score* = 0);

      Tie* clone() const override       { return new Tie(*this);  }
      ElementType type() const override { return ElementType::TIE; }

      bool isPartial() const { return _partialDirection >= 0; }
      bool isIncoming() const { return _partialDirection == 0; }
      void setPartialDirection(int direction) { _partialDirection = direction; }
      int partialDirection() const { return _partialDirection; }
      TieSegment* layoutPartial(System*);
      qreal incomingStartOffset() const; // free end, relative to the owner measure
      qreal incomingEndOffset(); // note-owned end, relative to its CR segment
      const char* spacingGripInset4(qreal& inset) const; // null on success; otherwise the reason the probe was skipped

      void setStartNote(Note* note);
      void setEndNote(Note* note)                 { setEndElement((Element*)note); }
      Note* startNote() const;
      Note* endNote() const;

      bool isInside() const { return _isInside; }
      int tiePlacement() const { return _tiePlacement; }
      void calculateIsInside();
      bool isOuterTieOfChord(bool start) const;
      void reset() override;
      QVariant getProperty(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      bool readProperties(XmlReader&) override;

      void calculateDirection();
      void write(XmlWriter& xml) const override;
      void slurPos(SlurPos*) override;

      TieSegment* layoutFor(System*);
      TieSegment* layoutBack(System*);

      TieSegment* frontSegment()               { return toTieSegment(Spanner::frontSegment()); }
      const TieSegment* frontSegment() const   { return toTieSegment(Spanner::frontSegment()); }
      TieSegment* backSegment()                { return toTieSegment(Spanner::backSegment());  }
      const TieSegment* backSegment() const    { return toTieSegment(Spanner::backSegment());  }
      TieSegment* segmentAt(int n)             { return toTieSegment(Spanner::segmentAt(n));   }
      const TieSegment* segmentAt(int n) const { return toTieSegment(Spanner::segmentAt(n));   }

      SlurTieSegment* newSlurTieSegment() override { return new TieSegment(score()); }
      };

}     // namespace Ms
#endif

