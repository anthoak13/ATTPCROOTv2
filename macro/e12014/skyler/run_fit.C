/*#include "TString.h"
#include "AtEventDrawTask.h"
#include "AtEventManager.h"

#include "FairParRootFileIo.h"
#include "FairRunAna.h"
*/
#include "FairLogger.h"

void run_fit(int runNum = 0)
{
   // Set the limits of the potential species to try to match to
   int Zcn = 83 + 2;  // Number of protons in the compound nucleus
   int Acn = 200 + 4; // Number of nucleons in the compound nucleus
   int Zmin = 26;     // Minimum Z to simulate for the fission fragments
   int Zmax = 59;     // Maximum Z to simulate for hte fission fragments

   auto verbSpec =
      fair::VerbositySpec::Make(fair::VerbositySpec::Info::severity, fair::VerbositySpec::Info::file_line_function);
   fair::Logger::DefineVerbosity("user1", verbSpec);
   // fair::Logger::SetVerbosity("user1");
   //  fair::Logger::SetConsoleSeverity("debug");

   TString InputDataFile = TString::Format("./data/output_digi%02d.root", runNum);
   TString OutputDataFile = TString::Format("./data/output_fit%02d.root", runNum);

   std::cout << "Opening: " << InputDataFile << std::endl;

   TString dir = getenv("VMCWORKDIR");
   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString mapFile = "e12014_pad_map_size.xml";
   TString parFile = "ATTPC.e12014.par";

   TString InputDataPath = InputDataFile;
   TString OutputDataPath = OutputDataFile;
   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString mapDir = dir + "/scripts/" + mapFile;
   TString tpcSharedInfoDir =
      "/home/adam/fair_install/tpcSharedInfo/";          // Directory containing the shared information for the TPC
   TString energyLossDir = tpcSharedInfoDir + "/eLoss/"; // Directory containing the energy loss tables

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

   E12014::fMap = fMap;

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
   auto pulse = std::make_shared<AtPulseLine>(fMap);
   pulse->SetSaveCharge(true);
   auto psa2 = std::make_shared<AtPSADeconvFit>();
   psa2->SetUseSimCharge(true);
   psa2->SetThreshold(25);

   auto fitter = std::make_shared<MCFitter::AtMCFission>(sim, cluster, pulse);
   fitter->SetPSA(psa2);
   fitter->SetNumIter(100);
   fitter->SetNumThreads(4);

   AtMCFitterTask *fitTask = new AtMCFitterTask(fitter);
   fitTask->SetPatternBranchName("AtFissionEvent");

   fRun->AddTask(fitTask);

   fRun->Init();

   TStopwatch timer;

   timer.Start();
   fRun->Run(0, 100);
   timer.Stop();

   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();
   cout << endl;
   cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
   cout << endl;
   std::cout << "Finished init" << std::endl;
}
