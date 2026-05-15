//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "inspectorArpeggio.h"
#include "libmscore/arpeggio.h"
#include "libmscore/chord.h"
#include "libmscore/score.h"
#include "libmscore/tremolo.h"

namespace Ms {

//---------------------------------------------------------
//   InspectorArpeggio
//---------------------------------------------------------

InspectorArpeggio::InspectorArpeggio(QWidget* parent)
   : InspectorElementBase(parent)
      {
      g.setupUi(addWidget());

      const std::vector<InspectorItem> iiList = {
            { Pid::PLAY,            0,    g.playArpeggio, g.resetPlayArpeggio},
            { Pid::TIME_STRETCH,    0,    g.stretch,      g.resetStretch }
            };
      const std::vector<InspectorPanel> ppList = {
            { g.title, g.panel }
            };

      mapSignals(iiList, ppList);
      }

//---------------------------------------------------------
//   InspectorArpeggio
//---------------------------------------------------------

void InspectorArpeggio::valueChanged(int idx)
      {
      // Update Score/PRE Events
      InspectorElementBase::valueChanged(idx);

      if (Arpeggio* arpeggio = toArpeggio(inspector->element())) {
            Score* score = arpeggio->score();
            if (Element* parent = arpeggio->parent()) {
                  if (parent->isChord()) {
                        Chord* chord = toChord(parent);
                        if (chord->tremolo() && chord->tremoloChordType() == TremoloChordType::TremoloSecondNote) {
                              chord = chord->tremolo()->chord1();
                              }
                        score->createPlayEvents(chord);
                        }
                  }
            }
      }

}

