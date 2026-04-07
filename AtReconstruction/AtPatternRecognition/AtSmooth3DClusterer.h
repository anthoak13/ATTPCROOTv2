#ifndef ATSMOOTH3DCLUSTERER_H
#define ATSMOOTH3DCLUSTERER_H

#include "AtPatternTransform.h"

#include "AtTrackClusterBuilder.h"

class AtPatternEvent;
class AtTrack;

namespace AtPATTERN {

/**
 * @brief Groups hits within each track into smoothed 3D clusters.
 *
 * Two-pass algorithm: first clusters hits by proximity to reference points,
 * then re-clusters at midpoints between adjacent clusters with half the radius.
 * Covariance matrices incorporate detector diffusion and pad resolution.
 *
 * Delegates to AtTools::ClusterizeSmooth3D for the algorithm, which is the same
 * implementation used by AtTrackTransformer::ClusterizeSmooth3D.
 *
 * @ingroup PatternTransforms
 */
class AtSmooth3DClusterer : public AtPatternTransform {
public:
   AtSmooth3DClusterer(double radius, double distance) : fRadius(radius), fDistance(distance) {}

   void Transform(AtPatternEvent &event) override;

   /// @internal Per-track clustering entry point. Called by AtFragmentMerger for
   /// post-merge re-clustering via the non-owning pointer set by SetClusterer().
   /// For event-level clustering, use Transform() instead.
   void ClusterizeTrack(AtTrack &track) const;

   void SetRadius(double radius) { fRadius = radius; }
   void SetDistance(double distance) { fDistance = distance; }
   void SetCovarianceMode(AtTools::CovarianceMode mode) { fCovarianceMode = mode; }

   /// Set diffusion and drift parameters for covariance calculation.
   /// @param coefT  Transverse diffusion coefficient [cm^2/us]
   /// @param coefL  Longitudinal diffusion coefficient [cm^2/us]
   /// @param driftVel  Electron drift velocity [cm/us]
   /// @param tbTime  Time bucket duration [us]
   /// @param padResXY  Pad resolution in X/Y [mm] (default: 8mm/sqrt(12))
   void SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime, double padResXY = -1);

private:
   double fRadius{20.0};
   double fDistance{15.0};
   double fCoefT{0.00009};
   double fCoefL{0.0000009};
   double fDriftVel{1.0};
   double fTBTime{0.320};
   double fPadResXY{2.3};
   AtTools::CovarianceMode fCovarianceMode{AtTools::CovarianceMode::TransformerDirect};
};

} // namespace AtPATTERN

#endif // ATSMOOTH3DCLUSTERER_H
