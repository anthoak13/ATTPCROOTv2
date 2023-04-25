#include "AtTestSimulation.h"

#include "AtMCPoint.h"
#include "AtSimpleSimulation.h"

#include <FairTask.h> // for InitStatus, kSUCCESS

#include <Math/Point3D.h>
#include <Math/Point3Dfwd.h> // for Math, XYZPoint
#include <Math/Vector3D.h>
#include <Math/Vector3Dfwd.h> // for XYZVector
#include <Math/Vector4D.h>    // for LorentzVector
#include <Math/Vector4Dfwd.h> // for PxPyPzEVector

#include <cmath>
using namespace ROOT::Math;

InitStatus AtTestSimulation::Init()
{
   auto ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(fatal) << "The IO manager was not instatiated before attempting to simulate an event.";
      return kFATAL;
   }

   ioMan->Register(fBranchName, "AtTPC", &fMCPoints, true);

   return kSUCCESS;
}

void AtTestSimulation::Exec(Option_t *)
{
   fSimulation->NewEvent();

   XYZPoint pos(0, 0, 500);
   XYZVector momDir = XYZVector(0, 0, 1).Unit();
   // Assume Pb208 (mass is 207.93 amu)
   double m = 207.93 * 931.4936; // Mev
   // Assume initial KE is 35 MeV/u
   double E = m + 35 * 207.93;
   // Grab the momentum
   double p = sqrt(E * E - m * m);
   PxPyPzEVector mom(momDir.X() * p, momDir.Y() * p, momDir.Z() * p, E);
   fSimulation->SimulateParticle(82, 208, pos, mom);
}
void AtTestSimulation::FillBranch()
{
   fMCPoints.Delete();
   for (auto &&point : fSimulation->GetPointsArray()) {
      new (fMCPoints[fMCPoints.GetEntries()]) AtMCPoint(std::move(point));
   }
}
ClassImp(AtTestSimulation);
