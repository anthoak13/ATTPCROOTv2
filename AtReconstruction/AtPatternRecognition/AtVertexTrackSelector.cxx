#include "AtVertexTrackSelector.h"

#include "AtHitCluster.h"
#include "AtPRAUtils.h"
#include "AtPatternEvent.h"
#include "AtTrack.h"

#include <FairLogger.h>

#include <algorithm>
#include <vector>

void AtPATTERN::AtVertexTrackSelector::Transform(AtPatternEvent &event)
{
   auto &tracks = event.GetTrackCand();
   if (tracks.empty())
      return;

   // Proxy for vertex Z: highest Z among all vertex-end (front) clusters
   double maxZ = -1e9;
   for (auto &tr : tracks) {
      auto *cl = tr.GetHitClusterArray();
      if (!cl->empty())
         maxZ = std::max(maxZ, cl->front().GetPosition().Z());
   }

   auto isPrimary = [this, maxZ](AtTrack &tr) {
      return AtPATTERN::IsVertexTrack(tr, maxZ, fVertexRadiusXY, fVertexZTolerance);
   };

   int nBefore = static_cast<int>(tracks.size());
   tracks.erase(std::remove_if(tracks.begin(), tracks.end(), [&isPrimary](AtTrack &tr) { return !isPrimary(tr); }),
                tracks.end());
   LOG(info) << "AtVertexTrackSelector: kept " << tracks.size() << "/" << nBefore
             << " primary tracks (vertexR<" << fVertexRadiusXY << "mm)";
}
