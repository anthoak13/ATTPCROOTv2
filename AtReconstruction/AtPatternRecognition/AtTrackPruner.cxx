#include "AtTrackPruner.h"

#include "AtHit.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"

#include <Math/Point3D.h>
#include <TMath.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void AtPATTERN::AtTrackPruner::Transform(AtPatternEvent &event)
{
   for (auto &track : event.GetTrackCand()) {
      auto &hitArray = track.GetHitArray();
      for (int i = static_cast<int>(hitArray.size()) - 1; i >= 0; --i) {
         try {
            if (IsOutlier(track, i))
               hitArray.erase(hitArray.begin() + i);
         } catch (std::exception &e) {
            std::cout << "AtTrackPruner: exception on hit " << i << ": " << e.what() << "\n";
         }
      }
   }
}

bool AtPATTERN::AtTrackPruner::IsOutlier(const AtTrack &track, int hitIdx) const
{
   const auto &hitArray = track.GetHitArray();
   int n = static_cast<int>(hitArray.size());
   int k = std::min(fKNN, n);

   const auto &refPos = hitArray.at(hitIdx)->GetPosition();

   std::vector<double> distances;
   distances.reserve(n);
   for (int i = 0; i < n; i++) {
      if (i == hitIdx)
         continue;
      distances.push_back(TMath::Sqrt((refPos - hitArray.at(i)->GetPosition()).Mag2()));
   }

   std::sort(distances.begin(), distances.end());

   double mean = 0.0;
   for (int i = 0; i < k && i < static_cast<int>(distances.size()); i++)
      mean += distances[i];
   mean /= k;

   double stdDev = 0.0;
   for (int i = 0; i < k && i < static_cast<int>(distances.size()); i++)
      stdDev += TMath::Power(distances[i] - mean, 2);
   stdDev = TMath::Sqrt(stdDev / k);

   double T = mean + stdDev * fStdDevMul;
   return T >= fKNNDist;
}
