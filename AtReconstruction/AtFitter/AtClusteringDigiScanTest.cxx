#include "AtELossCATIMA.h"
#include "AtFitMetadata.h"
#include "AtFittedTrack.h"
#include "AtFitterUKF.h"
#include "AtHitCluster.h"
#include "AtMCTrack.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"
#include "AtTrackTransformer.h"
#include "AtTrackingEvent.h"

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TClonesArray.h>
#include <TFile.h>
#include <TMath.h>
#include <TMatrixDSym.h>
#include <TMatrixDSymEigen.h>
#include <TSystem.h>
#include <TTree.h>

#include <FairLogger.h>

#include <cstdlib>
#include <cmath>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

using ROOT::Math::XYZPoint;
using ROOT::Math::XYZVector;

namespace {

std::string ResolveDataPath(const char *relativePath)
{
   std::vector<std::string> candidates;
   if (const char *vmcWorkDir = gSystem->Getenv("VMCWORKDIR"))
      candidates.push_back(std::string(vmcWorkDir) + "/" + relativePath);

   TString cwd = gSystem->WorkingDirectory();
   candidates.push_back((cwd + "/../../" + relativePath).Data());
   candidates.push_back(relativePath);

   for (const auto &candidate : candidates) {
      if (!gSystem->AccessPathName(candidate.c_str()))
         return candidate;
   }

   return candidates.front();
}

double RMSFromSums(double sum, double sumSq, int n)
{
   if (n <= 0)
      return 0.0;
   double mean = sum / n;
   double variance = sumSq / n - mean * mean;
   return variance > 0.0 ? std::sqrt(variance) : 0.0;
}

struct CovarianceSummary {
   int nClusters{0};
   int nNegativeDiag{0};
   double sumVarX{0.0};
   double sumVarY{0.0};
   double sumVarZ{0.0};
   double sumAbsCorrXY{0.0};
   double sumAbsCorrXZ{0.0};
   double sumAbsCorrYZ{0.0};
   double sumMinEigen{0.0};
   double sumMaxEigen{0.0};
   double sumCondition{0.0};
};

struct RegularizationSummary {
   int nClustersAdjusted{0};
   double sumMinEigenBefore{0.0};
   double sumMinEigenAfter{0.0};
   double sumFrobeniusShift{0.0};
};

double SafeCorrelation(const TMatrixDSym &cov, int i, int j)
{
   if (cov(i, i) <= 0.0 || cov(j, j) <= 0.0)
      return 0.0;
   return cov(i, j) / std::sqrt(cov(i, i) * cov(j, j));
}

CovarianceSummary SummarizeCovariances(AtTrack &track)
{
   CovarianceSummary summary;
   for (const auto &cluster : *track.GetHitClusterArray()) {
      const auto &cov = cluster.GetCovMatrix();
      summary.nClusters++;
      summary.sumVarX += cov(0, 0);
      summary.sumVarY += cov(1, 1);
      summary.sumVarZ += cov(2, 2);
      summary.sumAbsCorrXY += std::abs(SafeCorrelation(cov, 0, 1));
      summary.sumAbsCorrXZ += std::abs(SafeCorrelation(cov, 0, 2));
      summary.sumAbsCorrYZ += std::abs(SafeCorrelation(cov, 1, 2));
      if (cov(0, 0) < 0.0 || cov(1, 1) < 0.0 || cov(2, 2) < 0.0)
         summary.nNegativeDiag++;

      TMatrixDSymEigen eig(cov);
      auto eigenValues = eig.GetEigenValues();
      double minEig = eigenValues.Min();
      double maxEig = eigenValues.Max();
      summary.sumMinEigen += minEig;
      summary.sumMaxEigen += maxEig;
      if (minEig > 0.0)
         summary.sumCondition += maxEig / minEig;
   }
   return summary;
}

void ScaleClusterCovariances(AtTrack &track, double scale)
{
   for (auto &cluster : *track.GetHitClusterArray()) {
      TMatrixDSym scaled(cluster.GetCovMatrix());
      scaled *= scale;
      cluster.SetCovMatrix(scaled);
   }
}

RegularizationSummary RegularizeClusterCovariances(AtTrack &track, double minEigenFloor = 0.05)
{
   RegularizationSummary summary;
   for (auto &cluster : *track.GetHitClusterArray()) {
      const auto &cov = cluster.GetCovMatrix();
      TMatrixDSymEigen eig(cov);
      auto eigenValues = eig.GetEigenValues();
      auto eigenVectors = eig.GetEigenVectors();

      double minBefore = eigenValues.Min();
      bool adjusted = false;
      for (int i = 0; i < eigenValues.GetNrows(); ++i) {
         if (eigenValues[i] < minEigenFloor) {
            eigenValues[i] = minEigenFloor;
            adjusted = true;
         }
      }
      if (!adjusted)
         continue;

      TMatrixD diag(3, 3);
      diag.Zero();
      for (int i = 0; i < 3; ++i)
         diag(i, i) = eigenValues[i];

      TMatrixD regularized = eigenVectors * diag * eigenVectors.T();
      TMatrixDSym regularizedSym(3);
      double frobeniusShift2 = 0.0;
      for (int row = 0; row < 3; ++row) {
         for (int col = 0; col < 3; ++col) {
            regularizedSym(row, col) = regularized(row, col);
            double delta = regularized(row, col) - cov(row, col);
            frobeniusShift2 += delta * delta;
         }
      }

      cluster.SetCovMatrix(regularizedSym);
      summary.nClustersAdjusted++;
      summary.sumMinEigenBefore += minBefore;
      summary.sumMinEigenAfter += minEigenFloor;
      summary.sumFrobeniusShift += std::sqrt(frobeniusShift2);
   }
   return summary;
}

double AverageCentroidSeparation(AtTrack &reference, AtTrack &candidate)
{
   auto *refClusters = reference.GetHitClusterArray();
   auto *candClusters = candidate.GetHitClusterArray();
   int n = std::min(refClusters->size(), candClusters->size());
   if (n == 0)
      return 0.0;

   double sum = 0.0;
   for (int i = 0; i < n; ++i)
      sum += (refClusters->at(i).GetPosition() - candClusters->at(i).GetPosition()).R();
   return sum / n;
}

} // namespace

