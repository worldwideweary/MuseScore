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

#include "inspector.h"
#include "inspectorTextLine.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>

namespace Ms {

//---------------------------------------------------------
//   InspectorTextLine
//---------------------------------------------------------

InspectorTextLine::InspectorTextLine(QWidget* parent, bool tempoChange)
   : InspectorTextLineBase(parent)
      {
      ttl.setupUi(addWidget());

      const std::vector<InspectorItem> il = {
            { Pid::PLACEMENT,   0, ttl.placement,      ttl.resetPlacement        },
            { Pid::SYSTEM_FLAG, 0, ttl.systemTextLine, 0                         },
            };
      const std::vector<InspectorPanel> ppList = {
            { ttl.title, ttl.panel },
            };

      populatePlacement(ttl.placement);
      mapSignals(il, ppList);
      if (tempoChange) {
            ttl.systemTextLine->hide();
            QWidget* row = addWidget();
            QHBoxLayout* layout = new QHBoxLayout(row);
            layout->addWidget(new QLabel(tr("Target tempo:"), row));
            QDoubleSpinBox* amount = new QDoubleSpinBox(row);
            amount->setRange(1.0, 1000.0);
            amount->setDecimals(1);
            amount->setSingleStep(5.0);
            amount->setSuffix(" %");
            layout->addWidget(amount);
            mapSignals({ { Pid::TEMPO_CHANGE_FACTOR, 0, amount, nullptr } });
            }
      }
}

