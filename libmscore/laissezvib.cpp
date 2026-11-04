// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "laissezvib.h"
#include "chord.h"
#include "note.h"
#include "notedot.h"
#include "score.h"
#include "staff.h"
#include "tie.h"
#include "xml.h"

#include <cmath>
#include <QSet>

namespace Ms {
static const ElementStyle laissezVibStyle {
      { Sid::minLaissezVibLength, Pid::LAISSEZ_VIB_MIN_LENGTH },
      };

LaissezVib::LaissezVib(Score* score) : Element(score)
      {
      initElementStyle(&laissezVibStyle);
      }

// The caller owns the undo command, as for other palette operations.
void Score::cmdToggleLaissezVib(const LaissezVib* prototype)
      {
      // Snapshot before adding/removing children can update the selection.
      const std::vector<Note*> notes = selection().noteList();
      QSet<Note*> processed;
      for (Note* note : notes) {
            if (processed.contains(note))
                  continue;
            // Addition/removal propagates to the whole linked-note group. A range
            // covering both notation and TAB must not toggle that group twice.
            for (ScoreElement* linked : note->linkList()) {
                  if (linked->isNote())
                        processed.insert(toNote(linked));
                  }
            Element* existing = nullptr;
            for (Element* attached : note->el()) {
                  if (attached->isLaissezVib()) {
                        existing = attached;
                        break;
                        }
                  }
            if (existing) {
                  undoRemoveElement(existing);
                  continue;
                  }
            // Test the actual outgoing tie even if its destination is outside the range.
            if (note->tieFor() || note->partialTie(false))
                  continue;
            LaissezVib* lv = prototype ? prototype->clone() : new LaissezVib(this);
            lv->setScore(this);
            lv->setParent(note);
            lv->setTrack(note->track());
            lv->styleChanged();
            undoAddElement(lv);
            }
      }

QPointF LaissezVib::attachment(bool& up) const
      {
      const Note* note = parent() && parent()->isNote() ? toNote(parent()) : nullptr;
      up = _direction != Direction::DOWN;
      bool inside = _tiePlacement == 1;
      if (note) {
            Tie probe(score());
            probe.setStartNote(const_cast<Note*>(note));
            if (note->chord()->measure()) {
                  probe.setSlurDirection(_direction);
                  probe.calculateDirection();
                  up = probe.up();
                  }
            if (!_tiePlacement)
                  inside = score()->styleI(note->chord()->notes().size() > 1
                        ? Sid::tiePlacementChord : Sid::tiePlacementSingleNote) == 1;
            }
      return note ? tieNoteAttachment(note, up, inside, true)
            : QPointF(0, (up ? -1.0 : 1.0) * (inside ? .2 : .7) * spatium());
      }

qreal LaissezVib::layoutChord(Chord* chord)
      {
      qreal right = 0.0;
      for (Note* note : chord->notes()) {
            for (Element* attached : note->el()) {
                  if (!attached->isLaissezVib() || !attached->visible())
                        continue;
                  attached->layout();
                  right = qMax(right, note->x() + attached->x() + attached->bbox().right());
                  }
            }
      return right;
      }

void LaissezVib::layout()
      {
      const Note* note = parent() && parent()->isNote() ? toNote(parent()) : nullptr;
      bool up;
      QPointF point = attachment(up);
      const qreal sp = spatium();
      const qreal sign = up ? -1.0 : 1.0;
      const bool tab = note && note->staff() && note->staff()->isTabStaff(note->tick());
      _symbol = SymId::noSym;
      if (score()->styleB(Sid::laissezVibUseSmuflSym)
          && symIsValid(up ? SymId::articLaissezVibrerAbove : SymId::articLaissezVibrerBelow))
            _symbol = up ? SymId::articLaissezVibrerAbove : SymId::articLaissezVibrerBelow;
      const bool smufl = _symbol != SymId::noSym;
      if (smufl && tab)
            point.rx() -= qMin(qreal(0), symBbox(_symbol).left());

      // Starts may differ for inner ties, outer ties and dots. Studio keeps a
      // common right end, extending curves rather than moving their anchors.
      const qreal ownX = note ? note->x() : 0.0;
      qreal endX = ownX + point.x() + (smufl ? symBbox(_symbol).right() : qMax(.1, _minLength.val()) * sp);
      if (note) {
            for (const Note* sibling : note->chord()->notes()) {
                  for (const Element* attached : sibling->el()) {
                        if (!attached->isLaissezVib() || !attached->visible())
                              continue;
                        const LaissezVib* lv = toLaissezVib(attached);
                        bool siblingUp;
                        QPointF start = lv->attachment(siblingUp);
                        const SymId symbol = siblingUp ? SymId::articLaissezVibrerAbove : SymId::articLaissezVibrerBelow;
                        qreal width = qMax(.1, lv->_minLength.val()) * lv->spatium();
                        if (smufl && lv->symIsValid(symbol)) {
                              const QRectF bounds = lv->symBbox(symbol);
                              if (tab)
                                    start.rx() -= qMin(qreal(0), bounds.left());
                              width = bounds.right();
                              }
                        endX = qMax(endX, sibling->x() + start.x() + width);
                        }
                  }
            }
      if (smufl) {
            const QRectF bounds = symBbox(_symbol);
            point.setX(endX - ownX - bounds.right());
            setPos(point);
            setbbox(bounds);
            return;
            }
      setPos(point);
      const qreal length = endX - ownX - point.x();
      const qreal lengthInSp = length / sp;
      // Studio uses its normal tie profile for curved laissez-vibrer marks.
      // These are the default minimum/maximum shoulder heights in Studio 4.7.
      const qreal shoulder = qBound(.3, .3 + .3 * std::sqrt(qAbs(lengthInSp - 1.0)), 2.0) * sp;
      const qreal height = sign * shoulder;
      const qreal end = score()->styleP(Sid::tieEndWidth);
      const qreal normalThickness = score()->styleP(Sid::tieMidWidth) - end;
      const qreal minThickness = .15 * sp - end;
      const qreal minLength = score()->styleS(Sid::minTieLength).val();
      qreal thickness = normalThickness;
      // Studio makes short ties thinner, reaching normal thickness at 4 sp.
      if (minLength < 4.0 && lengthInSp <= 4.0)
            thickness = minThickness + (normalThickness - minThickness) * (lengthInSp - minLength) / (4.0 - minLength);
      thickness = qMax(0.0, qMin(thickness, normalThickness));
      _path = QPainterPath(QPointF());
      _path.cubicTo(length * .2, height - thickness, length * .8, height - thickness, length, 0.0);
      _path.cubicTo(length * .8, height + thickness, length * .2, height + thickness, 0.0, 0.0);
      _path.closeSubpath();
      const qreal halfEnd = end * .5;
      setbbox(_path.boundingRect().adjusted(-halfEnd, -halfEnd, halfEnd, halfEnd));
      }

void LaissezVib::draw(QPainter* painter) const
      {
      if (_symbol != SymId::noSym) {
            painter->setPen(curColor());
            drawSymbol(_symbol, painter);
            }
      else {
            QPen pen(curColor(), score()->styleP(Sid::tieEndWidth));
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            painter->setPen(pen);
            painter->setBrush(curColor());
            painter->drawPath(_path);
            }
      }

void LaissezVib::write(XmlWriter& xml) const
      {
      if (!xml.canWrite(this))
            return;
      xml.stag(this);
      Element::writeProperties(xml);
      writeProperty(xml, Pid::SLUR_DIRECTION);
      writeProperty(xml, Pid::LAISSEZ_VIB_MIN_LENGTH);
      if (_tiePlacement)
            xml.tag("tiePlacement", _tiePlacement == 1 ? "inside" : "outside");
      xml.etag();
      }

void LaissezVib::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef tag(e.name());
            if (tag == "tiePlacement") {
                  const QString text = e.readElementText();
                  _tiePlacement = text == "inside" ? 1 : text == "outside" ? 2 : 0;
                  }
            else if (readProperty(tag, e, Pid::SLUR_DIRECTION)
                  || readProperty(tag, e, Pid::LAISSEZ_VIB_MIN_LENGTH))
                  ;
            else if (!Element::readProperties(e))
                  e.unknown();
            }
      }

