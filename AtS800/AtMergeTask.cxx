#include "AtMergeTask.h"

#include "AtRawEvent.h"

#include <FairLogger.h>
#include <FairRootManager.h>
#include <FairTask.h>

#include <Math/ParamFunctor.h>
#include <TClonesArray.h>
#include <TF1.h>
#include "TAxis.h"
#include <TH1D.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TFile.h>
#include <TGraph.h>
#include <TObject.h>
#include <TKey.h>
#include <TList.h>
#include <TObject.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include "S800Ana.h"
#include "S800Calc.h"

#include <algorithm>
#include <cmath>
#include <iostream>

constexpr auto cRED = "\033[1;31m";
constexpr auto cYELLOW = "\033[1;33m";
constexpr auto cNORMAL = "\033[0m";
constexpr auto cGREEN = "\033[1;32m";
constexpr auto cBLUE = "\033[1;34m";

ClassImp(AtMergeTask);

AtMergeTask::AtMergeTask() : fLogger(FairLogger::GetLogger()), fS800CalcBr(new S800Calc)
{
   fcutPID1File.clear();
   fcutPID2File.clear();
   fcutPID3File.clear();

   fParameters.clear();
   fTofObjCorr.clear();
   fMTDCObjRange.clear();
   fMTDCXfRange.clear();
}

AtMergeTask::~AtMergeTask()
{
   fS800TsGraph->Delete();
   fS800TsFunc->Delete();
   fS800CalcBr->Delete();
   fRawEventArray->Delete();
   if (fShowTSDiagnostic) {
      diagFile->Close();
      fATTPCTsGraph->Delete();
      fATTPCandS800TsGraph->Delete();
      fdiffTsGraph->Delete();
   }
   delete fS800file;
   delete diagFile;
   delete diagCanvas;
}

void AtMergeTask::SetPersistence(Bool_t value)
{
   fIsPersistence = value;
}
void AtMergeTask::SetS800File(TString file)
{
   fS800File = file;
}
void AtMergeTask::SetGlom(Double_t glom)
{
   fGlom = glom;
}
void AtMergeTask::SetOptiEvtDelta(Int_t EvtDelta)
{
   fEvtDelta = EvtDelta;
}
void AtMergeTask::SetPID1cut(TString file)
{
   fcutPID1File.push_back(file);
}
void AtMergeTask::SetPID2cut(TString file)
{
   fcutPID2File.push_back(file);
}
void AtMergeTask::SetPID3cut(TString file)
{
   fcutPID3File.push_back(file);
}
void AtMergeTask::SetTsDelta(Int_t TsDelta)
{
   fTsDelta = TsDelta;
}
void AtMergeTask::SetParameters(std::vector<Double_t> vec)
{
   fParameters = vec;
}
void AtMergeTask::SetTofObjCorr(std::vector<Double_t> vec)
{
   fTofObjCorr = vec;
}
void AtMergeTask::SetMTDCObjRange(std::vector<Double_t> vec)
{
   fMTDCObjRange = vec;
}
void AtMergeTask::SetMTDCXfRange(std::vector<Double_t> vec)
{
   fMTDCXfRange = vec;
}
void AtMergeTask::SetATTPCClock(Bool_t value)
{
   fUseATTPCClock = value;
}
void AtMergeTask::SetATTPCClockFreq(Double_t value)
{
   fATTPCClockFreq = value;
}

void AtMergeTask::ShowTSDiagnostic(Bool_t value)
{
   fShowTSDiagnostic = value;
}

Int_t AtMergeTask::GetS800TsSize()
{
   return fTsEvtS800Size;
}
Int_t AtMergeTask::GetMergedTsSize()
{
   return fEvtMerged;
}

Bool_t AtMergeTask::isInGlom(Long64_t ts1, Long64_t ts2)
{
   bool is = false;

   if (ts1 > 0 && ts2 > 0 && fabs(ts1 - ts2) < fGlom)
      is = true;
   return is;
}

