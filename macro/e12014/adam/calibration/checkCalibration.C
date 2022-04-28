#include "../helper.h"

TH1F *hQfft = new TH1F("qfft", "Q_FFT", 1000, 0, 4096);
TH1F *hQcal = new TH1F("qcal", "Q_cal", 1000, 0, 4096);
TH1F *hQraw = new TH1F("qRaw", "Q_raw", 1000, 0, 4096);
TH1F *hQraw2 = new TH1F("qRaw2", "Q_raw", 1000, 0, 4096);
TString filePath = "/mnt/analysis/e12014/TPC/unpackedCalibration/run_%04d.root";

void fillEvent()
{
   std::cout << "Filling event " << reader->GetCurrentEntry() << std::endl;
   for (auto &hit : eventPtr->GetHitArray()) {
      hQcal->Fill(hit.GetCharge());
   }
   for (auto &hit : eventFilteredPtr->GetHitArray())
      hQfft->Fill(hit.GetCharge());
   for (auto &pad : rawEventPtr->GetPads()) {
      auto max = *std::max_element(pad->GetADC().begin(), pad->GetADC().end());
      hQraw->Fill(max);
   }
}

void checkCalibration(int runNum)
{
   std::cout << " Opening " << TString::Format(filePath, runNum) << std::endl;
   loadRun(TString::Format(filePath, runNum), "AtRawEvent", "AtRawEventCal", "AtEventCal", "AtEventFFTCal");
   hQfft->Reset();
   hQcal->Reset();
   hQraw->Reset();
   std::cout << "Filling histograms" << std::endl;
   while (nextEvent()) {
      fillEvent();
   }
}
