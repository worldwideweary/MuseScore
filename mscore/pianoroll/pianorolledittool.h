//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __PIANOROLLEDITTOOL_H__
#define __PIANOROLLEDITTOOL_H__

#include <QColor>

#include "preferences.h"

namespace Ms {

enum PianoRollEditTool
{
      SELECT,
      ADD,
      CUT,
      ERASE,
      EVENT_ADJUST,
      TIE, //deprecated
      APPEND_NOTE, //deprecated

      LAST  //Marker for end of list - not a tool
      };

enum class PianoRollScope {
      STAFF,
      PART,
      SCORE
      };

enum class Coloring {
      VOICING,
      STAFF,
      INSTRUMENT,
      };

enum class PianoRollOrientation {
      HORIZONTAL,
      VERTICAL
      };

enum class VerticalPitchLayout {
      CHROMATIC,
      KEYBOARD_ALIGNED
      };

class Note;

QColor pianoRollNoteColor(const Note* note,
                          Coloring coloring,
                          bool honorSelection,
                          bool honorCustomColor);

bool darkTheme();

} // namespace Ms

#endif

