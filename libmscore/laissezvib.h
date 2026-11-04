// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef __LAISSEZVIB_H__
#define __LAISSEZVIB_H__
#include "element.h"

namespace Ms {
// A note-owned visual release mark. It deliberately never joins the playback tie chain.
class LaissezVib final : public Element {
      Direction _direction = Direction::AUTO;
      int _tiePlacement = 0;
      Spatium _minLength = Spatium(2.0);
      QPainterPath _path;
      SymId _symbol = SymId::noSym;
      QPointF attachment(bool& up) const;
   public:
      LaissezVib(Score*);
      LaissezVib* clone() const override { return new LaissezVib(*this); }
      ElementType type() const override { return ElementType::LAISSEZ_VIB; }
      void layout() override;
      static qreal layoutChord(Chord*); // after every note's dots have been positioned
      void draw(QPainter*) const override;
      void read(XmlReader&) override;
      void write(XmlWriter&) const override;
      QVariant getProperty(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      void reset() override;
      };
}
#endif
