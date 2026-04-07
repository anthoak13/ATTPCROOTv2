#ifndef ATGROUPCLUSTERER_H
#define ATGROUPCLUSTERER_H

#include "AtPatternTransform.h"

#include "AtTrackClusterBuilder.h"
#include "AtTrackTransformer.h"

class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Groups consecutive hits within each track into fixed-size clusters.
 *
 * Simpler alternative to AtSmooth3DClusterer: no smoothing pass. Groups
 * `hitsPerCluster` consecutive hits and computes a charge-weighted centroid.
 *
 * @ingroup PatternTransforms
 */
class AtGroupClusterer : public AtPatternTransform {
public:
   explicit AtGroupClusterer(int hitsPerCluster = 15) : fHitsPerCluster(hitsPerCluster) {}

   void Transform(AtPatternEvent &event) override;

   void SetHitsPerCluster(int n) { fHitsPerCluster = n; }
   void SetCovarianceMode(AtTools::AtTrackTransformer::CovarianceMode mode) { fCovarianceMode = mode; }

   /// Set diffusion and drift parameters for covariance calculation.
   void SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime, double padResXY = -1);

private:
   int fHitsPerCluster{15};
   double fCoefT{0.00009};
   double fCoefL{0.0000009};
   double fDriftVel{1.0};
   double fTBTime{0.320};
   double fPadResXY{2.3};
   AtTools::AtTrackTransformer::CovarianceMode fCovarianceMode{
      AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect};
};

} // namespace AtPATTERN

#endif // ATGROUPCLUSTERER_H
