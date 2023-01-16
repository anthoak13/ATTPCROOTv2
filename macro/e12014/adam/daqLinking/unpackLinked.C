bool reduceFunc(AtRawEvent *evt)
{
   return (evt->GetNumPads() > 0) && evt->IsGood();
}

void unpackLinked(int tpcRunNum = 214, int nsclRunNum = 428)
{
   gSystem->Load("libAtReconstruction.so");

   TStopwatch timer;
   timer.Start();

   // Set the input/output directories
   TString inputDir = "/mnt/rawdata/e12014_attpc/h5";
   TString evtInputDir = "/mnt/analysis/e12014/HiRAEVT/mapped";
   TString outDir = "/mnt/analysis/e12014/TPC/unpackedLinked";
   TString evtOutDir = "/mnt/analysis/e12014/TPC/unpackedLinked";

   // outDir = "./";
   // evtOutDir = "./";
   /**** Should not have to change code between this line and the next star comment ****/

   // Set the in/out files
   TString inputFile = inputDir + TString::Format("/run_%04d.h5", tpcRunNum);
   TString outputFile = outDir + TString::Format("/run_%04d.root", tpcRunNum);
   TString evtOutputFile = evtOutDir + TString::Format("/evtRun_%04d.root", tpcRunNum);
   TString nsclTreeFile = evtInputDir + TString::Format("/mappedRun-%d.root", nsclRunNum);

   std::cout << "Unpacking run " << tpcRunNum << " from: " << inputFile << std::endl;
   std::cout << "Saving in: " << outputFile << std::endl;
   std::cout << "Linking NSCL run: " << nsclRunNum << " from: " << nsclTreeFile << std::endl
             << "Saving in: " << evtOutputFile << std::endl;

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
   AtRunAna *run = new AtRunAna();
   run->SetSink(new FairRootFileSink(outputFile));
   run->SetGeomFile(geoManFile);

   // Set the parameter file
   FairRuntimeDb *rtdb = run->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();

   std::cout << "Setting par file: " << digiParFile << std::endl;
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setSecondInput(parIo1);
   rtdb->getContainer("AtDigiPar");

   // Create the detector map
   auto fAtMapPtr = std::make_shared<AtTpcMap>();
   fAtMapPtr->ParseXMLMap(mapDir.Data());
   fAtMapPtr->GeneratePadPlane();

   /**** Should not have to change code between this line and the above star comment ****/
   auto threshold = 45;

   // Add aux pads to map
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

   // Create the data reduction task
   AtDataReductionTask *reduceTask = new AtDataReductionTask();
   reduceTask->SetInputBranch("AtRawEvent");
   reduceTask->SetReductionFunction(&reduceFunc);

   // Create the ch0 subtraction task
   AtFilterSubtraction *filter = new AtFilterSubtraction(fAtMapPtr);
   filter->SetThreshold(threshold);

   AtFilterTask *filterTask = new AtFilterTask(filter);
   filterTask->SetPersistence(kTRUE);
   filterTask->SetFilterAux(true);

   // Create the trapezoid filter task
   AtTrapezoidFilter *auxFilter = new AtTrapezoidFilter();
   auxFilter->SetM(17.5);
   auxFilter->SetRiseTime(4);
   auxFilter->SetTopTime(10);
   AtAuxFilterTask *auxFilterTask = new AtAuxFilterTask(auxFilter);
   auxFilterTask->SetInputBranchName("AtRawEventFiltered");
   auxFilterTask->AddAuxPad("IC");

   // Create PSA task
   auto psa = std::make_unique<AtPSAMax>();
   psa->SetThreshold(threshold);

   AtPSAtask *psaTask = new AtPSAtask(std::move(psa));
   psaTask->SetInputBranch("AtRawEventFiltered");
   psaTask->SetOutputBranch("AtEventFiltered");
   psaTask->SetPersistence(true);

   // Create DAQ linking task
   AtLinkDAQTask *linker = new AtLinkDAQTask();
   auto success = linker->SetInputTree(nsclTreeFile, "E12014");
   cout << success << endl;
   linker->SetEvtOutputFile(evtOutputFile);
   linker->SetEvtTimestamp("tstamp");
   linker->SetTpcTimestampIndex(1);
   linker->SetSearchMean(1);
   linker->SetSearchRadius(2);
   linker->SetCorruptedSearchRadius(1000);

   // Add unpacker to the run
   run->AddTask(unpackTask);
   run->AddTask(reduceTask);
   run->AddTask(filterTask);
   run->AddTask(auxFilterTask);
   run->AddTask(psaTask);
   run->AddTask(linker);

   std::cout << "***** Starting Init ******" << std::endl;
   run->Init();
   std::cout << "***** Ending Init ******" << std::endl;

   // Get the number of events and unpack the whole run
   auto numEvents = unpackTask->GetNumEvents();

   // numEvents = 5000;//217;
   // numEvents = 200;

   std::cout << "Unpacking " << numEvents << " events. " << std::endl;

   // return;
   run->Run(0, numEvents);

   std::cout << std::endl << std::endl;
   std::cout << "Done unpacking events" << std::endl << std::endl;
   std::cout << "- Output file : " << outputFile << std::endl << std::endl;
   // -----   Finish   -------------------------------------------------------
   timer.Stop();
   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();
   cout << endl << endl;
   cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
   cout << endl;
   // ------------------------------------------------------------------------

   return 0;
}
