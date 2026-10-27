//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __TEXTLINE_H__
#define __TEXTLINE_H__

#include "textlinebase.h"

namespace Ms {

class Note;

//---------------------------------------------------------
//   @@ TextLineSegment
//---------------------------------------------------------

class TextLineSegment : public TextLineBaseSegment {

      Sid getTextLinePos(bool above) const;
      Sid getPropertyStyle(Pid) const override;

   public:
      TextLineSegment(Spanner* sp, Score* s, bool system=false);

      ElementType type() const override       { return ElementType::TEXTLINE_SEGMENT; }
      TextLineSegment* clone() const override { return new TextLineSegment(*this); }

      virtual Element* propertyDelegate(Pid) override;

      TextLine* textLine() const;
      void layout() override;
      };

//---------------------------------------------------------
//   @@ TextLine
//---------------------------------------------------------

class TextLine : public TextLineBase {

      Sid getTextLinePos(bool above) const;
      Sid getPropertyStyle(Pid) const override;

   public:
      TextLine(Score* s, bool system=false);
      TextLine(const TextLine&);
      ~TextLine() {}

      virtual void undoChangeProperty(Pid id, const QVariant&, PropertyFlags ps) override;
      virtual SpannerSegment* layoutSystem(System*) override;

      TextLine* clone() const override   { return new TextLine(*this); }
      ElementType type() const override  { return ElementType::TEXTLINE; }

      void write(XmlWriter&) const override;
      void read(XmlReader&) override;

      void initStyle();

      LineSegment* createLineSegment() override;
      QVariant propertyDefault(Pid) const override;
      bool setProperty(Pid propertyId, const QVariant&) override;
      };

// A tempo line shares the text line layout, but has a distinct XML element.
class GradualTempoChangeSegment final : public TextLineSegment {
   public:
      GradualTempoChangeSegment(Spanner* sp, Score* s)
         : TextLineSegment(sp, s, true) {}
      ElementType type() const override { return ElementType::GRADUAL_TEMPO_CHANGE_SEGMENT; }
      GradualTempoChangeSegment* clone() const override { return new GradualTempoChangeSegment(*this); }
      Element* propertyDelegate(Pid id) override {
            if (id == Pid::TEMPO_CHANGE_TYPE || id == Pid::TEMPO_EASING_METHOD || id == Pid::TEMPO_CHANGE_FACTOR)
                  return spanner();
            return TextLineSegment::propertyDelegate(id);
            }
      };

class GradualTempoChange final : public TextLine {
      QString _tempoChangeType = "undefined";
      QString _easingMethod = "normal";
      qreal _factor = 1.0;
      bool _explicitFactor = false;

   public:
      GradualTempoChange(Score*);
      ElementType type() const override { return ElementType::GRADUAL_TEMPO_CHANGE; }
      GradualTempoChange* clone() const override { return new GradualTempoChange(*this); }
      LineSegment* createLineSegment() override;
      void write(XmlWriter&) const override;
      void read(XmlReader&) override;
      QVariant getProperty(Pid) const override;
      QVariant propertyDefault(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      qreal factor() const { return _factor; }
      const QString& easingMethod() const { return _easingMethod; }
      };

}     // namespace Ms
#endif

