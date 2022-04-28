/*#include "TString.h"
#include "AtEventDrawTask.h"
#include "AtEventManager.h"

#include "FairLogger.h"
#include "FairParRootFileIo.h"
#include "FairRunAna.h"
*/

void run_eve(int runNum = 193, TString OutputDataFile = "./output/output.reco_display.root")
{

   TString filePath = "/mnt/analysis/e12014/TPC/unpacked/run_%04d.root";
   filePath = "/mnt/analysis/e12014/TPC/unpackedCalibration/run_%04d.root";
   TString InputDataFile = TString::Format(filePath, runNum);
   std::cout << "Opening: " << InputDataFile << std::endl;

   TString dir = getenv("VMCWORKDIR");
   TString geoFile = "ATTPC_v1.1_geomanager.root";

   TString InputDataPath = InputDataFile;
   TString OutputDataPath = OutputDataFile;
   TString GeoDataPath = dir + "/geometry/" + geoFile;

   FairRunAna *fRun = new FairRunAna();
   FairRootFileSink *sink = new FairRootFileSink(OutputDataFile);
   FairFileSource *source = new FairFileSource(InputDataFile);
   fRun->SetSource(source);
   fRun->SetSink(sink);
   fRun->SetGeomFile(GeoDataPath);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParRootFileIo *parIo1 = new FairParRootFileIo();
   // parIo1->open("param.dummy.root");
   rtdb->setFirstInput(parIo1);

   FairRootManager *ioman = FairRootManager::Instance();

   AtEventManager *eveMan = new AtEventManager();
   AtEventDrawTask *eve = new AtEventDrawTask();
   auto fAtMapPtr = std::make_shared<AtTpcMap>();
   TString mapFile = gSystem->Getenv("VMCWORKDIR");
   mapFile += "/scripts/e12014_pad_mapping.xml";
   fAtMapPtr->ParseXMLMap(mapFile.Data());

   eve->Set3DHitStyleBox();
   eve->SetMultiHit(100); // Set the maximum number of multihits in the visualization
   eve->SetMap(fAtMapPtr);
   // eve->SetSaveTextData();
   // eve->SetRawEventBranch("AtRawEventSubtracted");
   eve->SetRawEventBranch("AtRawEventFFTCal");
   eve->SetEventBranch("AtEventFFTCal");

   eveMan->AddTask(eve);
   eveMan->Init();

   std::cout << "Finished init" << std::endl;
   // eveMan->RunEvent(27);
}
