#ifndef MS_PAPERIMAGE_H
#define MS_PAPERIMAGE_H

#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <cmath>

namespace Ms {

// Draw an image fitted to the page width, repeating vertically.
// Both pageRect and clipRect use the painter's current coordinates.
inline void drawVerticallyRepeatedPaper(QPainter* p,
                                        const QRectF& pageRect,
                                        const QRectF& clipRect,
                                        const QPixmap& image)
      {
      if (image.isNull() || pageRect.isEmpty())
            return;

      const QRectF visible = pageRect.intersected(clipRect);
      if (visible.isEmpty())
            return;

      const qreal tileHeight =
            pageRect.width() * image.height() / image.width();

      if (!(tileHeight > 0.0) || !std::isfinite(tileHeight))
            return;

      const qreal first = std::floor(
            (visible.top() - pageRect.top()) / tileHeight);

      const qreal last = std::ceil(
            (visible.bottom() - pageRect.top()) / tileHeight);

      p->save();
      p->setClipRect(visible, Qt::IntersectClip);

      for (qreal i = first; i < last; i += 1.0) {
            const QRectF tile(pageRect.left(),
                              pageRect.top() + i * tileHeight,
                              pageRect.width(),
                              tileHeight);

            p->drawPixmap(tile, image, QRectF(image.rect()));
            }

      p->restore();
      }

} // namespace Ms

#endif
