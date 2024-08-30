// Code to take MC tracks and digitize
#include "eventSim.h"

bool reduceFunc(AtRawEvent *evt);

void run_simp_fiss(int runNum = 0)
{

   delete gRandom;
   gRandom = new TRandom3;
   gRandom->SetSeed(0);

   //************ Things to change ************//
   int Zcn = 83 + 2;  // Number of protons in the compound nucleus
   int Acn = 200 + 4; // Number of nucleons in the compound nucleus
   int Zmin = 26;     // Minimum Z to simulate for the fission fragments
   int Zmax = 59;     // Maximum Z to simulate for hte fission fragments
   int zToSim = 50;

   fissionSim::beamZ = 83;                     // Number of protons in the beam
   fissionSim::beamA = 200;                    // Number of nucleons in the beam
   fissionSim::beamM = 199.9332;               // Mass of the beam in amu
                                               // fissionSim::massFrac = 0.56;
   fissionSim::massFrac = (float)zToSim / Zcn; // Mean of the FF mass distribution (as a fraction of Acn).

   fissionSim::massDev = 0;
   // 6; // Standard deviation of the FF mass distribution in amu. Set to 0 for single mass splitting.
   fissionSim::decayAngle =
      90 * TMath::DegToRad(); // Angle of the decay in CoM frame in radians (0 means sample the distribution)

   fissionSim::beamE = 2.70013e+03; // Get from LISE, beam energy in MeV
   fissionSim::beamEsig =
      1.28122e+02; // Spread in the beam energy from strageling. Either set to 0 or maintain the same ratio with beamE

   //************ End things to change ************//

   TString inOutDir = "./data/"; // Directory to save the output file
   TString tpcSharedInfoDir =
      "/home/adam/fair_install/tpcSharedInfo/";          // Directory containing the shared information for the TPC
   TString energyLossDir = tpcSharedInfoDir + "/eLoss/"; // Directory containing the energy loss tables
   TString outputFile = inOutDir + TString::Format("simFission%02d.root", runNum);
   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString scriptfile = "e12014_pad_mapping.xml";
   TString paramFile = "ATTPC.e12014.par";

   TString dir = getenv("VMCWORKDIR");
   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString digiParFile = paramFile;
   TString mapParFile = dir + "/scripts/" + scriptfile;

   TStopwatch timer;

   /** Create the run and set the output file and parameter file */
   FairRunAna *fRun = new FairRunAna();
   fRun->SetSink(new FairRootFileSink(outputFile));
   fRun->SetGeomFile(GeoDataPath);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setFirstInput(parIo1);

   // Create the detector map to pass to the simulation (this is the pad plane mapping)
   auto mapping = std::make_shared<AtTpcMap>();
   mapping->ParseXMLMap(mapParFile.Data());
   mapping->GeneratePadPlane();

   // Create the simulation object and set the distance step (how far should the particles move in each step)
   auto sim = std::make_unique<AtSimpleSimulation>(GeoDataPath.Data());
   sim->SetDistanceStep(1); // Units for distance are mm

   // Create the space charge model that we use to deform the tracks
   auto scModel = std::make_shared<AtLineChargeModel>();
   scModel->SetBeamLocation({0, -6, 0},
                            {10, 0, 1000}); // Set the beam location at two points (entrace window and pad plane)
   scModel->SetBeamRadius(5);
   // sim->SetSpaceChargeModel(scModel); // Add the space charge model to the simulation

   // Create and load energy loss models
   std::vector<std::pair<int, int>> ions;
   for (int i = Zmin; i <= Zmax; i++)
      ions.push_back(
         {i, std::round((double)i / Zcn * Acn)}); // Calculate the number of nucleons for each Z and add to the list

   // For all our ions, load the energy loss tables
   for (auto [Z, A] : ions) {
      auto eloss = std::make_shared<AtTools::AtELossTable>();
      std::cout << "Loading table for [Z,A]: " << "[" << Z << "," << A << "]" << std::endl;
      eloss->LoadLiseTable(TString::Format(energyLossDir + "/LISE/%d_%d.txt", Z, A).Data(), A,
                           0); // Note a different function call will be needed if loading SRIM tables
      sim->AddModel(Z, A, eloss);
   }

   // Load the energy loss table for the beam
   std::cout << "Loading beam energy loss tables" << std::endl;
   auto beamloss = std::make_shared<AtTools::AtELossTable>();
   beamloss->LoadLiseTable(
      TString::Format(energyLossDir + "/LISE/%d_%d.txt", fissionSim::beamZ, fissionSim::beamA).Data(),
      fissionSim::beamA, 0);
   sim->AddModel(fissionSim::beamZ, fissionSim::beamA, beamloss);

   /**  At this point, the simulation object is fully constructed and ready to be used. **/
   fissionSim::fSimulation = std::move(sim);
   fissionSim::ions = ions;

   // Create the task that will actually simulate events
   AtMacroTask *simTask = new AtMacroTask();
   simTask->AddInitFunction(fissionSim::Init);
   simTask->AddFunction(fissionSim::Exec);

   // Create the tasks to digitze the event (simulate detector effects and create the raw data)

   fRun->AddTask(simTask);

   fRun->Init();

   timer.Start();
   // fRun->Run(0, 5000);
   fRun->Run(0, 1);
   fissionSim::CleanUp();
   timer.Stop();

   std::cout << std::endl << std::endl;
   std::cout << "Macro finished succesfully." << std::endl << std::endl;
   // -----   Finish   -------------------------------------------------------

   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();
   cout << endl;
   cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
   cout << endl;
   // ------------------------------------------------------------------------
   return;
}

bool reduceFunc(AtRawEvent *evt)
{
   if (evt->GetEventID() % 2 == 0)
      return false;
   return (evt->GetNumPads() > 0) && evt->IsGood();
}