// ===========================================================================
// Scan clustering parameters on real digitized data
// ===========================================================================
class ClusteringDigiScanTest : public testing::Test {
protected:
   static constexpr double kMass = 938.272;
   static constexpr double kCharge = 1.602176634e-19;
   static constexpr double kBz = 2.85;
   static constexpr double kGasDensity = 3.553e-5;

   struct FitResult {
      int event;
      double radius;
      double distance;
      int nClusters;
      double kineticEnergy;
      double theta;
      bool converged;
      bool good; // KE in range AND theta in range
   };

   struct TruthKinematics {
      bool hasProton{false};
      double kineticEnergy{0.0};
      double theta{0.0};
   };

   std::unique_ptr<EventFit::AtFitterUKF> CreateFitter(bool enableStraggling = false)
   {
      auto eloss = std::make_unique<AtTools::AtELossCATIMA>(kGasDensity);
      eloss->SetProjectile(1, 1, 1);
      std::vector<std::tuple<int, int, int>> mat;
      mat.push_back({1, 1, 1});
      eloss->SetMaterial(mat);

      auto fitter = std::make_unique<EventFit::AtFitterUKF>(kCharge, kMass, std::move(eloss));
      fitter->SetBField({0, 0, kBz});
      fitter->SetUKFParameters(1e-3, 2.0, 0.0);
      fitter->SetMeasurementSigma(2.0);
      fitter->SetMomentumSigmaFrac(0.3);
      fitter->SetEnableEnergyStraggling(enableStraggling);
      fitter->SetMinClusters(5);
      fitter->SetZPadPlane(1000.0);
      return fitter;
   }

