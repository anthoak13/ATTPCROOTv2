/**
 * This file contains infomration for using the AtSimpleSimulation class to simulate fission events.
 * It mostly is responsible for generating fission events based on
 *
 */
#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TH2.h"
#include "TLorentzVector.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TString.h"
#include "TTree.h"

#include <fstream>
#include <iostream>
#include <numeric>
#include <set>

#include "Math/Vector3D.h"
#include "Math/Vector4D.h"

namespace fissionSim {

// Define constants and short names for various classes.
constexpr double AtoE = 939.0;
constexpr double c = 2.998e8; // In m/s
using VecXYZE = ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<>>;
using PxPyPzEVector = ROOT::Math::PxPyPzEVector;
using VecPolar = ROOT::Math::Polar3D<double>;
using XYZVector = ROOT::Math::XYZVector;
using Cartesian3D = ROOT::Math::Cartesian3D<>;
using XYZPoint = ROOT::Math::XYZPoint;
using vecInt = std::vector<Int_t>;
using test = AtTools::AtKinematics; // Literally this is just to force ROOT to load the AtTools lib

TF1 *fAsym = nullptr;   // Function describing the angular distribution of the fission fragments
TF1 *fInvSin = nullptr; // Function describing the angular distribution of the fission fragments (in some limit)

double angMin = TMath::Pi() / 2 - 1;
double angMax = TMath::Pi() / 2 + 1;

TF1 *csFit = nullptr;

// This are the important parameters to set in the simulation
int beamZ = 83;
int beamA = 200;
double beamM = 199.9332;
float massFrac = 0.56;
float massDev = 6;
float decayAngle =
   0; // Angle of the fission fragments in the CoM frame in radians. If 0, then sample from the angular distribution
float zCutoff = 300; // Only simulate events from the window to here (dist from pad plane)

// double beamOffsetX = -7.03043e+00;
// double beamOffsetXsig = 1.62047e+01;
// double beamOffsetY = 1.07526e+01;
// double beamOffsetYsig = 1.51736e+01;

double beamOffsetX = -0.3109;
double beamOffsetXsig = 10.108;
double beamOffsetY = -9.04402;
double beamOffsetYsig = 8.08237;

// double beamDirX = 6.93373e-03;
// double beamDirXsig = 2.66458e-02;
// double beamDirY = -1.79569e-02;
// double beamDirYsig = 3.15214e-02;

double beamDirX = 0.0186;
double beamDirXsig = 0.002203;
double beamDirY = 0.005707;
double beamDirYsig = 0.002309;

// double beamDirX1 = 0.00270859;
double beamDirX1 = 0.0186;
double beamDirX2 = -0.00106274;
// double beamDirXsig1 = 0.0145266;
double beamDirXsig1 = 0.00220254;
double beamDirXsig2 = 0;
// double beamDirY1 = -0.00245368;
double beamDirY1 = 0.00571;
double beamDirY2 = -0.00144553;
// double beamDirYsig1 = 0.0149746;
double beamDirYsig1 = 0.00230867;
double beamDirYsig2 = 0;

double beamE = 2.7000e+03;
double beamEsig = 1.28122e+02;
// double beamEsig = 0;

double vertexZ;
double vertexE;

std::vector<std::pair<int, int>> ions;

std::unique_ptr<AtSimpleSimulation> fSimulation{nullptr}; //!

ofstream outFile;

TH1D *simInfo = new TH1D("simInfo", "simInfo", 7, 0, 7);

TString vX = "vX";
TString vY = "vY";
TString vZ = "vZ";
TString vE = "vE";
TString A0 = "A0";
TString Z0 = "Z0";
TString ang = "ang";

/**  Function prototypes defined in this header files **/

// Sample masses of FF given Acn and the gaussian distribution parameters massFrac and massDev
vecInt getProducMasses(Int_t A, Float_t massFrac, Float_t massDev, TRandom *rand = new TRandom3());

// Different ways of generating Z of FF given sampled masses
vecInt getProductChargeSameDistro(Int_t Z, const vecInt &masses);
vecInt getProductChargeMaxBE(Int_t Z, const vecInt &masses);
vecInt getProductChargeMaxBEA(Int_t Z, const vecInt &masses);
vecInt getProductChargeDist(Int_t Z, const vecInt &masses);

// Return the four momenta of the products in the center of mass frame
std::vector<VecXYZE> getProductMomenta(const vecInt &fragA, const vecInt &fragZ, TRandom *rand, VecPolar &decayAng);

// Generate and simulate a fission event (end result is energy deposition in space in the TPC)
// Returns true if the event should be accepted.
bool generateEvent();
void Init();    // Called at the start of the run
void Exec();    // Called for each event
void CleanUp(); // Called at the end of the run

XYZVector SampleAsym()
{
   if (fAsym == nullptr) {
      fAsym = new TF1("asym",
                      "1 + [0] * cos(x) * cos(x) + [1] * cos(x) * cos(x) * cos(x) * cos(x)+ [2] * cos(x) * cos(x)* "
                      "cos(x) * cos(x)* cos(x) * cos(x)",
                      0, TMath::Pi());
      fAsym->SetParameters(0.2676, 0.7282, 0.5472);
   }

   double theta = fAsym->GetRandom();
   VecPolar decayAng(1, theta, gRandom->Uniform(TMath::TwoPi()));
   return {decayAng.x(), decayAng.y(), decayAng.z()};
}

XYZVector SampleInvSin()
{
   if (fInvSin == nullptr) {
      fInvSin = new TF1("invSin", "1 / sin(x)", angMin, angMax);
   }

   double theta = fInvSin->GetRandom();
   VecPolar decayAng(1, theta, gRandom->Uniform(TMath::TwoPi()));
   return {decayAng.x(), decayAng.y(), decayAng.z()};
}

void Init()
{
   fSimulation->RegisterBranch();
   outFile.open("simOutFile.txt");

   simInfo->GetXaxis()->SetBinLabel(1, vX);
   simInfo->GetXaxis()->SetBinLabel(2, vY);
   simInfo->GetXaxis()->SetBinLabel(3, vZ);
   simInfo->GetXaxis()->SetBinLabel(4, vE);
   simInfo->GetXaxis()->SetBinLabel(5, A0);
   simInfo->GetXaxis()->SetBinLabel(6, Z0);
   simInfo->GetXaxis()->SetBinLabel(7, ang);

   FairRootManager *ioManager = FairRootManager::Instance();
   if (ioManager == nullptr) {
      LOG(error) << "Cannot find RootManager!" << std::endl;
      return;
   }

   ioManager->Register("Info", "AtTPC", simInfo, true);
}

Int_t sumVector(const vecInt &vec)
{
   return std::accumulate(vec.cbegin(), vec.cend(), 0);
}

// Returns the average total kinetic energy from viola systematics in MeV
double violaEn(int A, int Z)
{
   return 0.1189 * Z * Z / TMath::Power(A, 1.0 / 3.0) + 7.3;
}

bool generateEvent()
{

   for (int i = 0; i < 8; i++) {
      simInfo->SetBinContent(i, 0);
   }

   XYZPoint beamOrigin;

   XYZVector beamDirection;

   /** Sample the beam direction, location, and energy and store it in the 4-vector beamMomentum*/
   beamOrigin.SetX(gRandom->Gaus(beamOffsetX, beamOffsetXsig));
   beamOrigin.SetY(gRandom->Gaus(beamOffsetY, beamOffsetYsig));
   beamOrigin.SetZ(0);

   beamDirection.SetX(gRandom->Gaus(beamDirX, beamDirXsig));
   beamDirection.SetY(gRandom->Gaus(beamDirY, beamDirYsig));
   beamDirection.SetZ(1);

   auto originEnergy = gRandom->Gaus(beamE, beamEsig);

   auto beamMomMag = sqrt(pow(originEnergy + beamM * AtoE, 2) - pow(beamM * AtoE, 2));
   VecXYZE beamMomentum = AtTools::Kinematics::Get4Vector(beamMomMag * beamDirection.Unit(), beamM * AtoE);
   // cout << "Beam Origin Energy: " << originEnergy << endl;
   // cout << "Beam Origin: (" << beamOrigin.X() << ", " << beamOrigin.Y() << ", " << beamOrigin.Z() << ")" << endl;
   // cout << "Beam Momentum: (" << beamMomentum.X() << ", " << beamMomentum.Y() << ", " << beamMomentum.Z() << ")" <<
   // endl; cout << "Beam Energy: " << beamMomentum.E()  - beamMomentum.M() << endl;

   // outFile << beamOrigin.X() << " " << beamOrigin.Y() << " " << beamOrigin.Z() << " ";

   /** Sample the location where the reaction should occur in the detector (in z-dir in mm) */
   auto vertexZ = gRandom->Uniform(1000 - zCutoff);
   cout << "Simulating beam to " << vertexZ << " mm" << endl;

   // Simulate the beam to the reaction vertex. The lambda function is used to stop the simulation when the beam reaches
   // the vertex. Returns the position and 4 momentum of the beam at the reaction vertex.
   auto beamPos =
      fSimulation->SimulateParticle(beamZ, beamA, beamOrigin, beamMomentum, [vertexZ](XYZPoint pos, VecXYZE mom) {
         return pos.Z() < vertexZ - 10;
         // return (mom.E() < vertexE);
      });

   //** Record data in output file and into the meta data histogram*/
   outFile << beamOrigin.X() << " " << beamOrigin.Y() << " " << beamOrigin.Z() << " "
           << beamMomentum.E() - beamMomentum.M() << " ";
   outFile << beamPos.first.X() << " " << beamPos.first.Y() << " " << beamPos.first.Z() << " "
           << beamPos.second.E() - beamPos.second.M() << " ";

   simInfo->Fill(vX, beamPos.first.X());
   simInfo->Fill(vY, beamPos.first.Y());
   simInfo->Fill(vZ, beamPos.first.Z());
   simInfo->Fill(vE, beamPos.second.E() - beamPos.second.M());

   cout << "Beam Simulated" << endl;

   cout << "Simulated vertex Position: (" << beamPos.first.X() << ", " << beamPos.first.Y() << ", "
        << 1000 - beamPos.first.Z() << ")" << endl;
   cout << "Simulated vertex Energy: " << beamPos.second.E() - beamPos.second.M() << endl;
   cout << "Number of hits registered: " << fSimulation->GetNumPoints() << endl;

   vecInt fragZ = {0, 0};
   vecInt fragA = {0, 0};

   // Generate the masses and charges of the fission fragments
   while (fragZ[0] == 0) {
      fragA = getProducMasses(beamA + 4, massFrac, massDev, gRandom);
      fragZ = getProductChargeDist(beamZ + 2, fragA);
      std::cout << "Trying " << fragA[0] << " " << fragZ[0] << std::endl;
   }

   // Generate an array of products in the rest frame using the supplied angle
   // or a random angle if 0 was supplied
   VecPolar decayDir(1, decayAngle, 0); // r, theta, phi

   auto decayMomenta = getProductMomenta(fragA, fragZ, gRandom, decayDir);

   outFile << fragA[0] << " " << fragZ[0] << " " << decayDir.Theta() << " ";

   simInfo->Fill(A0, fragA[0]);
   simInfo->Fill(Z0, fragZ[0]);
   simInfo->Fill(ang, decayDir.Theta());

   auto fBeamBoost = ROOT::Math::Boost(beamPos.second.BoostToCM());
   fBeamBoost.Invert();

   std::array<std::pair<XYZPoint, PxPyPzEVector>, 2> finalPos;
   bool keepEvent = true;
   double radCut = 50; // in mm
   for (int i = 0; i < 2; i++) {
      auto labP = fBeamBoost(decayMomenta[i]);

      cout << "Simulating fragment " << i << endl;
      // cout << "fragZ.size() " << fragZ.size() << "; fragA.size()" << fragA.size() << endl;
      cout << "Z: " << fragZ[i] << ", A: " << fragA[i] << endl;
      auto [fragPos, fragMom] = fSimulation->SimulateParticle(fragZ[i], fragA[i], beamPos.first, labP);

      // Only keep events that would have passed our trigger condition
      double fragRho = std::sqrt(fragPos.X() * fragPos.X() + fragPos.Y() * fragPos.Y());
      keepEvent &= fragRho > radCut;

      cout << "Number of hits registered: " << fSimulation->GetNumPoints() << endl;
   }
   return keepEvent;
}

vecInt getProductChargeMaxBE(Int_t Z, const vecInt &masses)
{
   auto asym = 23.2;
   auto coul = 0.714;
   Int_t A = sumVector(masses);

   Float_t numerator = 8 * asym * masses.at(0) * Z;
   numerator += coul * TMath::Power(masses.at(0), 2.0 / 3.0) * masses.at(1);
   numerator += coul * TMath::Power(masses.at(1), 2.0 / 3.0) * masses.at(0) * (2 * Z - 1);

   Float_t denominator = 8 * asym * A;
   denominator += 2 * coul * TMath::Power(masses.at(1), 2.0 / 3.0) * masses.at(0);
   denominator += 2 * coul * TMath::Power(masses.at(0), 2.0 / 3.0) * masses.at(1);

   Int_t Z1 = TMath::Nint(numerator / denominator);
   vecInt fragZ;
   fragZ.push_back(Z1);
   fragZ.push_back(Z - Z1);
   return fragZ;
}

vecInt getProductChargeDist(Int_t Z, const vecInt &masses)
{
   std::vector<int> distZ;
   for (auto [Z, A] : ions) {
      if (A == masses[0])
         distZ.push_back(Z);
   }

   if (distZ.size() > 0) {
      auto Z0 = distZ[gRandom->Integer(distZ.size())];
      auto Z1 = Z - Z0;
      return {Z0, Z1};
   }

   return {0, 0};
}

vecInt getProductChargeMaxBEA(Int_t Z, const vecInt &masses)
{
   // LDM parameters
   auto asym = 23.2;
   auto coul = 0.714;

   Int_t A = sumVector(masses);
   auto A1 = masses.at(0);
   auto A2 = masses.at(1);

   Float_t numAsym = A2 * A2 - A1 * A2 + 2 * Z * A1;
   Float_t numerator = 4 * asym * A1 * numAsym;

   numerator += coul * TMath::Power(A1, 2.0 / 3.0) * A2 * A2;
   numerator += coul * TMath::Power(A2, 2.0 / 3.0) * A1 * A1 * (2 * Z - 1);

   Float_t denominator = 8 * asym * (A1 * A1 + A2 * A2);
   denominator += 2 * coul * TMath::Power(A2, 2.0 / 3.0) * A1 * A1;
   denominator += 2 * coul * TMath::Power(A1, 2.0 / 3.0) * A2 * A2;

   Int_t Z1 = TMath::Nint(numerator / denominator);
   vecInt fragZ;
   fragZ.push_back(Z1);
   fragZ.push_back(Z - Z1);
   return fragZ;
}

std::vector<VecXYZE> getProductMomenta(const vecInt &fragA, const vecInt &fragZ, TRandom *rand, VecPolar &decayAng)
{

   Int_t Z = sumVector(fragZ);
   Int_t A = sumVector(fragA);

   // Get the masses of the fragments in MeV
   double m[2];
   for (int i = 0; i < 2; ++i)
      m[i] = fragA.at(i) * AtoE;

   // Get the KE of the products
   double kE = violaEn(A, Z);

   double gamma1 = AtTools::Kinematics::GetGamma(kE, m[0], m[1]);
   double p1 = AtTools::Kinematics::GetRelMom(gamma1, m[0]);

   // Set the momentum unit vector if nullptr
   if (decayAng.Theta() != 0)
      decayAng.SetPhi(rand->Uniform(TMath::TwoPi()));
   else {
      decayAng = SampleAsym();
      // decayAng = SampleInvSin();
   }

   cout << "Theta in CoM: " << decayAng.Theta() * TMath::RadToDeg() << " deg" << endl;

   // Set the momentum of first particles
   // decayAng.SetR(TMath::Sqrt(E[0] * E[0] - m[0] * m[0]));
   decayAng.SetR(p1);
   ROOT::Math::XYZVector pDir(decayAng);

   std::vector<VecXYZE> ret;
   ret.push_back(AtTools::Kinematics::Get4Vector(pDir, m[0]));
   ret.push_back(AtTools::Kinematics::Get4Vector(-pDir, m[1]));

   return ret;
}

vecInt getProducMasses(Int_t A, Float_t massFrac, Float_t massDev, TRandom *rand)
{
   Int_t A1 = TMath::Nint(rand->Gaus(A * massFrac, massDev));
   if (A1 < A / 2)
      A1 = A - A1;
   vecInt fragA;
   fragA.push_back(A1);
   fragA.push_back(A - A1);
   return fragA;
}

void Exec()
{

   while (true) {
      fSimulation->NewEvent();
      if (generateEvent())
         break;
      cout << "Failed to generate event that passes trigger. Trying again." << std::endl;
   }
   outFile << endl;
}

void CleanUp()
{
   outFile.close();
}

} // namespace fissionSim
