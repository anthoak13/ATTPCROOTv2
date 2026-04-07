#include "AtBeamTrackRejector.h"

#include "AtHitCluster.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"

#include <FairLogger.h>

#include <Math/Point3D.h>

#include <algorithm>
#include <cmath>
#include <vector>

using XYZPoint = ROOT::Math::XYZPoint;

void AtPATTERN::AtBeamTrackRejector::Transform(AtPatternEvent &event)
{
   auto &tracks = event.GetTrackCand();

   auto isBeamLike = [this](AtTrack &tr) {
      auto *cl = tr.GetHitClusterArray();
      if (cl->size() < 2)
         return true;
      auto vtx = cl->front().GetPosition();
      auto far = cl->back().GetPosition();
      auto dir = far - vtx;
      if (dir.R() < 1e-3)
         return true;
      double cosThDigi = -dir.Z() / dir.R();
      double thDigi = std::acos(std::min(1.0, std::max(-1.0, cosThDigi))) * 180.0 / M_PI;
      double thLab = 180.0 - thDigi;
      return (thLab < fMinLabTheta || thLab > (180.0 - fMinLabTheta));
   };

   int nBefore = static_cast<int>(tracks.size());
   tracks.erase(std::remove_if(tracks.begin(), tracks.end(), isBeamLike), tracks.end());
   LOG(info) << "AtBeamTrackRejector: removed " << (nBefore - static_cast<int>(tracks.size()))
             << " beam-like tracks (minLabTheta=" << fMinLabTheta << " deg)";
}
