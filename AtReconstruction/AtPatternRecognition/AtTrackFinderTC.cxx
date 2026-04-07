#include "AtTrackFinderTC.h"

#include "AtEvent.h"        // for AtEvent
#include "AtHit.h"          // for AtHit
#include "AtPatternEvent.h" // for AtPatternEvent
#include "AtTrack.h"        // for AtTrack

#include <Math/Point3D.h> // for PositionVector3D

#include "dnn.h"
#include "graph.h"
#include "option.h"
#include "pointcloud.h"
#include "postprocess.h"
#include "triplet.h" // for triplet, generate_triplets

#include <algorithm>
#include <cmath>    // for sqrt
#include <iostream> // for cout, cerr
#include <memory>   // for allocator_traits<>::value_...
#include <utility>  // for move

std::unique_ptr<AtPatternEvent> AtPATTERN::AtTrackFinderTC::FindTracks(AtEvent &event)
{
   Opt opt_params;
   int opt_verbose = opt_params.get_verbosity();

   opt_params.set_parameters(inputParams.s, inputParams.k, inputParams.n, inputParams.m, inputParams.r, inputParams.a,
                             inputParams.t);

   PointCloud cloud_xyz;
   eventToClusters(event, cloud_xyz);

   if (cloud_xyz.size() == 0) {
      std::cerr << "[Error] empty cloud " << std::endl;
      return nullptr;
   }

   if (opt_params.needs_dnn()) {
      double dnn = std::sqrt(first_quartile(cloud_xyz));
      if (opt_verbose > 0)
         std::cout << "AtPATTERN::AtTrackFinderTC - [Info] computed dnn: " << dnn << std::endl;
      opt_params.set_dnn(dnn);
      if (dnn == 0.0) {
         std::cerr << "AtPATTERN::AtTrackFinderTC - [Error] dnn computed as zero." << std::endl;
         return nullptr;
      }
   }

   // Step 1: smooth by position averaging of neighboring points
   PointCloud cloud_xyz_smooth;
   smoothen_cloud(cloud_xyz, cloud_xyz_smooth, opt_params.get_r());

   // Step 2: find triplets of approximately collinear points
   std::vector<triplet> triplets;
   generate_triplets(cloud_xyz_smooth, triplets, opt_params.get_k(), opt_params.get_n(), opt_params.get_a());

   // Step 3: single-link hierarchical clustering of triplets
   cluster_group cl_group;
   compute_hc(cloud_xyz_smooth, cl_group, triplets, opt_params.get_s(), opt_params.get_t(), opt_params.is_tauto(),
              opt_params.get_dmax(), opt_params.is_dmax(), opt_params.get_linkage(), opt_verbose);

   // Step 4: prune small clusters
   cleanup_cluster_group(cl_group, opt_params.get_m(), opt_verbose);
   cluster_triplets_to_points(triplets, cl_group);
   if (opt_params.is_dmax()) {
      cluster_group cleaned_up_cluster_group;
      for (auto &cl : cl_group)
         max_step(cleaned_up_cluster_group, cl, cloud_xyz, opt_params.get_dmax(), opt_params.get_m() + 2);
      cl_group = cleaned_up_cluster_group;
   }

   add_clusters(cloud_xyz, cl_group, opt_params.is_gnuplot());

   // Convert clusters to raw AtTrack candidates (hits only, no clustering)
   auto noisePoints = cloud_xyz;
   std::vector<AtTrack> tracks;
   BuildRawTracksFromClusters(cloud_xyz, cl_group, event, tracks, noisePoints);

   auto retEvent = std::make_unique<AtPatternEvent>();
   for (const auto &point : noisePoints)
      retEvent->AddNoise(event.GetHit(point.GetID()));
   for (auto &track : tracks)
      retEvent->AddTrack(std::move(track));

   return retEvent;
}

void AtPATTERN::AtTrackFinderTC::eventToClusters(AtEvent &event, PointCloud &cloud)
{
   int nHits = event.GetNumHits();
   for (int iHit = 0; iHit < nHits; iHit++) {
      Point point;
      const AtHit hit = event.GetHit(iHit);
      auto position = hit.GetPosition();
      point.x = position.X();
      point.y = position.Y();
      point.z = position.Z();
      point.SetID(iHit);
      cloud.push_back(point);
   }
}

void AtPATTERN::AtTrackFinderTC::BuildRawTracksFromClusters(PointCloud &cloud, const std::vector<cluster_t> &clusters,
                                                            AtEvent &event, std::vector<AtTrack> &tracks,
                                                            PointCloud &noisePoints)
{
   for (size_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index) {
      AtTrack track;
      const std::vector<size_t> &point_indices = clusters[cluster_index];
      if (point_indices.empty())
         continue;

      for (auto ind : point_indices) {
         const Point &point = cloud[ind];
         track.AddHit(event.GetHit(point.GetID()));
         for (auto p = noisePoints.begin(); p != noisePoints.end(); p++) {
            if (*p == point) {
               noisePoints.erase(p);
               break;
            }
         }
      }

      track.SetTrackID(cluster_index);
      tracks.push_back(track);
   }
}
