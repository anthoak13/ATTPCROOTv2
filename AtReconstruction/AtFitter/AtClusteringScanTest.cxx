#include "AtELossCATIMA.h"
#include "AtHitCluster.h"
#include "AtKinematics.h"
#include "AtPropagator.h"
#include "AtTrack.h"
#include "AtTrackTransformer.h"

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TMath.h>
#include <TMatrixD.h>

#include <TRandom.h>

#include <cmath>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "OpenKF/kalman_filter/TrackFitterUKF.h"

using ROOT::Math::Polar3DVector;
using ROOT::Math::XYZPoint;
using ROOT::Math::XYZVector;

namespace {

double RMSFromSums(double sum, double sumSq, int n)
{
   if (n <= 0)
      return 0.0;
   double mean = sum / n;
   double variance = sumSq / n - mean * mean;
   return variance > 0.0 ? std::sqrt(variance) : 0.0;
}

struct CovarianceTestParams {
   double coefT;
   double coefL;
   double driftVel;
   double tbTime;
   double padResXY;
   double padResZ;
};

AtHit MakeHit(const XYZPoint &pos, double charge, int timeStamp)
{
   AtHit hit;
   hit.SetPosition(pos);
   hit.SetCharge(charge);
   hit.SetTimeStamp(timeStamp);
   return hit;
}

XYZVector GetExpectedHitVariance(const AtHit &hit, const CovarianceTestParams &params)
{
   double driftTime = hit.GetPosition().Z() / (10.0 * params.driftVel);
   double varT = 100.0 * params.coefT * 2.0 * driftTime;
   double varL = 100.0 * params.coefL * 2.0 * driftTime;
   double tbResMM = params.driftVel * params.tbTime * 10.0;
   double varTB = tbResMM * tbResMM / 12.0;
   return {params.padResXY * params.padResXY + varT, params.padResXY * params.padResXY + varT,
           params.padResZ * params.padResZ + varTB + varL};
}

TMatrixDSym BuildExpectedTransformerDirectCovariance(const std::vector<AtHit> &hits, const CovarianceTestParams &params)
{
   double totalCharge = 0.0;
   double varX = 0.0;
   double varY = 0.0;
   double varZ = 0.0;

   for (const auto &hit : hits) {
      auto hitVar = GetExpectedHitVariance(hit, params);
      double q = hit.GetCharge();
      totalCharge += q;
      varX += q * q * hitVar.X();
      varY += q * q * hitVar.Y();
      varZ += q * q * hitVar.Z();
   }

   TMatrixDSym cov(3);
   cov.Zero();
   cov(0, 0) = varX / (totalCharge * totalCharge);
   cov(1, 1) = varY / (totalCharge * totalCharge);
   cov(2, 2) = varZ / (totalCharge * totalCharge);
   return cov;
}

AtHitCluster BuildExpectedOnlineCluster(const std::vector<AtHit> &hits, const CovarianceTestParams &params)
{
   AtHitCluster cluster;
   for (const auto &hit : hits) {
      AtHit hitWithVar(hit);
      hitWithVar.SetPositionVariance(GetExpectedHitVariance(hitWithVar, params));
      cluster.AddHit(hitWithVar);
   }
   return cluster;
}

AtTrack BuildTrackForClusterTest(const std::vector<AtHit> &hits)
{
   AtTrack track;
   for (const auto &hit : hits) {
      track.AddHit(std::make_unique<AtHit>(hit));
   }
   return track;
}

AtHitCluster ClusterSingleGroup(const std::vector<AtHit> &hits, const CovarianceTestParams &params,
                                AtTools::AtTrackTransformer::CovarianceMode mode)
{
   AtTrack track = BuildTrackForClusterTest(hits);
   AtTools::AtTrackTransformer transformer;
   transformer.SetDiffusionParams(params.coefT, params.coefL, params.driftVel, params.tbTime, params.padResXY);
   transformer.SetCovarianceMode(mode);
   transformer.ClusterizeByGroup(track, static_cast<int>(hits.size()));

   auto *clusters = track.GetHitClusterArray();
   EXPECT_EQ(clusters->size(), 1u);
   return clusters->at(0);
}

void ExpectMatrixNear(const TMatrixDSym &actual, const TMatrixDSym &expected, double tol)
{
   for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 3; ++col)
         EXPECT_NEAR(actual(row, col), expected(row, col), tol);
   }
}

} // namespace

