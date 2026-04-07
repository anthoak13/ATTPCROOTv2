#include "AtBeamTrackRejector.h"
#include "AtClusterOrderer.h"
#include "AtEvent.h"
#include "AtFragmentMerger.h"
#include "AtHit.h"
#include "AtHitCluster.h"
#include "AtPatternCircle2D.h"
#include "AtPatternEvent.h"
#include "AtSmooth3DClusterer.h"
#include "AtTrack.h"
#include "AtVertexTrackSelector.h"

#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <vector>

namespace {

using XYZPoint = ROOT::Math::XYZPoint;

std::shared_ptr<AtHitCluster> MakeCluster(double x, double y, double z, double charge = 1.0, int timeStamp = 0)
{
   auto cluster = std::make_shared<AtHitCluster>();
   cluster->SetPosition({x, y, z});
   cluster->SetCharge(charge);
   cluster->SetTimeStamp(timeStamp);
   return cluster;
}

AtTrack BuildTrack(std::initializer_list<std::tuple<double, double, double>> hits,
                   std::initializer_list<std::tuple<double, double, double>> clusters)
{
   AtTrack track;

   int hitId = 0;
   for (const auto &[x, y, z] : hits) {
      auto hit = std::make_unique<AtHit>();
      hit->SetHitID(hitId++);
      hit->SetPosition({x, y, z});
      hit->SetCharge(1.0);
      hit->SetTimeStamp(hitId);
      track.AddHit(std::move(hit));
   }

   int clusterId = 0;
   for (const auto &[x, y, z] : clusters) {
      auto cluster = MakeCluster(x, y, z, 1.0, clusterId);
      cluster->SetClusterID(clusterId++);
      track.AddClusterHit(cluster);
   }

   return track;
}

AtTrack BuildArcTrack(double radius, XYZPoint center, const std::vector<double> &angles, double zStart, double zStep)
{
   AtTrack track;
   int hitId = 0;
   for (std::size_t i = 0; i < angles.size(); ++i) {
      auto hit = std::make_unique<AtHit>();
      double angle = angles[i];
      hit->SetHitID(hitId++);
      hit->SetPosition(
         {center.X() + radius * std::cos(angle), center.Y() + radius * std::sin(angle), zStart + i * zStep});
      hit->SetCharge(100.0);
      hit->SetTimeStamp(i);
      track.AddHit(std::move(hit));
   }
   return track;
}

void ExpectPointNear(const XYZPoint &lhs, const XYZPoint &rhs, double tol = 1e-9)
{
   EXPECT_NEAR(lhs.X(), rhs.X(), tol);
   EXPECT_NEAR(lhs.Y(), rhs.Y(), tol);
   EXPECT_NEAR(lhs.Z(), rhs.Z(), tol);
}

} // namespace

TEST(AtTrackRefinerTest, OrderClustersAlongTrackStartsAtHighestZAndGreedilyWalksNearestNeighbor)
{
   // Minimal single-track geometry: one outgoing candidate with clusters already
   // lying along a monotonic trajectory in drift Z. This is physically reasonable
   // as a bare ordering fixture, but intentionally omits detector effects.
   AtPATTERN::AtClusterOrderer orderer;
   auto track = BuildTrack(
      {{0.0, 0.0, 100.0}, {0.5, 0.0, 95.0}, {1.0, 0.0, 90.0}, {2.0, 0.0, 80.0}},
      {{1.0, 0.0, 90.0}, {2.0, 0.0, 80.0}, {0.0, 0.0, 100.0}, {0.5, 0.0, 95.0}});

   AtPatternEvent event;
   event.AddTrack(track);
   orderer.Transform(event);

   auto *clusters = event.GetTrackCand().front().GetHitClusterArray();
   ASSERT_EQ(clusters->size(), 4u);
   ExpectPointNear(clusters->at(0).GetPosition(), {0.0, 0.0, 100.0});
   ExpectPointNear(clusters->at(1).GetPosition(), {0.5, 0.0, 95.0});
   ExpectPointNear(clusters->at(2).GetPosition(), {1.0, 0.0, 90.0});
   ExpectPointNear(clusters->at(3).GetPosition(), {2.0, 0.0, 80.0});
}

TEST(AtTrackRefinerTest, SelectAndMergeTracksRejectsBeamLikeTracksMergesFragmentsAndDropsIsolatedTracks)
{
   // Synthetic event-topology fixture representing:
   // 1) one primary emerging near the beam axis,
   // 2) one nearby downstream fragment that should merge into that primary,
   // 3) one isolated off-axis fragment that should be rejected,
   // 4) one beam-like straight-through candidate that should fail the angle cut.
   // This is physics-motivated policy coverage, not a detector-response test.
   AtPATTERN::AtBeamTrackRejector rejector(10.0);
   AtPATTERN::AtSmooth3DClusterer clusterer(5.0, 8.0);
   AtPATTERN::AtFragmentMerger merger(6.0, 5.0); // mergeDist=6mm, vertexRadiusXY=5mm
   merger.SetClusterer(&clusterer);
   AtPATTERN::AtVertexTrackSelector selector(5.0);

   AtTrack primary = BuildTrack(
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}},
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}});

   AtTrack fragment = BuildTrack(
      {{30.0, 0.0, 82.0}, {12.0, 0.0, 82.0}},
      {{30.0, 0.0, 82.0}, {12.0, 0.0, 82.0}});

   AtTrack isolated = BuildTrack(
      {{120.0, 0.0, 98.0}, {130.0, 0.0, 88.0}},
      {{120.0, 0.0, 98.0}, {130.0, 0.0, 88.0}});

   AtTrack beamLike = BuildTrack(
      {{0.0, 0.0, 110.0}, {0.0, 0.0, 70.0}},
      {{0.0, 0.0, 110.0}, {0.0, 0.0, 70.0}});

   AtPatternEvent event;
   event.AddTrack(primary);
   event.AddTrack(fragment);
   event.AddTrack(isolated);
   event.AddTrack(beamLike);

   rejector.Transform(event);
   merger.Transform(event);
   selector.Transform(event);

   auto &tracks = event.GetTrackCand();
   ASSERT_EQ(tracks.size(), 1u);
   auto &merged = tracks.front();
   EXPECT_EQ(merged.GetHitArray().size(), 4u);
   EXPECT_GE(merged.GetHitClusterArray()->size(), 2u);
   EXPECT_GT(merged.GetHitClusterArray()->front().GetPosition().Z(),
             merged.GetHitClusterArray()->back().GetPosition().Z());
}

