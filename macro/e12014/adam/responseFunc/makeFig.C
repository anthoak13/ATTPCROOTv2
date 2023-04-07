#ifndef __CLING__

#include "AtEvent.h"
#include "AtPSADeconv.h"
#include "AtPadArray.h"
#include "AtPadFFT.h"
#include "AtPadReference.h"
#include "AtPatternEvent.h"
#include "AtRawEvent.h"
#include "AtTpcMap.h"

#include <TCanvas.h>
#include <TChain.h>
#include <TCutG.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TGraph2DErrors.h>
#include <TH1.h>
#include <TRandom.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TSystem.h>
#include <TTreeReader.h>

#include <fstream>
#include <string>
#include <vector>
#endif

std::unique_ptr<TH1D> h;
std::unique_ptr<TH1D> h2;

double nominalResponseFunction(double reducedTime)
{
   double responce = pow(2.718, -3 * reducedTime) * sin(reducedTime) * pow(reducedTime, 3);
   return responce;
}

TF1 *resp = new TF1("resp", "(exp(-3*x/2.25) * sin(x/2.25) * (x/2.25)^3)/0.044089531", 0, 512);

void makeFig()
{
   TFile *respFile = new TFile("response4.root", "read");
   AtRawEvent *respEvent = (AtRawEvent *)respFile->GetObjectChecked("newResp", "AtRawEvent");
   AtTpcMap *map = new AtTpcMap();
   map->ParseXMLMap(TString(gSystem->Getenv("VMCWORKDIR")) + "/scripts/e12014_pad_map_size.xml");

   std::array<double, 512> avg;
   std::array<double, 512> var;
   avg.fill(0);
   var.fill(0);
   // TH1D *histAverage = new TH1D("hAvg", "Average", 512, 0, 511);

   int numGood = 0;
   AtPadReference ref{0, 0, 0, 0};
   for (int aget = 0; aget < 4; ++aget)
      for (int i = 0; i < 68; ++i) {
         ref.aget = aget;
         ref.ch = i;
         if (map->IsFPNchannel(ref))
            continue;
         // std::cout << "****" << i << "******" << endl;

         numGood++;
         for (int tb = 0; tb < 512; ++tb) {
            int padnum = map->GetPadNum(ref);
            double adcVal = respEvent->GetPad(padnum)->GetADC(tb);
            avg.at(tb) += adcVal;
         }
      }

   // Get average
   for (int i = 0; i < 512; ++i)
      avg[i] /= numGood;

   for (int i = 0; i < 1; ++i) {
      ref.ch = i;
      if (map->IsFPNchannel(ref))
         continue;

      for (int tb = 0; tb < 512; ++tb) {
         int padnum = map->GetPadNum(ref);
         double adcVal = respEvent->GetPad(padnum)->GetADC(tb);
         var[tb] += (adcVal - avg[tb]) * (adcVal - avg[tb]) / (numGood - 1);
      }
   }

   h = std::make_unique<TH1D>("hAvg", "", 512, 0, 511);
   h2 = std::make_unique<TH1D>("hVar", "Variance", 512, 0, 511);
   for (int tb = 0; tb < 512; ++tb) {
      h->SetBinContent(tb + 1, avg[tb]);
      h->SetBinError(tb + 1, sqrt(var[tb]));
      h2->SetBinContent(tb + 1, sqrt(var[tb]));
   }
   h->SetStats(false);
   h->GetXaxis()->SetTitle("Time Bucket");
   h->GetYaxis()->SetTitle("ADC (arb. units)");
}