// ===========================================================================
// Generate a realistic proton track using the propagator, then smear the
// hit positions to simulate digitization effects.
// ===========================================================================
class ClusteringScanTest : public testing::Test {
protected:
   static constexpr double kMass = 938.272;
   static constexpr double kCharge = 1.602176634e-19;
   static constexpr double kBz = 2.85;
   static constexpr double kGasDensity = 3.553e-5;

   // True initial state
   XYZPoint kVertex{0, 0, 170};                         // mm
   XYZVector kTrueMom{9.35463, -45.4279, 8.26042};      // MeV/c
   double kTrueP = kTrueMom.R();                         // ~47.1 MeV/c

   // Generate raw hits along a propagated track with position smearing.
   // hitSpacing_mm controls how far apart the hits are placed along the track
   // to simulate the density of pad hits from digitization (~1-2 mm).
   std::vector<XYZPoint> GenerateHits(double hitSpacing_mm = 1.5, double smear_mm = 1.0)
   {
      auto eloss = std::make_unique<AtTools::AtELossCATIMA>(kGasDensity);
      eloss->SetProjectile(1, 1, 1);
      std::vector<std::tuple<int, int, int>> mat;
      mat.push_back({1, 1, 1});
      eloss->SetMaterial(mat);

      AtTools::AtPropagator prop(kCharge, kMass, std::move(eloss));
      prop.SetBField({0, 0, kBz});
      prop.SetState(kVertex, kTrueMom);

      AtTools::AtRK4Stepper stepper;
      std::vector<XYZPoint> hits;
      hits.push_back(kVertex);

      double accumulated = 0;
      for (int i = 0; i < 10000; i++) { // Max iterations
         prop.PropagateOneStep(stepper);
         auto state = prop.GetState();
         if (AtTools::Kinematics::KE(state.fMom, kMass) < 0.05)
            break;

         accumulated += (state.fPos - state.fLastPos).R();
         if (accumulated < hitSpacing_mm)
            continue;
         accumulated = 0;

         // Smear position to simulate pad + diffusion resolution
         double sx = gRandom->Gaus(0, smear_mm);
         double sy = gRandom->Gaus(0, smear_mm);
         double sz = gRandom->Gaus(0, smear_mm * 0.5);
         XYZPoint smeared(state.fPos.X() + sx, state.fPos.Y() + sy, state.fPos.Z() + sz);
         hits.push_back(smeared);
      }
      return hits;
   }

   // Build an AtTrack from raw hit positions
   AtTrack BuildTrack(const std::vector<XYZPoint> &hits)
   {
      AtTrack track;
      for (size_t i = 0; i < hits.size(); i++) {
         auto hit = std::make_unique<AtHit>();
         hit->SetPosition(hits[i]);
         hit->SetCharge(100.0); // Arbitrary charge
         hit->SetTimeStamp(i);
         track.AddHit(std::move(hit));
      }
      // Set geometry params for Brho seed
      track.SetGeoRadius(350.0);
      track.SetGeoTheta(kTrueMom.Theta());
      track.SetGeoPhi(kTrueMom.Phi());
      return track;
   }

   // Create UKF instance
   std::unique_ptr<kf::TrackFitterUKF> CreateUKF()
   {
      auto eloss = std::make_unique<AtTools::AtELossCATIMA>(kGasDensity);
      eloss->SetProjectile(1, 1, 1);
      std::vector<std::tuple<int, int, int>> mat;
      mat.push_back({1, 1, 1});
      eloss->SetMaterial(mat);

      AtTools::AtPropagator propagator(kCharge, kMass, std::move(eloss));
      propagator.SetBField({0, 0, kBz});
      auto stepper = std::make_unique<AtTools::AtRK4AdaptiveStepper>();
      auto ukf = std::make_unique<kf::TrackFitterUKF>(std::move(propagator), std::move(stepper));
      ukf->setParameters(1e-3, 2.0, 0.0);
      ukf->fEnableEnStraggling = true;
      return ukf;
   }

