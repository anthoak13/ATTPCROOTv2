#ifndef ATFITTERUKF_H
#define ATFITTERUKF_H

#include "AtFitter.h"
#include "AtTrackTransformer.h"

#include <Math/Point3D.h>
#include <Math/Point3Dfwd.h>
#include <Math/Vector3D.h>
#include <Math/Vector3Dfwd.h>
#include <Rtypes.h>
#include <TMatrixD.h>

#include <memory>

class AtFitMetadata;
class AtFittedTrack;
class AtRawEvent;
class AtEvent;
class AtTrack;

namespace AtTools {
class AtELossModel;
}
namespace kf {
class TrackFitterUKF;
}

namespace EventFit {

/**
 * @brief FairTask-compatible UKF track fitter.
 *
 * Wraps kf::TrackFitterUKF and implements the EventFit::AtFitter interface so it
 * can be plugged into AtFitterTask.  For each AtTrack, the fitter:
 *   1. Seeds the momentum from the circle-fit Brho.
 *   2. Runs the UKF forward pass (predict+correct per cluster).
 *   3. Runs the RTS smoother.
 *   4. Returns an AtFittedTrack containing vertex kinematics and smoothed positions.
 *
 * Type convention (from CLAUDE.md): this class is never persisted to disk, so
 * plain C++ types (double/int/bool) are used — not ROOT typedefs.
 */
class AtFitterUKF : public AtFitter {
public:
   AtFitterUKF(double charge, double mass_MeV, std::unique_ptr<AtTools::AtELossModel> elossModel);
   ~AtFitterUKF(); // defined in .cxx so kf::TrackFitterUKF is complete there

   /// Set the magnetic field (Tesla).  May be called at any time; if the UKF is already initialized the update is propagated immediately.
   void SetBField(ROOT::Math::XYZVector bField);
   /// Set the electric field (V/m, default {0,0,0}).  May be called at any time; if the UKF is already initialized the update is propagated immediately.
   void SetEField(ROOT::Math::XYZVector eField);
   /// Set UKF sigma-point scaling parameters (alpha, beta, kappa).
   void SetUKFParameters(double alpha, double beta, double kappa);
   /// Set position measurement sigma (mm, default 1.0).
   void SetMeasurementSigma(double sigma_mm) { fMeasSigma_mm = sigma_mm; }
   /// Enable energy straggling in the propagator (default false).
   /// Only enable if the ELoss model supports GetRangeVariance() over the full energy range.
   void SetEnableEnergyStraggling(bool enable) { fEnableEnStraggling = enable; }
   /// Set fractional momentum uncertainty for the initial covariance (default 0.1).
   void SetMomentumSigmaFrac(double frac) { fMomSigmaFrac = frac; }
   /// Minimum number of clusters required to attempt a fit (default 3).
   void SetMinClusters(int n) { fMinClusters = n; }
   /// Override the momentum seed (MeV/c). If > 0, this value is used instead of Brho.
   void SetMomentumSeed(double p_MeV) { fMomentumSeed = p_MeV; }
   /// Number of fit iterations (default 1). With >1, the first pass uses wide covariance
   /// and subsequent passes use the previous result as seed with tighter covariance.
   void SetNIterations(int n) { fNIterations = n; }
   /// Use per-cluster covariance matrix from AtHitCluster::GetCovMatrix() instead of fixed sigma.
   void SetUsePerClusterCov(bool use) { fUsePerClusterCov = use; }
   /// Set the pad plane Z position (mm) for converting digi→lab coordinates.
   /// When set (>0), clusters are transformed to lab frame: Z_lab = ZPadPlane - Z_digi
   /// and sorted from vertex (high Z_lab) toward Bragg peak (low Z_lab).
   void SetZPadPlane(double z) { fZPadPlane = z; }
   void SetClusterCovarianceMode(AtTools::AtTrackTransformer::CovarianceMode mode) { fClusterCovarianceMode = mode; }
   void SetAdaptiveClustering(bool enabled) { fAdaptiveClustering = enabled; }

   /// AtFitter interface — no-op; UKF is created lazily on first GetFittedTrack() call.
   void Init() override {}

protected:
   /// Fit a single AtTrack and return a heap-allocated AtFittedTrack, or nullptr if
   /// the track has fewer than fMinClusters clusters or the UKF fails catastrophically.
   AtFittedTrack *GetFittedTrack(AtTrack *track, AtFitMetadata *fitMetadata = nullptr, AtRawEvent *rawEvent = nullptr,
                                 AtEvent *event = nullptr) override;

private:
   /// Create and configure the kf::TrackFitterUKF.  Moves fELossModel to the propagator.
   /// Must only be called once.
   void InitUKF();

   ROOT::Math::XYZPoint GetInitialPosition(AtTrack *track) const;
   ROOT::Math::XYZVector GetInitialMomentum(AtTrack *track) const;
   TMatrixD GetInitialCovariance(double p_mag_MeV) const;
   TMatrixD GetMeasCovariance() const;
   double GetBrho(AtTrack *track) const;

   // Particle / detector configuration (plain C++ types — not persisted to disk)
   double fCharge;                            // Charge in Coulombs
   double fMass_MeV;                          // Mass in MeV/c^2
   ROOT::Math::XYZVector fBField{0, 0, 2.85}; // Magnetic field, Tesla
   ROOT::Math::XYZVector fEField{0, 0, 0};    // Electric field, V/m

   std::unique_ptr<AtTools::AtELossModel> fELossModel; // Moved to propagator on first use

   // UKF tuning parameters
   double fAlpha{1e-3};
   double fBeta{2.0};
   double fKappa{0.0};
   double fMeasSigma_mm{1.0};
   double fMomSigmaFrac{0.1};
   int fMinClusters{3};
   bool fEnableEnStraggling{true};
   bool fUsePerClusterCov{false};
   double fZPadPlane{-1};      // If >0, convert digi→lab: Z_lab = fZPadPlane - Z_digi
   int fMaxFitTime_ms{2000};   // Maximum time per track fit in milliseconds
   double fMomentumSeed{-1};       // If >0, override Brho seed with this value (MeV/c)
   double fMinClusterSpacing{3.0}; // Minimum distance between clusters (mm)
   double fMinLabTheta{10.0};      // Minimum lab angle (degrees) — skip beam-like tracks
   int fNIterations{1};            // Number of fit iterations (1=single pass)
   bool fAdaptiveClustering{true}; // Re-cluster based on Brho momentum estimate
   AtTools::AtTrackTransformer::CovarianceMode fClusterCovarianceMode{
      AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect};

   // Lazily initialised on first GetFittedTrack() call
   std::unique_ptr<kf::TrackFitterUKF> fUKF;
};

} // namespace EventFit

#endif // ATFITTERUKF_H
