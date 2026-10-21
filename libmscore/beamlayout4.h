// Studio 4 beam placement, isolated from the legacy Beam layout path.
#ifndef MS_BEAMLAYOUT4_H
#define MS_BEAMLAYOUT4_H
#include <QPointF>
#include <vector>
namespace Ms {
class Beam;
class ChordRest;
// Returns false for cases whose modern adapter has not been implemented.
// Coordinates are page Y and beam-relative X, matching the Studio engine.
bool layoutBeam4(Beam*, const std::vector<ChordRest*>&, QPointF& start,
                 QPointF& end, qreal& beamDistance);
// Studio's tie-break rule for the distinct note positions of a beam.
// Line numbers increase downwards; middleLine is the staff's centre line.
bool beam4AutoStemUp(std::vector<int> noteLines, int middleLine);
// The fragment's chord/rest vector is in score order. True means left-facing.
bool beam4BeamletBefore(const std::vector<ChordRest*>&, int index, int level);
// Staff-gap and segment-stack portions of Studio's cross-staff layout.
qreal beam4MinStaffDistance(Beam*, int upperStaff, int lowerStaff);
qreal beam4CenterOffset(Beam*, const std::vector<ChordRest*>&);
}
#endif
