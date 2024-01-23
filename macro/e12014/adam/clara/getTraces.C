// Script to pull traces for a pad from a single event
std::vector<int> goodPads = {8593, 8595, 7934, 6689, 9164};
std::vector<int> badPads = {8590, 8373, 2378, 2124, 1319};


void getTraces(int eventNum = 0)
{
goodPads = badPads;
   TChain tpc_tree("cbmsim");
   tpc_tree.Add("Bi200Chi2.root");

   TTreeReader reader(&tpc_tree);
   TTreeReaderValue<TClonesArray> event(reader, "AtRawEventRaw");

   // Open file and skip header
   auto oFileName = "data/traces_bad.dat";
   std::ofstream traceFile(oFileName);
   if (!traceFile.good())
      std::cout << "Failed to open file" << std::endl;

   // Create pad plane and load map
   TString mapFile = "e12014_pad_mapping.xml"; //"Lookup20150611.xml";
   // Set directories
   TString dir = gSystem->Getenv("VMCWORKDIR");
   TString mapDir = dir + "/scripts/" + mapFile;

   auto fAtMapPtr = new AtTpcMap();
   fAtMapPtr->ParseXMLMap(mapDir.Data());
   fAtMapPtr->GeneratePadPlane();
   auto fPadPlane = fAtMapPtr->GetPadPlane();
   double fThreshold = 0;

   // Loop through every event
   reader.SetEntry(0);

   // Get the event
   AtRawEvent *eventPtr = (AtRawEvent *)(event->At(0));

   std::vector<std::array<double, 512>> traces;
   for (auto padID : goodPads) {

      auto pad = eventPtr->GetPad(padID);
      if (!pad) {
         std::cout << "Event had no pad " << padID << std::endl;
         std::cout << "Aborting!" << std::endl;
         return;
      }
      std::cout << "Getting trace for pad " << padID << " at " << fAtMapPtr->GetPadRef(padID) << endl;

      auto &rawTrace = pad->GetRawADC();
      auto &trace = pad->GetADC();
      auto &charge = pad->GetAugment<AtPadArray>("Qreco")->GetArray();
      traces.push_back(charge);
   }

   // Write header
   traceFile << "TB";
   for (auto padID : goodPads) {
      traceFile << ",Pad" << padID;
   }
   traceFile << std::endl;

   // Loop through and print in csv file and fill pad plane
   for (int i = 0; i < 512; ++i) {

      traceFile << i;

      for (auto &trace : traces) {
         traceFile << "," << trace[i];
         
      }
      traceFile << std::endl;
   }

      std::cout << "Done writing file" << std::endl;
   }
