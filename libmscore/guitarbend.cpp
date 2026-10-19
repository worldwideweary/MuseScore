//=============================================================================
//  MuseScore
//  Copyright (C) 2026 MuseScore contributors
//  SPDX-License-Identifier: GPL-2.0-only
//=============================================================================
#include "guitarbend.h"
#include "chord.h"
#include "note.h"
#include "navigate.h"
#include "stafftype.h"
#include "score.h"
#include "shape.h"
#include "segment.h"
#include "staff.h"
#include "system.h"
#include "symbol.h"
#include "stem.h"
#include "accidental.h"
#include "ledgerline.h"
#include "tie.h"
#include "xml.h"

namespace Ms {

static const ElementStyle guitarBendStyle {
      { Sid::guitarBendLineWidth, Pid::LINE_WIDTH },
      };

GuitarBend::GuitarBend(Score* s) : SLine(s, ElementFlag::MOVABLE)
      {
      setAnchor(Anchor::NOTE);
      setDiagonal(true);
      initElementStyle(&guitarBendStyle);
      }

QPointF GuitarBend::linePos(Grip grip, System** system) const
      {
      QPointF point = SLine::linePos(grip, system);
      const Note* note = grip == Grip::START ? startNote() : endNote();
      if (note && note->staff()->isTabStaff(note->tick()))
            point.rx() += 0.5 * (note->bbox().width() - note->headWidth());
      return point;
      }

Note* GuitarBend::startNote() const
      {
      return startElement() && startElement()->isNote() ? toNote(startElement()) : nullptr;
      }

Note* GuitarBend::endNote() const
      {
      return endElement() && endElement()->isNote() ? toNote(endElement()) : nullptr;
      }

bool GuitarBend::singleNote() const
      {
      return _bendType == GuitarBendType::SLIGHT_BEND || _bendType == GuitarBendType::DIP
            || _bendType == GuitarBendType::SCOOP;
      }

const Note* GuitarBend::frettingNote(const Note* note)
      {
      // Use one physical string/fret through bends and ties. Prefer a bend
      // over an overlapping dive, and reject malformed cyclic connectors.
      QSet<const Note*> seen;
      const Note* n = note;
      while (n && !seen.contains(n)) {
            seen.insert(n);
            const Note* previous = n->tieBack() ? n->tieBack()->startNote() : nullptr;
            const GuitarBend* incoming = nullptr;
            if (!previous) {
                  for (const Spanner* sp : n->spannerBack()) {
                        if (!sp->isGuitarBend())
                              continue;
                        const auto* bend = toGuitarBend(sp);
                        if (bend->singleNote() || bend->startNote() == n)
                              continue;
                        if (!incoming || (incoming->isDive() && !bend->isDive()))
                              incoming = bend;
                        }
                  if (incoming)
                        previous = incoming->startNote();
                  }
            if (!previous)
                  return n;
            n = previous;
            }
      return note;
      }

int GuitarBend::frettingPitch(const Note* note)
      {
      return frettingNote(note)->pitch();
      }

int GuitarBend::tabFretVisibility(const Note* note)
      {
      if (note->overrideBendVisibilityRules())
            return note->visible() ? 1 : 0;
      // Explicit grace-bend sources remain visible even when an older
      // saved layout left visible=0 on the note. Pre-bends normally show
      // the main fret instead; preserve Studio's alternate alignment style.
      for (const Spanner* sp : note->spannerFor()) {
            if (!sp->isGuitarBend())
                  continue;
            const auto* b = toGuitarBend(sp);
            if (b->bendType() == GuitarBendType::GRACE_NOTE_BEND)
                  return 1;
            if (b->bendType() == GuitarBendType::PRE_BEND || b->bendType() == GuitarBendType::PRE_DIVE)
                  return b->score()->styleB(Sid::alignPreBendAndPreDiveToGraceNote) ? 1 : 0;
            }
      for (const Spanner* sp : note->spannerBack()) {
            if (!sp->isGuitarBend())
                  continue;
            const auto* b = toGuitarBend(sp);
            if (b->singleNote())
                  continue;
            if (b->bendType() == GuitarBendType::PRE_BEND || b->bendType() == GuitarBendType::PRE_DIVE)
                  return b->score()->styleB(Sid::alignPreBendAndPreDiveToGraceNote) ? 0 : 1;
            if (b->bendAmount() < 0 && note->pitch() == frettingPitch(note)
                  && b->score()->styleB(Sid::showFretOnFullBendRelease))
                  return 1;
            return 0;
            }
      return -1; // no bend-specific visibility rule
      }

bool GuitarBend::hideTabFret(const Note* note)
      {
      return tabFretVisibility(note) == 0;
      }

void GuitarBend::updateTabNote(Note* note)
      {
      const Note* origin = frettingNote(note);
      if (origin != note && origin->string() >= 0 && origin->fret() >= 0) {
            note->undoChangeProperty(Pid::STRING, origin->string());
            note->undoChangeProperty(Pid::FRET, origin->fret());
            }
      const int show = tabFretVisibility(note);
      if (show >= 0)
            note->setVisible(show != 0);
      for (const Spanner* sp : note->spannerBack()) {
            if (!sp->isGuitarBend())
                  continue;
            const auto* b = toGuitarBend(sp);
            if (!b->singleNote() && b->bendAmount() < 0
                  && (!b->isDive() || b->totalBendAmount() == 0))
                  note->setStudioGhost(true);
            }
      }

const GuitarBend* GuitarBend::overlappingBendOrDive() const
      {
      if (!startNote() || !endNote() || singleNote())
            return nullptr;
      for (const Spanner* spanner : startNote()->spannerFor()) {
            if (!spanner->isGuitarBend() || spanner == this)
                  continue;
            const auto* other = toGuitarBend(spanner);
            if (!other->singleNote() && other->isDive() != isDive() && other->endNote() == endNote())
                  return other;
            }
      return nullptr;
      }

int GuitarBend::bendAmount() const
      {
      if (_bendType == GuitarBendType::SLIGHT_BEND)
            return 1;
      if (_bendType == GuitarBendType::SCOOP)
            return -2;
      if (_bendType == GuitarBendType::DIP)
            return _amount;
      if (!startNote() || !endNote())
            return 4; // palette
      const GuitarBend* overlap = overlappingBendOrDive();
      // Studio stores the independent bar interval because written pitches
      // contain the combined bend and dive. Do not count the dive twice.
      if (isDive() && overlap)
            return _amount;
      int amount = 2 * (endNote()->pitch() - startNote()->pitch())
            + qRound((endNote()->tuning() - startNote()->tuning()) / 50.0);
      if (overlap)
            amount -= overlap->bendAmount();
      return amount;
      }

GuitarBend* GuitarBend::precedingBend() const
      {
      const Note* note = startNote();
      QSet<const Note*> seen;
      while (note && !seen.contains(note)) {
            seen.insert(note);
            for (Spanner* sp : note->spannerBack()) {
                  if (!sp->isGuitarBend() || sp == this)
                        continue;
                  auto* bend = toGuitarBend(sp);
                  if (!bend->singleNote() && bend->isDive() == isDive()
                        && bend->endNote() == note && bend->startNote() != note)
                        return bend;
                  }
            note = note->tieBack() ? note->tieBack()->startNote() : nullptr;
            }
      return nullptr;
      }

int GuitarBend::totalBendAmount() const
      {
      int amount = bendAmount();
      if (_bendType == GuitarBendType::DIP || _bendType == GuitarBendType::SCOOP
            || _bendType == GuitarBendType::PRE_BEND || _bendType == GuitarBendType::PRE_DIVE)
            return amount;
      const Note* note = startNote();
      QSet<const Spanner*> seen;
      seen.insert(this);
      QSet<const Note*> notes;
      while (note && !notes.contains(note)) {
            notes.insert(note);
            if (note->tieBack() && note->tieBack()->startNote()) {
                  note = note->tieBack()->startNote();
                  continue;
                  }
            const GuitarBend* previous = nullptr;
            for (const Spanner* sp : note->spannerBack()) {
                  if (!sp->isGuitarBend() || seen.contains(sp))
                        continue;
                  const auto* b = toGuitarBend(sp);
                  if (b->isDive() == isDive() && !b->singleNote()) {
                        previous = b;
                        break;
                        }
                  }
            if (!previous)
                  break;
            seen.insert(previous);
            amount += previous->bendAmount();
            if (previous->bendType() == GuitarBendType::PRE_BEND || previous->bendType() == GuitarBendType::PRE_DIVE)
                  break;
            note = previous->startNote();
            }
      return amount;
      }

const GuitarBend* GuitarBend::followingPreBend() const
      {
      if (singleNote() || !endNote() || !startNote())
            return nullptr;
      const Note* arrival = endNote()->lastTiedNote();
      ChordRest* next = nextChordRest(arrival->chord());
      // Only adjacent grace/main chords qualify. Do not bridge a rest.
      while (next && next->isChord()) {
            for (const Note* note : toChord(next)->notes()) {
                  for (const Spanner* sp : note->spannerFor()) {
                        if (!sp->isGuitarBend() || sp == this)
                              continue;
                        const auto* b = toGuitarBend(sp);
                        const auto type = b->bendType();
                        if ((type != GuitarBendType::PRE_BEND && type != GuitarBendType::PRE_DIVE)
                              || b->isDive() != isDive() || !b->endNote())
                              continue;
                        if ((isDive() || b->endNote()->pitch() == arrival->pitch())
                              && b->bendAmount() == totalBendAmount()
                              && (isDive() || (frettingNote(note)->string() == frettingNote(startNote())->string()
                                    && frettingNote(note)->fret() == frettingNote(startNote())->fret())))
                              return b;
                        }
                  }
            if (!next->isGrace())
                  break;
            next = nextChordRest(next);
            }
      return nullptr;
      }

const Note* GuitarBend::holdEndNote() const
      {
      if (!endNote())
            return nullptr;
      const GuitarBend* following = followingPreBend();
      if (following)
            return following->endNote();
      const Note* last = endNote()->lastTiedNote();
      QSet<const Note*> seen;
      while (last && !seen.contains(last)) {
            seen.insert(last);
            const GuitarBend* continuing = nullptr;
            for (const Spanner* spanner : last->spannerFor()) {
                  if (!spanner->isGuitarBend())
                        continue;
                  const auto* bend = toGuitarBend(spanner);
                  // A finger bend does not release a held whammy level, and
                  // conversely a dive does not release a held finger bend.
                  if (bend->isDive() != isDive() && !bend->singleNote()
                        && !bend->overlappingBendOrDive() && bend->endNote() && bend->endNote() != last) {
                        continuing = bend;
                        break;
                        }
                  }
            if (!continuing)
                  break;
            last = continuing->endNote()->lastTiedNote();
            }
      return last;
      }

int GuitarBend::diveLevelIndex() const
      {
      const GuitarBend* root = this;
      QSet<const GuitarBend*> visited;
      while (root && !visited.contains(root)) {
            visited.insert(root);
            const GuitarBend* previous = root->precedingBend();
            if (!previous)
                  break;
            root = previous;
            }
      const int amount = totalBendAmount();
      QSet<int> nearerLevels;
      visited.clear();
      while (root && !visited.contains(root)) {
            visited.insert(root);
            const int level = root->totalBendAmount();
            if ((level < 0) == (amount < 0) && level && qAbs(level) < qAbs(amount))
                  nearerLevels.insert(level);
            const Note* last = root->endNote() ? root->endNote()->lastTiedNote() : nullptr;
            const GuitarBend* next = nullptr;
            if (last)
                  for (const Spanner* spanner : last->spannerFor()) {
                        if (!spanner->isGuitarBend())
                              continue;
                        const auto* candidate = toGuitarBend(spanner);
                        if (candidate->isDive() && !candidate->singleNote() && candidate->endNote() != last) {
                              next = candidate;
                              break;
                              }
                        }
            root = next;
            }
      return nearerLevels.size();
      }

QString GuitarBend::bendText() const
      {
      const int amount = totalBendAmount();
      if (!amount)
            return QString();
      const int a = qAbs(amount);
      QString text = a / 4 ? QString::number(a / 4) : QString();
      if (score()->styleB(Sid::useFractionCharacters)) {
            static const char* fractions[] = { "", "¼", "½", "¾" };
            text += QString::fromUtf8(fractions[a % 4]);
            }
      else if (a % 4) {
            if (!text.isEmpty())
                  text += ' ';
            static const char* fractions[] = { "", "1/4", "1/2", "3/4" };
            text += QLatin1String(fractions[a % 4]);
            }
      if (isDive() && amount > 0)
            text.prepend('+');
      else if (amount < 0)
            text.prepend('-');
      if (text == "1" && score()->styleB(Sid::guitarBendUseFull))
            text = "full";
      return text;
      }

LineSegment* GuitarBend::createLineSegment()
      {
      auto* s = new GuitarBendSegment(this, score());
      s->setTrack(track());
      s->setColor(color());
      return s;
      }

void GuitarBend::layout()
      {
      // Staff/type can be assigned after construction (including palette
      // drops). Resolve the matching width style before any geometry uses it.
      if (propertyFlags(Pid::LINE_WIDTH) == PropertyFlags::STYLED)
            setLineWidth(score()->styleP(getPropertyStyle(Pid::LINE_WIDTH)));
      if (!startNote() || !endNote()) {
            setLen(4.0 * spatium());
            frontSegment()->layout();
            return;
            }
      // A connected segment uses its predecessor's arrow endpoint. Lay out
      // predecessors first even when the skyline pass visits a QSet order.
      static thread_local QSet<const GuitarBend*> layingOut;
      if (layingOut.contains(this))
            return;
      layingOut.insert(this);
      if (GuitarBend* previous = precedingBend())
            previous->layout();
      SLine::layout();
      setPos(0.0, 0.0);
      layingOut.remove(this);
      }

void GuitarBend::scanElements(void* data, void (*func)(void*, Element*), bool all)
      {
      func(data, this);
      for (SpannerSegment* s : spannerSegments())
            if (!s->parent() || !s->parent()->isSystem())
                  s->scanElements(data, func, all);
      }

void GuitarBend::read(XmlReader& e)
      {
      eraseSpannerSegments();
      while (e.readNextStartElement()) {
            const QStringRef tag(e.name());
            if (tag == "guitarBendType") {
                  const QString text = e.readElementText();
                  bool ok = false;
                  int value = text.toInt(&ok);
                  if (!ok) {
                        const QStringList names { "bend", "pre-bend", "grace-note-bend", "slight-bend",
                              "dive", "pre-dive", "dip", "scoop" };
                        value = names.indexOf(text);
                        }
                  if (value >= 0 && value <= 7)
                        _bendType = GuitarBendType(value);
                  }
            else if (tag == "lineWidth") {
                  SLine::readProperties(e);
                  setPropertyFlags(Pid::LINE_WIDTH, PropertyFlags::UNSTYLED);
                  }
            else if (tag == "GuitarBendHold")
                  _holdXml = e.readXml();
            else if (tag == "direction")
                  _direction = Ms::readProperty(Pid::DIRECTION, e).value<Direction>();
            else if (tag == "bendShowHoldLine")
                  _hold = e.readInt();
            else if (tag == "bendStartTimeFactor")
                  _startTime = qBound(0.0, e.readDouble(), 1.0);
            else if (tag == "bendEndTimeFactor")
                  _endTime = qBound(0.0, e.readDouble(), 1.0);
            else if (tag == "bendTargetTimeFactor")
                  _targetTime = qBound(0.0, e.readDouble(), 1.0);
            else if (tag == "guitarBendAmount")
                  _amount = e.readInt();
            else if (tag == "play")
                  _play = e.readBool();
            else if (!SLine::readProperties(e))
                  e.unknown();
            }
      setAnchor(Anchor::NOTE);
      }

void GuitarBend::write(XmlWriter& xml) const
      {
      xml.stag(this);
      xml.tag("guitarBendType", int(_bendType));
      xml.tag("bendStartTimeFactor", _startTime);
      xml.tag("bendEndTimeFactor", _endTime);
      if (_bendType == GuitarBendType::DIP) {
            xml.tag("bendTargetTimeFactor", _targetTime);
            xml.tag("guitarBendAmount", _amount);
            }
      else if (isDive() && overlappingBendOrDive())
            xml.tag("guitarBendAmount", _amount);
      if (_hold)
            xml.tag("bendShowHoldLine", _hold);
      if (_direction != Direction::AUTO)
            xml.tag("direction", _direction == Direction::DOWN ? "down" : "up");
      if (!_play)
            xml.tag("play", false);
      SLine::writeProperties(xml);
      if (!_holdXml.isEmpty())
            xml.writeXml("GuitarBendHold", _holdXml);
      xml.etag();
      }

QVariant GuitarBend::getProperty(Pid id) const
      {
      switch (id) {
            case Pid::GUITAR_BEND_TYPE:
                  return int(_bendType);
            case Pid::GUITAR_BEND_AMOUNT:
                  return _amount;
            case Pid::BEND_START_TIME_FACTOR:
                  return _startTime;
            case Pid::BEND_END_TIME_FACTOR:
                  return _endTime;
            case Pid::BEND_TARGET_TIME_FACTOR:
                  return _targetTime;
            case Pid::BEND_SHOW_HOLD_LINE:
                  return _hold;
            case Pid::DIRECTION:
                  return QVariant::fromValue(_direction);
            case Pid::PLAY:
                  return _play;
            default:
                  return SLine::getProperty(id);
            }
      }

bool GuitarBend::setProperty(Pid id, const QVariant& v)
      {
      switch (id) {
            case Pid::GUITAR_BEND_TYPE:
                  if (v.toInt() < 0 || v.toInt() > 7)
                        return false;
                  _bendType = GuitarBendType(v.toInt());
                  break;
            case Pid::GUITAR_BEND_AMOUNT:
                  _amount = qBound(-48, v.toInt(), 48); break;
            case Pid::BEND_START_TIME_FACTOR:
                  _startTime = qBound(0.0, v.toDouble(), 1.0); break;
            case Pid::BEND_END_TIME_FACTOR:
                  _endTime = qBound(0.0, v.toDouble(), 1.0); break;
            case Pid::BEND_TARGET_TIME_FACTOR:
                  _targetTime = qBound(0.0, v.toDouble(), 1.0); break;
            case Pid::BEND_SHOW_HOLD_LINE:
                  _hold = qBound(0, v.toInt(), 2); break;
            case Pid::DIRECTION:
                  _direction = v.value<Direction>(); break;
            case Pid::PLAY:
                  _play = v.toBool(); break;
            default:
                  return SLine::setProperty(id, v);
            }
      triggerLayout();
      score()->setPlaylistDirty();
      return true;
      }

QVariant GuitarBend::propertyDefault(Pid id) const
      {
      switch (id) {
            case Pid::LINE_WIDTH:
                  return score()->styleP(getPropertyStyle(id));
            case Pid::GUITAR_BEND_TYPE:
                  return 0;
            case Pid::GUITAR_BEND_AMOUNT:
                  return -2;
            case Pid::BEND_START_TIME_FACTOR:
                  return 0.0;
            case Pid::BEND_END_TIME_FACTOR:
                  return 1.0;
            case Pid::BEND_TARGET_TIME_FACTOR:
                  return 0.25;
            case Pid::BEND_SHOW_HOLD_LINE:
                  return 0;
            case Pid::DIRECTION:
                  return QVariant::fromValue(Direction::AUTO);
            case Pid::PLAY:
                  return true;
            default:
                  return SLine::propertyDefault(id);
            }
      }

Sid GuitarBend::getPropertyStyle(Pid id) const
      {
      if (id != Pid::LINE_WIDTH)
            return SLine::getPropertyStyle(id);
      const bool tab = staff() && staff()->isTabStaff(tick());
      if (isDive())
            return tab ? Sid::guitarDiveLineWidthTab : Sid::guitarDiveLineWidth;
      return tab ? Sid::guitarBendLineWidthTab : Sid::guitarBendLineWidth;
      }

Element* GuitarBendSegment::propertyDelegate(Pid id)
      {
      switch (id) {
            case Pid::GUITAR_BEND_TYPE:
            case Pid::GUITAR_BEND_AMOUNT:
            case Pid::BEND_START_TIME_FACTOR:
            case Pid::BEND_END_TIME_FACTOR:
            case Pid::BEND_TARGET_TIME_FACTOR:
            case Pid::BEND_SHOW_HOLD_LINE:
            case Pid::LINE_WIDTH:
            case Pid::DIRECTION:
            case Pid::PLAY:
                  return guitarBend();
            default: return LineSegment::propertyDelegate(id);
            }
      }

int GuitarBendSegment::gripsCount() const
      {
      const GuitarBendType type = guitarBend()->bendType();
      if (type == GuitarBendType::DIP || type == GuitarBendType::SCOOP)
            return 0;
      return guitarBend()->isDive() ? 3 : 4;
      }

std::vector<QPointF> GuitarBendSegment::gripsPositions(const EditData&) const
      {
      if (!gripsCount())
            return {};
      std::vector<QPointF> grips(gripsCount());
      const QPointF origin = pagePos();
      grips[int(Grip::START)] = origin + _drawStart;
      grips[int(Grip::END)] = origin + _drawEnd;
      grips[int(Grip::MIDDLE)] = origin + (_drawStart + _drawEnd) * 0.5;
      if (gripsCount() > 3)
            grips[int(Grip::APERTURE)] = origin + _vertex;
      return grips;
      }

void GuitarBendSegment::startEditDrag(EditData& ed)
      {
      LineSegment::startEditDrag(ed);
      if (ElementEditData* data = ed.getData(this))
            data->pushProperty(Pid::BEND_VERTEX_OFF);
      }

void GuitarBendSegment::editDrag(EditData& ed)
      {
      const QPointF delta = ed.evtDelta;
      switch (ed.curGrip) {
            case Grip::START:
                  setOffset(offset() + delta);
                  setUserOff2(userOff2() - delta);
                  _vertexOff -= delta;
                  break;
            case Grip::END:
                  setUserOff2(userOff2() + delta);
                  break;
            case Grip::APERTURE:
                  _vertexOff += delta;
                  break;
            case Grip::MIDDLE:
                  setOffset(offset() + delta);
                  break;
            default:
                  return;
            }
      if (ed.curGrip == Grip::START || ed.curGrip == Grip::MIDDLE) {
            setOffsetChanged(true);
            if (isStyled(Pid::OFFSET))
                  setPropertyFlags(Pid::OFFSET, PropertyFlags::UNSTYLED);
            }
      triggerLayout();
      }

QVariant GuitarBendSegment::getProperty(Pid id) const
      {
      return id == Pid::BEND_VERTEX_OFF ? QVariant(_vertexOff) : LineSegment::getProperty(id);
      }

bool GuitarBendSegment::setProperty(Pid id, const QVariant& value)
      {
      if (id != Pid::BEND_VERTEX_OFF)
            return LineSegment::setProperty(id, value);
      _vertexOff = value.toPointF();
      triggerLayout();
      return true;
      }

QVariant GuitarBendSegment::propertyDefault(Pid id) const
      {
      return id == Pid::BEND_VERTEX_OFF ? QVariant(QPointF()) : LineSegment::propertyDefault(id);
      }

void GuitarBendSegment::reset()
      {
      undoResetProperty(Pid::BEND_VERTEX_OFF);
      LineSegment::reset();
      }

void GuitarBendSegment::spatiumChanged(qreal oldValue, qreal newValue)
      {
      LineSegment::spatiumChanged(oldValue, newValue);
      _vertexOff *= newValue / oldValue;
      }

void GuitarBendSegment::localSpatiumChanged(qreal oldValue, qreal newValue)
      {
      LineSegment::localSpatiumChanged(oldValue, newValue);
      _vertexOff *= newValue / oldValue;
      }

QFont GuitarBendSegment::labelFont(qreal pixelRatio) const
      {
      QFont font(score()->styleSt(Sid::bendFontFace));
      const FontStyle style = FontStyle(score()->styleI(Sid::bendFontStyle));
      font.setBold(style & FontStyle::Bold);
      font.setItalic(style & FontStyle::Italic);
      font.setUnderline(style & FontStyle::Underline);
      font.setStrikeOut(style & FontStyle::Strike);
      qreal size = score()->styleD(Sid::bendFontSize);
      if (score()->styleB(Sid::bendFontSpatiumDependent))
            size *= spatium() / SPATIUM20;
      font.setPointSizeF(qMax(0.1, size * pixelRatio));
      return font;
      }

void GuitarBendSegment::layout()
      {
      const GuitarBend* b = guitarBend();
      const qreal sp = spatium();
      const Note* n = b->startNote();
      if (!n) {
            // Palette samples have no staff or note anchors. Give each type
            // a deliberate preview instead of the generic notation-staff bend.
            _path = QPainterPath();
            _holdPath = QPainterPath();
            _arrow.clear();
            _text.clear();
            _textRect = QRectF();
            _scoopPos = QPointF();
            const GuitarBendType type = b->bendType();
            QRectF bounds;
            if (type == GuitarBendType::SCOOP)
                  bounds = symBbox(SymId::guitarVibratoBarScoop);
            else if (type == GuitarBendType::DIP) {
                  _path.moveTo(0.0, -sp);
                  _path.lineTo(sp, sp);
                  _path.lineTo(2.0 * sp, -sp);
                  }
            else if (type == GuitarBendType::DIVE) {
                  _path.moveTo(0.0, -sp);
                  _path.lineTo(1.5 * sp, sp);
                  _text = "-1";
                  _textPos = QPointF(1.7 * sp, sp);
                  }
            else if (type == GuitarBendType::PRE_DIVE) {
                  _path.moveTo(1.8 * sp, sp);
                  _path.lineTo(1.8 * sp, -sp);
                  _text = "pre";
                  _textPos = QPointF(-0.5 * sp, 0.0);
                  }
            else if (type == GuitarBendType::GRACE_NOTE_BEND) {
                  _path.moveTo(0.8 * sp, 0.7 * sp);
                  _path.lineTo(1.3 * sp, -0.8 * sp);
                  _path.lineTo(3.0 * sp, -0.35 * sp);
                  const QRectF head = symBbox(SymId::noteheadBlack);
                  bounds = QRectF(head.topLeft() * 0.7 + QPointF(0.0, sp), head.size() * 0.7);
                  bounds |= head.translated(3.0 * sp, 0.0);
                  // The regular arrival head sits on a short ledger line.
                  bounds |= QRectF(2.7 * sp, -0.1 * sp, head.width() + 0.6 * sp, 0.2 * sp);
                  }
            else {
                  const bool pre = type == GuitarBendType::PRE_BEND;
                  const QPointF tip(2.0 * sp, -sp);
                  _path.moveTo(pre ? 2.0 * sp : 0.0, pre ? 0.5 * sp : sp);
                  if (pre)
                        _path.lineTo(tip);
                  else
                        _path.cubicTo(QPointF(1.6 * sp, sp), QPointF(2.0 * sp, 0.5 * sp), tip);
                  _arrow << tip << tip + QPointF(-0.35 * sp, 0.75 * sp)
                        << tip + QPointF(0.35 * sp, 0.75 * sp);
                  if (pre || type == GuitarBendType::SLIGHT_BEND) {
                        _text = pre ? "pre" : QString::fromUtf8("¼");
                        _textPos = QPointF(0.4 * sp, -0.5 * sp);
                        if (pre) {
                              const QRectF ink = QFontMetricsF(labelFont(), MScore::paintDevice()).tightBoundingRect(_text);
                              _textPos.setX(_arrow.boundingRect().left() - 0.35 * sp - ink.right());
                              }
                        }
                  }
            bounds |= _path.boundingRect().adjusted(-0.1 * sp, -0.1 * sp, 0.1 * sp, 0.1 * sp);
            bounds |= _arrow.boundingRect();
            if (!_text.isEmpty()) {
                  _textRect = QFontMetricsF(labelFont(), MScore::paintDevice()).tightBoundingRect(_text).translated(_textPos);
                  bounds |= _textRect;
                  }
            setbbox(bounds);
            return;
            }
      const bool tab = n->staff()->isTabStaff(n->tick());
      const GuitarBendType type = b->bendType();
      const bool pre = type == GuitarBendType::PRE_BEND || type == GuitarBendType::PRE_DIVE;
      const bool small = b->singleNote();
      const Direction direction = b->getProperty(Pid::DIRECTION).value<Direction>();
      bool down = direction == Direction::DOWN;
      if (!tab && !small && direction == Direction::AUTO && b->endNote()) {
            const Chord* from = n->chord();
            const Chord* to = b->endNote()->chord();
            // Same precedence as Studio's computeUp(): voice, opposite
            // stems, single-note stems, then position among chord bends.
            if (from->measure()->hasVoices(b->staffIdx()))
                  down = !(b->track() % 2);
            else if (from->up() != to->up())
                  down = false;
            else if (from->notes().size() == 1 && from->stem())
                  down = from->up();
            else if (to->notes().size() == 1 && to->stem())
                  down = to->up();
            else {
                  int count = 0;
                  int index = 0;
                  const auto& notes = from->notes().size() > 1 ? from->notes() : to->notes();
                  for (const Note* note : notes) {
                        for (const Spanner* spanner : note->spannerFor()) {
                              if (!spanner->isGuitarBend())
                                    continue;
                              if (spanner == b)
                                    index = count;
                              ++count;
                              }
                        }
                  down = index < count / 2;
                  }
            }
      const qreal sign = down ? 1.0 : -1.0;
      QPointF start(0.0, sign * 0.65 * sp);
      // User endpoint offsets are applied only after default geometry. In
      // particular, TAB height and single-note defaults must not erase them.
      QPointF end(ipos2());
      if (isSingleBeginType())
            start.rx() += tab ? (n ? n->bbox().width() * 0.5 + 0.25 * sp : 0.5 * sp) : 0.6 * sp;
      if (tab && pre && b->endNote() && !score()->styleB(Sid::alignPreBendAndPreDiveToGraceNote))
            start = QPointF(end.x(), end.y() + sign * 0.65 * sp);
      if (small || (tab && pre))
            end = QPointF(start.x() + (pre ? 0.0 : 1.25 * sp), start.y());
      else if (!tab && isSingleEndType())
            end.rx() -= 0.5 * sp;
      _path = QPainterPath();
      _holdPath = QPainterPath();
      _arrow.clear();
      _text.clear();
      _textRect = QRectF();
      const bool release = b->bendAmount() < 0;
      const StaffType* tabType = tab ? n->staff()->staffTypeForElement(n) : nullptr;
      const qreal lineDist = tabType ? tabType->lineDistance().val() * sp : sp;
      const qreal staffTop = n && system() ? system()->staffYpage(staffIdx()) - n->pagePos().y() : 0.0;
      const qreal arrowWidth = score()->styleP(Sid::guitarBendArrowWidth);
      const qreal arrowHeight = score()->styleP(Sid::guitarBendArrowHeight);
      const bool tabArrow = tab && !b->isDive();
      bool curved = tabArrow;
      bool straight = false;
      QPointF vertex;
      auto clearStaffLine = [&](QPointF& point) {
            if (!n || !autoplace())
                  return;
            const StaffType* type = n->staff()->staffTypeForElement(n);
            const qreal spacing = type->lineDistance().val() * sp;
            if (spacing <= 0.0)
                  return;
            const int line = qRound((point.y() - staffTop) / spacing);
            if (line < 0 || line >= type->lines())
                  return;
            const qreal y = staffTop + line * spacing;
            const qreal clearance = 0.5 * score()->styleP(Sid::staffLineWidth) + 0.15 * sp;
            if (qAbs(point.y() - y) < clearance)
                  point.setY(y + sign * clearance);
            };
      if (tabArrow) {
            if (pre) {
                  const bool alignToGrace = score()->styleB(Sid::alignPreBendAndPreDiveToGraceNote);
                  const Note* anchor = alignToGrace ? n : b->endNote();
                  if (anchor) {
                        const QPointF noteOffset = alignToGrace ? QPointF() : ipos2();
                        start = noteOffset + QPointF(0.0, down ? anchor->bbox().bottom() + 0.25 * sp
                              : anchor->bbox().top() - 0.25 * sp);
                        }
                  }
            else if (n && isSingleBeginType() && !release) {
                  // The line origin is already at the fret centre. Start
                  // outside its upper-right corner, including cue-size frets.
                  start = QPointF(0.5 * n->bbox().width() + 0.25 * sp,
                        n->bbox().top() + 0.5 * b->lineWidth());
                  }
            const qreal top = staffTop - score()->styleD(Sid::guitarBendHeightAboveTABStaff) * lineDist;
            bool connected = false;
            if (!pre && isSingleBeginType()) {
                  const GuitarBend* previous = b->precedingBend();
                  if (previous && previous->totalBendAmount() != 0 && !previous->segmentsEmpty()) {
                        const auto* segment = static_cast<const GuitarBendSegment*>(previous->backSegment());
                        if (segment->system() == system() && !segment->arrow().isEmpty()) {
                              start = segment->pagePos() + segment->arrow().front() - pagePos() + offset();
                              start.ry() += (previous->bendAmount() < 0 ? -0.5 : 0.5) * previous->lineWidth();
                              connected = true;
                              }
                        }
                  }
            const qreal partialHeight = score()->styleP(Sid::guitarBendPartialBendHeight);
            if (pre) {
                  end.setX(start.x());
                  end.setY(top);
                  }
            else if (release) {
                  if (!connected)
                        start.setY(top - arrowHeight + 0.5 * b->lineWidth());
                  if (b->totalBendAmount() == 0 && b->endNote())
                        end.setY(ipos2().y() + b->endNote()->bbox().top() - arrowHeight - 0.25 * sp);
                  else
                        end.setY(start.y() + partialHeight);
                  }
            else {
                  end.setY(connected ? start.y() - partialHeight : top);
                  if (small)
                        end.setX(start.x() + 1.25 * sp);
                  end.setX(qMax(end.x(), start.x() + 0.75 * sp));
                  }
            vertex = pre ? (start + end) * 0.5 : QPointF(end.x(), start.y());
            }
      else if (type == GuitarBendType::DIP) {
            // The lowest point of a downward dip stays above both the note
            // and the staff, even for notes on upper ledger lines.
            const qreal noteTop = n ? n->bbox().top() : -0.5 * sp;
            const qreal base = qMin(staffTop - sp, noteTop - sp);
            start = QPointF(-sp, release ? base - 2.0 * sp : base);
            end = QPointF(sp, start.y());
            vertex = QPointF(0.0, release ? base : base - 2.0 * sp);
            }
      else if (type == GuitarBendType::SCOOP) {
            // Use the music font's actual scoop glyph, which is already
            // available in 3.x, rather than approximating it with a corner.
            const QRectF glyph = symBbox(SymId::guitarVibratoBarScoop);
            const qreal noteLeft = n ? -0.5 * (tab ? n->bbox().width() : n->headWidth()) : -0.5 * sp;
            const qreal y = glyph.height() + (tab && n ? -0.5 * n->bbox().height()
                  : (n && n->line() % 2 ? 0.0 : 0.35 * sp));
            _scoopPos = QPointF(noteLeft - 0.25 * sp - glyph.right(), y);
            start = _scoopPos;
            end = start;
            vertex = start;
            }
      else if (tab && b->isDive()) {
            const bool above = score()->styleB(Sid::guitarDivesAboveStaff);
            const bool negative = b->totalBendAmount() < 0;
            if (pre) {
                  const Note* anchor = score()->styleB(Sid::alignPreBendAndPreDiveToGraceNote)
                        ? n : b->endNote();
                  if (anchor) {
                        // A pre-dive clears the whole chord: below the lowest
                        // fret for a negative dive, above the highest otherwise.
                        anchor = negative ? anchor->chord()->downNote() : anchor->chord()->upNote();
                        start = anchor->pagePos() + QPointF(0.5 * anchor->bbox().width(),
                              negative ? anchor->bbox().bottom() + 0.25 * sp : anchor->bbox().top() - 0.25 * sp)
                              - pagePos() + offset();
                        end.setX(start.x());
                        }
                  }
            else if (const GuitarBend* previous = b->precedingBend()) {
                  if (previous->totalBendAmount() && !previous->segmentsEmpty()) {
                        const auto* segment = static_cast<const GuitarBendSegment*>(previous->backSegment());
                        if (segment->system() == system())
                              start = segment->path().pointAtPercent(1.0) + segment->pagePos() - pagePos() + offset();
                        }
                  }
            if (above) {
                  start.setY(staffTop - 2.5 * lineDist);
                  end.setY(start.y() + (negative ? 1.0 : -1.0) * sp);
                  }
            else if (b->totalBendAmount() == 0 && !pre && b->endNote())
                  end.setY(ipos2().y() + b->endNote()->bbox().top());
            else
                  end.setY(negative ? staffTop + (tabType->lines() + 0.5) * lineDist + b->diveLevelIndex() * sp
                        : staffTop - 1.5 * lineDist - b->diveLevelIndex() * sp);
            straight = true;
            vertex = (start + end) * 0.5;
            }
      else if (type == GuitarBendType::SLIGHT_BEND) {
            start = QPointF((n ? n->headWidth() * 0.5 : 0.5 * sp) + 0.25 * sp, -0.25 * sp);
            end = start + QPointF(1.25 * sp, sign * sp);
            vertex = QPointF(end.x(), start.y());
            curved = true;
            }
      else {
            // Start and end just outside their actual noteheads. A vertex
            // displaced perpendicular to their span avoids the very tall,
            // narrow triangles previously produced on grace-note bends.
            const Note* arrival = b->endNote();
            const Chord* fromChord = n->chord();
            const Chord* arrivalChord = arrival ? arrival->chord() : nullptr;
            const bool inside = arrivalChord && fromChord->notes().size() > 2 && arrivalChord->notes().size() > 2
                  && n != fromChord->upNote() && n != fromChord->downNote()
                  && arrival != arrivalChord->upNote() && arrival != arrivalChord->downNote();
            if (n && isSingleBeginType()) {
                  const qreal pad = 0.2 * sp * n->mag();
                  start = inside ? QPointF(0.5 * n->bbox().width() + pad, sign * pad)
                        : QPointF(pad, down ? n->bbox().bottom() + pad : n->bbox().top() - pad);
                  }
            if (arrival && isSingleEndType()) {
                  const qreal pad = 0.2 * sp * arrival->mag();
                  end = inside ? ipos2() + QPointF(-0.5 * arrival->bbox().width() - pad, sign * pad)
                        : QPointF(ipos2().x() - pad,
                              ipos2().y() + (down ? arrival->bbox().bottom() + pad : arrival->bbox().top() - pad));
                  }
            // Studio moves angular bend attachments away from staff-line
            // ink, rather than allowing a grace-note endpoint to land on it.
            clearStaffLine(start);
            clearStaffLine(end);
            // Studio's angular bends clear the chord shape at attachment
            // height. Include note and shared-group parentheses, which can
            // belong to a different note from the bend's anchor.
            auto clearChord = [&](const Note* anchor, QPointF& point, bool fromStart) {
                  const qreal margin = 0.1 * sp;
                  const qreal pad = 0.2 * sp;
                  auto clearInk = [&](const QRectF& ink) {
                        if (!ink.isEmpty() && ink.bottom() >= point.y() - margin && ink.top() <= point.y() + margin)
                              point.setX(fromStart ? qMax(point.x(), ink.right() + pad)
                                    : qMin(point.x(), ink.left() - pad));
                        };
                  Chord* chord = anchor->chord();
                  if (chord->stem() && chord->stem()->visible())
                        clearInk(chord->stem()->bbox().translated(chord->stem()->pagePos() - pagePos()));
                  for (const LedgerLine* line = chord->ledgerLines(); line; line = line->next())
                        if (line->visible())
                              clearInk(line->bbox().translated(line->pagePos() - pagePos()));
                  for (const Note* owner : anchor->chord()->notes()) {
                        clearInk(owner->bbox().translated(owner->pagePos() - pagePos()));
                        if (owner->accidental() && owner->accidental()->visible())
                              clearInk(owner->accidental()->bbox().translated(owner->accidental()->pagePos() - pagePos()));
                        for (const Element* child : owner->el()) {
                              if (!child->isSymbol() || !child->visible() || !score()->tagIsValid(child->tag()))
                                    continue;
                              const Symbol* symbol = toSymbol(child);
                              if (symbol->sym() != SymId::noteheadParenthesisLeft
                                    && symbol->sym() != SymId::noteheadParenthesisRight)
                                    continue;
                              const QPointF origin = child->pagePos() - pagePos();
                              QRectF ink = child->bbox().translated(origin);
                              if (ink.bottom() < point.y() - margin || ink.top() > point.y() + margin)
                                    continue;
                              if (!symbol->tabParenthesisPath().isEmpty()) {
                                    QPainterPath band;
                                    band.addRect(QRectF(ink.left() - sp, point.y() - margin,
                                          ink.width() + 2.0 * sp, 2.0 * margin));
                                    QPainterPathStroker outline;
                                    outline.setWidth(0.05 * symbol->spatium() * symbol->mag());
                                    outline.setCapStyle(Qt::RoundCap);
                                    outline.setJoinStyle(Qt::RoundJoin);
                                    const QPainterPath& path = symbol->tabParenthesisPath();
                                    const QPainterPath inkPath = path.united(outline.createStroke(path));
                                    const QPainterPath clipped = inkPath.translated(origin).intersected(band);
                                    if (clipped.isEmpty())
                                          continue;
                                    ink = clipped.boundingRect();
                                    }
                              point.setX(fromStart ? qMax(point.x(), ink.right() + pad)
                                    : qMin(point.x(), ink.left() - pad));
                              }
                        }
                  };
            const bool up = !down;
            const bool adjustStart = isSingleBeginType()
                  && ((fromChord->stem() && fromChord->up() && up)
                        || (up && n != fromChord->upNote()) || (!up && n != fromChord->downNote()));
            const bool adjustEnd = arrivalChord && isSingleEndType()
                  && ((arrivalChord->stem() && !arrivalChord->up() && !up)
                        || (up && arrival != arrivalChord->upNote()) || (!up && arrival != arrivalChord->downNote()));
            // A simple outside bend deliberately keeps its centred attachment,
            // even with parentheses. Only these cases invoke Studio's adjustX.
            if (inside || adjustStart || adjustEnd) {
                  if (isSingleBeginType())
                        clearChord(n, start, true);
                  if (arrival && isSingleEndType())
                        clearChord(arrival, end, false);
                  }
            const QPointF span = end - start;
            const qreal length = qMax(0.01 * sp, QLineF(start, end).length());
            const qreal height = qBound(0.75 * sp, 0.75 * sp + 0.1 * (span.x() - sp), 2.0 * sp);
            vertex = (start + end) * 0.5 + QPointF(-span.y(), span.x()) * (sign * height / length);
            clearStaffLine(vertex);
            }
      // Studio staggers different bend amounts within one TAB chord. Use
      // the first bend on a higher string, independent of layout visit order.
      bool staggerLabel = false;
      if (tabArrow && type != GuitarBendType::PRE_BEND && isSingleEndType()) {
            const GuitarBend* other = nullptr;
            for (const Note* note : n->chord()->notes()) {
                  if (note->string() >= n->string())
                        continue;
                  for (const Spanner* spanner : note->spannerFor()) {
                        if (spanner->isGuitarBend() && !toGuitarBend(spanner)->isDive()) {
                              other = toGuitarBend(spanner);
                              break;
                              }
                        }
                  if (other)
                        break;
                  }
            if (other && other->bendText() != b->bendText()) {
                  end += QPointF(sp, sp);
                  vertex.rx() += sp;
                  staggerLabel = true;
                  }
            }
      end += userOff2();
      vertex += _vertexOff;
      _drawStart = start;
      _drawEnd = end;
      _vertex = vertex;
      if (type != GuitarBendType::SCOOP)
            _path.moveTo(start);
      if (type == GuitarBendType::SCOOP) {
            // Native glyph is drawn below; it has no editable line path.
            }
      else if (curved)
            _path.cubicTo(start, vertex, end);
      else if (straight)
            _path.lineTo(end);
      else {
            _path.lineTo(vertex);
            _path.lineTo(end);
            }
      if (tabArrow && isSingleEndType()) {
            const qreal d = release ? 1.0 : -1.0;
            // The curve joins the arrow base; its cap is covered by the head.
            _arrow << end + QPointF(0.0, d * arrowHeight)
                  << end + QPointF(-0.5 * arrowWidth, 0.0)
                  << end + QPointF(0.5 * arrowWidth, 0.0);
            }
      if ((tab || type == GuitarBendType::DIP) && isSingleEndType() && type != GuitarBendType::SCOOP) {
            _text = b->bendText();
            const QFontMetricsF fm(labelFont(), MScore::paintDevice());
            const QRectF ink = fm.tightBoundingRect(_text);
            _textRect = ink;
            const QPointF labelAnchor = type == GuitarBendType::DIP
                  ? QPointF((start.x() + end.x()) * 0.5, start.y())
                  : (_arrow.isEmpty() ? end : _arrow.front());
            const bool belowStaff = tab && b->isDive() && type != GuitarBendType::DIP
                  && end.y() > staffTop && !score()->styleB(Sid::guitarDivesAboveStaff);
            if (tabArrow && release)
                  _textRect.moveTopLeft(labelAnchor + QPointF(-_textRect.width() * 0.5, 0.2 * sp));
            else if (belowStaff)
                  _textRect.moveTopLeft(labelAnchor + QPointF(-_textRect.width() * 0.5, 0.35 * sp));
            else
                  _textRect.moveBottomLeft(labelAnchor + QPointF(-_textRect.width() * 0.5, -0.2 * sp));
            if (staggerLabel)
                  _textRect.translate(0.35 * _textRect.width(), 0.0);
            // Draw from the measured ink baseline. Rectangle alignment adds
            // font line spacing and can make the visible arrow-label gap larger.
            _textPos = _textRect.topLeft() - ink.topLeft();
            }
      const int hold = b->getProperty(Pid::BEND_SHOW_HOLD_LINE).toInt();
      const Note* arrival = b->endNote();
      const Note* holdEnd = arrival && tab && !small ? b->holdEndNote() : nullptr;
      const bool fullRelease = b->totalBendAmount() == 0;
      if (tab && arrival && !small && isSingleEndType() && hold != 2
            && (hold == 1 || (!fullRelease && holdEnd != arrival))) {
            const Note* last = holdEnd;
            if (last && last->chord()->segment()->system() == system()) {
                  qreal right = last == arrival ? end.x() + 2.0 * sp
                        : last->pagePos().x() + 0.5 * last->bbox().width() - pagePos().x();
                  bool nextBend = false;
                  for (const Spanner* spanner : last->spannerFor())
                        nextBend |= spanner->isGuitarBend();
                  if (b->isDive() && !nextBend) {
                        // Studio's terminal hold reaches across the arrival's
                        // rhythmic segment, rather than stopping at its fret.
                        right = last->pagePos().x() + last->chord()->segment()->width()
                              - 1.5 * sp - pagePos().x();
                        }
                  const QPointF from = _arrow.isEmpty() ? end : _arrow.front();
                  if (right > from.x() + 0.2 * sp) {
                        _holdPath.moveTo(from + QPointF(b->isDive() ? 0.0 : 0.2 * sp, 0.0));
                        _holdPath.lineTo(QPointF(right, from.y()));
                        }
                  }
            }
      QPainterPathStroker stroker;
      stroker.setWidth(qMax(b->lineWidth(), 0.2 * sp));
      QRectF bounds = type == GuitarBendType::SCOOP
            ? symBbox(SymId::guitarVibratoBarScoop).translated(_scoopPos)
            : stroker.createStroke(_path).boundingRect();
      if (!_holdPath.isEmpty())
            bounds |= stroker.createStroke(_holdPath).boundingRect();
      if (!_arrow.isEmpty())
            bounds |= _arrow.boundingRect();
      if (!_text.isEmpty())
            bounds |= _textRect;
      setbbox(bounds);
      }

Shape GuitarBendSegment::shape() const
      {
      Shape result;
      if (guitarBend()->bendType() == GuitarBendType::SCOOP) {
            result.add(bbox());
            return result;
            }
      // Narrow rectangles follow the strokes so the empty area beneath a
      // large bend does not become one solid collision obstacle.
      const qreal pad = qMax(0.05 * spatium(), 0.5 * guitarBend()->lineWidth());
      auto addPath = [&](const QPainterPath& path) {
            if (path.isEmpty())
                  return;
            const int count = qBound(1, qCeil(path.length() / (0.2 * spatium())), 512);
            QPointF previous = path.pointAtPercent(0.0);
            for (int i = 1; i <= count; ++i) {
                  const QPointF point = path.pointAtPercent(qreal(i) / count);
                  result.add(QRectF(previous, point).normalized().adjusted(-pad, -pad, pad, pad));
                  previous = point;
                  }
            };
      addPath(_path);
      addPath(_holdPath);
      if (!_arrow.isEmpty())
            result.add(_arrow.boundingRect());
      if (!_text.isEmpty())
            result.add(_textRect);
      return result;
      }

QPen GuitarBendSegment::linePen() const
      {
      const GuitarBend* bend = guitarBend();
      QPen pen(curColor(), bend->lineWidth(), Qt::SolidLine, Qt::FlatCap,
            bend->bendType() == GuitarBendType::DIP ? Qt::BevelJoin : Qt::MiterJoin);
      const Note* note = bend->startNote();
      if ((!note || note->staff()->isTabStaff(note->tick())) && bend->bendType() == GuitarBendType::PRE_DIVE) {
            // Studio distributes a nominal 3/3 pattern, with a complete
            // dash at each end. Qt's default DashLine uses 4/2 and round
            // caps would further lengthen the ink into each gap.
            const qreal length = pen.widthF() > 0.0 ? _path.length() / pen.widthF() : 0.0;
            const int pairs = qMax(1, int(length / 6.0));
            const qreal dash = length > 0.0 ? qMin(3.0, length / 3.0) : 3.0;
            const qreal gap = length > 0.0 ? qMax(0.01, (length - dash * (pairs + 1)) / pairs) : 3.0;
            pen.setDashPattern(QVector<qreal> { dash, gap });
            }
      return pen;
      }

void GuitarBendSegment::draw(QPainter* p) const
      {
      p->setPen(linePen());
      p->setBrush(Qt::NoBrush);
      if (guitarBend()->bendType() == GuitarBendType::SCOOP)
            drawSymbol(SymId::guitarVibratoBarScoop, p, _scoopPos);
      else
            p->drawPath(_path);
      if (!guitarBend()->startNote() && guitarBend()->bendType() == GuitarBendType::GRACE_NOTE_BEND) {
            p->setPen(curColor());
            const qreal sp = spatium();
            p->save();
            p->translate(0.0, sp);
            p->scale(0.7, 0.7);
            drawSymbol(SymId::noteheadBlack, p, QPointF());
            p->restore();
            p->setPen(QPen(curColor(), 0.12 * sp, Qt::SolidLine, Qt::FlatCap));
            p->drawLine(QPointF(2.7 * sp, 0.0),
                  QPointF(3.3 * sp + symBbox(SymId::noteheadBlack).width(), 0.0));
            p->setPen(curColor());
            drawSymbol(SymId::noteheadBlack, p, QPointF(3.0 * sp, 0.0));
            }
      if (!_holdPath.isEmpty()) {
            p->setPen(QPen(curColor(), guitarBend()->lineWidth(), Qt::DashLine, Qt::FlatCap));
            p->drawPath(_holdPath);
            }
      if (!_arrow.isEmpty()) {
            p->setPen(Qt::NoPen);
            p->setBrush(curColor());
            p->drawPolygon(_arrow);
            }
      if (!_text.isEmpty()) {
            p->setFont(labelFont(MScore::pixelRatio));
            p->setPen(curColor());
            p->drawText(_textPos, _text);
            }
      }

} // namespace Ms
