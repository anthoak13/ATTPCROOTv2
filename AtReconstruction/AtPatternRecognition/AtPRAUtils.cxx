#include "AtPRAUtils.h"

#include "AtHitCluster.h"
#include "AtTrack.h"

#include <Math/Point3D.h>

#include <cmath>

bool AtPATTERN::IsVertexTrack(AtTrack &track, double vertexZ, double radiusXY, double zTolerance)
{
   const auto *cl = track.GetHitClusterArray();
   if (cl->empty())
      return false;
   auto vtx = cl->front().GetPosition();
   double rXY = std::sqrt(vtx.X() * vtx.X() + vtx.Y() * vtx.Y());
   return rXY < radiusXY && std::abs(vtx.Z() - vertexZ) < zTolerance;
}
