//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2010-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __NOTEEVENT_H__
#define __NOTEEVENT_H__

namespace Ms {

class XmlWriter;
class XmlReader;

//---------------------------------------------------------
//    NoteEvent
//---------------------------------------------------------

class NoteEvent {
      int _pitch;   // relative pitch to note pitch
      int _ontime;  // one unit is 1/1000 of nominal note len
      int _len;     // one unit is 1/1000 of nominal note len
      int _vOffset; // an attempt to allow variation within tremolos

   public:
      constexpr static int NOTE_LENGTH = 1000;

      NoteEvent() : _pitch(0), _ontime(0), _len(NOTE_LENGTH), _vOffset(0) {}
      NoteEvent(int pitch, int onTime, int length)
            : _pitch(pitch), _ontime(onTime), _len(length), _vOffset(0)
            {}
      NoteEvent(int pitch, int onTime, int length, int velocityOffset)
            : _pitch(pitch), _ontime(onTime), _len(length), _vOffset(velocityOffset)
            {}

      void read(XmlReader&);
      void write(XmlWriter&) const;

      int  pitch() const     { return _pitch; }
      int ontime() const     { return _ontime; }
      int offtime() const    { return _ontime + _len; }
      int len() const        { return _len; }
      int veloOff() const    { return _vOffset; }
      void setPitch(int v)   { _pitch = v; }
      void setOntime(int v)  { _ontime = v; }
      void setLen(int v)     { _len = v;    }
      void setVeloOff(int v) { _vOffset = v;}
      bool operator==(const NoteEvent&) const;
      };

//---------------------------------------------------------
//   NoteEventList
//---------------------------------------------------------

class NoteEventList : public QList<NoteEvent> {
   public:
      NoteEventList();

      int offtime() { return empty() ? 0 : std::max_element(cbegin(), cend(), [](const NoteEvent& n1, const NoteEvent& n2) { return n1.offtime() < n2.offtime(); })->offtime(); }
      };


}     // namespace Ms
#endif