   // Run UKF on hit clusters and return (momentum_error%, converged)
   struct FitResult {
      double momErr;
      double meanResid;
      int nClusters;
      bool converged;
   };

   FitResult RunUKF(kf::TrackFitterUKF &ukf, const std::vector<AtHitCluster> &clusters, bool usePerClusterCov)
   {
      FitResult result{0, 0, static_cast<int>(clusters.size()), false};
      if (clusters.size() < 5)
         return result;

      // Get ordered cluster positions (they're already sorted by ClusterizeSmooth3D)
      std::vector<XYZPoint> pts;
      for (auto &cl : clusters)
         pts.push_back(cl.GetPosition());

      // Filter by minimum spacing
      std::vector<XYZPoint> filtered;
      filtered.push_back(pts[0]);
      for (size_t i = 1; i < pts.size(); i++) {
         if ((pts[i] - filtered.back()).R() >= 2.0) // 2mm min spacing
            filtered.push_back(pts[i]);
      }
      result.nClusters = filtered.size();
      if (result.nClusters < 5)
         return result;

      // Setup
      double sigma_pos = 2.0;
      double sigma_mom = 0.15 * kTrueP;
      double sigma_ang = 5.0 * M_PI / 180.0;
      TMatrixD cov(6, 6);
      cov.Zero();
      for (int i = 0; i < 3; i++)
         cov(i, i) = sigma_pos * sigma_pos;
      cov(3, 3) = sigma_mom * sigma_mom;
      cov(4, 4) = sigma_ang * sigma_ang;
      cov(5, 5) = sigma_ang * sigma_ang;

      TMatrixD measCov(3, 3);
      measCov.Zero();
      for (int i = 0; i < 3; i++)
         measCov(i, i) = sigma_pos * sigma_pos;

      ukf.SetInitialState(filtered[0], kTrueMom, cov);
      ukf.SetMeasCov(measCov);

      try {
         for (size_t i = 1; i < filtered.size(); i++) {
            if (usePerClusterCov) {
               TMatrixD clusterCov(3, 3);
               clusterCov.Zero();
               const auto &srcCov = clusters[i].GetCovMatrix();
               for (int r = 0; r < 3; ++r) {
                  for (int c = 0; c < 3; ++c)
                     clusterCov(r, c) = srcCov(r, c);
                  if (clusterCov(r, r) < 0.01)
                     clusterCov(r, r) = 0.01;
               }
               ukf.SetMeasCov(clusterCov);
            }
            ukf.predictUKF(filtered[i]);
            ukf.correctUKF(filtered[i]);
         }
         ukf.smoothUKF();
      } catch (...) {
         return result;
      }

      result.converged = true;
      double pReco = ukf.GetSmoothedStates()[0][3];
      result.momErr = (pReco - kTrueP) / kTrueP * 100;

      auto &smoothed = ukf.GetSmoothedStates();
      double sumResid = 0;
      int n = std::min(smoothed.size(), filtered.size()) - 1;
      for (int i = 1; i <= n; i++) {
         XYZPoint sp(smoothed[i][0], smoothed[i][1], smoothed[i][2]);
         sumResid += (sp - filtered[i]).R();
      }
      result.meanResid = n > 0 ? sumResid / n : 0;

      return result;
   }

   std::vector<AtHitCluster> ClusterizeTrack(const std::vector<XYZPoint> &hits, double radius, double distance,
                                             AtTools::AtTrackTransformer::CovarianceMode covarianceMode)
   {
      AtTrack track = BuildTrack(hits);
      AtTools::AtTrackTransformer transformer;
      transformer.SetCovarianceMode(covarianceMode);
      transformer.ClusterizeSmooth3D(track, radius, distance);
      return *track.GetHitClusterArray();
   }
};