InitStatus AtMergeTask::Init()
{

   FairRootManager *ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(error) << "Cannot find RootManager!";
      return kERROR;
   }

   fRawEventArray = dynamic_cast<TClonesArray *>(ioMan->GetObject("AtRawEvent"));
   if (fRawEventArray == nullptr) {
      LOG(error) << "Cannot find AtRawEvent array!";
      return kERROR;
   }

   fTsEvtS800Size = 0;
   fEvtMerged = 0;
   fATTPCTs0 = -1;
   Long64_t S800Ts0 = 0; // special for use of internal AT-TPC TS
   fATTPCTsPrev = 0;

   fS800file = new TFile(fS800File); // NOLINT belongs to ROOT
   TTreeReader reader1("caltree", fS800file);
   TTreeReaderValue<Long64_t> ts(reader1, "fts");

   LOG(info) << cBLUE << "Loading S800 timestamps..." << cNORMAL;
   while (reader1.Next()) {
      fS800Ts.push_back((Long64_t)*ts);
      // fS800Ts.push_back((Long64_t) *ts - fTsDelta);//special for run180 e18027
      fS800Evt.push_back((Double_t)fTsEvtS800Size);
      //------- Special for using internal AT-TPC TS (ex: runs 144 to 168), comment if run149 (but keep the second
      // special part uncommented)
      if (fUseATTPCClock) {
         if (fTsEvtS800Size == 0)
            S800Ts0 = fS800Ts.at(0);
         fS800Ts.at(fTsEvtS800Size) -= S800Ts0;
         if (fTsEvtS800Size < 10)
            std::cout << "Ts S800 " << fS800Ts.at(fTsEvtS800Size) << " S800Ts0 " << S800Ts0 << std::endl;
      }
      //--------
      fTsEvtS800Size++;
   }
   ioMan->Register("s800cal", "S800", fS800CalcBr, fIsPersistence);

   vector<Double_t> S800_ts(fS800Ts.begin(), fS800Ts.end());

   // gROOT->SetBatch(kTRUE);//kTRUE not display the plots
   fS800TsGraph =                                            // NOLINT
      new TGraph(fTsEvtS800Size, &S800_ts[0], &fS800Evt[0]); // fTsEvtS800Size instead of 80 (just for the test file)
   // make a function of S800Evt vs S800TS, used then to search the S800 matching TS only among few S800 events, faster
   // than looping on all the events.
   fS800TsFunc = new TF1( // NOLINT
      "fS800TsFunc", [&](double *x, double *) { return fS800TsGraph->Eval(x[0]); }, 0, S800_ts.back(), 0);

   if (fShowTSDiagnostic){
      plotNames.push_back("S800Ts");
      plotNames.push_back("ATTPCTs");
      plotNames.push_back("S800ATTPCTs");
      plotNames.push_back("diffTs");
      plotNames.push_back("distDiffTs");
      diagFile = new TFile("TSDiagnostic.root", "RECREATE");
      diagFile->cd();
      diagCanvas = new TCanvas("diagCanvas", "TS diagnostic", 800, 800);
      diagCanvas->Divide(2,2);
      // TPad *pad1 = (TPad*)c1->cd(1);
      // TPad *pad2 = (TPad*)c1->cd(2);
      diagCanvas->cd(1);
      diagnosticPlotStyle(1);
      fS800TsGraph->Draw("AL");
      // fS800TsFunc->Draw("same");
      fS800TsGraph->Write();
      fS800TsFunc->Write();
      fS800file->cd();
   }

   fS800Ana.SetPID1cut(fcutPID1File);
   fS800Ana.SetPID2cut(fcutPID2File);
   fS800Ana.SetPID3cut(fcutPID3File);
   fS800Ana.SetMTDCXfRange(fMTDCXfRange);
   fS800Ana.SetMTDCObjRange(fMTDCObjRange);
   fS800Ana.SetTofObjCorr(fTofObjCorr);

   return kSUCCESS;
}