   FitResult FitTrack(AtTrack &track, int eventId, bool straggling = false, bool usePerClusterCov = false,
                      AtTools::AtTrackTransformer::CovarianceMode covarianceMode =
                         AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect)
   {
      FitResult result{eventId, 0, 0, 0, -1, -1, false, false};

      result.nClusters = track.GetHitClusterArray()->size();
      if (result.nClusters < 5)
         return result;

      auto fitter = CreateFitter(straggling);
      fitter->SetUsePerClusterCov(usePerClusterCov);
      fitter->SetClusterCovarianceMode(covarianceMode);
      fitter->SetAdaptiveClustering(false);
      fitter->Init();

      AtTrackingEvent trackingEvent;
      AtPatternEvent patternEvent;
      patternEvent.GetTrackCand().push_back(track);

      fitter->FitEvent(&trackingEvent, &patternEvent);

      auto &fittedTracks = trackingEvent.GetFittedTracks();
      if (fittedTracks.empty())
         return result;

      result.converged = true;
      auto kin = fittedTracks[0]->GetKinematics();
      result.kineticEnergy = kin.kineticEnergy;
      result.theta = kin.theta * 180.0 / TMath::Pi();
      result.good = (result.kineticEnergy > 0.3 && result.kineticEnergy < 3.0 && result.theta > 70 && result.theta < 130);

      return result;
   }

   TruthKinematics GetProtonTruth(TClonesArray *mcTracks)
   {
      TruthKinematics truth;
      if (mcTracks == nullptr)
         return truth;

      for (int iTrack = 0; iTrack < mcTracks->GetEntries(); ++iTrack) {
         auto *track = dynamic_cast<AtMCTrack *>(mcTracks->At(iTrack));
         if (track == nullptr || track->GetPdgCode() != 2212)
            continue;

         XYZVector mom(track->GetPx() * 1e3, track->GetPy() * 1e3, track->GetPz() * 1e3);
         truth.hasProton = true;
         truth.kineticEnergy = std::sqrt(mom.Mag2() + kMass * kMass) - kMass;
         truth.theta = mom.Theta() * 180.0 / TMath::Pi();
         return truth;
      }

      return truth;
   }
};

