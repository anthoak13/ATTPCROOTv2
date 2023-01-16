#include <TFile.h>
#include <TROOT.h>
#include <TString.h>

#include <iostream>
#include <vector>

void WriteCuts(int run = 214)
{
   std::vector<TString> elements = {"Au", "Bi", "Hg", "Pb", "Po", "Pt", "Tl"};
   int minA = 187;
   int maxA = 202;

   TString RunNumber = TString::Format("Run%d", run);
   TString InputDir = TString::Format("/mnt/projects/hira/15507/E12014AnalysisFramework/macros/Run%dYieldCut/", run);
   TString OutputDir = "/mnt/projects/hira/e12014/tpcSharedInfo/";

   // auto cutFile = std::make_unique<TFile>(OutputDir + RunNumber + "AverageYield.root", "RECREATE");
   auto cutFile = std::make_unique<TFile>(OutputDir + RunNumber + "Yield.root", "RECREATE");

   // Loop through every possible isotope and try to load the file
   for (auto elem : elements)
      for (int A = minA; A <= maxA; A++) {
         int error = 0;

         // TCutG *cut = (TCutG *)gROOT->Macro(InputDir + RunNumber + "AverageYieldCut" + elem + A, &error);
         TCutG *cut = (TCutG *)gROOT->Macro(InputDir + RunNumber + "YieldCut" + elem + A, &error);

         if (error == 0) {
            std::cout << elem << A << " " << error << " " << cut << std::endl;
            cut->SetName(elem + A);
            cut->Write();
         }
      }
}
