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

#include "chordrest.h"
#include "mscore.h"
#include "measure.h"
#include "score.h"
#include "staff.h"
#include "style.h"
#include "system.h"
#include "text.h"
#include "textlinebase.h"
#include "utils.h"
#include "xml.h"

#include <QLineF>
#include <QRectF>
#include <QPointF>

namespace Ms {

//---------------------------------------------------------
//   TextLineBaseSegment
//---------------------------------------------------------

TextLineBaseSegment::TextLineBaseSegment(Spanner* sp, Score* score, ElementFlags f)
   : LineSegment(sp, score, f)
      {
      _text    = new Text(score);
      _endText = new Text(score);
      _text->setParent(this);
      _endText->setParent(this);
      _text->setFlag(ElementFlag::MOVABLE, false);
      _endText->setFlag(ElementFlag::MOVABLE, false);
      }

TextLineBaseSegment::TextLineBaseSegment(const TextLineBaseSegment& seg)
   : LineSegment(seg)
      {
      _text    = seg._text->clone();
      _endText = seg._endText->clone();
      _text->setParent(this);
      _endText->setParent(this);
      layout();    // set the right _text
      }

TextLineBaseSegment::~TextLineBaseSegment()
      {
      delete _text;
      delete _endText;
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void TextLineBaseSegment::setSelected(bool f)
      {
      SpannerSegment::setSelected(f);
      _text->setSelected(f);
      _endText->setSelected(f);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TextLineBaseSegment::draw(QPainter* painter) const
      {
      TextLineBase* tl = textLineBase();
      QColor lineColor = curColor(tl->visible() && tl->lineVisible(), tl->lineColor());      
      QColor textColor = tl->color();
      qreal strokeWidth = tl->lineWidth();
      qreal lineAngle = -(textLine.angle());

      auto lineColorAlpha = lineColor.alpha();
      bool transparentLine = (lineColorAlpha == 0);
      QColor opaqueLineColor = lineColor;
      opaqueLineColor.setAlpha(255);

      // Playback marked color:
      auto t1 = spanner()->tick();
      auto t2 = spanner()->tick2();
      auto playingElement = score()->getLastCRSequenced();
      auto pos = playingElement ? playingElement->tick() : Fraction(-1,1);
      bool marked = (pos >= t1 && pos < t2);
      
      if (marked) {
            textColor = (track() == -1) ? MScore::selectColor[0] : MScore::selectColor[voice()];
            if (!tl->visible()) {
                  int red   = textColor.red();
                  int green = textColor.green();
                  int blue  = textColor.blue();
                  float tint = .6f;  // [0..1]  >=lighter
                  textColor = QColor(red + tint * (255 - red), green + tint * (255 - green), blue + tint * (255 - blue));
                  }
            }

      if (!_text->empty()) {
            _text->setVisible(tl->visible());
            bool isCenteredText = (tl->beginTextPlace() == PlaceText::CENTERED) || (tl->beginTextPlace() == PlaceText::CENTERED_BROKEN);
            auto horizon = isCenteredText ? textLine.center() : _text->ipos();
            auto userOffset = _text->offset();
            auto lineStrokeOffset = QPointF(0.0, _text->getVAlignOffset());
            // Note: TextBase stores angle, yet is unused at the moment. Separate angles in future?
            auto angle  = isCenteredText ? lineAngle : 0.0;

            // Apply user x-y + line width offsets after rotation
            painter->translate(horizon);
            painter->rotate(angle);
            painter->translate(lineStrokeOffset);
            painter->translate(userOffset);

                  _text->draw(painter);

            painter->translate(-userOffset);
            painter->translate(-lineStrokeOffset);
            painter->rotate(-angle);
            painter->translate(-horizon);
            }

      if (!_endText->empty()) {
            _endText->setVisible(tl->visible());
            auto horizon = textLine.p2();
            auto userOffset = _endText->offset();
            auto lineStrokeOffset = QPointF(0.0, _endText->getVAlignOffset());
            auto angle = lineAngle;

            // Apply user x-y + line-width offsets after rotation
            painter->translate(horizon);
            painter->rotate(angle);
            painter->translate(lineStrokeOffset);
            painter->translate(userOffset);

                  _endText->draw(painter);
            
            painter->translate(-userOffset);
            painter->translate(-lineStrokeOffset);
            painter->rotate(-angle);
            painter->translate(-horizon);
            }

      if ((npoints == 0) || (score() && (score()->printing() || !score()->showInvisible()) && !tl->lineVisible()))
            return;

      // color for line (text color comes from the text properties)
#if 0
      QColor color;
      if ((selected() && !(score() && score()->printing())) || !tl->visible() || !tl->lineVisible())
            color = curColor(tl->visible() && tl->lineVisible());
      else
            color = tl->lineColor();
#endif

      if (staff())
            strokeWidth *= mag();
      QPen pen(lineColor, strokeWidth, tl->lineStyle());
      QPen solidPen(lineColor, strokeWidth, Qt::SolidLine);

      // Δ from 3.6.2: MiterJoined Polylines for each segment + round-capped single-lined hooks for pedals
      pen.setCapStyle(Qt::FlatCap);
      solidPen.setCapStyle(Qt::FlatCap);
      solidPen.setJoinStyle(Qt::MiterJoin);
      pen.setJoinStyle(Qt::MiterJoin);
      //Replace generic Qt dash patterns with improved equivalents to show true dots
      QVector<qreal> dotted        = { 0.01, 1.99 }; // 0.01 for cap dots. tighter than default Qt Dotline (would be { 0.01, 2.99 }). 
// QUESTION: should just be 0.01 + 1.99 like dotted:?>    
      QVector<qreal> squared       = { 1.0, 2.00 }; // 0.01 for cap dots. tighter than default Qt Dotline (would be { 0.01, 2.99 }).      
      QVector<qreal> dashed        = { 3.0, 3.0 };   // Compensating for caps. Qt default DashLine is { 4.0, 2.0 }
      QVector<qreal> dashDotted    = { 3.0, 3.0, 0.01, 2.99 };
      QVector<qreal> dashDotDotted = { 3.0, 3.0, 0.01, 2.99, 0.01, 2.99 };
      QVector<qreal> customDashes  = { tl->dashLineLen(), tl->dashGapLen() };

      switch (tl->lineStyle()) {
            case Qt::DashLine:
                pen.setDashPattern(dashed);
                break;
            case Qt::DotLine:
                pen.setDashPattern(dotted);
                pen.setCapStyle(Qt::RoundCap); // round dots
                break;
            case Qt::DashDotLine:
                pen.setDashPattern(dashDotted);
                break;
            case Qt::DashDotDotLine:
                // Square dots instead:
                pen.setDashPattern(squared);
                break;
            case Qt::CustomDashLine:
                pen.setDashPattern(customDashes);
                break;
            case Qt::SolidLine:
                break;
            default:
                  break;
            }

      if (twoLines) {
            // Draw Hairpins
            // Need more precise control over terminal-points:
            pen.setJoinStyle(Qt::BevelJoin);
            painter->setPen(pen);

            if (!joinedHairpin.isEmpty() && tl->lineStyle() == Qt::SolidLine)
                  painter->drawPolyline(joinedHairpin);
            else  // Non solid-lines look better not joined?
                  painter->drawLines(&points[0], 2);
            }
      else {
            int start = 0;
            int end = npoints;
            auto angle = lineAngle;
            auto dx = textLine.p1().x();

            // Centered hooks are always solid
            if (tl->beginHookType() == HookType::HOOK_90T && (isSingleType() || isBeginType())) {
                  painter->setPen(solidPen);
                  painter->save();
                  painter->translate(+dx, 0.0);
                  painter->rotate(angle);
                  painter->translate(-dx, 0.0);

                        painter->drawLine(beginHookLine);

                  painter->rotate(-angle);
                  painter->restore();
                  start++;
                  }
            if (tl->endHookType() == HookType::HOOK_90T && (isSingleType() || isEndType())) {
                  painter->setPen(solidPen);
                  painter->save();
                  painter->translate(+dx, 0.0);
                  painter->rotate(angle);
                  painter->translate(-dx, 0.0);
                  
                        painter->drawLine(endHookLine);

                  painter->rotate(-angle); // superfluous before a restore?
                  painter->restore();
                  end--;
                  }
#if 0 // experiment
            if (tl->beginHookType() == HookType::HOOK_45 && (isSingleType() || isBeginType())) {
                  pen.setCapStyle(Qt::RoundCap);
                  painter->setPen(pen);
                  painter->drawLines(&points[0], 1);
                  start++;
                  }
            if (tl->endHookType() == HookType::HOOK_45 && (isSingleType() || isEndType())) {
                  pen.setCapStyle(Qt::RoundCap);
                  painter->setPen(pen);
                  painter->drawLines(&points[npoints-1], 1);
                  end--;
                  }
#endif
            if (tl->anchor() == Spanner::Anchor::NOTE) {
                  if (tl->startElement() != tl->endElement()) {
                        pen.setCapStyle(Qt::RoundCap);
                        }
                  }
            if (tl->isPedal() || tl->isPedalSegment()) {
                  pen.setCapStyle(Qt::RoundCap);
                  pen.setJoinStyle(Qt::MiterJoin);
                  }

            painter->setPen(pen);

            // Calculate dash-gap and draw hooks if not solid line
            auto adjustedLineLength = lineLength / strokeWidth;
            auto dash = tl->dashLineLen();
            auto gap = tl->dashGapLen();
            int numPairs;
            qreal newGap = 0;
            bool separateHooks = (tl->lineStyle() == Qt::DashDotDotLine) || (tl->lineStyle() == Qt::DotLine);
            bool square = tl->lineStyle() == Qt::DashDotDotLine;

            QVector<qreal> nDashes { dash, newGap };
            if (tl->beginHookType() == HookType::HOOK_45 || tl->beginHookType() == HookType::HOOK_90) {
                  qreal absD = sqrt(QPointF::dotProduct(points[start+1]-points[start], points[start+1]-points[start])) / strokeWidth;
                  numPairs = std::max(qreal(1), absD / (dash + gap));
                  nDashes[1] = (absD - dash * (numPairs + 1)) / numPairs;

                  // Alternative:
                  // if (tl->lineStyle() == Qt::CustomDashLine)
                        // painter->setPen(solidPen);

                  if (separateHooks) {
                        painter->save();
                        painter->translate(+dx, 0.0);
                        painter->rotate(angle);
                        painter->translate(-dx, 0.0);
                        QLineF reversedBeginHookLine(beginHookLine.p2(), beginHookLine.p1());
                        if (square) {
                              reversedBeginHookLine.translate(strokeWidth * 0.5, -strokeWidth * 0.5);
                              }

                        painter->drawLine(reversedBeginHookLine);

                        painter->rotate(-angle);
                        painter->restore();
                        start++;
                        }
                  }
            if (tl->endHookType() == HookType::HOOK_45 || tl->endHookType() == HookType::HOOK_90) {
                  qreal absD = sqrt(QPointF::dotProduct(points[end]-points[end-1], points[end]-points[end-1])) / strokeWidth;
                  numPairs = std::max(qreal(1), absD / (dash + gap));
                  nDashes[1] = (absD - dash * (numPairs + 1)) / numPairs;

                  // if (tl->lineStyle() == Qt::CustomDashLine)
                        // painter->setPen(solidPen);

                  if (separateHooks) {
                        painter->save();
                        painter->translate(+dx, 0.0);
                        painter->rotate(angle);
                        painter->translate(-dx, 0.0);
                        QLineF correctedHook = endHookLine;
                        if (square) {
                              correctedHook.translate(-strokeWidth * 0.5, -strokeWidth * 0.5);
                              }

                        painter->drawLine(correctedHook);

                        painter->rotate(-angle);
                        painter->restore();
                        end--;
                        }
                  }

            numPairs = std::max(qreal(1), adjustedLineLength / (dash + gap));
            nDashes[1] = (adjustedLineLength - dash * (numPairs + 1)) / numPairs;

            if (tl->lineStyle() != Qt::SolidLine) {
                  pen.setDashPattern(nDashes);
                  }
            if (tl->lineStyle() == Qt::CustomDashLine) {
                  // pen.setCapStyle(Qt::FlatCap);
                  painter->setPen(pen);
                  }

            QPolygonF totalLine;
            bool centeredBrokenText = (tl->beginTextPlace() == PlaceText::CENTERED_BROKEN && !_text->empty());
            auto unangledTextLine = textLine;
            unangledTextLine.setAngle(0.0);

            painter->translate(+unangledTextLine.p1());
            painter->rotate(angle);
            painter->translate(-unangledTextLine.p1());

            if (transparentLine && !qFuzzyCompare(strokeWidth, 0.0)) {
                  QRectF r(0.0, -strokeWidth * 0.5, lineLength, strokeWidth);
                  solidPen.setColor(opaqueLineColor);
                  solidPen.setWidth(3);
                  solidPen.setStyle(Qt::SolidLine);
                  painter->setBrush(Qt::NoBrush);
                  painter->setPen(solidPen);

                        painter->drawRect(r);

                  painter->translate(+unangledTextLine.p1());
                  painter->rotate(-angle);
                  painter->translate(-unangledTextLine.p1());
                  }
            else {
                  if (endHookLine.length() && !separateHooks) {
                        if (tl->endHookType() == HookType::HOOK_45 || tl->endHookType() == HookType::HOOK_90)
                              totalLine << endHookLine.p2();
                        }

                  if (centeredBrokenText) {
                        totalLine << brokenPoints[3] << brokenPoints[2];

                        painter->drawPolyline(totalLine);

                        totalLine.clear();
                        totalLine << brokenPoints[1] << brokenPoints[0];
                        }
                  else totalLine << unangledTextLine.p2() << unangledTextLine.p1();

                  if (beginHookLine.length() && !separateHooks) {
                        if (tl->beginHookType() == HookType::HOOK_45 || tl->beginHookType() == HookType::HOOK_90)
                              totalLine << beginHookLine.p1();
                        }

                  painter->drawPolyline(totalLine);

                  painter->translate(+unangledTextLine.p1());
                  painter->rotate(-angle);
                  painter->translate(-unangledTextLine.p1());

                  if (totalLine.isEmpty()) {
                        // Left-over for non-joined polylines. 90T hooks and custom dash lined hooks were already drawn
                        if (tl->endHookType() != HookType::NONE && tl->endHookType() != HookType::HOOK_90T && tl->lineStyle() != Qt::CustomDashLine) {
                              if (endHookLine.length()) {
                                    painter->rotate(angle);
                                    painter->drawLine(endHookLine);
                                    painter->rotate(-angle);
                                    }
                              }
                        QLineF reverseLine(textLine.p2(), textLine.p1());
                        if (tl->lineStyle() == Qt::DashDotDotLine || tl->lineStyle() == Qt::DotLine)
                              painter->drawLine(reverseLine);
                        else painter->drawLine(textLine);
                        if (tl->beginHookType() != HookType::NONE && tl->beginHookType() != HookType::HOOK_90T && tl->lineStyle() != Qt::CustomDashLine) {
                              if (beginHookLine.length()) {
                                    painter->rotate(angle);
                                    painter->drawLine(beginHookLine);
                                    painter->rotate(-angle);
                                    }
                              }
                        }
                  }

            }

      }

//---------------------------------------------------------
//   shape
//---------------------------------------------------------

Shape TextLineBaseSegment::shape() const
      {
      Shape shape;
      auto  tl = textLineBase();
      qreal lw  = textLineBase()->lineWidth();
      qreal lw2 = (lw * 0.5);

      if (!_text->empty())
            shape.add(_text->bbox());
      if (!_endText->empty())
            shape.add(_endText->bbox());

      if (twoLines) {
            // Hairpins:
            shape.add(QRectF(points[0].x(), points[0].y() - lw2,
               points[1].x() - points[0].x(), points[1].y() - points[0].y() + lw));
            shape.add(QRectF(points[2].x(), points[2].y() - lw2,
               points[3].x() - points[2].x(), points[3].y() - points[2].y() + lw));
            }
      else if (tl->lineVisible()) {
            if (!beginHookLine.isNull())
                  shape.add(beginHookBB);
            if (!textLine.isNull())
                  shape.add(lineBB);
            if (!endHookLine.isNull())
                  shape.add(endHookBB);
            }
      return shape;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void TextLineBaseSegment::layout()
      {

      // Lazy Functor Helpers: Form bounding box from line with "margins" (e.g. line stroke width)
      auto formBoundingBox = [](const QLineF& line, qreal xo, qreal yo, qreal wo, qreal ho) {
            qreal x = line.x1() + xo;
            qreal y = line.y1() + yo;
            qreal w = line.dx() + wo;
            qreal h = line.dy() + ho;
            return QRectF(x, y, w, h);
            };

      // Would be nice to have rectangle and point in one function, but for that would need templates
      auto rotateRectangle = [](auto& in, QPointF t1, qreal angle, QPointF t2) {
            QPolygonF polygonInput(in);
            auto theta = -(angle);
            QTransform transform = QTransform()
                  .translate(t1.x(), t1.y())
                  .rotate(theta)
                  .translate(t2.x(), t2.y())
                  ;
            QPolygonF rotatedPolygon = transform.map(polygonInput);
            return rotatedPolygon.boundingRect();
            };

//      auto rotatePoint = [](QPointF in, QPointF t1, qreal angle, QPointF t2) {
//            auto theta = -(angle);
//            QTransform transform = QTransform()
//                  .translate(t1.x(), t1.y())
//                  .rotate(theta)
//                  .translate(t2.x(), t2.y())
//                  ;
//            return transform.map(in);
//            };

      npoints      = 0;
      TextLineBase* tl = textLineBase();
      qreal lineStrokeWidth = tl->lineWidth();
      QColor textColor = tl->color();
      qreal _spatium = tl->spatium();
      bool isSingleOrBegin = isSingleBeginType();

      bool singleNoteAnchor = (tl->startElement() == tl->endElement());
      auto beginHookType = tl->beginHookType();
      auto endHookType = tl->endHookType();
      bool hasBeginHook = beginHookType != HookType::NONE;
      bool hasEndHook = endHookType != HookType::NONE;
      bool hasNoHooks = !hasBeginHook && !hasEndHook;
      bool isCenteredText = (tl->beginTextPlace() == PlaceText::CENTERED || tl->beginTextPlace() == PlaceText::CENTERED_BROKEN);
      bool isBroken       = (tl->beginTextPlace() == PlaceText::CENTERED_BROKEN);

      // Reset lines
      if (!beginHookLine.isNull())
            beginHookLine.setLine(0.0, 0.0, 0.0, 0.0);

      if (!endHookLine.isNull())
            endHookLine.setLine(0.0, 0.0, 0.0, 0.0);

      if (!textLine.isNull())
            textLine.setLine(0.0, 0.0, 0.0, 0.0);

      if (spanner()->placeBelow())
            rypos() = staff() ? staff()->height() : 0.0;

      // adjust Y pos to staffType offset
      if (staffType())
            rypos() += staffType()->yoffset().val() * spatium();

      if (!tl->diagonal())
            _offset2.setY(0);

      // Initialization of line information:
      QPointF pp1;
      QPointF pp2(pos2());
      bool isHorizontallyBackwards = pp1.x() > pp2.x();
      bool isDiagonal = (pp2.y() != 0);
      bool hasNoText = _text->empty() && _endText->empty();
      textLine.setPoints(pp1, pp2);
      lineLength = textLine.length();

      // Bounding-box: first phase
      setbbox(QRectF(pp1, pp2).normalized());

      if (isSingleOrBegin) {
            _text->setXmlText(tl->beginText());
            _text->setFamily(tl->beginFontFamily());
            _text->setSize(tl->beginFontSize());
            _text->setOffset(tl->beginTextOffset() * mag());
            _text->setAlign(tl->beginTextAlign());
            _text->setBold(tl->beginFontStyle() & FontStyle::Bold);
            _text->setItalic(tl->beginFontStyle() & FontStyle::Italic);
            _text->setUnderline(tl->beginFontStyle() & FontStyle::Underline);
            _text->setStrike(tl->beginFontStyle() & FontStyle::Strike);
            }
      else {
            _text->setXmlText(tl->continueText());
            _text->setFamily(tl->continueFontFamily());
            _text->setSize(tl->continueFontSize());
            _text->setOffset(tl->continueTextOffset() * mag());
            _text->setAlign(tl->continueTextAlign());
            _text->setBold(tl->continueFontStyle() & FontStyle::Bold);
            _text->setItalic(tl->continueFontStyle() & FontStyle::Italic);
            _text->setUnderline(tl->continueFontStyle() & FontStyle::Underline);
            _text->setStrike(tl->continueFontStyle() & FontStyle::Strike);
            }

      _text->setPlacement(Placement::ABOVE);
      _text->setTrack(track());
      _text->setVAlignOffset(lineStrokeWidth);
      _text->setAngle(isCenteredText ? textLine.angle() : 0.0);
      _text->setColor(textColor);
      _text->layout();

      if ((isSingleType() || isEndType())) {
            _endText->setXmlText(tl->endText());
            _endText->setFamily(tl->endFontFamily());
            _endText->setSize(tl->endFontSize());
            _endText->setOffset(tl->endTextOffset());
            _endText->setAlign(tl->endTextAlign());
            _endText->setBold(tl->endFontStyle() & FontStyle::Bold);
            _endText->setItalic(tl->endFontStyle() & FontStyle::Italic);
            _endText->setUnderline(tl->endFontStyle() & FontStyle::Underline);
            _endText->setStrike(tl->endFontStyle() & FontStyle::Strike);
            _endText->setPlacement(Placement::ABOVE);
            _endText->setTrack(track());
            _endText->setVAlignOffset(lineStrokeWidth);
            _endText->setAngle(textLine.angle());
            _endText->setColor(textColor);
            _endText->layout();
            }
      else {
            _endText->setXmlText("");
            }

      if (isDiagonal && hasNoText && hasNoHooks && !singleNoteAnchor) {
            npoints = 1;
            qreal w = abs(bbox().width());
            qreal h = abs(bbox().height());
            bool zeroWidth  = (w < 0.01);
            bool zeroHeight = (h < 0.01);
            w = zeroWidth  ? lineStrokeWidth : w;
            h = zeroHeight ? lineStrokeWidth : h;
            bbox().setWidth(w);
            bbox().setHeight(h);
            lineBB = bbox();
            return;
            }

      // Calculate dimensions of bounding-box based on initial text positioning and hooks [[or is not diagonal]]
      qreal x1 = qMin(0.0, pp2.x());
      qreal x2 = qMax(0.0, pp2.x());
      qreal y0 = -lineStrokeWidth;
      qreal y1 = qMin(0.0, pp2.y()) + y0;
      qreal y2 = qMax(0.0, pp2.y()) - y0;

      qreal l = 0.0;

      if (!_text->empty()) {
            qreal textlineTextDistance = _spatium * 0.5;
            if (((isSingleType() || isBeginType())
               && (tl->beginTextPlace() == PlaceText::LEFT || tl->beginTextPlace() == PlaceText::AUTO))
               || ((isMiddleType() || isEndType()) && (tl->continueTextPlace() == PlaceText::LEFT))) {
                  l = _text->pos().x() + _text->bbox().width() + textlineTextDistance;
                  }
            qreal h = _text->height();
            if (tl->beginTextPlace() == PlaceText::ABOVE)
                  y1 = qMin(y1, -h);
            else if (tl->beginTextPlace() == PlaceText::BELOW)
                  y2 = qMax(y2, h);
            else {
                  y1 = qMin(y1, -h * .5);
                  y2 = qMax(y2, h * .5);
                  }
            x2 = qMax(x2, _text->width());
            }

      if (hasEndHook) {
            qreal h = pp2.y() + tl->endHookHeight().val() * _spatium;
            if (h > y2)
                  y2 = h;
            else if (h < y1)
                  y1 = h;
            }

      if (hasBeginHook) {
            qreal h = tl->beginHookHeight().val() * _spatium;
            if (h > y2)
                  y2 = h;
            else if (h < y1)
                  y1 = h;
            }

      // Bounding-box: second phase (accommodate for potential zero width/height [horizontal/vertical line])
      qreal w = abs(x2 - x1);
      qreal h = abs(y2 - y1);
      bool zeroWidth  = (w < 0.01);
      bool zeroHeight = (h < 0.01);
      w = zeroWidth  ? lineStrokeWidth : w;
      h = zeroHeight ? lineStrokeWidth : h;

      // Convert "close-enough" verticality to absolute:
      bool isVertical = abs(textLine.dx()) < 1;
      if (isVertical)
            pp2.setX(pp1.x());

      // Bounding-box before further textual positioning:
      bbox().setRect(x1, y1, w, h);

      // Hook handling
      qreal beginHookWidth = 0.0;
      qreal endHookWidth   = 0.0;
      if (tl->lineVisible() || !score()->printing()) {
            pp1 = QPointF(l, 0.0);

            // 45-degree hooks slightly shorten textline for angle-width accommodation
            if (beginHookType == HookType::HOOK_45) {
                  beginHookWidth = fabs(tl->beginHookHeight().val() * _spatium * .4);
                  pp1.rx() += beginHookWidth;
                  textLine.setLine(pp1.x(), pp1.y(), pp2.x(), pp2.y());
                  }
            else beginHookWidth = 0;
            if (endHookType == HookType::HOOK_45) {
                  endHookWidth = fabs(tl->endHookHeight().val() * _spatium * .4);
                  pp2.rx() -= endHookWidth;
                  textLine.setLine(pp1.x(), pp1.y(), pp2.x(), pp2.y());
                  }
            else endHookWidth = 0;

            textLine.setP1(pp1);

            QPointF hookP1, hookP2;
            if (hasBeginHook && (isSingleType() || isBeginType())) {
                  qreal beginHookHeight = tl->beginHookHeight().val() * _spatium;
                  qreal x1 = pp1.x() - beginHookWidth;
                  qreal y1 = pp1.y() + beginHookHeight;
                  qreal x2 = pp1.x();
                  qreal y2 = (beginHookType == HookType::HOOK_90T) ? (pp1.y() - beginHookHeight) : pp1.y();
                  beginHookLine.setPoints(QPointF(x1, y1), QPointF(x2, y2));

                  if (beginHookType == HookType::HOOK_90T)
                        points[npoints++] = QPointF(x1, y1);
                  else  points[npoints++] = QPointF(x2, y2);

                  points[npoints] = pp1;
                  }

            // don't draw backwards lines (or hooks) if text is longer than nominal line length
            bool backwardsWithText = isHorizontallyBackwards && !_text->empty() && !isDiagonal;
            if (!backwardsWithText) {
                  points[npoints++] = pp1;
                  points[npoints]   = pp2;

                  textLine.setPoints(pp1, pp2);
                  lineLength = sqrt(QPointF::dotProduct(pp2-pp1, pp2-pp1));

                  if (hasEndHook && (isSingleType() || isEndType())) {
                        ++npoints;
                        qreal endHookHeight = tl->endHookHeight().val() * _spatium;
                        qreal x1 = textLine.length() + beginHookWidth + l;
                        // Rotation will later handle y-positioning
                        qreal y1 = (endHookType == HookType::HOOK_90T) ? (0.0 + endHookHeight) : 0.0;
                        qreal x2 = x1 + endHookWidth;
                        qreal y2 = (endHookType == HookType::HOOK_90T) ? (0.0 - endHookHeight) : (0.0 + endHookHeight);
                        endHookLine.setPoints(QPointF(x1, y1), QPointF(x2, y2));

                        points[npoints++] = QPointF(x1, y1);
                        if (tl->endHookType() == HookType::HOOK_90T)
                              points[++npoints] = hookP2;
                        }
                  }

            }

      if (!_text->empty()) {
            qreal xo = 0.0;
            qreal yo = 0.0;
            auto bb = _text->bbox();
            if (isCenteredText) {
                  xo = textLine.center().x() + _text->rxoffset();
                  yo = textLine.center().y() + _text->ryoffset();
                  if (isBroken) {
                        // Compute broken lines
                        //   Let painter rotate the broken-lines after forming
                        //   the points based on line length without reference to angle
                        QLineF horizontalTextLine(textLine);
                        horizontalTextLine.setAngle(0.0);
                        auto xUserOffset          = _text->offset().x();
                        auto textWidth            = _text->bbox().width();
                        auto lineWidth            = horizontalTextLine.length();
                        auto xCenter              = horizontalTextLine.center().x();

                        auto textLeftEdge         = xCenter - (textWidth * 0.5) - lineStrokeWidth - beginHookWidth + xUserOffset - (spatium() * 0.5);
                        auto textRightEdge        = xCenter + (textWidth * 0.5) + lineStrokeWidth - beginHookWidth + xUserOffset + (spatium() * 0.5);

                        auto leftProportion       = textLeftEdge  / lineWidth;
                        auto rightProportion      = textRightEdge / lineWidth;

                        auto beginEdgePoint = horizontalTextLine.pointAt(leftProportion);
                        auto endEdgePoint   = horizontalTextLine.pointAt(rightProportion);

                        brokenPoints[0] = horizontalTextLine.p1();
                        brokenPoints[1] = beginEdgePoint;
                        brokenPoints[2] = endEdgePoint;
                        brokenPoints[3] = horizontalTextLine.p2();
                        }
                  }
            _text->setPos(xo, yo);
            if (isCenteredText) {
                  auto aboutSelf = QPointF(0,0);
                  auto translate = _text->offset();
                  auto angle = textLine.angle();

                  // Z-Axis rotation, then apply user-offsets
                  bb = rotateRectangle(bb, aboutSelf, angle, translate);
                  bb.translate(textLine.center());
                  }
            else {
                  bb = _text->bbox().translated(_text->pos());
                  }
            _text->setbbox(bb);
            bbox() |= bb;
            }

      if (!_endText->empty()) {
            auto xo = bbox().right();
            auto yo = _endText->getVAlignOffset();
            auto bb = _endText->bbox();
            auto angle = textLine.angle();
            auto aboutSelf = QPointF(0,0);
            auto translate = _endText->offset();
            translate.rx() += (lineStrokeWidth * 0.5);

            // Z-Axis rotation, then apply user-offsets
            bb = rotateRectangle(bb, aboutSelf, angle, translate);
            bb.translate(textLine.p2());

            _endText->setPos(xo, yo);
            _endText->setbbox(bb);
            bbox() |= bb;
            }

      // MS3.6 didn't incorporate hooks or prepare for rotated objects into the main bounding-box.
      // Calculate rotation and position here, then store the results so that Shape() can add them easily:
      if (tl->lineVisible()) {
            auto angle = textLine.angle();
            auto translate = QPointF(textLine.p1().x(), 0.0);
            auto lw = lineStrokeWidth;
            auto lw2 = (lw * 0.5);
            if (!textLine.isNull()) {
                  QLineF flatLine = textLine;
                  flatLine.setAngle(0.0);
                  lineBB = formBoundingBox(flatLine, -lw2, -lw2, +lw, +lw);
                  auto translate = flatLine.p1();
                  lineBB = rotateRectangle(lineBB, translate, angle, -translate);

                  bbox() |= lineBB;
                  }
            if (!beginHookLine.isNull()) {
                  auto foeD5 = (tl->beginHookType() == HookType::HOOK_45);
                  QLineF swappedLine(beginHookLine.p1(), beginHookLine.p2());
                  swappedLine.setAngle(swappedLine.angle() + angle);
                  beginHookBB = formBoundingBox(swappedLine,
                                                foeD5 ? +lw2 : -lw2,
                                                foeD5 ? -lw2 : -lw2,
                                                foeD5 ? -lw  : +lw,
                                                foeD5 ? +lw  : +lw);
                  // If swappedLine's angle wasn't updated, then:
                  //    beginHookBB = rotateRectangle(beginHookBB, translate, angle, -translate);

                  bbox() |= beginHookBB;
                  }
            if (!endHookLine.isNull()) {
                  // Lacks "best practice" for 45's bounding-box, but works well enough:
                  auto fowtyFive = (tl->endHookType() == HookType::HOOK_45);
                  QLineF swappedLine(endHookLine.p2(), endHookLine.p1());
                  endHookBB = formBoundingBox(fowtyFive ? endHookLine : swappedLine, -lw2, -lw2, +lw, +lw);
                  endHookBB = rotateRectangle(endHookBB, translate, angle, -translate);

                  bbox() |= endHookBB;
                  }
            }
      }

//---------------------------------------------------------
//   spatiumChanged
//---------------------------------------------------------

void TextLineBaseSegment::spatiumChanged(qreal ov, qreal nv)
      {
      LineSegment::spatiumChanged(ov, nv);

      textLineBase()->spatiumChanged(ov, nv);
      _text->spatiumChanged(ov, nv);
      _endText->spatiumChanged(ov, nv);
      }

static constexpr std::array<Pid, 27> pids = { {
      Pid::LINE_VISIBLE,
      Pid::BEGIN_HOOK_TYPE,
      Pid::BEGIN_HOOK_HEIGHT,
      Pid::END_HOOK_TYPE,
      Pid::END_HOOK_HEIGHT,
      Pid::BEGIN_TEXT,
      Pid::BEGIN_TEXT_ALIGN,
      Pid::BEGIN_TEXT_PLACE,
      Pid::BEGIN_FONT_FACE,
      Pid::BEGIN_FONT_SIZE,
      Pid::BEGIN_FONT_STYLE,
      Pid::BEGIN_TEXT_OFFSET,
      Pid::CONTINUE_TEXT,
      Pid::CONTINUE_TEXT_ALIGN,
      Pid::CONTINUE_TEXT_PLACE,
      Pid::CONTINUE_FONT_FACE,
      Pid::CONTINUE_FONT_SIZE,
      Pid::CONTINUE_FONT_STYLE,
      Pid::CONTINUE_TEXT_OFFSET,
      Pid::END_TEXT,
      Pid::END_TEXT_ALIGN,
      Pid::END_TEXT_PLACE,
      Pid::END_FONT_FACE,
      Pid::END_FONT_SIZE,
      Pid::END_FONT_STYLE,
      Pid::END_TEXT_OFFSET,
      Pid::EN_PASSANT_MANIFEST,
      } };

//---------------------------------------------------------
//   propertyDelegate
//---------------------------------------------------------

Element* TextLineBaseSegment::propertyDelegate(Pid pid)
      {
      for (Pid id : pids) {
            if (pid == id)
                  return spanner();
            }
      return LineSegment::propertyDelegate(pid);
      }

//---------------------------------------------------------
//   TextLineBase
//---------------------------------------------------------

TextLineBase::TextLineBase(Score* s, ElementFlags f)
   : SLine(s, f)
      {
      setBeginHookHeight(Spatium(1.9));
      setEndHookHeight(Spatium(1.9));
      setEnPassantManifest(false);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void TextLineBase::write(XmlWriter& xml) const
      {
      if (!xml.canWrite(this))
            return;
      xml.stag(this);
      writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void TextLineBase::read(XmlReader& e)
      {
      eraseSpannerSegments();

      if (score()->mscVersion() < 301)
            e.addSpanner(e.intAttribute("id", -1), this);

      while (e.readNextStartElement()) {
            if (!readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   spatiumChanged
//---------------------------------------------------------

void TextLineBase::spatiumChanged(qreal /*ov*/, qreal /*nv*/)
      {
      }

//---------------------------------------------------------
//   writeProperties
//    write properties different from prototype
//---------------------------------------------------------

void TextLineBase::writeProperties(XmlWriter& xml) const
      {
      for (Pid pid : pids) {
            if (!isStyled(pid)) {
                  if ((pid == Pid::EN_PASSANT_MANIFEST) && MScore::testMode) {/*Skip during testing*/}
                  else writeProperty(xml, pid);
                  }
            }
      SLine::writeProperties(xml);
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool TextLineBase::readProperties(XmlReader& e)
      {
      const QStringRef& tag(e.name());
      for (Pid i : pids) {
            if (readProperty(tag, e, i)) {
                  setPropertyFlags(i, PropertyFlags::UNSTYLED);
                  return true;
                  }
            }
      return SLine::readProperties(e);
      }

//---------------------------------------------------------
//   TextLineBase::propertyId
//---------------------------------------------------------

Pid TextLineBase::propertyId(const QStringRef& name) const
      {
      for (Pid pid : pids) {
            if (propertyName(pid) == name)
                  return pid;
            }
      return SLine::propertyId(name);
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant TextLineBase::getProperty(Pid id) const
      {
      switch (id) {
            case Pid::BEGIN_TEXT:
                  return beginText();
            case Pid::BEGIN_TEXT_ALIGN:
                  return QVariant::fromValue(beginTextAlign());
            case Pid::CONTINUE_TEXT_ALIGN:
                  return QVariant::fromValue(continueTextAlign());
            case Pid::END_TEXT_ALIGN:
                  return QVariant::fromValue(endTextAlign());
            case Pid::BEGIN_TEXT_PLACE:
                  return int(_beginTextPlace);
            case Pid::BEGIN_HOOK_TYPE:
                  return int(_beginHookType);
            case Pid::BEGIN_HOOK_HEIGHT:
                  return _beginHookHeight;
            case Pid::BEGIN_FONT_FACE:
                  return _beginFontFamily;
            case Pid::BEGIN_FONT_SIZE:
                  return _beginFontSize;
            case Pid::BEGIN_FONT_STYLE:
                  return int(_beginFontStyle);
            case Pid::BEGIN_TEXT_OFFSET:
                  return _beginTextOffset;
            case Pid::CONTINUE_TEXT:
                  return continueText();
            case Pid::CONTINUE_TEXT_PLACE:
                  return int(_continueTextPlace);
            case Pid::CONTINUE_FONT_FACE:
                  return _continueFontFamily;
            case Pid::CONTINUE_FONT_SIZE:
                  return _continueFontSize;
            case Pid::CONTINUE_FONT_STYLE:
                  return int(_continueFontStyle);
            case Pid::CONTINUE_TEXT_OFFSET:
                  return _continueTextOffset;
            case Pid::END_TEXT:
                  return endText();
            case Pid::END_TEXT_PLACE:
                  return int(_endTextPlace);
            case Pid::END_HOOK_TYPE:
                  return int(_endHookType);
            case Pid::END_HOOK_HEIGHT:
                  return _endHookHeight;
            case Pid::END_FONT_FACE:
                  return _endFontFamily;
            case Pid::END_FONT_SIZE:
                  return _endFontSize;
            case Pid::END_FONT_STYLE:
                  return int(_endFontStyle);
            case Pid::END_TEXT_OFFSET:
                  return _endTextOffset;
            case Pid::LINE_VISIBLE:
                  return lineVisible();
            case Pid::EN_PASSANT_MANIFEST:
                  return enPassantManifest();
            default:
                  return SLine::getProperty(id);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool TextLineBase::setProperty(Pid id, const QVariant& v)
      {
      switch (id) {
            case Pid::BEGIN_TEXT_PLACE:
                  _beginTextPlace = PlaceText(v.toInt());
                  break;
            case Pid::BEGIN_TEXT_ALIGN:
                  _beginTextAlign = v.value<Align>();
                  break;
            case Pid::CONTINUE_TEXT_ALIGN:
                  _continueTextAlign = v.value<Align>();
                  break;
            case Pid::END_TEXT_ALIGN:
                  _endTextAlign = v.value<Align>();
                  break;
            case Pid::CONTINUE_TEXT_PLACE:
                  _continueTextPlace = PlaceText(v.toInt());
                  break;
            case Pid::END_TEXT_PLACE:
                  _endTextPlace = PlaceText(v.toInt());
                  break;
            case Pid::BEGIN_HOOK_HEIGHT:
                  _beginHookHeight = v.value<Spatium>();
                  break;
            case Pid::END_HOOK_HEIGHT:
                  _endHookHeight = v.value<Spatium>();
                  break;
            case Pid::BEGIN_HOOK_TYPE:
                  _beginHookType = HookType(v.toInt());
                  break;
            case Pid::END_HOOK_TYPE:
                  _endHookType = HookType(v.toInt());
                  break;
            case Pid::BEGIN_TEXT:
                  setBeginText(v.toString());
                  break;
            case Pid::BEGIN_TEXT_OFFSET:
                  setBeginTextOffset(v.toPointF());
                  break;
            case Pid::CONTINUE_TEXT_OFFSET:
                  setContinueTextOffset(v.toPointF());
                  break;
            case Pid::END_TEXT_OFFSET:
                  setEndTextOffset(v.toPointF());
                  break;
            case Pid::CONTINUE_TEXT:
                  setContinueText(v.toString());
                  break;
            case Pid::END_TEXT:
                  setEndText(v.toString());
                  break;
            case Pid::LINE_VISIBLE:
                  setLineVisible(v.toBool());
                  break;
            case Pid::BEGIN_FONT_FACE:
                  setBeginFontFamily(v.toString());
                  break;
            case Pid::BEGIN_FONT_SIZE:
                  if (v.toReal() <= 0)
                        qFatal("font size is %f", v.toReal());
                  setBeginFontSize(v.toReal());
                  break;
            case Pid::BEGIN_FONT_STYLE:
                  setBeginFontStyle(FontStyle(v.toInt()));
                  break;
            case Pid::CONTINUE_FONT_FACE:
                  setContinueFontFamily(v.toString());
                  break;
            case Pid::CONTINUE_FONT_SIZE:
                  setContinueFontSize(v.toReal());
                  break;
            case Pid::CONTINUE_FONT_STYLE:
                  setContinueFontStyle(FontStyle(v.toInt()));
                  break;
            case Pid::END_FONT_FACE:
                  setEndFontFamily(v.toString());
                  break;
            case Pid::END_FONT_SIZE:
                  setEndFontSize(v.toReal());
                  break;
            case Pid::END_FONT_STYLE:
                  setEndFontStyle(FontStyle(v.toInt()));
                  break;
            case Pid::EN_PASSANT_MANIFEST:
                  setEnPassantManifest(v.toBool());
                  break;
            default:
                  return SLine::setProperty(id, v);
            }
      triggerLayout();
      return true;
      }

 }

