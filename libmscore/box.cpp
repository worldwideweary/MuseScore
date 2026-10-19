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

//#include "barline.h"
#include "box.h"
#include "fret.h"
#include "chordlist.h"
#include "harmony.h"
#include "icon.h"
#include "image.h"
#include "layoutbreak.h"
#include "measure.h"
#include "segment.h"
#include <algorithm>
#include <iterator>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "mscore.h"
#include "pitchspelling.h"
#include "repeat.h"
#include "score.h"
#include "stafftext.h"
#include "symbol.h"
#include "system.h"
#include "text.h"
#include "textframe.h"
#include "undo.h"
#include "xml.h"

namespace Ms {

static const ElementStyle boxStyle {
      { Sid::systemFrameDistance,                Pid::TOP_GAP                 },
      { Sid::frameSystemDistance,                Pid::BOTTOM_GAP              },
      { Sid::paddingToNotationAbove,             Pid::PADDING_TO_NOTATION_ABOVE },
      { Sid::paddingToNotationBelow,             Pid::PADDING_TO_NOTATION_BELOW },
      };

static const ElementStyle hBoxStyle {
      };

//---------------------------------------------------------
//   Box
//---------------------------------------------------------

Box::Box(Score* score)
   : MeasureBase(score)
      {
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Box::layout()
      {
      MeasureBase::layout();
      for (Element* e : el()) {
            if (!e->isLayoutBreak())
                  e->layout();
            }
      }

//---------------------------------------------------------
//   computeMinWidth
//---------------------------------------------------------

void HBox::computeMinWidth()
      {
      setWidth(point(boxWidth()) + topGap() + bottomGap());  // top/bottom is really left/right
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Box::draw(QPainter* painter) const
      {
      if (score() && score()->printing())
            return;
      if (selected() || editMode || dropTarget() || score()->showFrames()) {
            qreal w = spatium() * .15;
            QPainterPathStroker stroker;
            stroker.setWidth(w);
            stroker.setJoinStyle(Qt::MiterJoin);
            stroker.setCapStyle(Qt::SquareCap);

            QVector<qreal> dashes ;
            dashes.append(1);
            dashes.append(3);
            stroker.setDashPattern(dashes);
            QPainterPath path;
            w *= .5;
            path.addRect(bbox().adjusted(w, w, -w, -w));
            QPainterPath stroke = stroker.createStroke(path);
            painter->setBrush(Qt::NoBrush);
            painter->fillPath(stroke, (selected() || editMode || dropTarget()) ? MScore::selectColor[0] : MScore::frameMarginColor);
            }
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Box::startEdit(EditData& ed)
      {
      Element::startEdit(ed);
      editMode   = true;
      }

//---------------------------------------------------------
//   edit
//---------------------------------------------------------

bool Box::edit(EditData&)
      {
      return false;
      }

//---------------------------------------------------------
//   startEditDrag
//---------------------------------------------------------

void Box::startEditDrag(EditData& ed)
      {
      ElementEditData* eed = ed.getData(this);
      if (isHBox())
            eed->pushProperty(Pid::BOX_WIDTH);
      else
            eed->pushProperty(Pid::BOX_HEIGHT);
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void Box::editDrag(EditData& ed)
      {
      if (isVBox()) {
            _boxHeight += Spatium(ed.delta.y() / spatium());
            if (ed.vRaster) {
                  qreal vRaster = 1.0 / MScore::vRaster();
                  int n = (int)lrint(_boxHeight.val() / vRaster);
                  _boxHeight = Spatium(vRaster * n);
                  }
            bbox().setRect(0.0, 0.0, system()->width(), point(boxHeight()));
            system()->setHeight(height());
            triggerLayout();
            }
      else {
            _boxWidth += Spatium(ed.delta.x() / spatium());
            if (ed.hRaster) {
                  qreal hRaster = 1.0 / MScore::hRaster();
                  int n = (int)lrint(_boxWidth.val() / hRaster);
                  _boxWidth = Spatium(hRaster * n);
                  }
            triggerLayout();
            }
      layout();
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Box::endEdit(EditData&)
      {
      editMode = false;
      layout();
      }

//---------------------------------------------------------
//   gripsPositions
//---------------------------------------------------------

std::vector<QPointF> HBox::gripsPositions(const EditData&) const
      {
      QRectF r(abbox());
      return { QPointF(r.right(), r.top() + r.height() * .5) };
      }

std::vector<QPointF> VBox::gripsPositions(const EditData&) const
      {
      QRectF r(abbox());
      return { QPointF(r.x() + r.width() * .5, r.bottom()) };
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Box::write(XmlWriter& xml) const
      {
      xml.stag(this);
      writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void Box::writeProperties(XmlWriter& xml) const
      {
      for (Pid id : {
         Pid::BOX_HEIGHT, Pid::BOX_WIDTH, Pid::TOP_GAP, Pid::BOTTOM_GAP,
         Pid::LEFT_MARGIN, Pid::RIGHT_MARGIN, Pid::TOP_MARGIN, Pid::BOTTOM_MARGIN, Pid::BOX_AUTOSIZE,
         Pid::PADDING_TO_NOTATION_ABOVE, Pid::PADDING_TO_NOTATION_BELOW }) {
            writeProperty(xml, id);
            }
      Element::writeProperties(xml);
      for (const Element* e : el())
            e->write(xml);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Box::read(XmlReader& e)
      {
      _leftMargin      = 0.0;
      _rightMargin     = 0.0;
      _topMargin       = 0.0;
      _bottomMargin    = 0.0;
      _boxHeight       = Spatium(0);     // override default set in constructor
      _boxWidth        = Spatium(0);
      MeasureBase::read(e);
      if (score()->mscVersion() < 302)
            _isAutoSizeEnabled = false; // disable auto-size for older scores by default.
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool Box::readProperties(XmlReader& e)
      {
      const QStringRef& tag(e.name());
      if (readProperty(tag, e, Pid::PADDING_TO_NOTATION_ABOVE)
            || readProperty(tag, e, Pid::PADDING_TO_NOTATION_BELOW))
            return true;
      if (tag == "height")
            _boxHeight = Spatium(e.readDouble());
      else if (tag == "width")
            _boxWidth = Spatium(e.readDouble());
      else if (tag == "topGap") {
            _topGap = e.readDouble();
            if (score()->mscVersion() >= 206)
                  _topGap *= score()->spatium();
            setPropertyFlags(Pid::TOP_GAP, PropertyFlags::UNSTYLED);
            }
      else if (tag == "bottomGap") {
            _bottomGap = e.readDouble();
             if (score()->mscVersion() >= 206)
                  _bottomGap *= score()->spatium();
            setPropertyFlags(Pid::BOTTOM_GAP, PropertyFlags::UNSTYLED);
            }
      else if (tag == "leftMargin")
            _leftMargin = e.readDouble();
      else if (tag == "rightMargin")
            _rightMargin = e.readDouble();
      else if (tag == "topMargin")
            _topMargin = e.readDouble();
      else if (tag == "bottomMargin")
            _bottomMargin = e.readDouble();
      else if (tag == "boxAutoSize")
            _isAutoSizeEnabled = e.readBool();
      else if (tag == "Text") {
            Text* t;
            if (isTBox()) {
                  t = toTBox(this)->text();
                  t->read(e);
                  }
            else {
                  t = new Text(score());
                  t->read(e);
                  if (t->empty())
                        qDebug("read empty text");
                  else
                        add(t);
                  }
            }
      else if (tag == "Symbol") {
            Symbol* s = new Symbol(score());
            s->read(e);
            add(s);
            }
      else if (tag == "Image") {
            if (MScore::noImages)
                  e.skipCurrentElement();
            else {
                  Image* image = new Image(score());
                  image->setTrack(e.track());
                  image->read(e);
                  add(image);
                  }
            }
      else if (tag == "FretDiagram") {
            FretDiagram* f = new FretDiagram(score());
            f->read(e);
            add(f);
            }
      else if (tag == "HBox") {
            HBox* hb = new HBox(score());
            hb->read(e);
            add(hb);
            }
      else if (tag == "VBox") {
            VBox* vb = new VBox(score());
            vb->read(e);
            add(vb);
            }
      else if (MeasureBase::readProperties(e))
            ;
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   add
///   Add new Element \a el to Box
//---------------------------------------------------------

void Box::add(Element* e)
      {
      if (e->isText())
            toText(e)->setLayoutToParentWidth(true);
      MeasureBase::add(e);
      }

QRectF Box::contentRect() const
      {
      QRectF result;

      for (const Element* element : el())
            result = result.united(element->bbox());

      return result;
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Box::getProperty(Pid propertyId) const
      {
      switch(propertyId) {
            case Pid::PADDING_TO_NOTATION_ABOVE:
                  return QVariant::fromValue(_paddingToNotationAbove);
            case Pid::PADDING_TO_NOTATION_BELOW:
                  return QVariant::fromValue(_paddingToNotationBelow);
            case Pid::BOX_HEIGHT:
                  return _boxHeight;
            case Pid::BOX_WIDTH:
                  return _boxWidth;
            case Pid::TOP_GAP:
                  return _topGap;
            case Pid::BOTTOM_GAP:
                  return _bottomGap;
            case Pid::LEFT_MARGIN:
                  return _leftMargin;
            case Pid::RIGHT_MARGIN:
                  return _rightMargin;
            case Pid::TOP_MARGIN:
                  return _topMargin;
            case Pid::BOTTOM_MARGIN:
                  return _bottomMargin;
            case Pid::BOX_AUTOSIZE:
                  return (score()->mscVersion() >= 302) ? _isAutoSizeEnabled : false;
            default:
                  return MeasureBase::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Box::setProperty(Pid propertyId, const QVariant& v)
      {
      score()->addRefresh(canvasBoundingRect());
      switch (propertyId) {
            case Pid::PADDING_TO_NOTATION_ABOVE:
                  _paddingToNotationAbove = Spatium(qMax(0.0, v.value<Spatium>().val()));
                  break;
            case Pid::PADDING_TO_NOTATION_BELOW:
                  _paddingToNotationBelow = Spatium(qMax(0.0, v.value<Spatium>().val()));
                  break;
            case Pid::BOX_HEIGHT:
                  _boxHeight = v.value<Spatium>();
                  break;
            case Pid::BOX_WIDTH:
                  _boxWidth = v.value<Spatium>();
                  break;
            case Pid::TOP_GAP:
                  _topGap = v.toDouble();
                  break;
            case Pid::BOTTOM_GAP:
                  _bottomGap = v.toDouble();
                  break;
            case Pid::LEFT_MARGIN:
                  _leftMargin = v.toDouble();
                  break;
            case Pid::RIGHT_MARGIN:
                  _rightMargin = v.toDouble();
                  break;
            case Pid::TOP_MARGIN:
                  _topMargin = v.toDouble();
                  break;
            case Pid::BOTTOM_MARGIN:
                  _bottomMargin = v.toDouble();
                  break;
            case Pid::BOX_AUTOSIZE:
                  _isAutoSizeEnabled = v.toBool();
                  break;
            default:
                  return MeasureBase::setProperty(propertyId, v);
            }
      triggerLayout();
      return true;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Box::propertyDefault(Pid id) const
      {
      switch(id) {
            case Pid::PADDING_TO_NOTATION_ABOVE:
                  return score()->styleV(Sid::paddingToNotationAbove);
            case Pid::PADDING_TO_NOTATION_BELOW:
                  return score()->styleV(Sid::paddingToNotationBelow);
            case Pid::BOX_HEIGHT:
            case Pid::BOX_WIDTH:
                  return Spatium(0.0);

            case Pid::TOP_GAP:
                  return isHBox() ? 0.0 : score()->styleP(Sid::systemFrameDistance);
            case Pid::BOTTOM_GAP:
                  return isHBox() ? 0.0 : score()->styleP(Sid::frameSystemDistance);

            case Pid::LEFT_MARGIN:
            case Pid::RIGHT_MARGIN:
            case Pid::TOP_MARGIN:
            case Pid::BOTTOM_MARGIN:
                  return 0.0;
            case Pid::BOX_AUTOSIZE:
                  return true;
            default:
                  return MeasureBase::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   copyValues
//---------------------------------------------------------

void Box::copyValues(Box* origin)
      {
      _boxHeight    = origin->boxHeight();
      _boxWidth     = origin->boxWidth();
      _paddingToNotationAbove = origin->paddingToNotationAbove();
      _paddingToNotationBelow = origin->paddingToNotationBelow();

      qreal factor  = magS() / origin->magS();
      _bottomGap    = origin->bottomGap() * factor;
      _topGap       = origin->topGap() * factor;
      _bottomMargin = origin->bottomMargin() * factor;
      _topMargin    = origin->topMargin() * factor;
      _leftMargin   = origin->leftMargin() * factor;
      _rightMargin  = origin->rightMargin() * factor;
      }

//---------------------------------------------------------
//   HBox
//---------------------------------------------------------

HBox::HBox(Score* score)
   : Box(score)
      {
      initElementStyle(&hBoxStyle);
      setBoxWidth(Spatium(5.0));
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void HBox::layout()
      {
      if (parent() && parent()->isVBox()) {
            VBox* vb = toVBox(parent());
            qreal x = vb->leftMargin() * DPMM;
            qreal y = vb->topMargin() * DPMM;
            qreal w = point(boxWidth());
            qreal h = vb->height() - (vb->topMargin() + vb->bottomMargin()) * DPMM;
            setPos(x, y);
            bbox().setRect(0.0, 0.0, w, h);
            }
      else if (system()) {
            bbox().setRect(0.0, 0.0, point(boxWidth()), system()->height());
            }
      else {
            bbox().setRect(0.0, 0.0, 50, 50);
            }
      Box::layout();
      }

//---------------------------------------------------------
//   layout2
//    height (bbox) is defined now
//---------------------------------------------------------

void HBox::layout2()
      {
      Box::layout();
      }

//---------------------------------------------------------
//   acceptDrop
//---------------------------------------------------------

bool Box::acceptDrop(EditData& data) const
      {
      if (data.dropElement->flag(ElementFlag::ON_STAFF))
            return false;
      if (MScore::debugMode)
            qDebug("<%s>", data.dropElement->name());
      ElementType t = data.dropElement->type();
      switch (t) {
            case ElementType::LAYOUT_BREAK:
            case ElementType::TEXT:
            case ElementType::STAFF_TEXT:
            case ElementType::IMAGE:
            case ElementType::SYMBOL:
            case ElementType::VBOX:
            case ElementType::HBOX:
            case ElementType::TBOX:
            case ElementType::MEASURE_LIST:
            case ElementType::MEASURE:
                  return true;
            case ElementType::ICON:
                  switch (toIcon(data.dropElement)->iconType()) {
                        case IconType::VFRAME:
                        case IconType::TFRAME:
                        case IconType::FFRAME:
                        case IconType::HFRAME:
                        case IconType::MEASURE:
                              return true;
                        default:
                              break;
                        }
                  break;
            case ElementType::BAR_LINE:
                  return isHBox();
            default:
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   drop
//---------------------------------------------------------

Element* Box::drop(EditData& data)
      {
      Element* e = data.dropElement;
      if (e->flag(ElementFlag::ON_STAFF))
            return 0;
      if (MScore::debugMode)
            qDebug("<%s>", e->name());
      switch (e->type()) {
            case ElementType::LAYOUT_BREAK:
                  {
                  LayoutBreak* lb = toLayoutBreak(e);
                  if (pageBreak() || lineBreak()) {
                        if (
                           (lb->isPageBreak() && pageBreak())
                           || (lb->isLineBreak() && lineBreak())
                           || (lb->isSectionBreak() && sectionBreak())
                           ) {
                              //
                              // if break already set
                              //
                              delete lb;
                              break;
                              }
                        for (Element* elem : el()) {
                              if (elem->type() == ElementType::LAYOUT_BREAK) {
                                    score()->undoChangeElement(elem, e);
                                    break;
                                    }
                              }
                        break;
                        }
                  lb->setTrack(0);       // these are system elements
                  lb->setParent(this);
                  score()->undoAddElement(lb);
                  return lb;
                  }

            case ElementType::STAFF_TEXT:
                  {
                  Text* text = new Text(score(), Tid::FRAME);
                  text->setParent(this);
                  text->setXmlText(toStaffText(e)->xmlText());
                  score()->undoAddElement(text);
                  delete e;
                  return text;
                  }

            case ElementType::HBOX:
            case ElementType::VBOX:
            case ElementType::TBOX:
                  {
                  if (auto mbSource = e->findMeasureBase()) {
                        auto mbDestination = this->findMeasureBase();
                        auto boxClone = mbSource->clone();
                        boxClone->setPrev(this->prevMM());
                        boxClone->setNext(mbDestination);
                        score()->undo(new InsertMeasures(boxClone, boxClone));
                        }
                  break;
                  }
            case ElementType::ICON:
                  switch (toIcon(e)->iconType()) {
                        case IconType::VFRAME:
                              score()->insertMeasure(ElementType::VBOX, this);
                              break;
                        case IconType::TFRAME:
                              score()->insertMeasure(ElementType::TBOX, this);
                              break;
                        case IconType::FFRAME:
                              score()->insertMeasure(ElementType::FBOX, this);
                              break;
                        case IconType::HFRAME:
                              score()->insertMeasure(ElementType::HBOX, this);
                              break;
                        case IconType::MEASURE:
                              score()->insertMeasure(ElementType::MEASURE, this);
                              break;
                        default:
                              break;
                        }
                  break;

            case ElementType::TEXT:
            case ElementType::IMAGE:
            case ElementType::SYMBOL:
                  e->setParent(this);
                  score()->undoAddElement(e);
                  return e;
            default:
                  return 0;
            }
      return 0;
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

QRectF HBox::drag(EditData& data)
      {
      QRectF r(canvasBoundingRect());
      qreal diff = data.evtDelta.x();
      qreal x1   = offset().x() + diff;
      if (parent()->type() == ElementType::VBOX) {
            VBox* vb = toVBox(parent());
            qreal x2 = parent()->width() - width() - (vb->leftMargin() + vb->rightMargin()) * DPMM;
            if (x1 < 0.0)
                  x1 = 0.0;
            else if (x1 > x2)
                  x1 = x2;
            }
      setOffset(QPointF(x1, 0.0));
//      setStartDragPosition(data.delta);
      return canvasBoundingRect() | r;
      }

//---------------------------------------------------------
//   isMovable
//---------------------------------------------------------

bool HBox::isMovable() const
      {
      return parent() && (parent()->isHBox() || parent()->isVBox());
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void HBox::writeProperties(XmlWriter& xml) const
      {
      writeProperty(xml, Pid::CREATE_SYSTEM_HEADER);
      Box::writeProperties(xml);
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool HBox::readProperties(XmlReader& e)
      {
      const QStringRef& tag(e.name());
      if (readProperty(tag, e, Pid::CREATE_SYSTEM_HEADER))
            ;
      else if (Box::readProperties(e))
            ;
      else
            return false;
      return true;
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant HBox::getProperty(Pid propertyId) const
      {
      switch (propertyId) {
            case Pid::CREATE_SYSTEM_HEADER:
                  return createSystemHeader();
            default:
                  return Box::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool HBox::setProperty(Pid propertyId, const QVariant& v)
      {
      switch (propertyId) {
            case Pid::CREATE_SYSTEM_HEADER:
                  setCreateSystemHeader(v.toBool());
                  triggerLayout();
                  break;
            default:
                  return Box::setProperty(propertyId, v);
            }
      return true;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant HBox::propertyDefault(Pid id) const
      {
      switch(id) {
            case Pid::CREATE_SYSTEM_HEADER:
                  return true;
            default:
                  return Box::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   VBox
//---------------------------------------------------------

VBox::VBox(Score* score)
   : Box(score)
      {
      initElementStyle(&boxStyle);
      setBoxHeight(Spatium(10.0));
      setLineBreak(true);
      }

qreal VBox::minHeight() const
      {
      return point(Spatium(10));
      }

qreal VBox::maxHeight() const
      {
      return point(Spatium(30));
      }

QVariant VBox::getProperty(Pid propertyId) const
      {
      switch (propertyId) {
            case Pid::BOX_AUTOSIZE:
                  return isAutoSizeEnabled();
            default:
                  return Box::getProperty(propertyId);
      }
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void VBox::layout()
      {
      setPos(QPointF());

      if (system())
            bbox().setRect(0.0, 0.0, system()->width(), point(boxHeight()));
      else
            bbox().setRect(0.0, 0.0, 50, 50);

      for (Element* e : el()) {
            if (!e->isLayoutBreak())
                  e->layout();
            }

      if (getProperty(Pid::BOX_AUTOSIZE).toBool()) {
            qreal contentHeight = contentRect().height();

            if (contentHeight < minHeight())
                  contentHeight = minHeight();

            setHeight(contentHeight);
            }

      MeasureBase::layout();
      }

//---------------------------------------------------------
//   startEditDrag
//---------------------------------------------------------

void VBox::startEditDrag(EditData& ed)
      {
      if (isAutoSizeEnabled()) {
            setAutoSizeEnabled(false);
            setBoxHeight(Spatium(height() / spatium()));
      }
      Box::startEditDrag(ed);
      }


//---------------------------------------------------------
//   layout
//---------------------------------------------------------

FBox::FBox(Score* score) : VBox(score)
      {
      setBoxHeight(Spatium(0.0));
      }

// Copy an XML subtree without depending on QtXml (not linked by libmscore).
static QByteArray diagramXml(QXmlStreamReader& reader)
      {
      QByteArray data;
      QXmlStreamWriter writer(&data);
      int depth = 0;
      do {
            if (reader.isStartElement()) {
                  writer.writeStartElement(reader.name().toString());
                  writer.writeAttributes(reader.attributes());
                  ++depth;
                  }
            else if (reader.isEndElement()) {
                  writer.writeEndElement();
                  --depth;
                  }
            else if (reader.isCharacters())
                  writer.writeCharacters(reader.text().toString());
            if (depth == 0)
                  break;
            reader.readNext();
            } while (!reader.atEnd());
      return data;
      }

static const QMap<QString, QByteArray>& legendDiagramDatabase()
      {
      static QMap<QString, QByteArray> database;
      static bool loaded = false;
      if (!loaded) {
            loaded = true;
            QFile file(":/data/harmony_to_diagram.xml");
            if (!file.open(QIODevice::ReadOnly)) {
                  qWarning("Cannot open fretboard legend diagram database");
                  return database;
                  }
            QXmlStreamReader xml(&file);
            while (!xml.atEnd()) {
                  xml.readNext();
                  if (!xml.isStartElement() || xml.name() != "HarmonyToDiagram")
                        continue;
                  QString name;
                  QByteArray diagram;
                  while (xml.readNextStartElement()) {
                        if (xml.name() == "FretDiagram")
                              diagram = diagramXml(xml);
                        else if (xml.name() == "Harmony") {
                              while (xml.readNextStartElement()) {
                                    if (xml.name() == "name")
                                          name = xml.readElementText();
                                    else
                                          xml.skipCurrentElement();
                                    }
                              }
                        else
                              xml.skipCurrentElement();
                        }
                  if (!name.isEmpty() && !diagram.isEmpty())
                        database.insert(name, diagram);
                  }
            }
      return database;
      }

static FretDiagram* makeLegendDiagram(Score* score, const Harmony* harmony)
      {
      auto* fd = new FretDiagram(score);
      // Match parsed pitch classes and chord descriptions, preserving m/M and
      // slash basses. A name-wide toLower() would turn major into minor chords.
      const auto& database = legendDiagramDatabase();
      QByteArray data;
      if (harmony->rootTpc() != Tpc::TPC_INVALID) {
            QString spelling = tpc2name(harmony->rootTpc(), NoteSpellingType::STANDARD, NoteCaseType::LOWER)
                  + harmony->hTextName();
            spelling.remove('=');
            if (harmony->baseTpc() != Tpc::TPC_INVALID)
                  spelling += '/' + tpc2name(harmony->baseTpc(), NoteSpellingType::STANDARD, NoteCaseType::LOWER);
            spelling.replace(QChar(0x266d), 'b');
            spelling.replace(QChar(0x266f), '#');
            if (!harmony->hTextName().contains('='))
                  data = database.value(spelling);
            }
      if (data.isEmpty() && harmony->descr()) {
            // Database roots use English names even in a German/solfege score.
            // Parse them separately from extensions so score spelling preferences
            // cannot select a B-natural diagram for a German B-flat chord.
            auto rootOf = [](const QString& name, int& consumed) {
                  consumed = 0;
                  if (name.isEmpty() || !QString("abcdefg").contains(name[0]))
                        return int(Tpc::TPC_INVALID);
                  consumed = 1;
                  int alter = 0;
                  while (consumed < name.size() && (name[consumed] == '#' || name[consumed] == 'b')) {
                        alter += name[consumed] == '#' ? 1 : -1;
                        ++consumed;
                        }
                  return step2tpc(QString(name[0].toUpper()), AccidentalVal(alter));
                  };
            for (auto it = database.constBegin(); it != database.constEnd(); ++it) {
                  int consumed = 0;
                  const int root = rootOf(it.key(), consumed);
                  if (root != harmony->rootTpc())
                        continue;
                  QString suffix = it.key().mid(consumed);
                  int bass = Tpc::TPC_INVALID;
                  const int slash = suffix.lastIndexOf('/');
                  if (slash >= 0) {
                        int bassSize = 0;
                        const int parsedBass = rootOf(suffix.mid(slash + 1), bassSize);
                        if (bassSize == suffix.size() - slash - 1 && parsedBass != Tpc::TPC_INVALID) {
                              bass = parsedBass;
                              suffix.truncate(slash);
                              }
                        }
                  if (bass != harmony->baseTpc())
                        continue;
                  ParsedChord parsed;
                  parsed.parse(suffix, score->style().chordList());
                  const auto* description = harmony->descr(suffix, &parsed);
                  if (!description || !(description->chord == harmony->descr()->chord))
                        continue;
                  data = it.value();
                  break;
                  }
            }
      if (!data.isEmpty()) {
            XmlReader xml(data);
            if (xml.readNextStartElement())
                  fd->read(xml);
            }
      // Unsupported chord names intentionally get an empty editable diagram.
      fd->add(harmony->clone());
      return fd;
      }

// RemoveElement normally re-adds a child at the end. A legend also needs its
// previous position when undo restores a chord symbol removed from the score.
class RemoveLegendDiagram : public RemoveElement {
      FBox* _frame;
      FretDiagram* _diagram;
      size_t _index;
   public:
      explicit RemoveLegendDiagram(FretDiagram* diagram) : RemoveElement(diagram),
            _frame(toFBox(diagram->parent())), _diagram(diagram),
            _index(std::distance(_frame->el().begin(), std::find(_frame->el().begin(), _frame->el().end(), diagram))) {}
      void undo(EditData* ed) override {
            RemoveElement::undo(ed);
            auto& elements = _frame->el();
            auto it = std::find(elements.begin(), elements.end(), _diagram);
            if (it != elements.end() && _index < elements.size())
                  std::rotate(elements.begin() + _index, it, it + 1);
            }
      };

bool FBox::syncDiagrams()
      {
      QMap<QString, const Harmony*> harmonies;
      QMap<QString, const FretDiagram*> sources;
      QStringList order;
      for (Measure* m = score()->firstMeasure(); m; m = m->nextMeasure()) {
            for (const Segment& segment : m->segments()) {
                  for (const Element* e : segment.annotations()) {
                        const FretDiagram* fd = e->isFretDiagram() ? toFretDiagram(e) : nullptr;
                        const Harmony* h = fd ? fd->harmony() : e->isHarmony() ? toHarmony(e) : nullptr;
                        if (!h || h->harmonyType() != HarmonyType::STANDARD)
                              continue;
                        const QString key = h->harmonyName();
                        if (key.isEmpty())
                              continue;
                        if (!harmonies.contains(key)) {
                              order.append(key);
                              harmonies.insert(key, h);
                              }
                        if (fd && !sources.contains(key))
                              sources.insert(key, fd);
                        }
                  }
            }
      _defaultOrder = order;
      QSet<QString> existing;
      QList<Element*> obsolete;
      for (Element* e : el()) {
            if (!e->isFretDiagram())
                  continue;
            const auto* fd = toFretDiagram(e);
            const QString key = fd->harmony() ? fd->harmony()->harmonyName() : QString();
            if (!harmonies.contains(key) || existing.contains(key))
                  obsolete.append(e);
            else
                  existing.insert(key);
            }
      bool changed = !obsolete.empty();
      for (Element* e : obsolete)
            score()->undo(new RemoveLegendDiagram(toFretDiagram(e)));
      for (const QString& key : order) {
            if (existing.contains(key))
                  continue;
            auto* fd = sources.contains(key) ? sources.value(key)->clone() : makeLegendDiagram(score(), harmonies.value(key));
            fd->setParent(this);
            fd->setTrack(0);
            fd->setVisible(true);
            fd->setOffset(QPointF());
            if (fd->harmony()) {
                  fd->harmony()->setOffset(QPointF());
                  fd->harmony()->setVisible(true);
                  }
            score()->undo(new AddElement(fd));
            changed = true;
            }
      return changed;
      }

bool FBox::acceptDrop(EditData& data) const
      {
      const auto t = data.dropElement->type();
      return (t == ElementType::LAYOUT_BREAK || t == ElementType::ICON
            || t == ElementType::MEASURE || t == ElementType::MEASURE_LIST)
            && Box::acceptDrop(data);
      }

static const std::array<Pid, 7> fretFrameProperties {{
      Pid::FRET_FRAME_TEXT_SCALE, Pid::FRET_FRAME_DIAGRAM_SCALE,
      Pid::FRET_FRAME_COLUMN_GAP, Pid::FRET_FRAME_ROW_GAP,
      Pid::FRET_FRAME_CHORDS_PER_ROW, Pid::FRET_FRAME_H_ALIGN,
      Pid::SIZE_SPATIUM_DEPENDENT
      }};

void FBox::layout()
      {
      setPos(QPointF());
      const qreal width = system() ? system()->width() : 50.0 * spatium();
      // Lay out break icons before placing cells: the base implementation
      // also lays out children, resetting fret-diagram positions to (0, 0).
      bbox().setWidth(width);
      MeasureBase::layout();
      const qreal unit = _scaleWithStaff ? spatium() : SPATIUM20;
      QList<FretDiagram*> diagrams;
      qreal cellWidth = 0.0;
      for (Element* e : el()) {
            if (!e->isFretDiagram()) {
                  e->layout();
                  continue;
                  }
            auto* fd = toFretDiagram(e);
            if (fd->harmony())
                  fd->harmony()->setVisible(fd->visible());
            if (!fd->visible())
                  continue;
            fd->setUserMag(_diagramScale * unit / spatium());
            if (fd->harmony())
                  fd->harmony()->render();
            fd->layout();
            QRectF contents = fd->bbox();
            if (fd->harmony())
                  contents |= fd->harmony()->bbox().translated(fd->harmony()->pos());
            cellWidth = qMax(cellWidth, contents.width());
            diagrams.append(fd);
            }
      const qreal colGap = _columnGap.val() * unit;
      const qreal rowGap = _rowGap.val() * unit;
      qreal y = topMargin() * DPMM;
      for (int i = 0; i < diagrams.size(); i += _chordsPerRow) {
            const int count = qMin(_chordsPerRow, diagrams.size() - i);
            const qreal rowWidth = count * cellWidth + (count - 1) * colGap;
            const qreal x = _horizontalAlign == 0 ? leftMargin() * DPMM
                  : _horizontalAlign == 1 ? width - rightMargin() * DPMM - rowWidth
                  : (width - rowWidth) * 0.5;
            qreal commonBaseline = 0.0;
            for (int j = 0; j < count; ++j) {
                  if (const auto* h = diagrams[i + j]->harmony())
                        commonBaseline = qMin(commonBaseline, h->y());
                  }
            qreal above = 0.0;
            qreal below = 0.0;
            for (int j = 0; j < count; ++j) {
                  auto* fd = diagrams[i + j];
                  QRectF contents = fd->bbox();
                  if (const auto* h = fd->harmony()) {
                        contents |= h->bbox().translated(h->pos());
                        // Align labels by moving the whole cell. Moving only
                        // the label imposes a vertical neighbor's marker gap
                        // on horizontal diagrams, whose markers are at the side.
                        contents.translate(0.0, commonBaseline - h->y());
                        }
                  above = qMax(above, -contents.top());
                  below = qMax(below, contents.bottom());
                  }
            for (int j = 0; j < count; ++j) {
                  auto* fd = diagrams[i + j];
                  const qreal baselineOffset = fd->harmony() ? commonBaseline - fd->harmony()->y() : 0.0;
                  fd->setPos(x + j * (cellWidth + colGap) + (cellWidth - fd->width()) * 0.5 - fd->bbox().left(),
                        y + above + baselineOffset);
                  }
            y += above + below;
            if (i + count < diagrams.size())
                  y += rowGap;
            }
      setbbox(QRectF(0.0, 0.0, width, qMax(unit, y + bottomMargin() * DPMM)));
      }

void FBox::add(Element* e)
      {
      if (e->isLayoutBreak()) {
            MeasureBase::add(e);
            return;
            }
      if (!e->isFretDiagram()) {
            qDebug("FBox::add: unsupported element %s", e->name());
            return;
            }
      e->setParent(this);
      e->setTrack(0);
      e->setFlag(ElementFlag::ON_STAFF, false);
      e->setFlag(ElementFlag::MOVABLE, false);
      e->setAutoplace(false);
      if (auto* harmony = toFretDiagram(e)->harmony()) {
            harmony->setTrack(0);
            harmony->setPlay(false);
            harmony->setAutoplace(false);
            harmony->setFlag(ElementFlag::ON_STAFF, false);
            }
      el().push_back(e);
      }

void FBox::writeProperties(XmlWriter& xml) const
      {
      for (Pid pid : fretFrameProperties)
            writeProperty(xml, pid);
      Box::writeProperties(xml);
      }

bool FBox::readProperties(XmlReader& xml)
      {
      const QStringRef tag = xml.name();
      for (Pid pid : fretFrameProperties) {
            if (readProperty(tag, xml, pid))
                  return true;
            }
      if (tag == "FretDiagram") {
            auto* fd = new FretDiagram(score());
            fd->setParent(this);
            fd->setTrack(0);
            fd->read(xml);
            add(fd);
            return true;
            }
      if (score()->mscVersion() >= 400 && (tag == "leftMargin" || tag == "rightMargin"
            || tag == "topMargin" || tag == "bottomMargin")) {
            const QString name = tag.toString();
            const qreal mm = xml.readDouble() * spatium() / DPMM;
            if (name == "leftMargin") setLeftMargin(mm);
            else if (name == "rightMargin") setRightMargin(mm);
            else if (name == "topMargin") setTopMargin(mm);
            else setBottomMargin(mm);
            return true;
            }
      return Box::readProperties(xml);
      }

QVariant FBox::getProperty(Pid pid) const
      {
      switch (pid) {
            case Pid::FRET_FRAME_TEXT_SCALE:
                  return _textScale;
            case Pid::FRET_FRAME_DIAGRAM_SCALE:
                  return _diagramScale;
            case Pid::FRET_FRAME_COLUMN_GAP:
                  return QVariant::fromValue(_columnGap);
            case Pid::FRET_FRAME_ROW_GAP:
                  return QVariant::fromValue(_rowGap);
            case Pid::FRET_FRAME_CHORDS_PER_ROW:
                  return _chordsPerRow;
            case Pid::FRET_FRAME_H_ALIGN:
                  return _horizontalAlign;
            case Pid::SIZE_SPATIUM_DEPENDENT:
                  return _scaleWithStaff;
            case Pid::FRET_FRAME_DIAGRAMS_ORDER: {
                  QStringList names;
                  for (const Element* e : el()) {
                        if (e->isFretDiagram() && toFretDiagram(e)->harmony())
                              names.append(toFretDiagram(e)->harmony()->harmonyName());
                        }
                  return names.join(QChar(0x1f));
                  }
            default: return VBox::getProperty(pid);
            }
      }

bool FBox::setProperty(Pid pid, const QVariant& value)
      {
      switch (pid) {
            case Pid::FRET_FRAME_TEXT_SCALE:
                  _textScale = qBound(0.1, value.toDouble(), 10.0); break;
            case Pid::FRET_FRAME_DIAGRAM_SCALE:
                  _diagramScale = qBound(0.1, value.toDouble(), 10.0); break;
            case Pid::FRET_FRAME_COLUMN_GAP:
                  _columnGap = Spatium(qMax(0.0, value.value<Spatium>().val())); break;
            case Pid::FRET_FRAME_ROW_GAP:
                  _rowGap = Spatium(qMax(0.0, value.value<Spatium>().val())); break;
            case Pid::FRET_FRAME_CHORDS_PER_ROW:
                  _chordsPerRow = qBound(1, value.toInt(), 100); break;
            case Pid::FRET_FRAME_H_ALIGN:
                  _horizontalAlign = qBound(0, value.toInt(), 2); break;
            case Pid::SIZE_SPATIUM_DEPENDENT:
                  _scaleWithStaff = value.toBool(); break;
            case Pid::FRET_FRAME_DIAGRAMS_ORDER: {
                  const QStringList names = value.toString().split(QChar(0x1f));
                  std::stable_sort(el().begin(), el().end(), [&names](const Element* a, const Element* b) {
                        auto index = [&names](const Element* e) {
                              if (!e->isFretDiagram() || !toFretDiagram(e)->harmony())
                                    return names.size();
                              const int i = names.indexOf(toFretDiagram(e)->harmony()->harmonyName());
                              return i < 0 ? names.size() : i;
                              };
                        return index(a) < index(b);
                        });
                  break;
                  }
            default: return Box::setProperty(pid, value);
            }
      triggerLayout();
      return true;
      }

QVariant FBox::propertyDefault(Pid pid) const
      {
      switch (pid) {
            case Pid::FRET_FRAME_TEXT_SCALE:
            case Pid::FRET_FRAME_DIAGRAM_SCALE:
                  return 1.0;
            case Pid::FRET_FRAME_COLUMN_GAP:
            case Pid::FRET_FRAME_ROW_GAP:
                  return QVariant::fromValue(Spatium(3.0));
            case Pid::FRET_FRAME_CHORDS_PER_ROW:
                  return 8;
            case Pid::FRET_FRAME_H_ALIGN:
                  return 2;
            case Pid::SIZE_SPATIUM_DEPENDENT:
                  return true;
            case Pid::FRET_FRAME_DIAGRAMS_ORDER:
                  return _defaultOrder.join(QChar(0x1f));
            default: return VBox::propertyDefault(pid);
            }
      }

//---------------------------------------------------------
//   accessibleExtraInfo
//---------------------------------------------------------

QString Box::accessibleExtraInfo() const
      {
      QString rez = "";
      for (Element* e : el())
            rez += " " + e->screenReaderInfo();
      return rez;
      }

//---------------------------------------------------------
//   accessibleExtraInfo
//---------------------------------------------------------

QString TBox::accessibleExtraInfo() const
      {
      QString rez = _text->screenReaderInfo();
      return rez;
      }

}

