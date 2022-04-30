bool reduceFunc(AtRawEvent *evt)
{
   return (evt->GetNumPads() > 0) && evt->IsGood();
}

// Requires the TPC run number
void unpack_e12014(int numEvents = -1, int runNumber = 174)
{
   // Load the library for unpacking and reconstruction
   gSystem->Load("libAtReconstruction.so");

   TStopwatch timer;
   timer.Start();

   // Set the input/output directories
   TString inputDir = "/mnt/rawdata/e12014_attpc/h5/";
   TString outDir = "./output";

   // Set the in/out files
   TString inputFile = inputDir + TString::Format("/run_%04d.h5", runNumber);
   TString outputFile = outDir + TString::Format("/run_%04d.root", runNumber);

   std::cout << "Unpacking run " << runNumber << " from: " << inputFile << std::endl;
   std::cout << "Saving in: " << outputFile << std::endl;

   // Set the mapping for the TPC
   TString mapFile = "e12014_pad_mapping.xml"; //"Lookup20150611.xml";
   TString parameterFile = "ATTPC.e12014.par";

   // Set directories
   TString dir = gSystem->Getenv("VMCWORKDIR");
   TString mapDir = dir + "/scripts/" + mapFile;
   TString geomDir = dir + "/geometry/";
   gSystem->Setenv("GEOMPATH", geomDir.Data());
   TString digiParFile = dir + "/parameters/" + parameterFile;
   TString geoManFile = dir + "/geometry/ATTPC_v1.1.root";

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

   fAtMapPtr->AddAuxPad({10, 0, 0, 0}, "MCP_US");
   fAtMapPtr->AddAuxPad({10, 0, 0, 34}, "TPC_Mesh");
   fAtMapPtr->AddAuxPad({10, 0, 1, 0}, "MCP_DS");
   fAtMapPtr->AddAuxPad({10, 0, 2, 34}, "IC");

   // Create the unpacker task

   auto unpacker = std::make_unique<AtHDFUnpacker>(fAtMapPtr);
   unpacker->SetInputFileName(inputFile.Data());
   unpacker->SetNumberTimestamps(2);
   unpacker->SetBaseLineSubtraction(true);

   auto unpackTask = new AtUnpackTask(std::move(unpacker));
   unpackTask->SetPersistence(true);
   /*

   AtHDFParserTask *unpackTask = new AtHDFParserTask();
   unpackTask->SetPersistence(true);
   unpackTask->SetMap(fAtMapPtr);
   unpackTask->SetFileName(inputFile.Data());
   unpackTask->SetOldFormat(false);
   unpackTask->SetNumberTimestamps(2);
   unpackTask->SetBaseLineSubtraction(true);

   unpackTask->SetAuxChannel({10, 0, 0, 0}, "MCP_US");
   unpackTask->SetAuxChannel({10, 0, 0, 34}, "TPC_Mesh");
   unpackTask->SetAuxChannel({10, 0, 1, 0}, "MCP_DS");
   unpackTask->SetAuxChannel({10, 0, 2, 34}, "IC");
   */

   // Create data reduction task
   AtDataReductionTask *reduceTask = new AtDataReductionTask();
   reduceTask->SetInputBranch("AtRawEvent");
   reduceTask->SetReductionFunction(&reduceFunc);

   auto threshold = 45;

   AtFilterSubtraction *filter = new AtFilterSubtraction(fAtMapPtr);
   filter->SetThreshold(threshold);
   filter->SetIsGood(false);

   AtFilterTask *filterTask = new AtFilterTask(filter);
   filterTask->SetPersistence(kTRUE);
   filterTask->SetFilterAux(true);

   AtPSASimple2 *psa = new AtPSASimple2();
   psa->SetThreshold(threshold);
   psa->SetMaxFinder();

   AtPSAtask *psaTask = new AtPSAtask(psa);
   psaTask->SetInputBranch("AtRawEventFiltered");
   psaTask->SetOutputBranch("AtEventFiltered");
   psaTask->SetPersistence(kTRUE);

   // Add unpacker to the run
   run->AddTask(unpackTask);
   run->AddTask(reduceTask);
   run->AddTask(filterTask);
   run->AddTask(psaTask);

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