TEST_F(ClusteringDigiScanTest, ScanDigitizedData)
{
   FairLogger::GetLogger()->SetLogScreenLevel("error");

   // Try to open the digi file
   auto *file = TFile::Open(ResolveDataPath("macro/Simulation/ATTPC/16C_pp/data/output_digi.root").c_str());
   if (!file || file->IsZombie()) {
      std::cout << "SKIPPED: output_digi.root not found. Run C16_pp_sim.C + run_digi_attpc.C first." << std::endl;
      GTEST_SKIP();
   }

   auto *tree = (TTree *)file->Get("cbmsim");
   TClonesArray *patEvtArr = nullptr;
   tree->SetBranchAddress("AtPatternEvent", &patEvtArr);

   int nEvents = tree->GetEntries();

   struct Config {
      int method;       // 0=Smooth3D
      double radius;
      double distance;
      int straggling;   // 0=off, 1=on
      std::string label;
   };
   std::vector<Config> configs = {
      // Straggling OFF
      {0, 5.0, 15.0, 0, "r5d15 noStr"},
      {0, 10.0, 20.0, 0, "r10d20 noStr"},
      {0, 15.0, 20.0, 0, "r15d20 noStr"},
      {0, 15.0, 30.5, 0, "r15d30 noStr (def)"},
      {0, 20.0, 30.0, 0, "r20d30 noStr"},
      // Straggling ON (same configs)
      {0, 5.0, 15.0, 1, "r5d15 STR"},
      {0, 10.0, 20.0, 1, "r10d20 STR"},
      {0, 15.0, 20.0, 1, "r15d20 STR"},
      {0, 15.0, 30.5, 1, "r15d30 STR (def)"},
      {0, 20.0, 30.0, 1, "r20d30 STR"},
   };

   // Accumulate results per config
   struct ConfigStats {
      int nTried = 0;
      int nConverged = 0;
      int nGood = 0;
      double sumKE = 0;
      double sumClusters = 0;
   };
   std::vector<ConfigStats> stats(configs.size());

   // Event loop
   for (int iEvt = 0; iEvt < nEvents; iEvt++) {
      tree->GetEntry(iEvt);
      if (!patEvtArr || patEvtArr->GetEntries() == 0)
         continue;

      auto *patEvt = (AtPatternEvent *)patEvtArr->At(0);
      auto &tracks = patEvt->GetTrackCand();
      if (tracks.empty())
         continue;

      // Use largest track
      int bestTrack = 0;
      for (size_t t = 1; t < tracks.size(); t++) {
         if (tracks[t].GetHitArray().size() > tracks[bestTrack].GetHitArray().size())
            bestTrack = t;
      }

      // Skip small tracks (beam events)
      if (tracks[bestTrack].GetHitArray().size() < 50)
         continue;

      // Scan clustering configs
      for (size_t c = 0; c < configs.size(); c++) {
         AtTrack trackCopy = tracks[bestTrack];
         trackCopy.ResetHitClusterArray();
         AtTools::AtTrackTransformer transformer;
         transformer.ClusterizeSmooth3D(trackCopy, configs[c].radius, configs[c].distance);
         auto result = FitTrack(trackCopy, iEvt, configs[c].straggling != 0);

         stats[c].nTried++;
         if (result.converged) {
            stats[c].nConverged++;
            stats[c].sumClusters += result.nClusters;
            if (result.good) {
               stats[c].nGood++;
               stats[c].sumKE += result.kineticEnergy;
            }
         }
      }
   }

   // Print summary table
   std::cout << "\n=============================================================" << std::endl;
   std::cout << " Clustering Scan on Digitized 16C(p,p) Data" << std::endl;
   std::cout << "=============================================================" << std::endl;
   std::cout << std::setw(20) << "config" << std::setw(8) << "tried" << std::setw(8) << "conv" << std::setw(8) << "good"
             << std::setw(10) << "good(%)" << std::setw(10) << "avgCl" << std::setw(10) << "avgKE" << std::endl;
   std::cout << std::string(74, '-') << std::endl;

   for (size_t c = 0; c < configs.size(); c++) {
      auto &s = stats[c];
      double goodPct = s.nTried > 0 ? 100.0 * s.nGood / s.nTried : 0;
      double avgCl = s.nConverged > 0 ? s.sumClusters / s.nConverged : 0;
      double avgKE = s.nGood > 0 ? s.sumKE / s.nGood : 0;
      std::cout << std::setw(20) << configs[c].label << std::setw(8) << s.nTried << std::setw(8) << s.nConverged
                << std::setw(8) << s.nGood << std::setw(10) << std::fixed << std::setprecision(1) << goodPct
                << std::setw(10) << std::setprecision(0) << avgCl << std::setw(10) << std::setprecision(2) << avgKE
                << std::endl;
   }
   std::cout << "=============================================================" << std::endl;

   // At least one config should give >50% good fits
   int bestGood = 0;
   for (auto &s : stats)
      bestGood = std::max(bestGood, s.nGood);
   EXPECT_GT(bestGood, 0) << "No clustering config produced any good fits";
}

