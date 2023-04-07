#ifndef __CLING__

#include "AtArrayAug.h"
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
#include <THStack.h>
#include <TRandom.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TSystem.h>
#include <TTreeReader.h>

#include <fstream>
#include <string>
#include <vector>
#endif

std::unique_ptr<TH1D> hLow;
std::unique_ptr<TH1D> hQ;
std::unique_ptr<TH1D> hHigh;
THStack *stack;

TCanvas c = new TCanvas("c1", "Stacked Histogram");
void create_stack();

void scaleHist(TH1 *hist, double scalar, double offset)
{
   for (int i = 1; i <= hist->GetNbinsX(); i++) {
      double bin_content = hist->GetBinContent(i);
      hist->SetBinContent(i, bin_content * scalar + offset);
   }
}

void FilterArtifact()
{
   TChain runChain("cbmsim");
   runChain.Add("./data/output_digi20.root");
   TTreeReader *reader = new TTreeReader(&runChain);
   TTreeReaderValue<TClonesArray> *rawEventReader = new TTreeReaderValue<TClonesArray>(*reader, "AtRawEvent");
   reader->SetEntry(0);
   AtRawEvent *rawEvent = dynamic_cast<AtRawEvent *>(rawEventReader->Get()->At(0));

   auto pad = rawEvent->GetPad(4733);
   hLow = pad->GetADCHistrogram();
   hLow = pad->GetAugment<AtPadArray>("Qreco")->GetHist("Qreco");
   hQ = pad->GetAugment<AtPadArray>("Q")->GetHist("Q");

   double scale = hLow->GetMaximum() / hQ->GetMaximum();
   scaleHist(hQ.get(), scale, 0);
   scaleHist(hLow.get(), 1, 4);

   TChain runChain2("cbmsim");
   runChain2.Add("./data/output_digi120.root");
   TTreeReader *reader2 = new TTreeReader(&runChain2);
   TTreeReaderValue<TClonesArray> *rawEventReader2 = new TTreeReaderValue<TClonesArray>(*reader2, "AtRawEvent");
   reader2->SetEntry(0);
   AtRawEvent *rawEvent2 = dynamic_cast<AtRawEvent *>(rawEventReader2->Get()->At(0));

   pad = rawEvent2->GetPad(4733);
   hHigh = pad->GetAugment<AtPadArray>("Qreco")->GetHist("Qreco");

   scale = hLow->GetMaximum() / hHigh->GetMaximum();
   scaleHist(hHigh.get(), scale, 4);

   create_stack();
}

void create_stack()
{
   TLegend *leg = new TLegend(0.45, 0.7, 0.9, 0.9);
   leg->AddEntry(hLow.get(), "Reco Charge (w_{c} = 20)", "l");
   leg->AddEntry(hQ.get(), "True Charge", "l");
   leg->AddEntry(hHigh.get(), "Reco Charge (w_{c} = 120)", "l");

   hLow->GetXaxis()->SetTitle("Time Bucket");
   hLow->GetYaxis()->SetTitle("Charge (arb. units)");
   hLow->SetLineColor(kRed);
   hLow->SetStats(false);
   hLow->SetTitle("");
   hHigh->SetLineColor(kBlue);
   hQ->SetLineColor(kBlack);

   hLow->Draw();
   hQ->Draw("same");
   hHigh->Draw("same");
   leg->Draw("same");

   /*   if (stack)
         delete stack;
      stack = new THStack("stack", "Stacked Histograms");
      stack->Add(h20.get());
      stack->Add(h50.get());
      stack->Add(h100.get());

      // draw the stacked histogram
      TCanvas *c1 = new TCanvas("c1", "Stacked Histogram");
      stack->Draw("hist");
      stack->GetXaxis()->SetTitle("Time Bucket");
      stack->GetYaxis()->SetTitle("Charge (arb. units)");
   */
}
