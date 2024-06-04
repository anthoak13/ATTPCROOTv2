

void readSimulationFile()
{

   FairRunAna *run = new FairRunAna(); // Forcing a dummy run

   TString digiFileName = "/mnt/analysis/e18008/rootAna/giraud/simulation/digi/attpcdigi_d2He_100_run0_Ex10_testUpdates.root";
   TFile *file = new TFile(digiFileName, "READ");
   TTree *tree = (TTree *)file->Get("cbmsim");
   Int_t nEvents = tree->GetEntries();
   std::cout << " Number of events : " << nEvents << std::endl;

   TTreeReader reader("cbmsim", file);
   TTreeReaderValue<TClonesArray> rawEventArray(reader, "AtRawEvent");

   TFile *outfile;
   TString outFileNameHead = "anaTest.root";
   outfile = TFile::Open(outFileNameHead, "recreate");

   //-----
   Int_t ivt = 0, nPads = 0, numPad = 0, validPad = 0;

   //-----

   TTree *anatree = new TTree("anatree", "analysis TTree");
   anatree->Branch("ivt", &ivt);
   anatree->Branch("nPads", &nPads);
   anatree->Branch("numPad", &numPad);
   anatree->Branch("validPad", &validPad);


   /// --------------------- Event loop -------------------------------------------
   for (Int_t i = 0; i < nEvents; i++) {
      std::cout << " Event Number : " << i << "\n";
      reader.Next();
      if (i % 2 != 0)
         continue;

      AtRawEvent *rawEvent = (AtRawEvent *)rawEventArray->At(0);
      if (rawEvent) {

        //Please refer to AtData/AtRawEvent and AtData/AtPad classes for more details about the get functions
        std::vector<std::unique_ptr<AtPad>>& pads = rawEvent->GetPads();
        std::cout<<"pads size: "<<rawEvent->GetNumPads()<<" "<<pads.size()<<std::endl;

        nPads = rawEvent->GetNumPads();

        for (size_t iPad = 0; iPad < nPads; iPad++) {
          numPad = pads.at(iPad)->GetPadNum();
          validPad = pads.at(iPad)->GetValidPad();
          std::cout<<"iPad info: "<<numPad<<" "<<validPad<<std::endl;
        }

        ivt = i;
        anatree->Fill();
      }    // if rawEvent null pointer
   }      // Event loop

   /// --------------------- End event loop ---------------------------------------

   outfile->cd();
   anatree->Write();

   outfile->Close();

} // end main