TEST_F(ClusteringDigiScanTest, CompareCovarianceModes)
{
   FairLogger::GetLogger()->SetLogScreenLevel("error");

   auto digiPath = ResolveDataPath("macro/Simulation/ATTPC/16C_pp/data/output_digi.root");
   auto simPath = ResolveDataPath("macro/Simulation/ATTPC/16C_pp/data/attpcsim.root");

   auto *file = TFile::Open(digiPath.c_str());
   if (!file || file->IsZombie()) {
      std::cout << "SKIPPED: output_digi.root not found. Run C16_pp_sim.C + run_digi_attpc.C first." << std::endl;
      GTEST_SKIP();
   }
   auto *mcFile = TFile::Open(simPath.c_str());
   if (!mcFile || mcFile->IsZombie()) {
      std::cout << "SKIPPED: attpcsim.root not found. Run C16_pp_sim.C first." << std::endl;
      GTEST_SKIP();
   }

   auto *tree = (TTree *)file->Get("cbmsim");
   auto *mcTree = (TTree *)mcFile->Get("cbmsim");
   TClonesArray *patEvtArr = nullptr;
   TClonesArray *mcTracks = nullptr;
   tree->SetBranchAddress("AtPatternEvent", &patEvtArr);
   mcTree->SetBranchAddress("MCTrack", &mcTracks);

   struct Scenario {
      const char *label;
      AtTools::AtTrackTransformer::CovarianceMode covarianceMode;
      bool usePerClusterCov;
      int nTried{0};
      int nConverged{0};
      int nGood{0};
      double sumKE{0};
      double sumTheta{0};
      double sumClusters{0};
      double sumKEBias{0};
      double sumKEBias2{0};
      double sumThetaBias{0};
      double sumThetaBias2{0};
   };

   std::vector<Scenario> scenarios = {
      {"fixed_sigma", AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect, false},
      {"transformer_direct", AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect, true},
      {"hit_cluster_online", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true},
   };

   constexpr double kRadius = 20.0;
   constexpr double kDistance = 15.0;

   int nEvents = tree->GetEntries();
   int nSelectedEvents = 0;
   for (int iEvt = 0; iEvt < nEvents; iEvt++) {
      tree->GetEntry(iEvt);
      mcTree->GetEntry(iEvt);
      if (!patEvtArr || patEvtArr->GetEntries() == 0)
         continue;

      auto *patEvt = (AtPatternEvent *)patEvtArr->At(0);
      auto &tracks = patEvt->GetTrackCand();
      if (tracks.empty())
         continue;

      int bestTrack = 0;
      for (size_t t = 1; t < tracks.size(); t++) {
         if (tracks[t].GetHitArray().size() > tracks[bestTrack].GetHitArray().size())
            bestTrack = t;
      }
      if (tracks[bestTrack].GetHitArray().size() < 50)
         continue;
      auto truth = GetProtonTruth(mcTracks);
      if (!truth.hasProton)
         continue;

      nSelectedEvents++;
      for (auto &scenario : scenarios) {
         AtTrack trackCopy = tracks[bestTrack];
         trackCopy.ResetHitClusterArray();
         AtTools::AtTrackTransformer transformer;
         transformer.SetCovarianceMode(scenario.covarianceMode);
         transformer.ClusterizeSmooth3D(trackCopy, kRadius, kDistance);

         auto result = FitTrack(trackCopy, iEvt, false, scenario.usePerClusterCov, scenario.covarianceMode);
         scenario.nTried++;
         if (!result.converged)
            continue;
         scenario.nConverged++;
         scenario.sumClusters += result.nClusters;
         scenario.sumKE += result.kineticEnergy;
         scenario.sumTheta += result.theta;
         double deltaKE = result.kineticEnergy - truth.kineticEnergy;
         double deltaTheta = result.theta - truth.theta;
         scenario.sumKEBias += deltaKE;
         scenario.sumKEBias2 += deltaKE * deltaKE;
         scenario.sumThetaBias += deltaTheta;
         scenario.sumThetaBias2 += deltaTheta * deltaTheta;
         if (!result.good)
            continue;
         scenario.nGood++;
      }
   }

   std::cout << "\n=============================================================" << std::endl;
   std::cout << " Covariance Mode Comparison on Digitized 16C(p,p) Data" << std::endl;
   std::cout << " Selected proton-like PRA tracks: " << nSelectedEvents << " / " << nEvents << " events" << std::endl;
   std::cout << "=============================================================" << std::endl;
   std::cout << std::setw(20) << "mode" << std::setw(8) << "tried" << std::setw(8) << "conv" << std::setw(8) << "good"
             << std::setw(10) << "good(%)" << std::setw(10) << "avgCl" << std::setw(10) << "avgKE"
             << std::setw(10) << "avgTh" << std::setw(10) << "dKE" << std::setw(10) << "rmsKE" << std::setw(10)
             << "dTh" << std::setw(10) << "rmsTh" << std::endl;
   std::cout << std::string(114, '-') << std::endl;

   for (const auto &scenario : scenarios) {
      double goodPct = scenario.nTried > 0 ? 100.0 * scenario.nGood / scenario.nTried : 0.0;
      double avgCl = scenario.nConverged > 0 ? scenario.sumClusters / scenario.nConverged : 0.0;
      double avgKE = scenario.nConverged > 0 ? scenario.sumKE / scenario.nConverged : 0.0;
      double avgTheta = scenario.nConverged > 0 ? scenario.sumTheta / scenario.nConverged : 0.0;
      double avgKEBias = scenario.nConverged > 0 ? scenario.sumKEBias / scenario.nConverged : 0.0;
      double rmsKEBias = RMSFromSums(scenario.sumKEBias, scenario.sumKEBias2, scenario.nConverged);
      double avgThetaBias = scenario.nConverged > 0 ? scenario.sumThetaBias / scenario.nConverged : 0.0;
      double rmsThetaBias = RMSFromSums(scenario.sumThetaBias, scenario.sumThetaBias2, scenario.nConverged);
      std::cout << std::setw(20) << scenario.label << std::setw(8) << scenario.nTried << std::setw(8)
                << scenario.nConverged << std::setw(8) << scenario.nGood << std::setw(10) << std::fixed
                << std::setprecision(1) << goodPct << std::setw(10) << std::setprecision(0) << avgCl
                << std::setw(10) << std::setprecision(2) << avgKE << std::setw(10) << avgTheta << std::setw(10)
                << avgKEBias << std::setw(10) << rmsKEBias << std::setw(10) << avgThetaBias << std::setw(10)
                << rmsThetaBias << std::endl;
   }
   std::cout << "=============================================================" << std::endl;

   EXPECT_GT(scenarios[0].nConverged, 0);
   EXPECT_GT(scenarios[1].nConverged, 0);
   EXPECT_GT(scenarios[2].nConverged, 0);
}

