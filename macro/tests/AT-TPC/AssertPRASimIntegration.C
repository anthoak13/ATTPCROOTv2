void AssertPRASimIntegration(TString fileName = "", Long64_t minEntries = 4, Long64_t minPatternEvents = 1,
                             Long64_t minPatternTracks = 1, Long64_t minFittedEvents = 1, Long64_t minFittedTracks = 1)
{
   gSystem->Load("libAtData");
   gSystem->Load("libAtReconstruction");

   TString dir = getenv("VMCWORKDIR");
   if (dir.IsNull()) {
      std::cerr << "VMCWORKDIR is not set. Source build/config.sh first." << std::endl;
      gSystem->Exit(1);
      return;
   }

   if (fileName.IsNull())
      fileName = dir + "/macro/tests/AT-TPC/data/pra-sim-integration/output_reco_ukf.root";

   TFile file(fileName, "READ");
   if (file.IsZombie()) {
      std::cerr << "Failed to open " << fileName << std::endl;
      gSystem->Exit(1);
      return;
   }

   auto *tree = dynamic_cast<TTree *>(file.Get("cbmsim"));
   if (tree == nullptr) {
      std::cerr << "No cbmsim tree in " << fileName << std::endl;
      gSystem->Exit(1);
      return;
   }

   auto *patternArray = new TClonesArray("AtPatternEvent");
   auto *trackingArray = new TClonesArray("AtTrackingEvent");
   tree->SetBranchAddress("AtPatternEvent", &patternArray);
   tree->SetBranchAddress("AtTrackingEvent", &trackingArray);

   Long64_t nEntries = tree->GetEntries();
   Long64_t eventsWithPattern = 0;
   Long64_t totalPatternTracks = 0;
   Long64_t eventsWithFitted = 0;
   Long64_t totalFittedTracks = 0;

   for (Long64_t i = 0; i < nEntries; ++i) {
      tree->GetEntry(i);

      if (patternArray->GetEntriesFast() > 0) {
         auto *patternEvent = static_cast<AtPatternEvent *>(patternArray->At(0));
         auto nTracks = static_cast<Long64_t>(patternEvent->GetTrackCand().size());
         totalPatternTracks += nTracks;
         if (nTracks > 0)
            ++eventsWithPattern;
      }

      if (trackingArray->GetEntriesFast() > 0) {
         auto *trackingEvent = static_cast<AtTrackingEvent *>(trackingArray->At(0));
         auto nTracks = static_cast<Long64_t>(trackingEvent->GetFittedTracks().size());
         totalFittedTracks += nTracks;
         if (nTracks > 0)
            ++eventsWithFitted;
      }
   }

   std::cout << "Simulation-based PRA integration summary for " << fileName << std::endl;
   std::cout << "  entries: " << nEntries << std::endl;
   std::cout << "  events with PRA tracks: " << eventsWithPattern << std::endl;
   std::cout << "  total PRA tracks: " << totalPatternTracks << std::endl;
   std::cout << "  events with fitted tracks: " << eventsWithFitted << std::endl;
   std::cout << "  total fitted tracks: " << totalFittedTracks << std::endl;

   bool ok = true;
   ok &= nEntries >= minEntries;
   ok &= eventsWithPattern >= minPatternEvents;
   ok &= totalPatternTracks >= minPatternTracks;
   ok &= eventsWithFitted >= minFittedEvents;
   ok &= totalFittedTracks >= minFittedTracks;

   if (!ok) {
      std::cerr << "Integration assertions failed." << std::endl;
      gSystem->Exit(1);
      return;
   }
}
