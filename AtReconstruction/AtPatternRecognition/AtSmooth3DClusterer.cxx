#include "AtSmooth3DClusterer.h"

#include "AtPatternEvent.h"
#include "AtTrack.h"
#include "AtTrackClusterBuilder.h"

void AtPATTERN::AtSmooth3DClusterer::SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime,
                                                        double padResXY)
{
   fCoefT = coefT;
   fCoefL = coefL;
   fDriftVel = driftVel;
   fTBTime = tbTime;
   if (padResXY > 0)
      fPadResXY = padResXY;
}

void AtPATTERN::AtSmooth3DClusterer::Transform(AtPatternEvent &event)
{
   for (auto &track : event.GetTrackCand())
      ClusterizeTrack(track);
}

void AtPATTERN::AtSmooth3DClusterer::ClusterizeTrack(AtTrack &track) const
{
   AtTools::AtTrackClusterBuilderConfig cfg;
   cfg.coefT = fCoefT;
   cfg.coefL = fCoefL;
   cfg.driftVel = fDriftVel;
   cfg.samplingRate = fTBTime;
   cfg.padResXY = fPadResXY;
   cfg.padResZ = fPadResXY * 1.5;
   cfg.covarianceMode = fCovarianceMode;
   AtTools::ClusterizeSmooth3D(track, fRadius, fDistance, cfg);
}
