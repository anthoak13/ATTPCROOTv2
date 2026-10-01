// Code to take MC tracks and digitize
#include "RunConfig.h"
#include "eventSim.h"

#include <random>

bool reduceFunc(AtRawEvent *evt);

void run_simp_fiss(TString cfgFile = "")
{
   RunConfig cfg(cfgFile);

   delete gRandom;
   gRandom = new TRandom3;
   gRandom->SetSeed(cfg.GetInt("seed", std::random_device{}() & 0x7fffffff));

   //************ Things to change (or set in a study file) ************//
   int num_events = cfg.GetInt("events", 500);
   auto [Zcn, Acn] = CompoundNucleus(cfg); // The nucleus that fissions
   int zToSim = cfg.GetInt("sim.zToSim", 50);
   std::string elossType = cfg.GetStr("sim.eloss", "LISE"); // LISE or SRIM

   fissionSim::beamZ = 83;       // Number of protons in the beam
   fissionSim::beamA = 200;      // Number of nucleons in the beam
   fissionSim::beamM = 199.9332; // Mass of the beam in amu
   fissionSim::cnZ = Zcn;
   fissionSim::cnA = Acn;
   fissionSim::massFrac = (float)zToSim / Zcn; // Mean of the FF mass distribution (as a fraction of Acn).
   // Standard deviation of the FF mass distribution in amu. Set to 0 for single mass splitting.
   fissionSim::massDev = cfg.Get("sim.massDev", 0);
   // Angle of the decay in CoM frame in degrees (0 means sample the distribution)
   fissionSim::decayAngle = cfg.Get("sim.decayAngle", 90) * TMath::DegToRad();
   fissionSim::zCutoff = cfg.Get("sim.zCutoff", 300); // Only simulate vertices from the window to here (mm)

   fissionSim::beamE = cfg.Get("sim.beamE", 2.70013e+03); // Get from LISE, beam energy in MeV
   // Spread in the beam energy from strageling. Either set to 0 or maintain the same ratio with beamE
   fissionSim::beamEsig = cfg.Get("sim.beamEsig", 1.28122e+02);

   //************ End things to change ************//

   TString outputFile = cfg.File("output", "sim");
   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString scriptfile = "e12014_pad_mapping.xml";
   TString paramFile = "ATTPC.e12014.par";

   TString dir = getenv("VMCWORKDIR");
   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString digiParFile = dir + "/parameters/" + paramFile;
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
   auto ions = IonList(cfg);
   for (auto [Z, A] : ions) {
      std::cout << "Loading " << elossType << " table for [Z,A]: [" << Z << "," << A << "]" << std::endl;
      sim->AddModel(Z, A, LoadELoss(elossType, Z, A));
   }

   // Load the energy loss table for the beam
   std::cout << "Loading beam energy loss tables" << std::endl;
   sim->AddModel(fissionSim::beamZ, fissionSim::beamA, LoadELoss(elossType, fissionSim::beamZ, fissionSim::beamA));

   /**  At this point, the simulation object is fully constructed and ready to be used. **/
   fissionSim::fSimulation = std::move(sim);
   fissionSim::ions = ions;
   fissionSim::CheckMassFrac();

   // Create the task that will actually simulate events
   AtMacroTask *simTask = new AtMacroTask();
   simTask->AddInitFunction(fissionSim::Init);
   simTask->AddFunction(fissionSim::Exec);

   // Create the tasks to digitze the event (simulate detector effects and create the raw data)

   fRun->AddTask(simTask);

   cfg.CheckUnused();
   fRun->Init();

   timer.Start();
   fRun->Run(0, num_events);
   fissionSim::CleanUp();
   cfg.Finish(outputFile);
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