void AtMergeTask::Exec(Option_t *opt)
{
   fS800CalcBr->Clear();

   if (fRawEventArray->GetEntriesFast() == 0)
      return;

   auto *rawEvent = dynamic_cast<AtRawEvent *>(fRawEventArray->At(0));
   Long64_t AtTPCTs = -1;
   if (!fUseATTPCClock) {
      if (rawEvent->GetTimestamps().size() == 1) {
         LOG(warning)
            << cYELLOW
            << " AtMergeTask : only TS based on internal AT-TPC clock available, check fUseATTPCClock unpacking macro"
            << cNORMAL << std::endl;
         return;
      } else
         AtTPCTs = rawEvent->GetTimestamp(1);
   }
   if (fUseATTPCClock) {
      AtTPCTs = rawEvent->GetTimestamp(0);
      if (fATTPCTs0 == -1)
         fATTPCTs0 = AtTPCTs; // special run 275, comment this line and uncomment the following line
      // if(fATTPCTs0==-1) fATTPCTs0=902487436452;// special run 275 the first event is not in s800 data so substract
      // the TS of the second evt
      AtTPCTs =
         (AtTPCTs - fATTPCTs0) / fATTPCClockFreq; // 9.9994347//run146 164 9.9994345 run155 9.999435 run160 9.999434
      // std::cout<<"debug "<<fCounter<<" "<<AtTPCTs<<" "<<fATTPCTsPrev<<" "<<AtTPCTs-fATTPCTsPrev<<std::endl;
      if ((AtTPCTs - fATTPCTsPrev) > 1e+8)
         AtTPCTs -= 429521035; // sometimes the ATTPC TS jump (?)
      if ((AtTPCTs - fATTPCTsPrev) < -1e+8)
         AtTPCTs += 429521035;
      fATTPCTsPrev = AtTPCTs;
   }
   int minj = 0, maxj = 0;
   Double_t S800EvtMatch = -1;
   // Define the AtTPC entries range where the matching timestamp
   // Should be, to not loop over all the AtTPC entries.
   // If AT-TPC is triggering on ext. trigger only then there shouldn't be a difference on the number of S800 and AT-TPC event.
   // Allow a small event number delta.
   minj = (int)fS800TsFunc->Eval(AtTPCTs) - fEvtDelta; 
   maxj = (int)fS800TsFunc->Eval(AtTPCTs) + fEvtDelta;

   std::cout << "\nBefore matching, TS AtTPC: " << AtTPCTs << std::endl;

   for (int i = minj; i < maxj; i++) {
      if (i >= 0 && i < fTsEvtS800Size) {
         if (i > 0 && isInGlom(fS800Ts.at(i - 1), fS800Ts.at(i)))
            std::cout << " -- Warning -- Timestamp of consecutive entries from S800 root file within the glom"
                      << std::endl;
         else {
            // Is there a way to check that with the At-TPC "event by event" processing?
            /*if(isInGlom(TsEvtAtTPC.at(i-1),TsEvtAtTPC.at(i)) )
            {
            cout<<" -- Warning -- Timestamp of consecutive entries from AtTPC root file within the glom"<<endl;
          }
          else*/
            if (isInGlom(fS800Ts.at(i) + fTsDelta, AtTPCTs)) { // fTsDelta constant offset likely from the length of the
                                                               // sync signal between S800 and At-TPC
               // if(isInGlom(fS800Ts.at(i),AtTPCTs) ){//special run180
               S800EvtMatch = (int)fS800Evt.at(i);
               std::cout << "Matched S800 event "<<S800EvtMatch<<" with AtTPC event "<<i<<" within AtTPC event window [" 
                         << minj << ", " << maxj<<"]" <<std::endl;
               std::cout << "TS AtTPC: " << AtTPCTs << " TS S800: " << fS800Ts.at(i) 
                         << " diff: " << AtTPCTs - fS800Ts.at(i)<<std::endl;
               fEvtMerged++;
               break;
            } else
               S800EvtMatch = -1;
            // std::cout<<" NOT in glom "<<minj<<" "<<maxj<<" "<<i<<" "<<fS800Ts.at(i)<<" "<<AtTPCTs<<"
            // "<<fS800Ts.at(i)-AtTPCTs<<" "<<S800EvtMatch<<std::endl;
         }
      }
   }

   if (fShowTSDiagnostic) {
      diagFile->cd();
      Long64_t S800TsForPlot = 0;
      if (S800EvtMatch => 0) {
         S800TsForPlot = fS800Ts.at(S800EvtMatch);
      }

      // Plot evt number vs timestamp
      if (fATTPCTsGraph == nullptr) {
         fATTPCTsGraph = new TGraph(1);
         fATTPCTsGraph->SetPoint(fATTPCTsGraph->GetN(), AtTPCTs, (Double_t)rawEvent->GetEventID());
         diagCanvas->cd(1);
         diagnosticPlotStyle(2);
         fATTPCTsGraph->Draw("L same");
      } else {
         fATTPCTsGraph->SetPoint(fATTPCTsGraph->GetN(), AtTPCTs, (Double_t)rawEvent->GetEventID());
         diagCanvas->Update();
         fATTPCTsGraph->Write(plotNames.at(1), kOverwrite);
      }

      // Plot s800 timestamp vs AT-TPC timestamp
      if (fATTPCandS800TsGraph == nullptr) {
         fATTPCandS800TsGraph = new TGraph(1);
         fATTPCandS800TsGraph->SetPoint(fATTPCandS800TsGraph->GetN(), S800TsForPlot, AtTPCTs);
         diagCanvas->cd(2);
         diagnosticPlotStyle(3);
         fATTPCandS800TsGraph->Draw("AL");
      } else {
         fATTPCandS800TsGraph->SetPoint(fATTPCandS800TsGraph->GetN(), S800TsForPlot, AtTPCTs);
         diagCanvas->Update();
         fATTPCandS800TsGraph->Write(plotNames.at(2), kOverwrite);
      }

      // Plot event number vs timestamps difference
      if (fdiffTsGraph == nullptr) {
         fdiffTsGraph = new TGraph(1);
         fdiffTsGraph->SetPoint(fdiffTsGraph->GetN(), (Double_t)rawEvent->GetEventID(), AtTPCTs - S800TsForPlot);
         diagCanvas->cd(3);
         diagnosticPlotStyle(4);
         fdiffTsGraph->Draw("AL");
      } else {
         fdiffTsGraph->SetPoint(fdiffTsGraph->GetN(), (Double_t)rawEvent->GetEventID(), AtTPCTs - S800TsForPlot);
         diagCanvas->Update();
         fdiffTsGraph->Write(plotNames.at(3), kOverwrite);
      }

      // Plot timestamps difference in TH1
      if (fdiffTsHist == nullptr) {
         fdiffTsHist = new TH1D("defaultName", "defaultTitle", 4000, -2000, 2000);
         // std::cout<<"Simon - TS diff "<<AtTPCTs - S800TsForPlot<<std::endl;
         fdiffTsHist->Fill(AtTPCTs - S800TsForPlot);
         diagCanvas->cd(4);
         diagnosticPlotStyle(5);
         fdiffTsHist->Draw();
      } else {
         fdiffTsHist->Fill(AtTPCTs - S800TsForPlot);
         // std::cout<<"Simon - TS diff "<<AtTPCTs - S800TsForPlot<<std::endl;
         diagCanvas->Update();
         fdiffTsHist->Write(plotNames.at(4), kOverwrite);
      }
      fS800file->cd();
   }


   if (S800EvtMatch < 0)
      LOG(warning) << cRED << "NO TS MATCHING !" << cNORMAL;

   if (S800EvtMatch > 0) {
      TTreeReader reader2("caltree", fS800file);
      TTreeReaderValue<S800Calc> *readerValueS800Calc = nullptr;
      readerValueS800Calc = new TTreeReaderValue<S800Calc>(reader2, "s800calc"); // NOLINT

      reader2.SetEntry(S800EvtMatch);

      *fS800CalcBr = (S800Calc)*readerValueS800Calc->Get();

      Bool_t isIn = kFALSE;
      isIn = fS800Ana.isInPID(fS800CalcBr);
      fS800CalcBr->SetIsInCut(isIn);
      rawEvent->SetIsExtGate(isIn);
   }
}