// ===========================================================================
// Test: scan clustering radius and distance parameters
// ===========================================================================
TEST_F(ClusteringScanTest, RadiusDistanceScan)
{
   // Generate raw hits
   auto hits = GenerateHits(1.5, 1.0);
   ASSERT_GT(hits.size(), 50u) << "Not enough hits generated";

   auto ukf = CreateUKF();

   struct Config {
      double radius;
      double distance;
   };
   std::vector<Config> configs = {
      {5.0, 10.0},  {5.0, 15.0},  {10.0, 10.0}, {10.0, 15.0}, {10.0, 20.0},
      {15.0, 15.0}, {15.0, 20.0}, {15.0, 30.0}, {15.0, 30.5}, {20.0, 20.0},
      {20.0, 30.0}, {20.0, 40.0}, {25.0, 30.0}, {25.0, 40.0},
   };

   std::cout << "\n"
             << std::setw(10) << "radius" << std::setw(10) << "distance" << std::setw(10) << "clusters"
             << std::setw(12) << "mom_err(%)" << std::setw(10) << "resid(mm)" << std::setw(10) << "status"
             << std::endl;
   std::cout << std::string(62, '-') << std::endl;

   int nConverged = 0;
   for (auto &cfg : configs) {
      AtTrack track = BuildTrack(hits);
      AtTools::AtTrackTransformer transformer;
      transformer.ClusterizeSmooth3D(track, cfg.radius, cfg.distance);

      auto *clusters = track.GetHitClusterArray();
      auto result = RunUKF(*ukf, *clusters, false);

      std::string status = result.converged ? "OK" : "FAIL";
      if (result.converged && std::abs(result.momErr) > 5.0)
         status = "WARN";

      std::cout << std::setw(10) << cfg.radius << std::setw(10) << cfg.distance << std::setw(10) << result.nClusters
                << std::setw(12) << (result.converged ? std::to_string(result.momErr).substr(0, 6) : "---")
                << std::setw(10) << (result.converged ? std::to_string(result.meanResid).substr(0, 5) : "---")
                << std::setw(10) << status << std::endl;

      if (result.converged)
         nConverged++;
   }

   // At least half should converge
   EXPECT_GT(nConverged, static_cast<int>(configs.size()) / 2)
      << "Too many clustering configurations failed to converge";
}

