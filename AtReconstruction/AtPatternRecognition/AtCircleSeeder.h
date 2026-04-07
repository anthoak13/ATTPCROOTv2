#ifndef ATCIRCLESEEDER_H
#define ATCIRCLESEEDER_H

#include "AtPatternTransform.h"

class AtPatternEvent;
class AtTrack;

namespace AtPATTERN {

/**
 * @brief Sets initial geometric parameters on each track via circle and line fits.
 *
 * Fits a 2D circle to hits near the vertex end to extract the center, radius,
 * and azimuthal angle (phi). Then maps hits to arc-length vs Z space and fits
 * a line to extract the dip angle (theta). Sets GeoCenter, GeoRadius, GeoPhi,
 * and GeoTheta on each track with sufficient hits.
 *
 * @ingroup PatternTransforms
 */
class AtCircleSeeder : public AtPatternTransform {
public:
   void Transform(AtPatternEvent &event) override;

   /// Max fraction of hits (from vertex end) used for circle fit.
   void SetRadiusFitFraction(double frac) { fRadiusFitFraction = frac; }
   void SetMinHitsRadius(int n) { fMinHitsRadius = n; }
   void SetMaxHitsRadius(int n) { fMaxHitsRadius = n; }

private:
   void SeedTrack(AtTrack &track) const;

   double fRadiusFitFraction{1.0};
   int fMinHitsRadius{3};
   int fMaxHitsRadius{1000};
};

} // namespace AtPATTERN

#endif // ATCIRCLESEEDER_H