void AtMergeTask::diagnosticPlotStyle(Int_t plotId){
   switch (plotId)
   {
   case 1:
      fS800TsGraph->SetName(plotNames.at(plotId-1));
      fS800TsGraph->SetTitle("S800 & AT-TPC TS");
      fS800TsGraph->GetXaxis()->SetTitle("Timestamp");
      fS800TsGraph->GetYaxis()->SetTitle("Event number");
      fS800TsGraph->GetXaxis()->CenterTitle(true);
      fS800TsGraph->GetYaxis()->CenterTitle(true);
      fS800TsGraph->GetXaxis()->SetMaxDigits(4);
      fS800TsGraph->GetYaxis()->SetMaxDigits(4);
      fS800TsGraph->SetLineColor(kBlue);
      break;
   case 2:
      fATTPCTsGraph->SetName(plotNames.at(plotId-1));
      fATTPCTsGraph->SetTitle("S800 & AT-TPC TS");
      fATTPCTsGraph->GetXaxis()->SetTitle("Timestamp");
      fATTPCTsGraph->GetYaxis()->SetTitle("Event number");
      fATTPCTsGraph->GetXaxis()->CenterTitle(true);
      fATTPCTsGraph->GetYaxis()->CenterTitle(true);
      fATTPCTsGraph->GetXaxis()->SetMaxDigits(4);
      fATTPCTsGraph->GetYaxis()->SetMaxDigits(4);
      fATTPCTsGraph->SetLineColor(kRed);
      break;
   case 3:
      fATTPCandS800TsGraph->SetName(plotNames.at(plotId-1));
      fATTPCandS800TsGraph->SetTitle("Timestamps");
      fATTPCandS800TsGraph->GetXaxis()->SetTitle("S800 TS");
      fATTPCandS800TsGraph->GetYaxis()->SetTitle("AT-TPC TS");
      fATTPCandS800TsGraph->GetXaxis()->CenterTitle(true);
      fATTPCandS800TsGraph->GetYaxis()->CenterTitle(true);
      fATTPCandS800TsGraph->GetXaxis()->SetMaxDigits(4);
      fATTPCandS800TsGraph->GetYaxis()->SetMaxDigits(4);
      break;
   case 4:
      fdiffTsGraph->SetName(plotNames.at(plotId-1));
      fdiffTsGraph->SetTitle("Timestamps diff.");
      fdiffTsGraph->GetXaxis()->SetTitle("Event number");
      fdiffTsGraph->GetYaxis()->SetTitle("AT-TPC - S800 TS");
      fdiffTsGraph->GetXaxis()->CenterTitle(true);
      fdiffTsGraph->GetYaxis()->CenterTitle(true);
      fdiffTsGraph->GetXaxis()->SetMaxDigits(4);
      fdiffTsGraph->GetYaxis()->SetMaxDigits(4);
      break;
   case 5:
      fdiffTsHist->SetName(plotNames.at(plotId-1));
      fdiffTsHist->SetTitle("Timestamps diff.");
      fdiffTsHist->GetXaxis()->SetTitle("AT-TPC - S800 TS");
      fdiffTsHist->GetYaxis()->SetTitle("Events");
      fdiffTsHist->GetXaxis()->CenterTitle(true);
      fdiffTsHist->GetYaxis()->CenterTitle(true);
      fdiffTsHist->GetXaxis()->SetMaxDigits(4);
      fdiffTsHist->GetYaxis()->SetMaxDigits(4);
      break;
   default:
      LOG(WARNING) << cRED << "AtMergeTask::diagnosticPlotStyle - unknown plotId" << cNORMAL;
      break;
   }
}
