//=============================================================================
//  Awl
//  Audio Widget Library
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "pitchedit.h"
#include "utils.h"

namespace Awl {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

PitchEdit::PitchEdit(QWidget* parent)
  : QSpinBox(parent)
      {
      setRange(0, 127);
      deltaMode = false;
      }

//---------------------------------------------------------
//   validate
//---------------------------------------------------------

QValidator::State PitchEdit::validate(QString& input, int& /*pos*/) const
      {
      QString s = input.trimmed();

      s.replace(QChar(0x266F), QChar('#')); // ♯ -> #
      s.replace(QChar(0x266D), QChar('b')); // ♭ -> b

      if (s.isEmpty())
            return QValidator::Intermediate;

      //
      // Complete pitch name.
      //
      const QRegExp complete(
            "^[A-Ga-g](#{0,3}|b{0,3})-?[0-9]$");

      if (complete.exactMatch(s)) {
            int parsedPitch;
            int parsedTpc;

            if (parsePitchText(s, parsedPitch, parsedTpc))
                  return QValidator::Acceptable;

            return QValidator::Invalid;
            }

      //
      // Legal partial input while typing:
      // C
      // C#
      // C##
      // C-
      // etc.
      //
      const QRegExp partial(
            "^[A-Ga-g](#{0,3}|b{0,3})?-?$");

      if (partial.exactMatch(s))
            return QValidator::Intermediate;

      return QValidator::Invalid;
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void PitchEdit::keyPressEvent(QKeyEvent* ev)
      {
      if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
            int newPitch;
            int newTpc;

            if (parsePitchText(lineEdit()->text(), newPitch, newTpc))
                  _typedTpc = newTpc;
            else
                  _typedTpc = Ms::TPC_INVALID;

            interpretText();

            emit returnPressed();
            ev->accept();
            return;
            }

      if (ev->key() == Qt::Key_Escape) {
            emit escapePressed();
            return;
            }

      QSpinBox::keyPressEvent(ev);
      }

//---------------------------------------------------------
//   textFromValue
//---------------------------------------------------------

QString PitchEdit::textFromValue(int v) const
      {
      if (deltaMode)
            return QString("%1").arg(v);
      else
            return pitch2string(v);
      }

//---------------------------------------------------------
//   valueFromText
//---------------------------------------------------------

int PitchEdit::valueFromText(const QString& text) const
      {
      int newPitch;
      int newTpc;

      if (!parsePitchText(text, newPitch, newTpc))
            return value();

      return newPitch;
      }

//---------------------------------------------------------
//   parsePitchText
//---------------------------------------------------------

bool PitchEdit::parsePitchText(const QString& text,
                              int& newPitch,
                              int& newTpc) const
      {
      QString s = text.trimmed();

      // Accept the accidental glyphs produced by pitch2string()
      // as well as ASCII accidentals typed by the user.
      s.replace(QChar(0x266F), QChar('#')); // ♯
      s.replace(QChar(0x266D), QChar('b')); // ♭

      const QRegExp rx(
            "^([A-Ga-g])(#{0,3}|b{0,3})(-?[0-9])$");

      if (!rx.exactMatch(s))
            return false;

      const QString stepName = rx.cap(1);
      const QString accidentalText = rx.cap(2);

      Ms::AccidentalVal accidental = Ms::AccidentalVal::NATURAL;

      if (accidentalText == "#")
            accidental = Ms::AccidentalVal::SHARP;
      else if (accidentalText == "##")
            accidental = Ms::AccidentalVal::SHARP2;
      else if (accidentalText == "###")
            accidental = Ms::AccidentalVal::SHARP3;
      else if (accidentalText == "b")
            accidental = Ms::AccidentalVal::FLAT;
      else if (accidentalText == "bb")
            accidental = Ms::AccidentalVal::FLAT2;
      else if (accidentalText == "bbb")
            accidental = Ms::AccidentalVal::FLAT3;

      bool ok = false;
      const int octave = rx.cap(3).toInt(&ok);
      if (!ok)
            return false;

      newTpc = Ms::step2tpc(stepName, accidental);

      if (!Ms::tpcIsValid(newTpc))
            return false;

      newPitch = (octave + 1) * 12 + Ms::tpc2pitch(newTpc);

      return Ms::pitchIsValid(newPitch);
      }

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void PitchEdit::setDeltaMode(bool val)
      {
      deltaMode = val;
      if (deltaMode)
            setRange(-127, 127);
      else
            setRange(0, 127);
      }

//---------------------------------------------------------
//   typedTpc
//---------------------------------------------------------

int PitchEdit::typedTpc() const
      {
      return _typedTpc;
      }

}

