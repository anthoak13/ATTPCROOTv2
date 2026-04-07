#include "AtGroupClusterer.h"

#include "AtHit.h"
#include "AtHitCluster.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"
#include "AtTrackClusterBuilder.h"

#include <algorithm>
#include <memory>
#include <vector>

void AtPATTERN::AtGroupClusterer::SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime,
                                                     double padResXY)
{
   fCoefT = coefT;
   fCoefL = coefL;
   fDriftVel = driftVel;
   fTBTime = tbTime;
   if (padResXY > 0)
      fPadResXY = padResXY;
}

void AtPATTERN::AtGroupClusterer::Transform(AtPatternEvent &event)
{
   AtTools::AtTrackClusterBuilderConfig builderConfig;
   builderConfig.coefT = fCoefT;
   builderConfig.coefL = fCoefL;
   builderConfig.driftVel = fDriftVel;
   builderConfig.samplingRate = fTBTime;
   builderConfig.padResXY = fPadResXY;
   builderConfig.padResZ = fPadResXY * 1.5;
   builderConfig.covarianceMode = fCovarianceMode;
   AtTools::AtTrackClusterBuilder clusterBuilder(builderConfig);

   for (auto &track : event.GetTrackCand()) {
      auto hitArray = track.GetHitArrayObject();
      if (hitArray.empty())
         continue;

      int nHits = static_cast<int>(hitArray.size());
      int clusterID = 0;

      for (int start = 0; start < nHits; start += fHitsPerCluster) {
         int end = std::min(start + fHitsPerCluster, nHits);
         if (end - start < 2)
            continue;

         std::vector<AtHit> clusterHits(hitArray.begin() + start, hitArray.begin() + end);
         auto hitCluster = clusterBuilder.BuildCluster(clusterHits, clusterID);
         if (!hitCluster)
            continue;
         clusterID++;
         track.AddClusterHit(hitCluster);
      }
   }
}
