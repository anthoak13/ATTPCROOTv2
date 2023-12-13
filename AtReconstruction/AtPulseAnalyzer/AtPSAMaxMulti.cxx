#include "AtPSAMaxMulti.h"

#include "AtHit.h"
#include "AtPad.h"

#include <FairLogger.h>

#include <Math/Point3D.h>    // for PositionVector3D
#include <Math/Point3Dfwd.h> // for XYZPoint

#include <algorithm>
#include <array>    // for array
#include <iterator> // for distance
#include <memory>   // for unique_ptr, make_unique
#include <numeric>
#include <utility> // for pair


AtPSAMax::HitVector AtPSAMaxMulti::AnalyzePad(AtPad *pad)
{
   //Copy pad
   AtPad padCopy = (*pad);
   HitVector ret;
   while (true)
   {
      auto hit = extractHit(&padCopy);
      if(hit == nullptr)
      break;

      auto tb = hit->GetTimeStamp();
      for(int i = tb-fSubtractWindow; i < tb+fSubtractWindow; ++i)
      {
         padCopy.SetADC(i,0);
      }
      ret.push_back(std::move(hit));
   }
   return ret;
}


ClassImp(AtPSAMaxMulti);
