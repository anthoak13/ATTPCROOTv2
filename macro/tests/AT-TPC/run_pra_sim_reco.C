void run_pra_sim_reco(Int_t nEvents = 8, TString outputDir = "")
{
   FairLogger::GetLogger()->SetLogScreenLevel("ERROR");

   TString dir = getenv("VMCWORKDIR");
   if (dir.IsNull()) {
      std::cerr << "VMCWORKDIR is not set. Source build/config.sh first." << std::endl;
      gSystem->Exit(1);
      return;
   }

   if (outputDir.IsNull())
      outputDir = dir + "/macro/tests/AT-TPC/data/pra-sim-integration";

   TString simFile = outputDir + "/attpcsim.root";
   TString recoFile = outputDir + "/output_reco_ukf.root";
   TString paramFileName = dir + "/parameters/ATTPC.e20009_sim.par";
   TString mapParFile = dir + "/scripts/Lookup20150611.xml";
   TString inhibitFile = dir + "/macro/tests/AT-TPC/data/inhibit.txt";

   FairRunAna *fRun = new FairRunAna();
   fRun->SetSource(new FairFileSource(simFile));
   fRun->SetOutputFile(recoFile);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();
   parIo1->open(paramFileName.Data(), "in");
   rtdb->setFirstInput(parIo1);

   auto mapping = std::make_shared<AtTpcMap>();
   mapping->ParseXMLMap(mapParFile.Data());
   mapping->GeneratePadPlane();
   mapping->ParseInhibitMap(inhibitFile.Data(), AtMap::InhibitType::kTotal);

   AtClusterizeTask *clusterizer = new AtClusterizeTask();
   clusterizer->SetPersistence(kFALSE);

   AtPulseTask *pulse = new AtPulseTask(std::make_shared<AtPulse>(mapping));
   pulse->SetPersistence(kTRUE);
   pulse->SetSaveMCInfo();

   auto psa = std::make_unique<AtPSAMax>();
   psa->SetThreshold(0);
   AtPSAtask *psaTask = new AtPSAtask(std::move(psa));
   psaTask->SetPersistence(kTRUE);

   AtPRAtask *praTask = new AtPRAtask();
   praTask->SetPersistence(kTRUE);

   double charge = 1.602176634e-19;
   double mass = 938.272;
   auto eloss = std::make_unique<AtTools::AtELossCATIMA>(3.553e-5);
   eloss->SetProjectile(1, 1, 1);
   std::vector<std::tuple<int, int, int>> material;
   material.push_back(std::make_tuple(1, 1, 1));
   eloss->SetMaterial(material);

   auto ukfFitter = std::make_unique<EventFit::AtFitterUKF>(charge, mass, std::move(eloss));
   ukfFitter->SetBField(ROOT::Math::XYZVector(0, 0, 2.85));
   ukfFitter->SetUKFParameters(1e-3, 2.0, 0.0);
   ukfFitter->SetMeasurementSigma(2.0);
   ukfFitter->SetMomentumSigmaFrac(0.3);
   ukfFitter->SetEnableEnergyStraggling(false);
   ukfFitter->SetMinClusters(10);
   ukfFitter->SetZPadPlane(1000.0);

   AtFitterTask *fitterTask = new AtFitterTask(std::move(ukfFitter));
   fitterTask->SetInputBranch("AtPatternEvent");
   fitterTask->SetOutputBranch("AtTrackingEvent");
   fitterTask->SetFitMetadataBranch("AtFitMetadata");
   fitterTask->SetPersistence(kTRUE);

   fRun->AddTask(clusterizer);
   fRun->AddTask(pulse);
   fRun->AddTask(psaTask);
   fRun->AddTask(praTask);
   fRun->AddTask(fitterTask);

   fRun->Init();
   fRun->Run(0, nEvents);

   std::cout << "Simulation-based PRA integration reco output: " << recoFile << std::endl;
}