TEST_F(ClusteringScanTest, CovarianceMethodComparison)
{
   gRandom->SetSeed(12345);

   struct Scenario {
      const char *label;
      AtTools::AtTrackTransformer::CovarianceMode covMode;
      bool usePerClusterCov;
      int nTried{0};
      int nConverged{0};
      double sumMomErr{0};
      double sumMomErr2{0};
      double sumResid{0};
      double sumResid2{0};
      double sumClusters{0};
      double sumDeltaMomVsFixed{0};
      double sumDeltaResidVsFixed{0};
   };

   std::vector<Scenario> scenarios = {
      {"fixed_sigma", AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect, false},
      {"transformer_direct", AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect, true},
      {"hit_cluster_online", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true},
   };

   constexpr double kRadius = 20.0;
   constexpr double kDistance = 15.0;
   constexpr int kTrials = 20;

   for (int trial = 0; trial < kTrials; ++trial) {
      auto hits = GenerateHits(1.5, 1.0);
      ASSERT_GT(hits.size(), 50u);

      double fixedMomErr = std::numeric_limits<double>::quiet_NaN();
      double fixedResid = std::numeric_limits<double>::quiet_NaN();

      for (auto &scenario : scenarios) {
         auto clusters = ClusterizeTrack(hits, kRadius, kDistance, scenario.covMode);
         auto ukf = CreateUKF();
         auto result = RunUKF(*ukf, clusters, scenario.usePerClusterCov);
         scenario.nTried++;
         scenario.sumClusters += result.nClusters;
         if (!result.converged)
            continue;
         scenario.nConverged++;
         scenario.sumMomErr += result.momErr;
         scenario.sumMomErr2 += result.momErr * result.momErr;
         scenario.sumResid += result.meanResid;
         scenario.sumResid2 += result.meanResid * result.meanResid;

         if (scenario.label == std::string("fixed_sigma")) {
            fixedMomErr = result.momErr;
            fixedResid = result.meanResid;
         } else if (!std::isnan(fixedMomErr) && !std::isnan(fixedResid)) {
            scenario.sumDeltaMomVsFixed += result.momErr - fixedMomErr;
            scenario.sumDeltaResidVsFixed += result.meanResid - fixedResid;
         }
      }
   }

   std::cout << "\nCovariance comparison on synthetic tracks\n"
             << std::setw(20) << "mode" << std::setw(8) << "tried" << std::setw(8) << "conv" << std::setw(12)
             << "avgMomErr" << std::setw(12) << "rmsMomErr" << std::setw(12) << "avgResid" << std::setw(12)
             << "rmsResid" << std::setw(10) << "avgCl" << std::setw(12) << "dMomVsFix" << std::setw(12)
             << "dResVsFix" << std::endl;
   std::cout << std::string(110, '-') << std::endl;

   for (const auto &scenario : scenarios) {
      double avgMomErr = scenario.nConverged > 0 ? scenario.sumMomErr / scenario.nConverged : 0.0;
      double rmsMomErr = RMSFromSums(scenario.sumMomErr, scenario.sumMomErr2, scenario.nConverged);
      double avgResid = scenario.nConverged > 0 ? scenario.sumResid / scenario.nConverged : 0.0;
      double rmsResid = RMSFromSums(scenario.sumResid, scenario.sumResid2, scenario.nConverged);
      double avgClusters = scenario.nTried > 0 ? scenario.sumClusters / scenario.nTried : 0.0;
      double avgDeltaMomVsFixed = scenario.nConverged > 0 ? scenario.sumDeltaMomVsFixed / scenario.nConverged : 0.0;
      double avgDeltaResidVsFixed = scenario.nConverged > 0 ? scenario.sumDeltaResidVsFixed / scenario.nConverged : 0.0;
      std::cout << std::setw(20) << scenario.label << std::setw(8) << scenario.nTried << std::setw(8)
                << scenario.nConverged << std::setw(12) << std::fixed << std::setprecision(3) << avgMomErr
                << std::setw(12) << rmsMomErr << std::setw(12) << avgResid << std::setw(12) << rmsResid
                << std::setw(10) << std::setprecision(1) << avgClusters << std::setw(12) << std::setprecision(3)
                << avgDeltaMomVsFixed << std::setw(12) << avgDeltaResidVsFixed << std::endl;
   }

   EXPECT_GE(scenarios[0].nConverged, kTrials / 2);
   EXPECT_GE(scenarios[1].nConverged, kTrials / 2);
   EXPECT_GE(scenarios[2].nConverged, kTrials / 2);
}

TEST_F(ClusteringScanTest, TransformerDirectPreservesLegacyCovarianceFormula)
{
   const CovarianceTestParams params{0.00018, 0.0000025, 1.6, 0.25, 1.8, 2.7};
   std::vector<AtHit> hits = {
      MakeHit({1.0, -0.5, 120.0}, 10.0, 3),
      MakeHit({1.8, 0.2, 126.0}, 20.0, 4),
      MakeHit({2.2, 0.9, 132.0}, 30.0, 5),
      MakeHit({3.1, 1.4, 138.0}, 15.0, 6),
   };

   auto cluster = ClusterSingleGroup(hits, params, AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect);
   auto expectedCov = BuildExpectedTransformerDirectCovariance(hits, params);

   double expectedCharge = 0.0;
   double expectedX = 0.0;
   double expectedY = 0.0;
   double expectedZ = 0.0;
   int expectedTime = 0;
   for (const auto &hit : hits) {
      expectedCharge += hit.GetCharge();
      expectedX += hit.GetPosition().X() * hit.GetCharge();
      expectedY += hit.GetPosition().Y() * hit.GetCharge();
      expectedZ += hit.GetPosition().Z();
      expectedTime += hit.GetTimeStamp();
   }
   expectedX /= expectedCharge;
   expectedY /= expectedCharge;
   expectedZ /= hits.size();
   expectedTime /= hits.size();

   EXPECT_NEAR(cluster.GetCharge(), expectedCharge, 1e-9);
   EXPECT_NEAR(cluster.GetPosition().X(), expectedX, 1e-9);
   EXPECT_NEAR(cluster.GetPosition().Y(), expectedY, 1e-9);
   EXPECT_NEAR(cluster.GetPosition().Z(), expectedZ, 1e-9);
   EXPECT_EQ(cluster.GetTimeStamp(), expectedTime);
   ExpectMatrixNear(cluster.GetCovMatrix(), expectedCov, 1e-9);
}

