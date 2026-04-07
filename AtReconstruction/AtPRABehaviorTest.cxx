#include "AtBeamTrackRejector.h"
#include "AtCircleSeeder.h"
#include "AtClusterOrderer.h"
#include "AtEvent.h"
#include "AtFragmentMerger.h"
#include "AtHit.h"
#include "AtHitCluster.h"
#include "AtPatternCircle2D.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"
#include "AtTrackFinderTC.h"
#include "AtVertexTrackSelector.h"

#include "cluster.h"
#include "pointcloud.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using XYZPoint = ROOT::Math::XYZPoint;

class TestTrackFinderTC : public AtPATTERN::AtTrackFinderTC {
public:
   using AtPATTERN::AtTrackFinderTC::BuildRawTracksFromClusters;
};

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

AtEvent BuildFinderEvent(std::initializer_list<std::tuple<double, double, double>> hits)
{
   AtEvent event;
   int hitId = 0;
   for (const auto &[x, y, z] : hits) {
      auto hit = std::make_unique<AtHit>();
      hit->SetHitID(hitId++);
      hit->SetPosition({x, y, z});
      hit->SetCharge(1.0);
      hit->SetTimeStamp(hitId);
      event.AddHit(std::move(hit));
   }
   return event;
}

std::vector<XYZPoint> GetClusterPositions(AtTrack &track)
{
   std::vector<XYZPoint> positions;
   for (const auto &cluster : *track.GetHitClusterArray())
      positions.push_back(cluster.GetPosition());
   return positions;
}

AtTrack BuildArcTrack(double radius, XYZPoint center, const std::vector<double> &angles, double zStart, double zStep)
{
   AtTrack track;
   int hitId = 0;
   for (std::size_t i = 0; i < angles.size(); ++i) {
      auto hit = std::make_unique<AtHit>();
      double angle = angles[i];
      hit->SetHitID(hitId++);
      hit->SetPosition({center.X() + radius * std::cos(angle), center.Y() + radius * std::sin(angle), zStart + i * zStep});
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

TEST(AtPRABehaviorTest, ClusterOrdererStartsAtHighestZAndGreedilyWalksNearestNeighbor)
{
   // Minimal single-track geometry used to characterize the current ordering rule.
   // Physically this stands in for one already-found outgoing trajectory with no
   // branching or detector complications.
   AtPATTERN::AtClusterOrderer orderer;
   auto track = BuildTrack(
      {{0.0, 0.0, 100.0}, {0.5, 0.0, 95.0}, {1.0, 0.0, 90.0}, {2.0, 0.0, 80.0}},
      {{1.0, 0.0, 90.0}, {2.0, 0.0, 80.0}, {0.0, 0.0, 100.0}, {0.5, 0.0, 95.0}});

   AtPatternEvent event;
   event.AddTrack(track);
   orderer.Transform(event);

   auto ordered = GetClusterPositions(event.GetTrackCand().front());
   ASSERT_EQ(ordered.size(), 4u);
   ExpectPointNear(ordered[0], {0.0, 0.0, 100.0});
   ExpectPointNear(ordered[1], {0.5, 0.0, 95.0});
   ExpectPointNear(ordered[2], {1.0, 0.0, 90.0});
   ExpectPointNear(ordered[3], {2.0, 0.0, 80.0});
}

TEST(AtPRABehaviorTest, BeamTrackRejectorRemovesBeamLikeTracks)
{
   // Track directed along Z (beam-like) should be rejected.
   AtPATTERN::AtBeamTrackRejector rejector(10.0);

   AtTrack beamLike = BuildTrack(
      {{0.0, 0.0, 110.0}, {0.0, 0.0, 70.0}},
      {{0.0, 0.0, 110.0}, {0.0, 0.0, 70.0}});

   AtTrack scattered = BuildTrack(
      {{0.0, 0.0, 100.0}, {50.0, 0.0, 60.0}},
      {{0.0, 0.0, 100.0}, {50.0, 0.0, 60.0}});

   AtPatternEvent event;
   event.AddTrack(beamLike);
   event.AddTrack(scattered);

   rejector.Transform(event);
   ASSERT_EQ(event.GetTrackCand().size(), 1u);
}

TEST(AtPRABehaviorTest, VertexTrackSelectorKeepsPrimaryTracks)
{
   // Track near beam axis at max Z is primary; off-axis track is not.
   AtPATTERN::AtVertexTrackSelector selector(80.0);

   AtTrack primary = BuildTrack(
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}},
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}});

   AtTrack isolated = BuildTrack(
      {{120.0, 0.0, 98.0}, {130.0, 0.0, 88.0}},
      {{120.0, 0.0, 98.0}, {130.0, 0.0, 88.0}});

   AtPatternEvent event;
   event.AddTrack(primary);
   event.AddTrack(isolated);

   selector.Transform(event);
   ASSERT_EQ(event.GetTrackCand().size(), 1u);
}

