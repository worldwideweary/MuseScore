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
      const QString s = input.trimmed();

      if (s.isEmpty())
            return QValidator::Intermediate;

      //
      // Complete pitch name.
      //
      const QRegExp complete(
            "^[A-Ga-g](#{0,3}|b{0,3})-?[0-9]$");

      if (complete.exactMatch(s)) {
            const int pitch = valueFromText(s);

            if (_typedTpc != Ms::TPC_INVALID
                && Ms::pitchIsValid(pitch))
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
      const QString s = text.trimmed();

      _typedTpc = Ms::TPC_INVALID;

      //
      // Full pitch syntax:
      //
      //   C4
      //   F#4
      //   Gb4
      //   C##5
      //   Ebb3
      //   C-1
      //
      const QRegExp rx(
            "^([A-Ga-g])(#{0,3}|b{0,3})(-?[0-9])$");

      if (!rx.exactMatch(s))
            return value();

      const QString stepName = rx.cap(1);
      const QString accidentalText = rx.cap(2);

      bool octaveOk = false;
      const int octave = rx.cap(3).toInt(&octaveOk);

      if (!octaveOk)
            return value();

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

      const int tpc = Ms::step2tpc(stepName, accidental);

      if (!Ms::tpcIsValid(tpc))
            return value();

      //
      // tpc2pitch() is relative to C of the octave and can legitimately
      // produce values outside 0..11 for spellings such as B# or Cb.
      //
      const int pitch =
            (octave + 1) * Ms::PITCH_DELTA_OCTAVE
            + Ms::tpc2pitch(tpc);

      if (!Ms::pitchIsValid(pitch))
            return value();

      _typedTpc = tpc;
      return pitch;
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

