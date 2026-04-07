void run_pra_ukf_attpc(TString inputFile = "macro/tests/AT-TPC/data/run_0174.root",
                       TString parFile = "macro/tests/AT-TPC/data/par_attpc.root",
                       TString outputFile = "/tmp/run_0174_pra_ukf.root", int maxEvents = 20)
{
   gSystem->Load("libAtData");
   gSystem->Load("libAtTools");
   gSystem->Load("libAtReconstruction");

   TStopwatch timer;
   timer.Start();

   TString dir = gSystem->Getenv("VMCWORKDIR");
   TString geoFile = dir + "/geometry/ATTPC_v1.1.root";

   auto *run = new FairRunAna();
   run->SetSource(new FairFileSource(inputFile));
   run->SetSink(new FairRootFileSink(outputFile));
   run->SetGeomFile(geoFile);

   auto *rtdb = run->GetRuntimeDb();
   auto *parIo = new FairParRootFileIo();
   parIo->open(parFile);
   rtdb->setFirstInput(parIo);

   auto *praTask = new AtPRAtask();
   praTask->SetInputBranch("AtEventCleaned");
   praTask->SetOutputBranch("AtPatternEvent");
   praTask->SetPersistence(kTRUE);

   constexpr double charge_p = 1.602176634e-19;
   constexpr double mass_p = 938.272;

   auto eloss = std::make_unique<AtTools::AtELossTable>(0);
   eloss->LoadSrimTable(std::string((dir + "/resources/energy_loss/HinH.txt").Data()));

   auto ukfFitter = std::make_unique<EventFit::AtFitterUKF>(charge_p, mass_p, std::move(eloss));
   ukfFitter->SetBField({0, 0, 2.85});
   ukfFitter->SetEField({0, 0, 0});
   ukfFitter->SetUKFParameters(1e-3, 2, 0);
   ukfFitter->SetMeasurementSigma(1.0);
   ukfFitter->SetMomentumSigmaFrac(0.1);
   ukfFitter->SetMinClusters(3);
   ukfFitter->SetEnableEnergyStraggling(true);

   auto *fitterTask = new AtFitterTask(std::move(ukfFitter));
   fitterTask->SetInputBranch("AtPatternEvent");
   fitterTask->SetOutputBranch("AtTrackingEvent");
   fitterTask->SetEventBranch("AtEventCleaned");
   fitterTask->SetPersistence(kTRUE);

   run->AddTask(praTask);
   run->AddTask(fitterTask);

   run->Init();
   run->Run(0, maxEvents);

   timer.Stop();
   timer.Print();
}