TEST(AtPRABehaviorTest, FragmentMergerMergesNearbyFragments)
{
   // Fragment whose near end is within 6mm of the primary far end should merge.
   // vertexRadiusXY=5mm ensures only the origin track (rXY=0) is primary;
   // the fragment at rXY=12mm is non-primary and merges into the primary.
   AtPATTERN::AtFragmentMerger merger(6.0, 5.0);

   AtTrack primary = BuildTrack(
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}},
      {{0.0, 0.0, 100.0}, {10.0, 0.0, 85.0}});

   AtTrack fragment = BuildTrack(
      {{12.0, 0.0, 82.0}, {20.0, 0.0, 78.0}},
      {{12.0, 0.0, 82.0}, {20.0, 0.0, 78.0}});

   AtPatternEvent event;
   event.AddTrack(primary);
   event.AddTrack(fragment);

   merger.Transform(event);

   ASSERT_EQ(event.GetTrackCand().size(), 1u);
   EXPECT_EQ(event.GetTrackCand().front().GetHitArray().size(), 4u);
   // Clusters cleared after merge — caller must re-cluster (no clusterer set)
   EXPECT_TRUE(event.GetTrackCand().front().GetHitClusterArray()->empty());
}

TEST(AtPRABehaviorTest, CircleSeederCharacterizesCurrentCurvedTrackSeeding)
{
   // Clean curved-track arc representing an idealized charged-particle trajectory
   // in field. This is intended to lock down geometric seeding behavior rather
   // than to emulate full detector response.
   AtPATTERN::AtCircleSeeder seeder;
   auto track = BuildArcTrack(100.0, {0.0, 0.0, 0.0}, {2.30, 2.10, 1.90, 1.70, 1.50, 1.30, 1.10, 0.90}, 20.0, 8.0);

   AtPatternEvent event;
   event.AddTrack(track);
   seeder.Transform(event);

   auto &seeded = event.GetTrackCand().front();
   auto center = seeded.GetGeoCenter();
   EXPECT_NEAR(center.first, 0.0, 1.0);
   EXPECT_NEAR(center.second, 0.0, 1.0);
   EXPECT_NEAR(seeded.GetGeoRadius(), 100.0, 1.0);
   EXPECT_TRUE(std::isnan(seeded.GetGeoTheta()));
   EXPECT_NE(seeded.GetGeoPhi(), 0.0);

   auto *pattern = dynamic_cast<const AtPatterns::AtPatternCircle2D *>(seeded.GetPattern());
   ASSERT_NE(pattern, nullptr);
   EXPECT_NEAR(pattern->GetCenter().X(), 0.0, 1.0);
   EXPECT_NEAR(pattern->GetCenter().Y(), 0.0, 1.0);
   EXPECT_NEAR(pattern->GetRadius(), 100.0, 1.0);
}

TEST(AtPRABehaviorTest, TrackFinderTCRawCandidatesBuildingProducesTracksWithHitsOnly)
{
   // Hand-built cluster layout used to mark the seam between raw TC candidate
   // construction and the later refinement/seeding pass. After BuildRawTracksFromClusters,
   // tracks have hits but no clusters (clustering is a separate step).
   TestTrackFinderTC finder;

   auto event = BuildFinderEvent({{0.0, 0.0, 102.0},
                                  {3.0, 0.0, 98.0},
                                  {6.0, 0.0, 94.0},
                                  {10.0, 0.0, 88.0},
                                  {120.0, 0.0, 100.0},
                                  {124.0, 0.0, 96.0},
                                  {128.0, 0.0, 92.0},
                                  {132.0, 0.0, 88.0}});

   PointCloud cloud;
   for (int i = 0; i < event.GetNumHits(); ++i) {
      Point point;
      auto position = event.GetHit(i).GetPosition();
      point.x = position.X();
      point.y = position.Y();
      point.z = position.Z();
      point.SetID(i);
      cloud.push_back(point);
   }

   std::vector<cluster_t> clusters = {{0, 1, 2, 3}, {4, 5, 6, 7}};
   auto noisePoints = cloud;
   std::vector<AtTrack> rawTracks;

   finder.BuildRawTracksFromClusters(cloud, clusters, event, rawTracks, noisePoints);

   ASSERT_EQ(rawTracks.size(), 2u);
   EXPECT_TRUE(noisePoints.empty());
   for (auto &track : rawTracks) {
      EXPECT_EQ(track.GetHitArray().size(), 4u);
      // No clustering step — cluster array must be empty
      EXPECT_TRUE(track.GetHitClusterArray()->empty());
      EXPECT_EQ(track.GetPattern(), nullptr);
   }
}
