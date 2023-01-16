
void checkICLink(Int_t runNumber = 214)
{
   gSystem->Load("libAtReconstruction.so");

   TString inDir = "/mnt/analysis/e12014/TPC/unpackedLinked";
   TChain evtTr("E12014");
   evtTr.Add(TString::Format(inDir + "/evtRun_%04d.root", runNumber));
   TChain tpcTr("cbmsim");
   tpcTr.Add(TString::Format(inDir + "/run_%04d.root", runNumber));
   evtTr.AddFriend(&tpcTr);

   TTreeReader reader(&evtTr);
   TTreeReaderValue<HTMusicIC> ic(reader, "MUSIC");
   TTreeReaderValue<TClonesArray> eventArray(reader, "AtRawEvent");

   std::vector<double> icRatio;
   std::vector<double> icPeaks;
   std::vector<double> ics;

   while (reader.Next()) {
      AtRawEvent *event = dynamic_cast<AtRawEvent *>(eventArray->At(0));

      // Search through and get the IC pad
      std::vector<Short_t> trace;
      for (auto &[name, pad] : (event->GetAuxPads()))
         if (name == "IC") {
            for (int i = 0; i < 512; ++i)
               trace.push_back(pad.GetRawADC(i));
            break;
         }

      Short_t icPeak = *std::max_element(std::begin(trace), std::end(trace));
      if (icPeak > 0 && ic->GetEnergy(0) > 0) {
         icRatio.push_back(icPeak / ic->GetEnergy(0));
         icPeaks.push_back(icPeak);
         ics.push_back(ic->GetEnergy(0));
      }
   }

   TGraph *gr = new TGraph(icRatio.size());
   TGraph *gr2 = new TGraph(icRatio.size());
   TGraph *gr3 = new TGraph(icRatio.size());
   for (int i = 0; i < icRatio.size(); ++i) {
      gr->SetPoint(i, i, icRatio[i]);
      gr2->SetPoint(i, i, icPeaks[i]);
      gr3->SetPoint(i, i, ics[i]);
   }

   gr->Draw();
}

/*
   hash = HDFParserTask->CalculateHash(10, 0, 2, 34);
   HDFParserTask->SetAuxChannel(hash, "IC");
*/
