//#include "/mnt/user/anthonya/attpcroot/macro/e12014/adam/helper.h"
#include <TCanvas.h>
#include <TGraph.h>

#include <map>
#include <set>

#include "../helper.h"

#ifndef __CLING__
#include "../build/include/AtPatternY.h"
#include "../build/include/AtRadialChargeModel.h"
#endif

TCanvas *canv = new TCanvas("canv", "Test", 600, 600);
TH1F *hAdcSum = new TH1F("adcsum", "ADC Sum", 512, 0, 511);
TH1F *hAdcSumFull = new TH1F("adcsumfull", "ADC Sum Full", 512, 0, 511);
TGraph *grVertex = new TGraph();
TGraph *grVertexFull = new TGraph();

TH1F *hVertex1 = new TH1F("vertex1", "Vertex Y", 100, 0, 1000);
TH1F *hVertex2 = new TH1F("vertex2", "Vertex ADC", 100, 0, 1000);
TH1F *hVertexDiff = new TH1F("vertexdiff", "Vertex Diff", 25, -100, 100);
TH1F *hVertexDiffSC = new TH1F("vertexdiffsc", "Vertex Diff SC", 25, -100, 100);
TH2F *hVertSC = new TH2F("vertCorr", "ADC vs SC Corrected", 100, 400, 1000, 100, 400, 1000);
TH2F *hVert = new TH2F("vertCorr2", "ADC vs Not Corrected", 100, 400, 1000, 100, 400, 1000);

void vertex(bool withFit = false)
{
   loadRun("/mnt/analysis/e12014/TPC/reduced/run_0210.root", "AtRawEventFiltered", "AtRawEventFiltered",
           "AtEventFiltered", withFit ? "AtPatternEvent" : "AtPatternEventNoFit");
   canv->Divide(1, 2);
}

std::pair<int, double> vertexFromHist(TH1F *hist, int tMin = 0, int tMax = 500)
{
   double zk = 1000;
   double entTB = 455;
   double TBTime = 320;
   double driftVelocity = 0.82;

   // Find the vertex from the beam adc
   double maxDiff = 0;
   int vertexTB = 0;

   for (int i = tMin + 2; i < tMax; i++) {
      double diff = hist->GetBinContent(i + 3) - hist->GetBinContent(i - 1);
      if (diff > maxDiff) {
         maxDiff = diff;
         vertexTB = i;
      }
   }

   return {vertexTB, zk - (entTB - vertexTB) * TBTime * driftVelocity / 100.};
}

int getBeamPads(int eventNum)
{
   loadEvent(eventNum);

   auto &tracks = patternEventPtr->GetTrackCand();
   if (tracks.size() <= 0) {
      std::cout << "No tracks in event " << eventNum << std::endl;
      return 0;
   }

   std::map<int, double> beamAndTBMin;
   std::set<int> beamPadNums;
   int minTB = 512;
   for (const auto &hit : tracks[0].GetHitArray()) {
      if (hit.GetPosition().Rho() > 20)
         continue;

      beamPadNums.insert(hit.GetPadNum());
      if (hit.GetTimeStamp() < minTB)
         minTB = hit.GetTimeStamp();
   }

   std::cout << "Keeping pads: ";
   for (auto padNum : beamPadNums)
      std::cout << padNum << " ";
   std::cout << std::endl;
   std::cout << "Min TB is: " << minTB << std::endl;

   hAdcSum->Reset();
   hAdcSumFull->Reset();
   for (int i = 0; i < 512; ++i) {
      for (auto pad : beamPadNums) {
         if (i >= minTB && i < 450)
            hAdcSum->Fill(i, rawEventPtr->GetPad(pad)->GetRawADC(i));
         hAdcSumFull->Fill(i, rawEventPtr->GetPad(pad)->GetRawADC(i));
      }
   }
   return minTB;
}