QVariant LaissezVib::getProperty(Pid id) const
      {
      switch (id) {
            case Pid::SLUR_DIRECTION: return QVariant::fromValue(_direction);
            case Pid::TIE_PLACEMENT: return _tiePlacement;
            case Pid::LAISSEZ_VIB_MIN_LENGTH: return QVariant::fromValue(_minLength);
            default: return Element::getProperty(id);
            }
      }

bool LaissezVib::setProperty(Pid id, const QVariant& value)
      {
      switch (id) {
            case Pid::SLUR_DIRECTION: _direction = value.value<Direction>(); break;
            case Pid::TIE_PLACEMENT:
                  if (value.toInt() < 0 || value.toInt() > 2)
                        return false;
                  _tiePlacement = value.toInt();
                  break;
            case Pid::LAISSEZ_VIB_MIN_LENGTH: _minLength = Spatium(qMax(.1, value.value<Spatium>().val())); break;
            default: return Element::setProperty(id, value);
            }
      triggerLayout();
      return true;
      }

QVariant LaissezVib::propertyDefault(Pid id) const
      {
      switch (id) {
            case Pid::SLUR_DIRECTION: return QVariant::fromValue(Direction::AUTO);
            case Pid::TIE_PLACEMENT: return 0;
            case Pid::LAISSEZ_VIB_MIN_LENGTH: return score()->styleV(Sid::minLaissezVibLength);
            default: return Element::propertyDefault(id);
            }
      }

void LaissezVib::reset()
      {
      Element::reset();
      undoResetProperty(Pid::SLUR_DIRECTION);
      undoResetProperty(Pid::TIE_PLACEMENT);
      undoResetProperty(Pid::LAISSEZ_VIB_MIN_LENGTH);
      }
}