TEST_F(ClusteringScanTest, HitClusterOnlineMatchesAtHitClusterAggregation)
{
   const CovarianceTestParams params{0.00012, 0.0000015, 1.3, 0.32, 2.0, 3.0};
   std::vector<AtHit> hits = {
      MakeHit({0.0, 0.0, 80.0}, 8.0, 1),
      MakeHit({1.5, 0.5, 82.0}, 12.0, 2),
      MakeHit({2.2, 1.2, 84.0}, 18.0, 3),
      MakeHit({2.8, 1.9, 86.0}, 10.0, 4),
   };

   auto cluster = ClusterSingleGroup(hits, params, AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline);
   auto expectedOnlineCluster = BuildExpectedOnlineCluster(hits, params);

   ExpectMatrixNear(cluster.GetCovMatrix(), expectedOnlineCluster.GetCovMatrix(), 1e-9);
   for (int axis = 0; axis < 3; ++axis)
      EXPECT_GE(cluster.GetCovMatrix()(axis, axis), 0.0);
}

TEST_F(ClusteringScanTest, CovarianceModesRespondMonotonicallyToDriftDistanceAndAveraging)
{
   const CovarianceTestParams params{0.00015, 0.0000018, 1.5, 0.28, 1.9, 2.85};

   std::vector<AtHit> nearHits = {
      MakeHit({1.0, -0.5, 40.0}, 10.0, 1),
      MakeHit({1.0, -0.5, 40.0}, 12.0, 2),
      MakeHit({1.0, -0.5, 40.0}, 14.0, 3),
   };
   std::vector<AtHit> farHits = {
      MakeHit({1.0, -0.5, 240.0}, 10.0, 1),
      MakeHit({1.0, -0.5, 240.0}, 12.0, 2),
      MakeHit({1.0, -0.5, 240.0}, 14.0, 3),
   };
   std::vector<AtHit> averagedHits = {
      MakeHit({1.0, -0.5, 40.0}, 10.0, 1),
      MakeHit({1.0, -0.5, 40.0}, 12.0, 2),
      MakeHit({1.0, -0.5, 40.0}, 14.0, 3),
      MakeHit({1.0, -0.5, 40.0}, 10.0, 4),
      MakeHit({1.0, -0.5, 40.0}, 12.0, 5),
      MakeHit({1.0, -0.5, 40.0}, 14.0, 6),
   };

   const auto directNear =
      ClusterSingleGroup(nearHits, params, AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect);
   const auto directFar =
      ClusterSingleGroup(farHits, params, AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect);
   const auto directAveraged =
      ClusterSingleGroup(averagedHits, params, AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect);

   auto nearHitVariance = GetExpectedHitVariance(MakeHit({1.0, -0.5, 40.0}, 10.0, 1), params);
   auto farHitVariance = GetExpectedHitVariance(MakeHit({1.0, -0.5, 240.0}, 10.0, 1), params);

   std::vector<AtHit> onlineBaseHits = {
      MakeHit({0.0, 0.0, 40.0}, 10.0, 1),
      MakeHit({1.0, 0.4, 42.0}, 12.0, 2),
      MakeHit({2.0, 0.8, 44.0}, 14.0, 3),
   };
   std::vector<AtHit> onlineAveragedHits = onlineBaseHits;
   onlineAveragedHits.insert(onlineAveragedHits.end(), onlineBaseHits.begin(), onlineBaseHits.end());

   const auto onlineBase = BuildExpectedOnlineCluster(onlineBaseHits, params);
   const auto onlineAveraged =
      BuildExpectedOnlineCluster(onlineAveragedHits, params);

   for (int axis = 0; axis < 3; ++axis) {
      double nearVar = axis == 0 ? nearHitVariance.X() : (axis == 1 ? nearHitVariance.Y() : nearHitVariance.Z());
      double farVar = axis == 0 ? farHitVariance.X() : (axis == 1 ? farHitVariance.Y() : farHitVariance.Z());
      EXPECT_GT(directFar.GetCovMatrix()(axis, axis), directNear.GetCovMatrix()(axis, axis));
      EXPECT_LT(directAveraged.GetCovMatrix()(axis, axis), directNear.GetCovMatrix()(axis, axis));
      EXPECT_GT(farVar, nearVar);
      EXPECT_LE(onlineAveraged.GetCovMatrix()(axis, axis), onlineBase.GetCovMatrix()(axis, axis));
      EXPECT_GE(directNear.GetCovMatrix()(axis, axis), 0.0);
      EXPECT_GE(directFar.GetCovMatrix()(axis, axis), 0.0);
      EXPECT_GE(directAveraged.GetCovMatrix()(axis, axis), 0.0);
      EXPECT_GE(onlineBase.GetCovMatrix()(axis, axis), 0.0);
      EXPECT_GE(onlineAveraged.GetCovMatrix()(axis, axis), 0.0);
   }
}

