// Unpacks tpc files from /mnt/rawdata/ to /mnt/analysis/e12014/TPC/unpacked

// Requires the TPC run number
void unpackCalibration(int runNumber)
{
   // Load the library for unpacking and reconstruction
   gSystem->Load("libAtReconstruction.so");

   TStopwatch timer;
   timer.Start();

   // Set the input/output directories
   TString inputDir = "/mnt/rawdata/e12014_attpc/h5";
   TString outDir = "/mnt/analysis/e12014/TPC/unpackedCalibration";

   /**** Should not have to change code between this line and the next star comment ****/

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
   run->SetOutputFile(outputFile);
   run->SetGeomFile(geoManFile);

   // Set the parameter file
   FairRuntimeDb *rtdb = run->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();

   std::cout << "Setting par file: " << digiParFile << std::endl;
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setSecondInput(parIo1);
   rtdb->getContainer("AtDigiPar");

   // Create the detector map
   auto mapping = std::make_shared<AtTpcMap>();
   mapping->ParseXMLMap(mapDir.Data());
   mapping->GeneratePadPlane();

   /**** Should not have to change code between this line and the above star comment ****/
   mapping->AddAuxPad({10, 0, 0, 0}, "MCP_US");
   mapping->AddAuxPad({10, 0, 0, 34}, "TPC_Mesh");
   mapping->AddAuxPad({10, 0, 1, 0}, "MCP_DS");
   mapping->AddAuxPad({10, 0, 2, 34}, "IC");

   // Create the unpacker task
   auto unpacker = std::make_unique<AtHDFUnpacker>(mapping);
   unpacker->SetInputFileName(inputFile.Data());
   unpacker->SetNumberTimestamps(2);
   unpacker->SetBaseLineSubtraction(true);

   auto unpackTask = new AtUnpackTask(std::move(unpacker));
   unpackTask->SetPersistence(true);
   run->AddTask(unpackTask);

   /***** Calibrate raw data ****/
   AtFilterCalibrate *calRawFilter = new AtFilterCalibrate();
   calRawFilter->SetCalibrationFile("output/raw/calibrationFormated.txt");
   AtFilterTask *calRawTask = new AtFilterTask(calRawFilter);
   calRawTask->SetPersistence(true);
   calRawTask->SetFilterAux(false);
   calRawTask->SetInputBranch("AtRawEvent");
   calRawTask->SetOutputBranch("AtRawEventCal");
   run->AddTask(calRawTask);

   /***** Calibrate FFT data ****/
   auto *filterFFT = new AtFilterFFT();
   filterFFT->SetSaveTransform(false);
   filterFFT->AddFreqRange({0, 0.4, 1, 0.95});
   filterFFT->AddFreqRange({1, 0.95, 4, 1});
   filterFFT->AddFreqRange({4, 1, 25, 1});
   filterFFT->AddFreqRange({25, 1, 90, 0.7});
   filterFFT->AddFreqRange({90, 0.7, 257, 0.7});
   AtFilterTask *fftTask = new AtFilterTask(filterFFT);
   fftTask->SetPersistence(true);
   fftTask->SetFilterAux(false);
   fftTask->SetInputBranch("AtRawEvent");
   fftTask->SetOutputBranch("AtRawEventFFT");
   run->AddTask(fftTask);

   auto *filterCalRaw = new AtFilterCalibrate();
   filterCalRaw->SetCalibrationFile("output/filteredBaseline/calibrationFormated.txt");
   auto *calFFTTask = new AtFilterTask(filterCalRaw);
   calFFTTask->SetPersistence(true);
   calFFTTask->SetFilterAux(false);
   calFFTTask->SetInputBranch("AtRawEventFFT");
   calFFTTask->SetOutputBranch("AtRawEventFFTCal");
   run->AddTask(calFFTTask);

   AtPSASimple2 *psa = new AtPSASimple2();
   psa->SetThreshold(45);
   psa->SetMaxFinder();

   /**** PSA For raw data ****/
   AtPSAtask *psaRawTask = new AtPSAtask(psa);
   psaRawTask->SetInputBranch("AtRawEventCal");
   psaRawTask->SetOutputBranch("AtEventCal");
   psaRawTask->SetPersistence(true);
   run->AddTask(psaRawTask);

   /**** PSA For FFT data ****/
   AtPSAtask *psaFFTTask = new AtPSAtask(psa);
   psaFFTTask->SetInputBranch("AtRawEventFFTCal");
   psaFFTTask->SetOutputBranch("AtEventFFTCal");
   psaFFTTask->SetPersistence(true);
   run->AddTask(psaFFTTask);
   // Add unpacker to the run

   run->Init();

   // Get the number of events and unpack the whole run
   auto numEvents = unpackTask->GetNumEvents();

   // numEvents = 1700;//217;
   // numEvents = 2;

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

bool reduceFunc(AtRawEvent *evt)
{
   // return (evt->GetNumPads() > 0);
   return (evt->GetNumPads() > 250) && evt->IsGood();
}
