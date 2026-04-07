#include "AtTrackClusterBuilder.h"

#include "AtHit.h"
#include "AtHitCluster.h"

#include <gtest/gtest.h>

#include <cmath>
#include <tuple>
#include <vector>

namespace {

std::vector<AtHit> BuildHits(std::initializer_list<std::tuple<double, double, double, double, int>> hits)
{
   std::vector<AtHit> out;
   out.reserve(hits.size());
   for (const auto &[x, y, z, q, t] : hits) {
      AtHit hit;
      hit.SetPosition({x, y, z});
      hit.SetCharge(q);
      hit.SetTimeStamp(t);
      out.push_back(hit);
   }
   return out;
}

void ExpectNear(double lhs, double rhs, double tol = 1e-9)
{
   EXPECT_NEAR(lhs, rhs, tol);
}

AtTools::AtTrackClusterBuilderConfig MakeConfig(AtTools::CovarianceMode mode)
{
   AtTools::AtTrackClusterBuilderConfig config;
   config.covarianceMode = mode;
   return config;
}

AtHit::XYZVector GetDefaultHitVariance(const AtHit &hit)
{
   constexpr double coefT = 0.00009;
   constexpr double coefL = 0.0000009;
   constexpr double driftVel = 1.0;
   constexpr double tbTime = 0.320;
   constexpr double padResXY = 2.3;
   constexpr double padResZ = padResXY * 1.5;

   double driftTime = hit.GetPosition().Z() / (10.0 * driftVel);
   double varT = 100.0 * coefT * 2.0 * driftTime;
   double varL = 100.0 * coefL * 2.0 * driftTime;
   double tbResMM = driftVel * tbTime * 10.0;
   double varTB = tbResMM * tbResMM / 12.0;
   return {padResXY * padResXY + varT, padResXY * padResXY + varT, padResZ * padResZ + varTB + varL};
}

} // namespace

TEST(AtTrackClusterBuilderTest, TransformerDirectPreservesCurrentCentroidAndCovariance)
{
   auto hits = BuildHits({
      {0.0, 0.0, 10.0, 1.0, 0},
      {1.0, 0.0, 20.0, 2.0, 1},
      {2.0, 0.0, 30.0, 3.0, 2},
   });

   AtTools::AtTrackClusterBuilder builder(MakeConfig(AtTools::CovarianceMode::TransformerDirect));
   auto cluster = builder.BuildCluster(hits, 4);

   ASSERT_NE(cluster, nullptr);
   EXPECT_EQ(cluster->GetClusterID(), 4);
   ExpectNear(cluster->GetCharge(), 6.0);
   ExpectNear(cluster->GetPosition().X(), 1.3333333333333333);
   ExpectNear(cluster->GetPosition().Y(), 0.0);
   ExpectNear(cluster->GetPosition().Z(), 20.0);
   EXPECT_EQ(cluster->GetTimeStamp(), 1);

   const auto &cov = cluster->GetCovMatrix();
   ExpectNear(cov(0, 0), 2.0752222222222225);
   ExpectNear(cov(0, 1), 0.0);
   ExpectNear(cov(0, 2), 0.0);
   ExpectNear(cov(1, 1), 2.0752222222222225);
   ExpectNear(cov(1, 2), 0.0);
   ExpectNear(cov(2, 2), 4.9607818518518515);
}

TEST(AtTrackClusterBuilderTest, OnlineModeKeepsCurrentCentroidButUsesOnlineCovariance)
{
   auto hits = BuildHits({
      {0.0, 0.0, 10.0, 1.0, 0},
      {1.0, 0.0, 20.0, 2.0, 1},
      {2.0, 0.0, 30.0, 3.0, 2},
   });

   AtTools::AtTrackClusterBuilder directBuilder(
      MakeConfig(AtTools::CovarianceMode::TransformerDirect));
   AtTools::AtTrackClusterBuilder onlineBuilder(MakeConfig(AtTools::CovarianceMode::HitClusterOnline));

   auto directCluster = directBuilder.BuildCluster(hits, 1);
   auto onlineCluster = onlineBuilder.BuildCluster(hits, 1);

   ASSERT_NE(directCluster, nullptr);
   ASSERT_NE(onlineCluster, nullptr);

   ExpectNear(onlineCluster->GetCharge(), directCluster->GetCharge());
   ExpectNear(onlineCluster->GetPosition().X(), directCluster->GetPosition().X());
   ExpectNear(onlineCluster->GetPosition().Y(), directCluster->GetPosition().Y());
   ExpectNear(onlineCluster->GetPosition().Z(), directCluster->GetPosition().Z());
   EXPECT_EQ(onlineCluster->GetTimeStamp(), directCluster->GetTimeStamp());

   AtHitCluster expected;
   for (auto &hit : hits) {
      hit.SetPositionVariance(GetDefaultHitVariance(hit));
      expected.AddHit(hit);
   }

   const auto &onlineCov = onlineCluster->GetCovMatrix();
   const auto &expectedCov = expected.GetCovMatrix();
   for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 3; ++col)
         ExpectNear(onlineCov(row, col), expectedCov(row, col));
   }

   EXPECT_GT(std::abs(onlineCov(0, 0) - directCluster->GetCovMatrix()(0, 0)), 1e-6);
}

TEST(AtTrackClusterBuilderTest, DiagOnlyModeZerosOffDiagonalTermsFromOnlineCovariance)
{
   auto hits = BuildHits({
      {0.0, 0.0, 10.0, 1.0, 0},
      {1.0, 0.5, 20.0, 2.0, 1},
      {2.0, 1.0, 30.0, 3.0, 2},
   });

   AtTools::AtTrackClusterBuilder onlineBuilder(MakeConfig(AtTools::CovarianceMode::HitClusterOnline));
   AtTools::AtTrackClusterBuilder diagBuilder(
      MakeConfig(AtTools::CovarianceMode::HitClusterOnlineDiagOnly));

   auto onlineCluster = onlineBuilder.BuildCluster(hits, 2);
   auto diagCluster = diagBuilder.BuildCluster(hits, 2);

   ASSERT_NE(onlineCluster, nullptr);
   ASSERT_NE(diagCluster, nullptr);

   const auto &onlineCov = onlineCluster->GetCovMatrix();
   const auto &diagCov = diagCluster->GetCovMatrix();
   for (int idx = 0; idx < 3; ++idx)
      ExpectNear(diagCov(idx, idx), onlineCov(idx, idx));

   ExpectNear(diagCov(0, 1), 0.0);
   ExpectNear(diagCov(0, 2), 0.0);
   ExpectNear(diagCov(1, 2), 0.0);
   EXPECT_NE(std::abs(onlineCov(0, 2)), 0.0);
}
