#include <FairParAsciiFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>

#include <TStopwatch.h>
#include <TString.h>

#include <iostream>

void unpack_e20020_full(int numEvents = -1, TString fileName = "run_0160")
{
   // Load the library for unpacking and reconstruction
   gSystem->Load("libAtReconstruction.so");

   TStopwatch timer;
   timer.Start();

   TString parameterFile = "ATTPC.e20020.par";
   TString mappath = "";
   TString filepath = "/mnt/analysis/attpc/";
   TString fileExt = ".h5";
   TString inputFile = filepath + fileName + fileExt;
   TString scriptfile = "e12014_pad_mapping.xml";
   TString dir = getenv("VMCWORKDIR");
   TString mapDir = dir + "/scripts/" + scriptfile;
   TString dataDir = dir + "/macro/data/";
   TString geomDir = dir + "/geometry/";
   gSystem->Setenv("GEOMPATH", geomDir.Data());
   TString outputFile = "output/" + fileName + ".root";
   TString loggerFile = dataDir + "ATTPCLog.log";
   TString digiParFile = dir + "/parameters/" + parameterFile;
   TString geoManFile = dir + "/geometry/ATTPC_He1bar_v2.root";

   // Create a run
   FairRunAna *run = new FairRunAna();
   run->SetSink(new FairRootFileSink(outputFile));
   run->SetGeomFile(geoManFile);

   // Set the parameter file
   FairRuntimeDb *rtdb = run->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();

   std::cout << "Setting par file: " << digiParFile << std::endl;
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setFirstInput(parIo1);
   std::cout << "Getting containers..." << std::endl;
   // We must get the container before initializing a run
   rtdb->getContainer("AtDigiPar");

   // Create the detector map
   auto fAtMapPtr = std::make_shared<AtTpcMap>();
   fAtMapPtr->ParseXMLMap(mapDir.Data());
   fAtMapPtr->GeneratePadPlane();
   // fAtMapPtr->GenerateAtTpc();

   auto unpacker = std::make_unique<AtHDFUnpacker>(fAtMapPtr);
   unpacker->SetInputFileName(inputFile.Data());
   unpacker->SetNumberTimestamps(2);
   unpacker->SetBaseLineSubtraction(true);

   auto unpackTask = new AtUnpackTask(std::move(unpacker));
   unpackTask->SetPersistence(false);
   /*
   AtHDFParserTask *unpackTask = new AtHDFParserTask();
   unpackTask->SetPersistence(false);
   unpackTask->SetMap(fAtMapPtr);
   unpackTask->SetFileName(inputFile.Data());
   unpackTask->SetOldFormat(false);
   unpackTask->SetNumberTimestamps(2);
   unpackTask->SetBaseLineSubtraction(true);
   */
   auto threshold = 20;

   AtPSASimple2 *psa = new AtPSASimple2();
   psa->SetThreshold(threshold);
   psa->SetMaxFinder();

   // Create PSA task
   AtPSAtask *psaTask = new AtPSAtask(psa);
   psaTask->SetPersistence(kTRUE);

   AtPRAtask *praTask = new AtPRAtask();
   praTask->SetPersistence(kTRUE);
   praTask->SetTcluster(5.0);
   praTask->SetMaxNumHits(3000);
   praTask->SetMinNumHits(200);

   run->AddTask(unpackTask);
   run->AddTask(psaTask);
   run->AddTask(praTask);

   std::cout << "***** Starting Init ******" << std::endl;
   run->Init();
   std::cout << "***** Ending Init ******" << std::endl;

   timer.Stop();
   std::cout << "Finished init in " << timer.RealTime() << " s real time." << std::endl;
   std::cout << "Finished init in " << timer.CpuTime() << " s CPU time." << std::endl;

   // Get the number of events and unpack the whole run
   if (numEvents == -1)
      numEvents = unpackTask->GetNumEvents();
   std::cout << "Unpacking " << numEvents << " events. " << std::endl;

   timer.Start(true);
   run->Run(0, numEvents);
   timer.Stop();

   std::cout << std::endl << std::endl;
   std::cout << "Done unpacking events" << std::endl << std::endl;
   std::cout << "- Output file : " << outputFile << std::endl << std::endl;

   std::cout << std::endl << std::endl;
   std::cout << "Finished run in " << timer.RealTime() << " s real time." << std::endl;
   std::cout << "Finished run in " << timer.CpuTime() << " s CPU time." << std::endl;
   std::cout << std::endl;

   return 0;
}