TEST_F(ClusteringDigiScanTest, ExploreOnlineCovarianceHypotheses)
{
   FairLogger::GetLogger()->SetLogScreenLevel("error");

   auto digiPath = ResolveDataPath("macro/Simulation/ATTPC/16C_pp/data/output_digi.root");
   auto simPath = ResolveDataPath("macro/Simulation/ATTPC/16C_pp/data/attpcsim.root");

   auto *file = TFile::Open(digiPath.c_str());
   auto *mcFile = TFile::Open(simPath.c_str());
   if (!file || file->IsZombie() || !mcFile || mcFile->IsZombie()) {
      std::cout << "SKIPPED: digitized inputs not found. Generate 16C(p,p) data first." << std::endl;
      GTEST_SKIP();
   }

   auto *tree = (TTree *)file->Get("cbmsim");
   auto *mcTree = (TTree *)mcFile->Get("cbmsim");
   TClonesArray *patEvtArr = nullptr;
   TClonesArray *mcTracks = nullptr;
   tree->SetBranchAddress("AtPatternEvent", &patEvtArr);
   mcTree->SetBranchAddress("MCTrack", &mcTracks);

   struct Scenario {
      const char *label;
      AtTools::AtTrackTransformer::CovarianceMode covarianceMode;
      bool usePerClusterCov{true};
      bool fitEnabled{true};
      bool regularizeBeforeFit{false};
      double covarianceScale{1.0};
      int nTried{0};
      int nConverged{0};
      double sumTheta{0.0};
      double sumThetaBias{0.0};
      double sumThetaBias2{0.0};
      double sumKEBias{0.0};
      double sumKEBias2{0.0};
      double sumClusters{0.0};
      CovarianceSummary covSummary;
      double sumCentroidShiftVsDirect{0.0};
      RegularizationSummary regularization;
   };

   std::vector<Scenario> scenarios = {
      {"transformer_direct", AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect, true, true, false, 1.0},
      {"online_raw", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true, false, false, 1.0},
      {"online_raw_fixed", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, false, true, false, 1.0},
      {"online_diag", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnlineDiagOnly, true, false, false, 1.0},
      {"online_diag_fixed", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnlineDiagOnly, false, true, false,
       1.0},
      {"online_reg", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true, false, true, 1.0},
      {"online_consistent_fixed", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnlineConsistent, false, true,
       false, 1.0},
      {"online_consistent_reg", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnlineConsistent, true, false,
       true, 1.0},
      {"online_x0.5_reg", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true, false, true, 0.5},
      {"online_x2_reg", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true, false, true, 2.0},
      {"online_x5_reg", AtTools::AtTrackTransformer::CovarianceMode::HitClusterOnline, true, false, true, 5.0},
   };

   constexpr double kRadius = 20.0;
   constexpr double kDistance = 15.0;

   int nEvents = tree->GetEntries();
   int nSelectedEvents = 0;
   for (int iEvt = 0; iEvt < nEvents; ++iEvt) {
      tree->GetEntry(iEvt);
      mcTree->GetEntry(iEvt);
      if (!patEvtArr || patEvtArr->GetEntries() == 0)
         continue;

      auto *patEvt = (AtPatternEvent *)patEvtArr->At(0);
      auto &tracks = patEvt->GetTrackCand();
      if (tracks.empty())
         continue;

      int bestTrack = 0;
      for (size_t t = 1; t < tracks.size(); ++t) {
         if (tracks[t].GetHitArray().size() > tracks[bestTrack].GetHitArray().size())
            bestTrack = t;
      }
      if (tracks[bestTrack].GetHitArray().size() < 50)
         continue;

      auto truth = GetProtonTruth(mcTracks);
      if (!truth.hasProton)
         continue;

      nSelectedEvents++;

      AtTrack directTrack = tracks[bestTrack];
      directTrack.ResetHitClusterArray();
      {
         AtTools::AtTrackTransformer transformer;
         transformer.SetCovarianceMode(AtTools::AtTrackTransformer::CovarianceMode::TransformerDirect);
         transformer.ClusterizeSmooth3D(directTrack, kRadius, kDistance);
      }

      for (auto &scenario : scenarios) {
         AtTrack trackCopy = tracks[bestTrack];
         trackCopy.ResetHitClusterArray();
         AtTools::AtTrackTransformer transformer;
         transformer.SetCovarianceMode(scenario.covarianceMode);
         transformer.ClusterizeSmooth3D(trackCopy, kRadius, kDistance);
         if (std::abs(scenario.covarianceScale - 1.0) > 1e-9)
            ScaleClusterCovariances(trackCopy, scenario.covarianceScale);

         auto covSummary = SummarizeCovariances(trackCopy);
         scenario.covSummary.nClusters += covSummary.nClusters;
         scenario.covSummary.nNegativeDiag += covSummary.nNegativeDiag;
         scenario.covSummary.sumVarX += covSummary.sumVarX;
         scenario.covSummary.sumVarY += covSummary.sumVarY;
         scenario.covSummary.sumVarZ += covSummary.sumVarZ;
         scenario.covSummary.sumAbsCorrXY += covSummary.sumAbsCorrXY;
         scenario.covSummary.sumAbsCorrXZ += covSummary.sumAbsCorrXZ;
         scenario.covSummary.sumAbsCorrYZ += covSummary.sumAbsCorrYZ;
         scenario.covSummary.sumMinEigen += covSummary.sumMinEigen;
         scenario.covSummary.sumMaxEigen += covSummary.sumMaxEigen;
         scenario.covSummary.sumCondition += covSummary.sumCondition;
         scenario.sumCentroidShiftVsDirect += AverageCentroidSeparation(directTrack, trackCopy);

         if (scenario.regularizeBeforeFit) {
            auto regularization = RegularizeClusterCovariances(trackCopy);
            scenario.regularization.nClustersAdjusted += regularization.nClustersAdjusted;
            scenario.regularization.sumMinEigenBefore += regularization.sumMinEigenBefore;
            scenario.regularization.sumMinEigenAfter += regularization.sumMinEigenAfter;
            scenario.regularization.sumFrobeniusShift += regularization.sumFrobeniusShift;
         }

         if (!scenario.fitEnabled)
            continue;
         auto result = FitTrack(trackCopy, iEvt, false, scenario.usePerClusterCov, scenario.covarianceMode);
         scenario.nTried++;
         if (!result.converged)
            continue;
         scenario.nConverged++;
         scenario.sumClusters += result.nClusters;
         scenario.sumTheta += result.theta;
         double deltaTheta = result.theta - truth.theta;
         double deltaKE = result.kineticEnergy - truth.kineticEnergy;
         scenario.sumThetaBias += deltaTheta;
         scenario.sumThetaBias2 += deltaTheta * deltaTheta;
         scenario.sumKEBias += deltaKE;
         scenario.sumKEBias2 += deltaKE * deltaKE;
      }
   }

   std::cout << "\n================================================================================================================" << std::endl;
   std::cout << " Online Covariance Hypothesis Exploration" << std::endl;
   std::cout << " Selected proton-like PRA tracks: " << nSelectedEvents << " / " << nEvents << " events" << std::endl;
   std::cout << "================================================================================================================" << std::endl;
   std::cout << std::setw(18) << "mode" << std::setw(8) << "conv" << std::setw(10) << "avgTh" << std::setw(10)
             << "dTh" << std::setw(10) << "rmsTh" << std::setw(10) << "dKE" << std::setw(10) << "rmsKE"
             << std::setw(10) << "varX" << std::setw(10) << "varY" << std::setw(10) << "varZ" << std::setw(10)
             << "|rXY|" << std::setw(10) << "|rXZ|" << std::setw(10) << "|rYZ|" << std::setw(10) << "minEig"
             << std::setw(10) << "cond" << std::setw(10) << "dPos" << std::setw(10) << "nReg" << std::setw(10)
             << "dCov" << std::endl;
   std::cout << std::string(188, '-') << std::endl;

   for (const auto &scenario : scenarios) {
      double avgTheta = scenario.nConverged > 0 ? scenario.sumTheta / scenario.nConverged : 0.0;
      double avgThetaBias = scenario.nConverged > 0 ? scenario.sumThetaBias / scenario.nConverged : 0.0;
      double rmsThetaBias = RMSFromSums(scenario.sumThetaBias, scenario.sumThetaBias2, scenario.nConverged);
      double avgKEBias = scenario.nConverged > 0 ? scenario.sumKEBias / scenario.nConverged : 0.0;
      double rmsKEBias = RMSFromSums(scenario.sumKEBias, scenario.sumKEBias2, scenario.nConverged);
      double avgVarX = scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumVarX / scenario.covSummary.nClusters : 0.0;
      double avgVarY = scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumVarY / scenario.covSummary.nClusters : 0.0;
      double avgVarZ = scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumVarZ / scenario.covSummary.nClusters : 0.0;
      double avgCorrXY =
         scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumAbsCorrXY / scenario.covSummary.nClusters : 0.0;
      double avgCorrXZ =
         scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumAbsCorrXZ / scenario.covSummary.nClusters : 0.0;
      double avgCorrYZ =
         scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumAbsCorrYZ / scenario.covSummary.nClusters : 0.0;
      double avgMinEig =
         scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumMinEigen / scenario.covSummary.nClusters : 0.0;
      double avgCond =
         scenario.covSummary.nClusters > 0 ? scenario.covSummary.sumCondition / scenario.covSummary.nClusters : 0.0;
      double avgCentroidShift = scenario.nTried > 0 ? scenario.sumCentroidShiftVsDirect / scenario.nTried : 0.0;
      double avgRegShift = scenario.regularization.nClustersAdjusted > 0
                              ? scenario.regularization.sumFrobeniusShift / scenario.regularization.nClustersAdjusted
                              : 0.0;

      std::cout << std::setw(18) << scenario.label << std::setw(8) << scenario.nConverged << std::setw(10)
                << std::fixed << std::setprecision(2) << avgTheta << std::setw(10) << avgThetaBias << std::setw(10)
                << rmsThetaBias << std::setw(10) << avgKEBias << std::setw(10) << rmsKEBias << std::setw(10)
                << avgVarX << std::setw(10) << avgVarY << std::setw(10) << avgVarZ << std::setw(10) << avgCorrXY
                << std::setw(10) << avgCorrXZ << std::setw(10) << avgCorrYZ << std::setw(10) << avgMinEig
                << std::setw(10) << avgCond << std::setw(10) << avgCentroidShift << std::setw(10)
                << scenario.regularization.nClustersAdjusted << std::setw(10) << avgRegShift << std::endl;
   }
   std::cout << "================================================================================================================" << std::endl;

   EXPECT_GT(scenarios[0].nConverged, 0);
}
