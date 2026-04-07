#ifndef ATBEAMTRACKREJECTOR_H
#define ATBEAMTRACKREJECTOR_H

#include "AtPatternTransform.h"

class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Removes beam-like tracks from the pattern event.
 *
 * A track is considered beam-like if its lab-frame scattering angle is
 * below minLabTheta degrees (forward) or above (180 - minLabTheta) degrees
 * (backward). Lab angle is computed from the direction between the vertex-end
 * cluster and the far-end cluster.
 *
 * @ingroup PatternTransforms
 */
class AtBeamTrackRejector : public AtPatternTransform {
public:
   explicit AtBeamTrackRejector(double minLabTheta = 10.0) : fMinLabTheta(minLabTheta) {}

   void Transform(AtPatternEvent &event) override;

   void SetMinLabTheta(double deg) { fMinLabTheta = deg; }

private:
   double fMinLabTheta{10.0};
};

} // namespace AtPATTERN

#endif // ATBEAMTRACKREJECTOR_H