void drawVertex()
{

   hVertex1->Reset();
   hVertex2->Reset();
   hVertexDiff->Reset();
   hVert->Reset();

   std::ifstream iFile("./goodEvents.csv");
   while (!iFile.eof()) {
      int event = 0;
      iFile >> event;

      auto minTB = getBeamPads(event);
      auto vertex = vertexFromHist(hAdcSum, minTB, 450);
      auto pattern = dynamic_cast<const AtPatterns::AtPatternY *>(patternEventPtr->GetTrackCand()[0].GetPattern());
      double vertPattern = pattern->GetVertex().Z();

      hVertex1->Fill(vertex.second);
      hVertex2->Fill(vertPattern);
      hVertexDiff->Fill(vertPattern - vertex.second);
      hVert->Fill(vertPattern, vertex.second);
   }

   canv->cd(1);
   hVertexDiff->Draw("hist");
   // hVertex1->Draw("hist");
   canv->cd(2);
   hVert->SetStats(0);
   hVert->GetZaxis()->SetRangeUser(0, 2);
   hVert->Draw("col");
   // hVertex2->Draw("hist");
}
double EField(double rho, double z)
{
   double lambda = 5.28e-8;               // SI
   constexpr double rBeam = 2;            // in cm
   constexpr double eps = 8.85418782E-12; // SI
   constexpr double pi = 3.14159265358979;
   constexpr double eps2pi = 2 * pi * eps;
   rho /= 100.; // Convert units from cm to m

   double field;
   if (rho > 2)
      field = lambda / eps2pi / rho; // v/m
   else
      field = lambda / eps2pi / rBeam / rBeam * rho; // v/m
   return field / 100.;                              // V/cm
}

void drawVertexSpaceCharge()
{

   hVertex1->Reset();
   hVertex2->Reset();
   hVertexDiff->Reset();
   hVert->Reset();

   std::ifstream iFile("./goodEvents.csv");
   auto model = new AtRadialChargeModel(&EField);
   model->SetStepSize(0.1);

   while (!iFile.eof()) {
      int event = 0;
      iFile >> event;

      auto minTB = getBeamPads(event);
      auto vertex = vertexFromHist(hAdcSum, minTB, 450);
      auto pattern = dynamic_cast<const AtPatterns::AtPatternY *>(patternEventPtr->GetTrackCand()[0].GetPattern());

      auto patternSC = pattern->Clone();
      std::vector<AtHit> correctedHits;
      std::cout << "Correcting space charge" << std::endl;
      for (const auto &hit : patternEventPtr->GetTrackCand()[0].GetHitArray()) {
         auto correctedHit = hit;
         correctedHit.SetPosition(model->CorrectSpaceCharge(hit.GetPosition()));
         correctedHits.push_back(correctedHit);
      }
      patternSC->FitPattern(correctedHits, 20);

      double vertPattern = dynamic_cast<AtPatterns::AtPatternY *>(patternSC.get())->GetVertex().Z();
      double vertexPre = pattern->GetVertex().Z();

      hVertex1->Fill(vertex.second);
      hVertex2->Fill(vertPattern);
      hVertexDiff->Fill(vertPattern - vertex.second);
      hVertexDiffSC->Fill(vertPattern - vertexPre);
      hVertSC->Fill(vertPattern, vertex.second);
      hVert->Fill(vertexPre, vertex.second);
   }

   canv->cd(1);
   hVertexDiff->Draw("hist");
   // hVertex1->Draw("hist");
   canv->cd(2);
   hVert->SetStats(0);
   hVert->GetZaxis()->SetRangeUser(0, 2);
   hVert->Draw("col");
   // hVertex2->Draw("hist");
}

void drawBeamPads(int i)
{
   auto minTB = getBeamPads(i);
   auto vertex = vertexFromHist(hAdcSum, minTB, 450);
   auto vertexFull = vertexFromHist(hAdcSumFull);

   canv->cd(1);
   hAdcSum->SetStats(0);
   hAdcSum->Draw("hist");
   grVertex->Clear();
   grVertex->Set(2);
   grVertex->SetPoint(0, vertex.first, -4000);
   grVertex->SetPoint(1, vertex.first, 4000);
   grVertex->SetLineColor(4);
   grVertex->Draw("SAMEL");

   canv->cd(2);
   hAdcSumFull->SetStats(0);
   hAdcSumFull->Draw("hist");
   grVertexFull->Clear();
   grVertexFull->Set(2);
   grVertexFull->SetPoint(0, vertexFull.first, -4000);
   grVertexFull->SetPoint(1, vertexFull.first, 4000);
   grVertexFull->SetLineColor(4);
   grVertexFull->Draw("SAMEL");

   canv->SaveAs(Form("pics/Event_%d.png", i));
}
