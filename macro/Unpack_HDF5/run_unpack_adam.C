#define cRED "\033[1;31m"
#define cYELLOW "\033[1;33m"
#define cNORMAL "\033[0m"
#define cGREEN "\033[1;32m"

#include <iostream>
using namespace std;

void run_unpack_adam(std::string dataFile = "/mnt/projects/hira/anthonya/fission/run_0240.h5", TString parameterFile = "ATTPC.e15250.par", TString mappath="")
{

  // -----   Timer   --------------------------------------------------------
 TStopwatch timer;
 timer.Start();
 // ------------------------------------------------------------------------

  gSystem->Load("libXMLParser.so");

  // -----------------------------------------------------------------
  // Set file names
  TString scriptfile = "Lookup20150611.xml";
  TString dir = getenv("VMCWORKDIR");
  TString scriptdir = dir + "/scripts/"+ scriptfile;
  TString dataDir   = dir + "/macro/data/";
  TString geomDir   = dir + "/geometry/";
  gSystem -> Setenv("GEOMPATH", geomDir.Data());

  TString outputFile  = "output.root";

  //TString mcParFile   = dataDir + name + ".params.root";

  TString loggerFile  = dataDir + "ATTPCLog.log";
  TString digiParFile = dir + "/parameters/" + parameterFile;
  TString geoManFile  = dir + "/geometry/ATTPC_v1.1.root";

  TString inimap   = mappath + "inhib.txt";
  TString lowgmap  = mappath + "lowgain.txt";
  TString xtalkmap = mappath + "beampads_e15503b.txt";

   std::cout << loggerFile << std::endl;

  // -----------------------------------------------------------------
  // Logger
  FairLogger *fLogger = FairLogger::GetLogger();
  fLogger -> SetLogFileName(TString(dir + "/macro/Unpack_HDF5/ATTPCLog.log").Data());
  fLogger -> SetLogToScreen(kTRUE);
  fLogger -> SetLogToFile(kTRUE);
  fLogger -> SetLogVerbosityLevel("LOW");

  FairRunAna* run = new FairRunAna();
  run -> SetOutputFile(outputFile);
  run -> SetGeomFile(geoManFile);

  FairRuntimeDb* rtdb = run->GetRuntimeDb();
  FairParAsciiFileIo* parIo1 = new FairParAsciiFileIo();
  parIo1 -> open(digiParFile.Data(), "in");

  //FairParRootFileIo* parIo2 = new FairParRootFileIo();
  //parIo2 -> open("param.dummy_proto.root");
 // rtdb -> setFirstInput(parIo2);

  rtdb -> setSecondInput(parIo1);

  ATHDFParserTask* HDFParserTask = new ATHDFParserTask();
  HDFParserTask->SetPersistence(kTRUE);
  HDFParserTask->SetATTPCMap(scriptdir.Data());
  HDFParserTask->SetFileName(dataFile);
  HDFParserTask->SetOldFormat(false);
  
  ATPSATask *psaTask = new ATPSATask();
  psaTask -> SetPersistence(kTRUE);
  psaTask -> SetThreshold(10);
  psaTask -> SetPSAMode(2); //NB: 1 is ATTPC - 2 is pATTPC - 3 Filter for ATTPC - 4: Full Time Buckets
  //psaTask -> SetPeakFinder(); //NB: Use either peak finder of maximum finder but not both at the same time
  psaTask -> SetMaxFinder();
  //psaTask -> SetBaseCorrection(kTRUE); //Directly apply the base line correction to the pulse amplitude to correct for the mesh induction. If false the correction is just saved
  //psaTask -> SetTimeCorrection(kFALSE); //Interpolation around the maximum of the signal peak
  
  
  run -> AddTask(HDFParserTask);
  //run -> AddTask(psaTask);

  run -> Init();

  // Only look at first 6 events
  run->Run(0,10);
  //run -> RunOnTBData();


  std::cout << std::endl << std::endl;
  std::cout << "Macro finished succesfully."  << std::endl << std::endl;
  std::cout << "- Output file : " << outputFile << std::endl << std::endl;
  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  cout << endl << endl;
  cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
  cout << endl;
  // ------------------------------------------------------------------------

  gApplication->Terminate();

}