// ===========================================================================
// Test: statistical scan — run multiple trials with smeared hits
// ===========================================================================
TEST_F(ClusteringScanTest, StatisticalScan)
{
   auto ukf = CreateUKF();

   struct Config {
      double radius;
      double distance;
      std::string label;
   };
   std::vector<Config> configs = {
      {5.0, 10.0, "r5_d10"},   {10.0, 15.0, "r10_d15"}, {15.0, 20.0, "r15_d20"},
      {15.0, 30.5, "r15_d30"}, {20.0, 30.0, "r20_d30"}, {25.0, 40.0, "r25_d40"},
   };

   int nTrials = 20;

   std::cout << "\n--- Statistical scan (" << nTrials << " trials per config) ---\n" << std::endl;
   std::cout << std::setw(12) << "config" << std::setw(8) << "fit" << std::setw(8) << "fail" << std::setw(12)
             << "bias(%)" << std::setw(12) << "rms(%)" << std::setw(10) << "resid" << std::setw(10) << "clusters"
             << std::endl;
   std::cout << std::string(72, '-') << std::endl;

   for (auto &cfg : configs) {
      std::vector<double> errs;
      std::vector<double> resids;
      std::vector<int> nCls;
      int nFail = 0;

      for (int t = 0; t < nTrials; t++) {
         auto hits = GenerateHits(1.5, 1.0);
         AtTrack track = BuildTrack(hits);
         AtTools::AtTrackTransformer transformer;
         transformer.ClusterizeSmooth3D(track, cfg.radius, cfg.distance);

         auto result = RunUKF(*ukf, *track.GetHitClusterArray(), false);
         if (result.converged && std::abs(result.momErr) < 50) {
            errs.push_back(result.momErr);
            resids.push_back(result.meanResid);
            nCls.push_back(result.nClusters);
         } else {
            nFail++;
         }
      }

      if (errs.empty()) {
         std::cout << std::setw(12) << cfg.label << std::setw(8) << 0 << std::setw(8) << nFail
                   << "  (all failed)" << std::endl;
         continue;
      }

      double mean = 0, rms = 0, mResid = 0, mCl = 0;
      for (auto v : errs)
         mean += v;
      for (auto v : resids)
         mResid += v;
      for (auto v : nCls)
         mCl += v;
      mean /= errs.size();
      mResid /= resids.size();
      mCl /= nCls.size();
      for (auto v : errs)
         rms += (v - mean) * (v - mean);
      rms = std::sqrt(rms / errs.size());

      std::cout << std::setw(12) << cfg.label << std::setw(8) << errs.size() << std::setw(8) << nFail
                << std::setw(12) << std::fixed << std::setprecision(2) << mean << std::setw(12) << rms
                << std::setw(10) << mResid << std::setw(10) << std::setprecision(0) << mCl << std::endl;

      // Basic sanity: bias should be reasonable and convergence rate > 50%
      EXPECT_LT(std::abs(mean), 10.0) << cfg.label << " has excessive bias";
   }
}
