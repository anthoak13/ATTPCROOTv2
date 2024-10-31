// Code to take MC tracks and digitize
// #include "eventCombine.h"
#include "DigiSimInfo.h"
#include "response.h"

bool reduceFunc(AtRawEvent *evt);

void slurm_fit_sim(int runNum, int numThreads, bool saveRawEvent)
{
   auto verbSpec =
      fair::VerbositySpec::Make(fair::VerbositySpec::Info::severity, fair::VerbositySpec::Info::file_line_function);
   fair::Logger::DefineVerbosity("user1", verbSpec);
   fair::Logger::SetVerbosity("user1");

   TString inOutDir = "./data/";
   // TString outputFile = inOutDir + "output_digiLg.root";
   TString outputFile = inOutDir + TString::Format("output_digi%02d.root", runNum);
   outputFile = inOutDir + TString::Format("output_digi%02d.root", runNum);
   TString scriptfile = "e12014_pad_mapping.xml";
   TString paramFile = "ATTPC.e12014.par";

   TString dir = getenv("VMCWORKDIR");

   // TString mcFile = "./data/sim_attpc.root";
   // TString mcFile = inOutDir + "symFissionLg.root";
   TString mcFile = inOutDir + TString::Format("simFission%02d.root", runNum);
   TString sharedInfoDir =
      "/home/faculty/aanthony/fission/data/e12014/tpcSharedInfo/"; // Directory containing the shared information for
                                                                   // the TPC

   // Create the full parameter file paths
   // TString digiParFile = dir + "/parameters/" + paramFile;
   TString digiParFile = paramFile;
   TString mapParFile = dir + "/scripts/" + scriptfile;

   // -----   Timer   --------------------------------------------------------
   TStopwatch timer;

   // ------------------------------------------------------------------------

   // __ Run ____________________________________________
   FairRunAna *fRun = new FairRunAna();
   FairFileSource *source = new FairFileSource(mcFile);
   fRun->SetSource(source);
   fRun->SetOutputFile(outputFile);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setFirstInput(parIo1);

   // Create the detector map to pass to the simulation
   auto mapping = std::make_shared<AtTpcMap>();
   mapping->ParseXMLMap(mapParFile.Data());
   mapping->GeneratePadPlane();

   std::ifstream file(sharedInfoDir + "/e12014_zap.csv");
   if (!file.is_open())
      LOG(error) << "Failed to open smart zap file";

   // Clear out the header
   std::string temp;
   std::getline(file, temp);
   std::getline(file, temp);

   for (auto &row : CSVRange<int>(file)) {
      LOG(debug) << "Inhibiting " << row[4];
      mapping->InhibitPad(row[4], AtMap::InhibitType::kLowGain);
   }

   // __ AT digi tasks___________________________________
   // AtClusterizeTask *clusterizer = new AtClusterizeTask(std::make_shared<AtClusterize>());
   AtClusterizeTask *clusterizer = new AtClusterizeTask(std::make_shared<AtClusterizeLine>());
   clusterizer->SetPersistence(false);

   // AtPulseLineTask *pulse = new AtPulseLineTask();

   // AtPulseTask *pulse = new AtPulseTask(std::make_shared<AtPulse>(mapping));
   auto pulse = std::make_shared<AtPulseLine>(mapping, Response::GetResponse);
   pulse->SetSaveCharge(saveRawEvent);
   pulse->SetLowGain(0.19);
   // pulse->SetLowGain(1);
   Response::scaling = 0.01 * 0.75;
   AtPulseTask *pulseTask = new AtPulseTask(pulse);
   pulseTask->SetPersistence(saveRawEvent);

   /*AtMacroTask *combTask = new AtMacroTask();
   combTask->AddInitFunction(eventComb::init);
   combTask->AddFunction(eventComb::combine);*/

   /**** PSA Task ****/
   AtRawEvent *respAvgEvent;
   TFile *f2 = new TFile(sharedInfoDir + "respAvg.root");
   f2->GetObject("avgResp", respAvgEvent);
   f2->Close();

   auto threshold = 50;
   auto maxThreshold = 1500;

   auto psa = std::make_unique<AtPSADeconvFit>();
   psa->SetResponse(*respAvgEvent);
   psa->SetThreshold(15); // Threshold in charge units
   psa->SetFilterOrder(6);
   psa->SetCutoffFreq(75);
   psa->SetThreshold(10);
   auto psaBeam = std::make_unique<AtPSATBAvg>();
   psaBeam->SetThreshold(45);
   auto psaComp = std::make_unique<AtPSAComposite>(std::move(psaBeam), std::move(psa), 20);
   AtPSAtask *psaTask = new AtPSAtask(std::move(psaComp));
   psaTask->SetInputBranch("AtRawEvent");
   psaTask->SetOutputBranch("AtEvent");
   psaTask->SetPersistence(true);

   /**** Space charge correction ****/
   // auto SCModel = std::make_unique<AtRadialChargeModel>(E12014SC(nsclRunNum));
   auto SCModel = std::make_unique<AtLineChargeModel>();
   // SCModel->SetLambda(E12014SC(nsclRunNum).GetLambda());
   //  SCModel->SetStepSize(0.1);
   // SCModel->SetBeamLocation({-7, 10, 0}, {0, 7, 1000});
   SCModel->SetBeamLocation({0, -6, 0}, {10, 0, 1000});
   auto scTask = new AtSpaceChargeCorrectionTask(std::move(SCModel));
   scTask->SetInputBranch("AtEvent");
   scTask->SetOutputBranch("AtEventCorr");

   /**** Y-pattern fit ****/
   auto method = std::make_unique<SampleConsensus::AtSampleConsensus>(
      SampleConsensus::Estimators::kYRANSAC, AtPatterns::PatternType::kFission, RandomSample::SampleMethod::kY);
   method->SetDistanceThreshold(20);
   method->SetNumIterations(500);
   method->SetMinHitsPattern(150);
   method->SetChargeThreshold(10); //-1 implies no charge-weighted fitting
   method->SetFitPattern(true);
   auto sacTask = new AtSampleConsensusTask(std::move(method));
   sacTask->SetPersistence(false);
   sacTask->SetInputBranch("AtEvent");
   sacTask->SetOutputBranch("AtPatternEvent");

   /******** Create fission task ********/
   AtFissionTask *fissionTask = new AtFissionTask(0);
   fissionTask->SetUncorrectedEventBranch("AtEvent");
   fissionTask->SetPatternBranch("AtPatternEvent");
   fissionTask->SetOutBranch("AtFissionEvent");
   fissionTask->SetPersistance(true);

   AtMacroTask *infoTask = new AtMacroTask();
   infoTask->AddInitFunction(digiSimInfo::Init);

   fRun->AddTask(clusterizer); // This task turns energy into electons in the gas
   fRun->AddTask(pulseTask);   // This task turns electorns in gas to traces on the pad plabe
   fRun->AddTask(psaTask);     // This turns waveforms on pads into hits in space
   fRun->AddTask(sacTask);     // This task finds the fisison Y-pattern
   fRun->AddTask(fissionTask); // This task assembles the fisison events
   fRun->AddTask(infoTask);    //

   /** Begin MC Fit **/
   int Zcn = 83 + 2;  // Number of protons in the compound nucleus
   int Acn = 200 + 4; // Number of nucleons in the compound nucleus
   int Zmin = 26;     // Minimum Z to simulate for the fission fragments
   int Zmax = 59;     // Maximum Z to simulate for hte fission fragments

   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString energyLossDir = sharedInfoDir + "/eLoss/"; // Directory containing the energy loss tables

   E12014::fMap = mapping;
   //  __ Init and run ___________________________________

   // Create underlying simulation class
   auto sim = std::make_shared<AtSimpleSimulation>(GeoDataPath.Data());

   auto scModel = std::make_shared<AtRadialChargeModel>(nullptr);
   scModel->SetStepSize(0.1);
   scModel->SetBeamLocation({0, -6, 0}, {10, 0, 1000});
   // sim->SetSpaceChargeModel(scModel);

   // Create and load energy loss models
   std::vector<std::pair<int, int>> ions;
   for (int i = Zmin; i <= Zmax; i++)
      ions.push_back(
         {i, std::round((double)i / Zcn * Acn)}); // Calculate the number of nucleons for each Z and add to the list

   // For all our ions, load the energy loss tables
   for (auto [Z, A] : ions) {
      auto eloss = std::make_shared<AtTools::AtELossTable>();
      eloss->LoadLiseTable(TString::Format(energyLossDir + "/LISE/%d_%d.txt", Z, A).Data(), A,
                           0); // Note a different function call will be needed if loading SRIM tables
      sim->AddModel(Z, A, eloss);
   }

   auto cluster = std::make_shared<AtClusterizeLine>();
   auto pulseSim = std::make_shared<AtPulseLine>(mapping);
   pulseSim->SetSaveCharge(true);
   auto psa2 = std::make_shared<AtPSADeconvFit>();
   psa2->SetUseSimCharge(true);
   psa2->SetThreshold(25);

   auto fitter = std::make_shared<MCFitter::AtMCFission>(sim, cluster, pulseSim);
   fitter->SetPSA(psa2);
   fitter->SetNumIter(100);
   fitter->SetNumThreads(numThreads);
   fitter->SetNumRounds(2);

   AtMCFitterTask *fitTask = new AtMCFitterTask(fitter);
   fitTask->SetPatternBranchName("AtFissionEvent");

   fRun->AddTask(fitTask);

   Response::Init();
   fRun->Init();

   timer.Start();
   // fRun->Run(0, 20001);
   fRun->Run();
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
}

bool reduceFunc(AtRawEvent *evt)
{
   /*if(evt->GetEventID() % 2 == 0)
      return false;
   return (evt->GetNumPads() > 0) && evt->IsGood();*/
   return true;
}
