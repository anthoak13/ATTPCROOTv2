#include "AtClusterOrderer.h"

#include "AtHitCluster.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"

#include <vector>

void AtPATTERN::AtClusterOrderer::Transform(AtPatternEvent &event)
{
   for (auto &track : event.GetTrackCand())
      OrderTrack(track);
}

void AtPATTERN::AtClusterOrderer::OrderTrack(AtTrack &track) const
{
   auto *clusters = track.GetHitClusterArray();
   int nCl = static_cast<int>(clusters->size());
   if (nCl < 3)
      return;

   // Find cluster with highest Z as seed (vertex end)
   int seedIdx = 0;
   for (int i = 1; i < nCl; i++) {
      if (clusters->at(i).GetPosition().Z() > clusters->at(seedIdx).GetPosition().Z())
         seedIdx = i;
   }

   // Greedy nearest-neighbor walk from the seed
   std::vector<int> order;
   std::vector<bool> used(nCl, false);
   order.push_back(seedIdx);
   used[seedIdx] = true;

   for (int step = 1; step < nCl; step++) {
      auto current = clusters->at(order.back()).GetPosition();
      double bestDist = 1e9;
      int bestIdx = -1;
      for (int j = 0; j < nCl; j++) {
         if (used[j])
            continue;
         double d = (clusters->at(j).GetPosition() - current).R();
         if (d < bestDist) {
            bestDist = d;
            bestIdx = j;
         }
      }
      if (bestIdx < 0)
         break;
      order.push_back(bestIdx);
      used[bestIdx] = true;
   }

   std::vector<AtHitCluster> ordered;
   ordered.reserve(order.size());
   for (int idx : order)
      ordered.push_back(clusters->at(idx));
   *clusters = std::move(ordered);
}
