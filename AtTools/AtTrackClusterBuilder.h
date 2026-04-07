#ifndef ATTRACKCLUSTERBUILDER_H
#define ATTRACKCLUSTERBUILDER_H

#include "AtHit.h"

#include <TMatrixDSymfwd.h>
#include <TMatrixTSym.h>

#include <memory>
#include <vector>

class AtHitCluster;
class AtTrack;

namespace AtTools {

/// Covariance calculation mode for hit clustering.
enum class CovarianceMode {
   TransformerDirect,           ///< Diagonal covariance from per-hit diffusion variance (fast, default)
   HitClusterOnline,            ///< Full covariance from online hit-variance propagation
   HitClusterOnlineDiagOnly,    ///< Full variance computed, only diagonal stored
   HitClusterOnlineConsistent,  ///< Full AtHitCluster with internal covariance
};

struct AtTrackClusterBuilderConfig {
   double coefT{0.00009};
   double coefL{0.0000009};
   double driftVel{1.0};
   double samplingRate{0.320};
   double padResXY{2.3};
   double padResZ{3.45};
   CovarianceMode covarianceMode{CovarianceMode::TransformerDirect};
};

class AtTrackClusterBuilder {
public:
   explicit AtTrackClusterBuilder(AtTrackClusterBuilderConfig config);

   std::shared_ptr<AtHitCluster> BuildCluster(const std::vector<AtHit> &hits, int clusterID) const;

private:
   struct TransformerDirectClusterStats {
      double x{0};
      double y{0};
      double z{0};
      double sigmaX2{0};
      double sigmaY2{0};
      double sigmaZ2{0};
      double totalCharge{0};
      int timeStamp{0};
      int nHits{0};
      bool valid{false};
   };

   AtHit::XYZVector GetPerHitVariance(const AtHit &hit) const;
   TransformerDirectClusterStats BuildTransformerDirectClusterStats(const std::vector<AtHit> &hits) const;
   TMatrixDSym BuildTransformerDirectCovariance(const TransformerDirectClusterStats &stats) const;
   AtHitCluster BuildHitClusterOnlineCluster(const std::vector<AtHit> &hits) const;
   TMatrixDSym BuildDiagonalOnlyCovariance(const TMatrixDSym &src) const;

   AtTrackClusterBuilderConfig fConfig;
};

/// @brief Smoothed 3D clustering algorithm — single authoritative implementation.
///
/// Groups consecutive hits into clusters using a sliding reference point, then
/// applies a smoothing pass that re-clusters at midpoints between adjacent clusters
/// with half the radius.  The reference position advances only when a hit exceeds
/// @p distance from the current reference (hits closer than @p distance are skipped
/// with `continue`, leaving the reference anchored until the next far hit).
///
/// Both AtTrackTransformer::ClusterizeSmooth3D and AtSmooth3DClusterer::ClusterizeTrack
/// delegate here so the behaviour is guaranteed identical.
///
/// @param track     Track whose raw hits are clustered (modified in-place).
/// @param radius    Clustering radius — hits within this distance of the reference
///                  are assigned to the current cluster [mm].
/// @param distance  Minimum advance distance — reference only moves when a hit is
///                  at least this far away [mm].
/// @param config    Detector physics and covariance-mode configuration.
void ClusterizeSmooth3D(AtTrack &track, double radius, double distance, const AtTrackClusterBuilderConfig &config);

} // namespace AtTools

#endif
