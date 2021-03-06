void run_eve(int runNum = 206, TString  OutputDataFile = "output.reco_display.root")
{
  TString  InputDataFile = TString::Format("/mnt/analysis/e12014/TPC/unpacked/run_%04d.root", runNum);
  
  FairLogger *fLogger = FairLogger::GetLogger();
  fLogger -> SetLogToScreen(kTRUE);
  fLogger->SetLogVerbosityLevel("MEDIUM");
  TString dir = getenv("VMCWORKDIR");
  TString geoFile = "ATTPC_v1.1_geomanager.root";

  
  TString InputDataPath = InputDataFile;
  TString OutputDataPath = OutputDataFile;
  TString GeoDataPath = dir + "/geometry/" + geoFile;

  FairRunAna *fRun= new FairRunAna();
  fRun -> SetInputFile(InputDataPath);
  fRun -> SetOutputFile(OutputDataPath);
  fRun -> SetGeomFile(GeoDataPath);

  FairRuntimeDb* rtdb = fRun->GetRuntimeDb();
  FairParRootFileIo* parIo1 = new FairParRootFileIo();
  //parIo1->open("param.dummy.root");
  rtdb->setFirstInput(parIo1);

  FairRootManager* ioman = FairRootManager::Instance();

  AtEventManager *eveMan = new AtEventManager();
  AtEventDrawTask* eve = new AtEventDrawTask();
  eve->Set3DHitStyleBox();
  eve->SetMultiHit(100); //Set the maximum number of multihits in the visualization
  eve->SetSaveTextData();
  //eve->UnpackHoughSpace();

  eveMan->AddTask(eve);
  eveMan->Init();

  //eveMan->RunEvent(27);
}
