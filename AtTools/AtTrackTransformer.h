#ifndef ATTRACKTRANSFORMER_H
#define ATTRACKTRANSFORMER_H

#include "AtTrackClusterBuilder.h" // for AtTools::CovarianceMode type alias

#include <Rtypes.h>

class AtTrack;

namespace AtTools {

class AtTrackTransformer {

public:
   /// Backward-compatible alias — enum is now defined in AtTrackClusterBuilder.h
   using CovarianceMode = AtTools::CovarianceMode;

   AtTrackTransformer();
   ~AtTrackTransformer();

   /// Set diffusion and drift parameters for cluster covariance calculation.
   /// If not called, defaults are used (which may not match the detector).
   /// @param coefT Transverse diffusion coefficient [cm^2/us]
   /// @param coefL Longitudinal diffusion coefficient [cm^2/us]
   /// @param driftVel Electron drift velocity [cm/us]
   /// @param tbTime Time bucket duration [us]
   /// @param padResXY Pad position resolution in X/Y [mm] (default: padSize/sqrt(12))
   void SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime, double padResXY = -1);
   void SetCovarianceMode(CovarianceMode mode) { fCovarianceMode = mode; }
   CovarianceMode GetCovarianceMode() const { return fCovarianceMode; }

   void ClusterizeSmooth3D(AtTrack &track, Float_t radius, Float_t distance);

   /// @brief Group consecutive hits into clusters of fixed size along the track.
   ///
   /// Simpler alternative to ClusterizeSmooth3D: no smoothing pass, no midpoint
   /// insertion. Groups `hitsPerCluster` consecutive hits and computes charge-weighted
   /// centroid. Produces clusters that sit directly on the track.
   /// @param track Track with raw hits
   /// @param hitsPerCluster Number of hits per cluster (default 15 → ~18 clusters for 280 hits)
   void ClusterizeByGroup(AtTrack &track, int hitsPerCluster = 15);
   const std::tuple<Double_t, Double_t> GetPIDFromHits(AtTrack &track, Double_t theta);

   Bool_t FindVertexTrack(AtTrack *trA, AtTrack *trB);

   Bool_t MergeTracks(std::vector<AtTrack *> *trackCandSource, std::vector<AtTrack> *trackDest,
                      Bool_t enableSingleVertexTrack, Double_t clusterRadius, Double_t clusterDistance);

private:
   double fCoefT{0.00009};      ///< Transverse diffusion coefficient [cm^2/us]
   double fCoefL{0.0000009};    ///< Longitudinal diffusion coefficient [cm^2/us]
   double fDriftVel{1.0};       ///< Electron drift velocity [cm/us]
   double fTBTime{0.320};       ///< Time bucket duration [us]
   double fPadResXY{2.3};       ///< Pad position resolution in X/Y [mm] (default: 8mm/sqrt(12))
   CovarianceMode fCovarianceMode{CovarianceMode::TransformerDirect};
};

} // namespace AtTools

#endif
