//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2007-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "image.h"
#include "imageStore.h"
#include "mscore.h"
#include "score.h"
#include "undo.h"
#include "xml.h"

namespace Ms {

//---------------------------------------------------------
//   propertyList
//---------------------------------------------------------

static bool   defaultAutoScale        = false;
static bool   defaultLockAspectRatio  = true;
static bool   defaultSizeIsSpatium    = true;
static qreal  defaultFrameWidth       = 0.0;
static QColor defaultFrameColor       = MScore::defaultColor;

//---------------------------------------------------------
//   Image
//---------------------------------------------------------

Image::Image(Score* s)
   : BSymbol(s, ElementFlag::MOVABLE)
      {
      imageType        = ImageType::NONE;
      rasterDoc        = 0;
      _size            = QSizeF(0, 0);
      _storeItem       = 0;
      _dirty           = false;
      _lockAspectRatio = defaultLockAspectRatio;
      _autoScale       = defaultAutoScale;
      _sizeIsSpatium   = defaultSizeIsSpatium;
      _linkIsValid     = false;
      _used            = true;
      _frameColor      = defaultFrameColor;
      _frameWidth      = defaultFrameWidth;
      }

Image::Image(const Image& img)
   : BSymbol(img)
      {
      imageType        = img.imageType;
      buffer           = img.buffer;
      _size            = img._size;
      _lockAspectRatio = img._lockAspectRatio;
      _autoScale       = img._autoScale;
      _dirty           = img._dirty;
      _storeItem       = img._storeItem;
      _sizeIsSpatium   = img._sizeIsSpatium;
      _frameColor      = img._frameColor;
      _frameWidth      = img._frameWidth;
      if (_storeItem)
            _storeItem->reference(this);
      _linkPath        = img._linkPath;
      _linkIsValid     = img._linkIsValid;
      _used            = true;
      if (imageType == ImageType::RASTER)
            rasterDoc = img.rasterDoc ? new QImage(*img.rasterDoc) : 0;
      else if (imageType == ImageType::SVG)
            svgDoc = img.svgDoc ? new QSvgRenderer(_storeItem->buffer()) : 0;
      setZ(img.z());
      }

//---------------------------------------------------------
//   Image
//---------------------------------------------------------

Image::~Image()
      {
      if (_storeItem)
            _storeItem->dereference(this);
      if (imageType == ImageType::SVG)
            delete svgDoc;
      else if (imageType == ImageType::RASTER)
            delete rasterDoc;
      }

//---------------------------------------------------------
//   setImageType
//---------------------------------------------------------

void Image::setImageType(ImageType t)
      {
      imageType = t;
      if (imageType == ImageType::SVG)
            svgDoc = 0;
      else if (imageType == ImageType::RASTER)
            rasterDoc = 0;
      else
            qDebug("illegal image type");
      }

//---------------------------------------------------------
//   imageSize
//---------------------------------------------------------

QSizeF Image::imageSize() const
      {
      if (!isValid())
            return QSizeF();
      return imageType == ImageType::RASTER ? rasterDoc->size() : svgDoc->defaultSize();
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Image::draw(QPainter* painter) const
      {
      bool emptyImage = false;
      if (imageType == ImageType::SVG) {
            if (!svgDoc)
                  emptyImage = true;
            else
                  svgDoc->render(painter, bbox());
            }
      else if (imageType == ImageType::RASTER) {
            if (rasterDoc == nullptr)
                  emptyImage = true;
            else {
                  painter->save();
                  QSizeF s;
                  if (_sizeIsSpatium)
                        s = _size * spatium();
                  else
                        s = _size * DPMM;
                  if (score() && score()->printing() && !MScore::svgPrinting) {
                        // use original image size for printing, but not for svg for reasonable file size.
                        painter->scale(s.width() / rasterDoc->width(), s.height() / rasterDoc->height());
                        painter->drawPixmap(QPointF(0, 0), QPixmap::fromImage(*rasterDoc));
                        }
                  else {
                        QTransform t = painter->transform();
                        QSize ss = QSizeF(s.width() * t.m11(), s.height() * t.m22()).toSize();
                        t.setMatrix(1.0, t.m12(), t.m13(), t.m21(), 1.0, t.m23(), t.m31(), t.m32(), t.m33());
                        painter->setWorldTransform(t);
                        if ((buffer.size() != ss || _dirty) && rasterDoc && !rasterDoc->isNull()) {
                              buffer = QPixmap::fromImage(rasterDoc->scaled(ss, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                              _dirty = false;
                              }
                        if (buffer.isNull())
                              emptyImage = true;
                        else
                              painter->drawPixmap(QPointF(0.0, 0.0), buffer);
                        }
                  painter->restore();
                  }
            }
      else if (imageType == ImageType::NONE) {
            emptyImage = true;
            }
      if (emptyImage) {
            // Make use of [Element color] to use as a fill
            // (highlighting, aleatoric boxes as 100% transparency + frame, etc)
            QBrush br;
            br.setColor(color());
            br.setStyle(Qt::SolidPattern);
            painter->setBrush(br);
            painter->setPen(Qt::NoPen);
            painter->drawRect(bbox());
            }
      if (_frameWidth != 0.0) {
            QPen pen(Qt::NoBrush, _frameWidth * 0.10, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
            pen.setColor(_frameColor);
            painter->setPen(pen);
            // Top + Bottom
            painter->drawLine(0.0, 0.0, bbox().width(), 0.0);
            painter->drawLine(0.0, bbox().height(), bbox().width(), bbox().height());
            // Left + Right
            painter->drawLine(0.0, 0.0, 0.0, bbox().height());
            painter->drawLine(bbox().width(), 0.0, bbox().width(), bbox().height());
            }
      if (selected() && !(score() && score()->printing())) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(MScore::selectColor[0]);
            painter->drawRect(bbox());
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Image::write(XmlWriter& xml) const
      {
      // attempt to convert the _linkPath to a path relative to the score
      //
      // TODO : on Save As, score()->fileInfo() still contains the old path and fname
      //          if the Save As path is different, image relative path will be wrong!
      //
      QString relativeFilePath= QString();
      if (!_linkPath.isEmpty() && _linkIsValid) {
            QFileInfo fi(_linkPath);
            // score()->fileInfo()->canonicalPath() would be better
            // but we are saving under a temp file name and the 'final' file
            // might not exist yet, so canonicalFilePath() may return only "/"
            // OTOH, the score 'final' file name is practically always canonical, at this point
            QString scorePath = score()->masterScore()->fileInfo()->absolutePath();
            QString imgFPath  = fi.canonicalFilePath();
            // if imgFPath is in (or below) the directory of scorePath
            if (imgFPath.startsWith(scorePath, Qt::CaseSensitive)) {
                  // relative img path is the part exceeding scorePath
                  imgFPath.remove(0, scorePath.size());
                  if(imgFPath.startsWith('/'))
                        imgFPath.remove(0, 1);
                  relativeFilePath = imgFPath;
                  }
            // try 1 level up
            else {
                  // reduce scorePath by one path level
                  fi.setFile(scorePath);
                  scorePath = fi.path();
                  // if imgFPath is in (or below) the directory up the score directory
                  if (imgFPath.startsWith(scorePath, Qt::CaseSensitive)) {
                        // relative img path is the part exceeding new scorePath plus "../"
                        imgFPath.remove(0, scorePath.size());
                        if (!imgFPath.startsWith('/'))
                              imgFPath.prepend('/');
                        imgFPath.prepend("..");
                        relativeFilePath = imgFPath;
                        }
                  }
            }
      // if no match, use full _linkPath
      if (relativeFilePath.isEmpty())
            relativeFilePath = _linkPath;

      xml.stag(this);
      BSymbol::writeProperties(xml);
      // keep old "path" tag, for backward compatibility and because it is used elsewhere
      // (for instance by Box:read(), Measure:read(), Note:read(), ...)
      xml.tag("path", _storeItem ? _storeItem->hashName() : relativeFilePath);
      xml.tag("linkPath", relativeFilePath);

      writeProperty(xml, Pid::AUTOSCALE);
      writeProperty(xml, Pid::SIZE);
      writeProperty(xml, Pid::LOCK_ASPECT_RATIO);
      writeProperty(xml, Pid::SIZE_IS_SPATIUM);
      writeProperty(xml, Pid::IMAGE_FRAME_WIDTH);
      writeProperty(xml, Pid::IMAGE_FRAME_COLOR);

      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Image::read(XmlReader& e)
      {
      if (score()->mscVersion() <= 114)
            _sizeIsSpatium = false;

      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "autoScale")
                  readProperty(e, Pid::AUTOSCALE);
            else if (tag == "size")
                  readProperty(e, Pid::SIZE);
            else if (tag == "lockAspectRatio")
                  readProperty(e, Pid::LOCK_ASPECT_RATIO);
            else if (tag == "imageFrameWidth")
                  readProperty(e, Pid::IMAGE_FRAME_WIDTH);
            else if (tag == "imageFrameColor")
                  readProperty(e, Pid::IMAGE_FRAME_COLOR);
            else if (tag == "sizeIsSpatium")
                  // setting this using the property Pid::SIZE_IS_SPATIUM breaks, because the
                  // property setter attempts to maintain a constant size. If we're reading, we
                  // don't want to do that, because the stored size will be in:
                  //    mm if size isn't spatium
                  //    sp if size is spatium
                  _sizeIsSpatium = e.readBool();
            else if (tag == "path")
                  _storePath = e.readElementText();
            else if (tag == "linkPath")
                  _linkPath = e.readElementText();
            else if (tag == "subtype")    // obsolete
                  e.skipCurrentElement();
            else if (!BSymbol::readProperties(e))
                  e.unknown();
            }

      // once all paths are read, load img or retrieve it from store
      // loading from file is tried first to update the stored image, if necessary

      qDebug("linkPath <%s>", qPrintable(_linkPath));
      qDebug("storePath <%s>", qPrintable(_storePath));

      QString path;
      bool    loaded = false;
      // if a store path is given, attempt to get the image from the store
      if (!_storePath.isEmpty()) {
            _storeItem = imageStore.getImage(_storePath);
            if (_storeItem) {
                  _storeItem->reference(this);
                  loaded = true;
                  }
            // if no image in store, attempt to load from path (for backward compatibility)
            else
                  loaded = load(_storePath);
            path = _storePath;
            }
      // if no success from store path, attempt loading from link path (for .mscx files)
      if (!loaded) {
            _linkIsValid = load(_linkPath);
            path = _linkPath;
            }

      if (path.endsWith(".svg") || path.endsWith(".svgz"))
            setImageType(ImageType::SVG);
      else
            setImageType(ImageType::RASTER);
      }

//---------------------------------------------------------
//   load
//    load image from file and put into ImageStore
//    return true on success
//---------------------------------------------------------

bool Image::load(const QString& ss)
      {
      qDebug("Image::load <%s>", qPrintable(ss));
      QString path(ss);
      // if file path is relative, prepend score path
      QFileInfo fi(path);
      if (fi.isRelative()) {
            path.prepend(score()->masterScore()->fileInfo()->absolutePath() + "/");
            fi.setFile(path);
            }

      _linkIsValid = false;                     // assume link fname is invalid
      QFile f(path);
      if (!f.open(QIODevice::ReadOnly)) {
            qDebug("Image::load<%s> failed", qPrintable(path));
            return false;
            }
      QByteArray ba = f.readAll();
      f.close();

      _linkIsValid = true;
      _linkPath = fi.canonicalFilePath();
      _storeItem = imageStore.add(_linkPath, ba);
      _storeItem->reference(this);
      if (path.endsWith(".svg") || path.endsWith(".svgz"))
            setImageType(ImageType::SVG);
      else
            setImageType(ImageType::RASTER);
      return true;
      }

//---------------------------------------------------------
//   loadFromData
//    load image from data and put into ImageStore
//    return true on success
//---------------------------------------------------------

bool Image::loadFromData(const QString& ss, const QByteArray& ba)
      {
      qDebug("Image::loadFromData <%s>", qPrintable(ss));

      _linkIsValid = false;
      _linkPath = "";
      _storeItem = imageStore.add(ss, ba);
      _storeItem->reference(this);
      if (ss.endsWith(".svg") || ss.endsWith(".svgz"))
            setImageType(ImageType::SVG);
      else
            setImageType(ImageType::RASTER);
      return true;
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void Image::startEditDrag(EditData& data)
      {
      BSymbol::startEditDrag(data);
      ElementEditData* eed = data.getData(this);

      eed->pushProperty(Pid::SIZE);
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void Image::editDrag(EditData& ed)
      {
      auto cursorShape = ed.view->cursor().shape();
      auto sz  = _size;
      auto xOff = offset().x();
      auto yOff = offset().y();

      qreal ratio = _size.width() / _size.height();
      qreal dx = ed.delta.x();
      qreal dy = ed.delta.y();
      qreal dxOffset = dx;
      qreal dyOffset = dy;
      if (_sizeIsSpatium) {
            qreal _spatium = spatium();
            dx /= _spatium;
            dy /= _spatium;
            }
      else {
            dx /= DPMM;
            dy /= DPMM;
            }

      // Note: the corner grips will bypass [lock aspect ratio], side grips obey locking rule
      switch (int(ed.curGrip)) {
      case 0: // Top-Left
            cursorShape = Qt::SizeFDiagCursor;
            rxoffset() += dxOffset;
            ryoffset() += dyOffset;
            _size.setWidth(_size.width()   - dx);
            _size.setHeight(_size.height() - dy);
            break;
      case 1: // Top-Right
            cursorShape = Qt::SizeBDiagCursor;
            ryoffset() += dyOffset;
            _size.setWidth(_size.width()   + dx);
            _size.setHeight(_size.height() - dy);
            break;
      case 2: // Bottom-Right
            cursorShape = Qt::SizeFDiagCursor;
            _size.setWidth(_size.width()   + dx);
            _size.setHeight(_size.height() + dy);
            break;
      case 3: // Bottom-Left
            cursorShape = Qt::SizeBDiagCursor;
            rxoffset() += dxOffset;
            _size.setWidth(_size.width()   - dx);
            _size.setHeight(_size.height() + dy);
            break;
      case 4: // Top
            cursorShape = Qt::SizeVerCursor;
            ryoffset() += dyOffset;
            _size.setHeight(_size.height() - dy);
            if (_lockAspectRatio)
                  _size.setWidth(_size.height() * ratio);
            break;
      case 5: // Right
            cursorShape = Qt::SizeHorCursor;
            _size.setWidth(_size.width() + dx);
            if (_lockAspectRatio)
                  _size.setHeight(_size.width() / ratio);
            break;
      case 6: // Bottom
            cursorShape = Qt::SizeVerCursor;
            _size.setHeight(_size.height() + dy);
            if (_lockAspectRatio)
                  _size.setWidth(_size.height() * ratio);
            break;
      case 7: // Left
            cursorShape = Qt::SizeHorCursor;
            rxoffset() += dxOffset;
            _size.setWidth(_size.width() - dx);
            if (_lockAspectRatio)
                  _size.setHeight(_size.width() / ratio);
            break;
      }

      if (_size.width() < 0.1) {
            rxoffset() = xOff;
            _size = sz;
            }
      if (_size.height() < 0.1) {
            ryoffset() = yOff;
            _size = sz;
            }

      ed.view->setCursor(QCursor(cursorShape));
      layout();
      }

//---------------------------------------------------------
//   gripsPositions
//---------------------------------------------------------

std::vector<QPointF> Image::gripsPositions(const EditData&) const
      {
      QRectF r(pageBoundingRect());
      return {
            r.topLeft(),
            r.topRight(),
            r.bottomRight(),
            r.bottomLeft(),
            QPointF(r.x() + (r.width() * 0.5), r.top()),
            QPointF(r.right(), r.y() + (r.height() * 0.5)),
            QPointF(r.x() + (r.width() * 0.5), r.bottom()),
            QPointF(r.left(), r.y() + (r.height() * 0.5))
            };
      }

//---------------------------------------------------------
//   pixel2Size
//---------------------------------------------------------

QSizeF Image::pixel2size(const QSizeF& s) const
      {
      return s / (_sizeIsSpatium ? spatium() : DPMM);
      }

//---------------------------------------------------------
//   size2pixel
//---------------------------------------------------------

QSizeF Image::size2pixel(const QSizeF& s) const
      {
      return s * (_sizeIsSpatium ? spatium() : DPMM);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Image::layout()
      {
      setPos(0.0, 0.0);
      if (imageType == ImageType::SVG && !svgDoc) {
            if (_storeItem)
                  svgDoc = new QSvgRenderer(_storeItem->buffer());
            }
      else if (imageType == ImageType::RASTER && !rasterDoc) {
            if (_storeItem) {
                  rasterDoc = new QImage;
                  rasterDoc->loadFromData(_storeItem->buffer());
                  if (!rasterDoc->isNull())
                        _dirty = true;
                  }
            }
      if (_size.isNull())
            _size = pixel2size(imageSize());

      // if autoscale && inside a box, scale to box relevant size
      if (autoScale() && parent() && ((parent()->isHBox() || parent()->isVBox()))) {
            if (_lockAspectRatio) {
                  qreal f = _sizeIsSpatium ? spatium() : DPMM;
                  QSizeF size(imageSize());
                  qreal ratio = size.width() / size.height();
                  qreal w = parent()->width();
                  qreal h = parent()->height();
                  if ((w / h) < ratio) {
                        _size.setWidth(w / f);
                        _size.setHeight((w / ratio) / f);
                        }
                  else {
                        _size.setHeight(h / f);
                        _size.setWidth(h * ratio / f);
                        }
                  }
            else
                  _size = pixel2size(parent()->bbox().size());
            }

      // in any case, adjust position relative to parent
      setbbox(QRectF(QPointF(), size2pixel(_size)));
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Image::getProperty(Pid propertyId) const
      {
      switch(propertyId) {
            case Pid::AUTOSCALE:
                  return autoScale();
            case Pid::SIZE:
                  return size();
            case Pid::LOCK_ASPECT_RATIO:
                  return lockAspectRatio();
            case Pid::IMAGE_FRAME_WIDTH:
                  return getFrameWidth();
            case Pid::IMAGE_FRAME_COLOR:
                  return getFrameColor();
            case Pid::SIZE_IS_SPATIUM:
                  return sizeIsSpatium();
            default:
                  return Element::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Image::setProperty(Pid propertyId, const QVariant& v)
      {
      bool rv = true;
      score()->addRefresh(canvasBoundingRect());
      switch(propertyId) {
            case Pid::AUTOSCALE:
                  setAutoScale(v.toBool());
                  break;
            case Pid::SIZE:
                  setSize(v.toSizeF());
                  break;
            case Pid::LOCK_ASPECT_RATIO:
                  setLockAspectRatio(v.toBool());
                  break;
            case Pid::IMAGE_FRAME_COLOR:
                  setFrameColor(v.value<QColor>());
                  break;
            case Pid::IMAGE_FRAME_WIDTH:
                  setFrameWidth(v.toReal());
                  break;
            case Pid::SIZE_IS_SPATIUM:
                  {
                  QSizeF s = size2pixel(_size);
                  setSizeIsSpatium(v.toBool());
                  _size = pixel2size(s);
                  }
                  break;
            default:
                  rv = Element::setProperty(propertyId, v);
                  break;
            }
      setGenerated(false);
      _dirty = true;
      triggerLayout();
      return rv;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Image::propertyDefault(Pid id) const
      {
      switch(id) {
            case Pid::AUTOSCALE:
                  return defaultAutoScale;
            case Pid::SIZE:
                  return pixel2size(imageSize());
            case Pid::LOCK_ASPECT_RATIO:
                  return defaultLockAspectRatio;
            case Pid::SIZE_IS_SPATIUM:
                  return defaultSizeIsSpatium;
            case Pid::IMAGE_FRAME_COLOR:
                  return defaultFrameColor;
            case Pid::IMAGE_FRAME_WIDTH:
                  return defaultFrameWidth;
            default:
                  return Element::propertyDefault(id);
            }
      }
}

