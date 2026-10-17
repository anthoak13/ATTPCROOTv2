#include <TClonesArray.h>
#include <TFile.h>
#include <TH1D.h>
#include <TNamed.h>
#include <TString.h>
#include <TTree.h>

#include "RunConfig.h"

#include <fstream>
#include <iostream>
#include <memory>

/**
 * Write the results file of a fit (fission.py --results): what plot_fit.C needs, without the raw events,
 * so it is small enough to copy off a cluster.
 *
 * The output has a cbmsim tree with the fit file's branch names, one entry per event: AtMCResult and
 * SimInfo, SimEvent holding only the best fit (entry 0, since the results are sorted by objective), and
 * the digi file's AtEvent and AtFissionEvent. It holds the fit's RunConfig record, and the fit's .cfg is
 * copied next to it (<out>.cfg) last, so plot_fit.C and fission.py treat it like a finished fit file.
 *
 *    root -l -b -q 'slim_fit.C("data/smoke-base.fit.root", "data/smoke-base.fit.results.root")'
 */
void slim_fit(TString fitFile, TString outFile)
{
   auto fail = [](TString message) {
      std::cerr << "slim_fit: " << message << std::endl;
      gSystem->Exit(1);
   };
   if (!fitFile.EndsWith(".root") || !outFile.EndsWith(".root"))
      fail("file names must end in .root");

   std::unique_ptr<TFile> fit(TFile::Open(fitFile));
   auto record = fit ? fit->Get<TNamed>("RunConfig") : nullptr;
   if (!record)
      fail(fitFile + " is not a finished fit file (no RunConfig record)");
   RunConfig cfg;
   cfg.Parse(record->GetTitle());
   TString digiFile = cfg.GetStr("input", "");
   std::unique_ptr<TFile> digi(TFile::Open(digiFile));
   if (!digi || digi->IsZombie())
      fail("cannot open the fit's input " + digiFile);

   // The fit processes every digi event, so the trees align entry by entry
   auto fitTree = fit->Get<TTree>("cbmsim");
   auto digiTree = digi->Get<TTree>("cbmsim");
   if (!fitTree || !digiTree)
      fail("no cbmsim tree in " + TString(fitTree ? digiFile : fitFile));
   if (fitTree->GetEntries() != digiTree->GetEntries())
      fail(TString::Format("%s has %lld events but its input %s has %lld", fitFile.Data(), fitTree->GetEntries(),
                           digiFile.Data(), digiTree->GetEntries()));

   TClonesArray *results = nullptr, *simEvents = nullptr, *events = nullptr, *fissionEvents = nullptr;
   TH1D *simInfo = nullptr;
   fitTree->SetBranchStatus("*", false);
   for (auto name : {"AtMCResult", "SimEvent", "SimInfo"})
      fitTree->SetBranchStatus(TString(name) + "*", true);
   digiTree->SetBranchStatus("*", false);
   for (auto name : {"AtEvent", "AtFissionEvent"})
      digiTree->SetBranchStatus(TString(name) + "*", true);
   if (fitTree->SetBranchAddress("AtMCResult", &results) < 0 || fitTree->SetBranchAddress("SimEvent", &simEvents) < 0 ||
       fitTree->SetBranchAddress("SimInfo", &simInfo) < 0 || digiTree->SetBranchAddress("AtEvent", &events) < 0 ||
       digiTree->SetBranchAddress("AtFissionEvent", &fissionEvents) < 0)
      fail("missing branches in " + fitFile + " or " + digiFile);

   TFile out(outFile, "RECREATE");
   if (out.IsZombie())
      fail("cannot write " + outFile);
   auto tree = new TTree("cbmsim", "fit results"); // Owned by out
   TClonesArray best("AtEvent");
   auto bestPtr = &best;
   if (fitTree->GetEntries() > 0) {
      fitTree->GetEntry(0);
      digiTree->GetEntry(0);
   }
   tree->Branch("AtMCResult", &results);
   tree->Branch("SimEvent", &bestPtr);
   tree->Branch("SimInfo", &simInfo);
   tree->Branch("AtEvent", &events);
   tree->Branch("AtFissionEvent", &fissionEvents);

   for (Long64_t i = 0; i < fitTree->GetEntries(); ++i) {
      fitTree->GetEntry(i);
      digiTree->GetEntry(i);
      best.Delete();
      if (simEvents->GetEntriesFast() > 0)
         new (best[0]) AtEvent(*static_cast<AtEvent *>(simEvents->At(0)));
      tree->Fill();
   }
   out.cd();
   tree->Write();
   record->Write("RunConfig");
   auto nEvents = tree->GetEntries();
   out.Close();

   // Same text as the fit's .cfg (its completion marker), written last so it marks this file finished
   TString fitCfg = fitFile(0, fitFile.Length() - 5) + ".cfg";
   TString outCfg = outFile(0, outFile.Length() - 5) + ".cfg";
   std::ifstream in(fitCfg.Data());
   if (!in)
      fail("cannot read " + fitCfg);
   std::ofstream(outCfg.Data()) << in.rdbuf();
   std::cout << "Wrote " << outFile << " (" << nEvents << " events)" << std::endl;
}
