//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "chord.h"
#include "score.h"
#include "stemslash.h"
#include "beam.h"
#include "hook.h"
#include "staff.h"
#include <cmath>

namespace Ms {

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void StemSlash::draw(QPainter* painter) const
      {
      qreal lw = lineWidth();
      painter->setPen(QPen(curColor(), lw, Qt::SolidLine, Qt::FlatCap));
      painter->drawLine(line);
      }

qreal StemSlash::lineWidth() const
      {
      return _lineWidth4 > 0 ? _lineWidth4 : score()->styleP(Sid::stemWidth);
      }

//---------------------------------------------------------
//   setLine
//---------------------------------------------------------

void StemSlash::setLine(const QLineF& l)
      {
      line = l;
      qreal w = lineWidth() * .5;
      setbbox(QRectF(line.p1(), line.p2()).normalized().adjusted(-w, -w, 2.0*w, 2.0*w));
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void StemSlash::layout()
      {
      Stem* stem = chord()->stem();
      _lineWidth4 = 0;
      const Chord* c = chord();
      const Beam* beam = c->beam();
      if (score()->styleB(Sid::beamLayout4) && stem && c->isGrace()
          && c->staff() && c->staff()->isPitchedStaff(c->tick())
          && !c->staffMove() && (!beam || !beam->cross())) {
            // Studio v4.7.5 layoutStemSlash, using its default slash styles:
            // position 2sp, angle 40 degrees and thickness .125sp. Evolution
            // has curved flags and no independent slash-style properties.
            const qreal sp = spatium();
            const qreal mag = c->mag();
            const qreal sign = c->up() ? -1 : 1;
            const Hook* hook = c->hook();
            const qreal right = stem->x() + stem->lineWidthMag() / 2;
            qreal tip = stem->y() + stem->p2().y();
            // Studio's stem bbox covers the beam's outside half width;
            // Evolution's stem line ends at its center (and bbox adds a cap).
            if (beam) tip += sign * beam->point(score()->styleS(Sid::beamWidth)) * beam->mag() / 2;
            const QPointF start(right - score()->noteHeadWidth() * mag / 2,
                  tip - sign * mag * 2 * sp * (hook ? 1 : .66));
            qreal angle = 40 * std::acos(qreal(-1)) / 180;
            QPointF end;
            if (hook) {
                  const QString font = score()->styleSt(Sid::musicalSymbolFont);
                  if (c->beams() >= 2 && (font == "Bravura" || font == "Finale Maestro" || font == "Gonville"))
                        angle *= 1.2;
                  end.setX(hook->bbox().translated(hook->pos()).right());
                  end.setY(start.y() + sign * (end.x() - start.x()) * std::tan(angle));
                  }
            else if (beam) {
                  const QLineF* primary = beam->primarySegment4();
                  const qreal beamAngle = primary && primary->dx() > 0 ? std::atan(primary->dy() / primary->dx()) : 0;
                  angle += sign * beamAngle / 2;
                  const bool obtuse = (c->up() && beamAngle < 0) || (!c->up() && beamAngle > 0);
                  const qreal length = 2 * sp * (obtuse ? 1.1 : 1);
                  end = start + QPointF(length * std::cos(angle), sign * length * std::sin(angle));
                  }
            else {
                  end.setX(right + score()->noteHeadWidth() * mag / 2 - stem->lineWidthMag());
                  end.setY(start.y() + sign * (end.x() - start.x()) * std::tan(angle));
                  }
            _lineWidth4 = .125 * sp * mag;
            setLine(QLineF(start, end));
            return;
            }
      qreal h2;
      qreal _spatium = spatium();
      qreal l = chord()->up() ? _spatium : -_spatium;
      QPointF p(stem->hookPos());
      qreal x = p.x() + _spatium * .1;
      qreal y = p.y();

      if (chord()->beam()) {
            y += l * .3;
            h2 = l * .8;
            }
      else {
            y += l * 1.2;
            h2 = l * .4;
            }
      qreal w  = chord()->upNote()->bboxRightPos() * .7;
      setLine(QLineF(QPointF(x + w, y - h2), QPointF(x - w, y + h2)));
      }



}

