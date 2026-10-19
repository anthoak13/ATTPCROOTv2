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

   auto scModel = std::make_shared<AtRadialChargeModel>(nullptr);
   scModel->SetStepSize(0.1);
   scModel->SetBeamLocation({0, -6, 0}, {10, 0, 1000});
   // sim->SetSpaceChargeModel(scModel);

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
   pulse->SetUseFixedSample(cfg.GetInt("fit.fixedPattern", 0));      // 1: fixed diffusion pattern, not random points
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

   // 1: give the fit the true fragment directions and vertex (from the simulation the digitization read) in place
   // of the Y-pattern's, to separate the pattern's errors from the rest of the fit. Diagnostics only.
   if (cfg.GetInt("fit.truePattern", 0)) {
      auto simFile = RunConfig::FromOutput(InputDataFile).GetStr("input", "");
      if (simFile.empty())
         throw std::runtime_error("fit.truePattern: no simulation file recorded in " + std::string(InputDataFile));
      auto *truthTask = new AtMacroTask();
      static std::unique_ptr<TFile> truthFile;
      static TTree *truthTree = nullptr;
      static TClonesArray *truthPoints = nullptr, *fissionArray = nullptr;
      static TH1D *truthInfo = nullptr;
      static long truthEntry = 0;
      static std::shared_ptr<MCFitter::AtMCFission> truthFitter;
      truthFitter = fitter;
      truthFile.reset(TFile::Open(simFile.c_str()));
      truthTree = truthFile->Get<TTree>("cbmsim");
      truthTree->SetBranchAddress("AtTpcPoint", &truthPoints);
      truthTree->SetBranchAddress("Info", &truthInfo);
      truthTask->AddInitFunction(
         [] { fissionArray = dynamic_cast<TClonesArray *>(FairRootManager::Instance()->GetObject("AtFissionEvent")); });
      truthTask->AddFunction([] {
         truthTree->GetEntry(truthEntry++);
         auto *event = fissionArray->GetEntriesFast() ? dynamic_cast<AtFissionEvent *>(fissionArray->At(0)) : nullptr;
         auto *pat = event ? event->GetYPattern() : nullptr;
         if (!pat)
            return;
         ROOT::Math::XYZVector dir[2];
         bool found[2] = {false, false};
         for (int i = 0; i < truthPoints->GetEntriesFast(); ++i) {
            auto *point = dynamic_cast<AtMCPoint *>(truthPoints->At(i));
            int frag = point->GetTrackID() - 2; // Track 1 is the beam
            if (frag >= 0 && frag < 2 && !found[frag]) {
               dir[frag] = {point->GetPx(), point->GetPy(), point->GetPz()};
               found[frag] = true;
            }
         }
         if (!found[0] || !found[1])
            return;
         // Pattern fragment 0 is the true fragment whose azimuth is closer
         auto gap = [](double a, double b) { return std::abs(TVector2::Phi_mpi_pi(a - b)); };
         double ph0 = pat->GetFragmentDirection(0).Phi(), ph1 = pat->GetFragmentDirection(1).Phi();
         bool direct = gap(ph0, dir[0].Phi()) + gap(ph1, dir[1].Phi()) < gap(ph0, dir[1].Phi()) + gap(ph1, dir[0].Phi());
         auto &d0 = direct ? dir[0] : dir[1];
         auto &d1 = direct ? dir[1] : dir[0];
         truthFitter->SetNextEventMeans({{"th0", d0.Theta()},
                                         {"ph0", d0.Phi()},
                                         {"th1", d1.Theta()},
                                         {"ph1", d1.Phi()},
                                         {"vX", truthInfo->GetBinContent(1)},
                                         {"vY", truthInfo->GetBinContent(2)},
                                         {"vZ", truthInfo->GetBinContent(3)}});
      });
      fRun->AddTask(truthTask);
   }

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
