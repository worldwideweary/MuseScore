// Copyright (C) 2026. Licensed under GNU GPL version 2 or later.
#ifndef MS_BARLINEGEOMETRY_H
#define MS_BARLINEGEOMETRY_H

#include <QtGlobal>

namespace Ms {
class BarLine;
// Local painted right edge, including scaled repeat dots and repeat tips.
qreal startRepeatRightEdge(const BarLine*);
}

#endif
