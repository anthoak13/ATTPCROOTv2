#include <TH1F.h>
#include <TString.h>

//#include <AtTpcMap.h>

#include <fstream>
#include <map>

#include "/mnt/simulations/attpcroot/adam/ATTPCROOTv2/AtTools/AtCSVReader.h"

void calibrationComparison()
{
   std::ifstream rawFile(
      "/mnt/simulations/attpcroot/adam/ATTPCROOTv2/macro/e12014/adam/unpack/output/raw/calibration.csv");
   std::ifstream fftFile(
      "/mnt/simulations/attpcroot/adam/ATTPCROOTv2/macro/e12014/adam/unpack/output/filteredBaseline/calibration.csv");
   if (!rawFile) {
      std::cout << "Failed to open rawFile" << std::endl;
      return;
   }
   if (!fftFile) {
      std::cout << "Failed to open fftFile" << std::endl;
      return;
   }
   auto fMap = std::make_shared<AtTpcMap>();
   TString mapFile = gSystem->Getenv("VMCWORKDIR");
   mapFile += "/scripts/e12014_pad_mapping.xml";
   fMap->ParseXMLMap(mapFile.Data());

   /* Get Chi2 o
   auto chiRaw = new TH1F("chi2Raw", "Chi2/DoF", 100, 0, 1);
   auto chiFFT = new TH1F("chi2FFT", "Chi2/DoF", 100, 0, 1);
   for (auto &row : CSVRange<double>(rawFile)) {
      auto padRef = fMap->GetPadRef(row[0]);
      if (padRef.cobo == 2 && padRef.asad == 3 && (padRef.aget == 0 || padRef.aget == 3))
         continue;
      chiRaw->Fill(row[1]);
   }
   for (auto &row : CSVRange<double>(fftFile)) {
      auto padRef = fMap->GetPadRef(row[0]);
      if (padRef.cobo == 2 && padRef.asad == 3 && (padRef.aget == 0 || padRef.aget == 3))
         continue;

      chiFFT->Fill(row[1]);
   }
   */
   std::map<int, double> rawGains;
   std::map<int, double> fftGains;
   for (auto &row : CSVRange<double>(rawFile))
      rawGains[row[0]] = row[2];
   for (auto &row : CSVRange<double>(fftFile))
      fftGains[row[0]] = row[2];
   auto hist = new TH1F("gainRatio", "Ratio of Gains", 1000, .8, 1.2);
   for (auto &[padNum, gain] : rawGains) {
      if (fftGains.count(padNum) != 0)
         hist->Fill(gain / fftGains[padNum]);
   }
   hist->Draw();
}
