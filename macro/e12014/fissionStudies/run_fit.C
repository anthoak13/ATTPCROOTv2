#include "DigiSimInfo.h"
#include "RunConfig.h"

#include <random>

void run_fit(TString cfgFile = "")
{
   RunConfig cfg(cfgFile);

   int seed = cfg.GetInt("fit.seed", std::random_device{}() & 0x7fffffff);
   delete gRandom;
   gRandom = new TRandom3;
   gRandom->SetSeed(seed);

   TString InputDataFile = cfg.File("input", "digi");
   TString OutputDataFile = cfg.File("output", "fit");
   // LISE or SRIM. Defaults to the simulation's table; set it explicitly only to study a mismatch.
   std::string elossType = cfg.GetStr("fit.eloss", cfg.GetStr("sim.eloss", "LISE"));

   std::cout << "Opening: " << InputDataFile << std::endl;

   TString dir = getenv("VMCWORKDIR");
   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString mapFile = "e12014_pad_map_size.xml";
   TString parFile = "ATTPC.e12014.par";

   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString mapDir = dir + "/scripts/" + mapFile;

   FairRunAna *fRun = new FairRunAna();
   FairRootFileSink *sink = new FairRootFileSink(OutputDataFile);
   FairFileSource *source = new FairFileSource(InputDataFile);
   fRun->SetSource(source);
   fRun->SetSink(sink);
   fRun->SetGeomFile(GeoDataPath);

   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();
   parIo1->open(dir + "/parameters/" + parFile, "in");
   fRun->GetRuntimeDb()->setFirstInput(parIo1);
   fRun->GetRuntimeDb()->getContainer("AtDigiPar");

   auto fMap = std::make_shared<AtTpcMap>();
   fMap->ParseXMLMap(mapDir.Data());
   InhibitZapPads(*fMap); // Digi turned these down, so the charge objective skips them

   E12014::fMap = fMap;

   // Create underlying simulation class
   auto sim = std::make_shared<AtSimpleSimulation>(GeoDataPath.Data());
   sim->SetDistanceStep(cfg.Get("fit.step", 1)); // Step along the tracks (mm)

   // 1: the fit simulates the line-charge model the hits were corrected with, about the same beam line. Its
   // lambda comes from each event (digi.lambda, or 0 without the correction).
   if (cfg.GetInt("fit.spaceCharge", 0))
      sim->SetSpaceChargeModel(MakeSpaceCharge("line", 0, cfg.GetStr("digi.scBeam", "0 -6 10 0")));

   // Create and load energy loss models
   auto ions = IonList(cfg);
   for (auto [Z, A] : ions)
      sim->AddModel(Z, A, LoadELoss(elossType, Z, A));

   auto cluster = std::make_shared<AtClusterizeLine>();
   auto pulse = std::make_shared<AtPulseLine>(fMap);
   pulse->SetSaveCharge(true);
   pulse->SetDoConvolution(false); // The PSA reads the charge, so the traces are left empty
   pulse->SetNumIntegrationPoints(cfg.GetInt("fit.intPoints", 1000)); // Points sampling each line's diffusion
   pulse->SetNumSigmaToIntegrateZ(cfg.GetInt("fit.zSigma", 3));       // Longitudinal diffusion range integrated
   pulse->SetUseFixedSample(cfg.GetInt("fit.fixedPattern", 0));       // 1: fixed diffusion pattern, not random points
   pulse->SetGainFluctuations(cfg.GetInt("fit.gainFluct", 1));        // 0: every deposit gets the mean gain
   auto psa2 = std::make_shared<AtPSADeconvFit>();
   psa2->SetUseSimCharge(true); // Needed by SetDoConvolution(false)
   psa2->SetThreshold(cfg.Get("fit.psaThreshold", 25));

   auto fitter = std::make_shared<MCFitter::AtMCFission>(sim, cluster, pulse);
   fitter->SetPSA(psa2);
   auto [Zcn, Acn] = CompoundNucleus(cfg); // Must match the ion list the tables were loaded for
   fitter->SetCN({Zcn, Acn});
   fitter->SetZRange(ions.front().first, ions.back().first); // Only try Z with a table loaded
   fitter->SetNumIter(cfg.GetInt("fit.iter", 100));
   fitter->SetNumThreads(cfg.GetInt("fit.threads", 4)); // Keep in sync with FIT_THREADS_DEFAULT in fission.py
   fitter->SetNumRounds(cfg.GetInt("fit.rounds", 2));
   fitter->SetNumEventsToSave(cfg.GetInt("fit.saveEvents", 10)); // Simulated events kept per event, best first
   fitter->SetTimeEvent(cfg.GetInt("fit.timeEvent", 0));
   fitter->SetSeed(seed); // With the event number, round and thread, fixes every draw of the fit

   AtMCFitterTask *fitTask = new AtMCFitterTask(fitter);
   fitTask->SetPatternBranchName("AtFissionEvent");
   fitTask->SetSaveEvent(true);
   fitTask->SetSaveRawEvent(true);

   fRun->AddTask(fitTask);

   // Carry the simulation truth into the fit file so plots only need this file
   AtMacroTask *infoTask = new AtMacroTask();
   infoTask->AddInitFunction([] { digiSimInfo::Forward("SimInfo", "SimInfo"); });
   fRun->AddTask(infoTask);

   cfg.CheckUnused({"sim.", "digi."}); // Earlier stages' settings, passed on to describe the input
   fRun->Init();
   // The fit reads only the hits. Skipping the 16 MB/event traces saves main-thread time while the workers wait.
   source->GetInChain()->SetBranchStatus("AtRawEvent*", 0);

   TStopwatch timer;

   timer.Start();
   fRun->Run(); // All events in the input file
   cfg.Finish(OutputDataFile);
   timer.Stop();

   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();
   cout << endl;
   cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
   cout << endl;
}
