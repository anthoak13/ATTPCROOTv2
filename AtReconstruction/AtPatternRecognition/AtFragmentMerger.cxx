#include "AtFragmentMerger.h"

#include "AtClusterOrderer.h"
#include "AtHitCluster.h"
#include "AtPRAUtils.h"
#include "AtPatternEvent.h"
#include "AtSmooth3DClusterer.h"
#include "AtTrack.h"

#include <FairLogger.h>

#include <Math/Point3D.h>

#include <algorithm>
#include <cmath>
#include <vector>

using XYZPoint = ROOT::Math::XYZPoint;

void AtPATTERN::AtFragmentMerger::Transform(AtPatternEvent &event)
{
   auto &tracks = event.GetTrackCand();
   if (tracks.empty())
      return;

   // Proxy for vertex Z: highest Z among all front (vertex-end) clusters
   double maxZ = -1e9;
   for (auto &tr : tracks) {
      auto *cl = tr.GetHitClusterArray();
      if (!cl->empty())
         maxZ = std::max(maxZ, cl->front().GetPosition().Z());
   }

   auto isPrimary = [&](AtTrack &tr) {
      return AtPATTERN::IsVertexTrack(tr, maxZ, fVertexRadiusXY, fVertexZTolerance);
   };

   std::vector<bool> primary(tracks.size());
   for (size_t i = 0; i < tracks.size(); ++i)
      primary[i] = isPrimary(tracks[i]);

   auto getFarEnd = [](AtTrack &tr) -> XYZPoint {
      auto *cl = tr.GetHitClusterArray();
      if (cl->empty())
         return {0, 0, 0};
      return cl->back().GetPosition();
   };

   bool mergedAny = true;
   while (mergedAny) {
      mergedAny = false;
      for (size_t i = 0; i < tracks.size() && !mergedAny; i++) {
         if (!primary[i])
            continue;

         XYZPoint farEnd = getFarEnd(tracks[i]);

         for (size_t j = 0; j < tracks.size() && !mergedAny; j++) {
            if (i == j || primary[j])
               continue;

            const auto *clJ = tracks[j].GetHitClusterArray();
            if (clJ->empty())
               continue;

            double d1 = (farEnd - clJ->front().GetPosition()).R();
            double d2 = (farEnd - clJ->back().GetPosition()).R();

            if (std::min(d1, d2) < fMergeDist) {
               for (auto &hit : tracks[j].GetHitArray())
                  tracks[i].AddHit(hit->Clone());

               tracks.erase(tracks.begin() + j);
               primary.erase(primary.begin() + j);

               tracks[i].ResetHitClusterArray();

               if (fClusterer != nullptr) {
                  // Per-merge re-cluster and re-order so the far-end position used
                  // for the next fragment search reflects the newly merged track.
                  fClusterer->ClusterizeTrack(tracks[i]);
                  if (fOrderer)
                     fOrderer->OrderTrack(tracks[i]);
               }

               mergedAny = true;
               LOG(info) << "AtFragmentMerger: merged fragment (d=" << std::min(d1, d2) << "mm), "
                         << tracks[i].GetHitArray().size() << " hits now";
            }
         }
      }
   }
   // Non-primary tracks that were not merged are left in the event.
   // Use AtVertexTrackSelector after this transform to remove them.
}
