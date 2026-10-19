/**
 * One CSV row per simulated event of the sim files matching `glob` (relative to FISSION_DATA): the chunk,
 * entry, fragment 0's Z (SimInfo, the Z the fit is compared with) and the true azimuth (rad) of each
 * fragment's initial momentum. The simple simulation numbers the tracks of an event from 1: the beam, then
 * fragment 0, then fragment 1. The azimuth is the same in the simulation and detector frames (they differ
 * in z only), so tune_report.py uses it to tell which fitted fragment is which true one.
 *
 *    root -l -b -q 'sim_truth.C("tune_data90-fine.sim.c*.root", "truth.csv")'
 */
#include "RunConfig.h"

void sim_truth(TString glob, TString csv)
{
   std::ofstream out(csv.Data());
   out << "chunk,entry,z0,phi0,phi1\n";
   TString dir = RunConfig::DataDir();
   std::unique_ptr<TSystemDirectory> sysDir(new TSystemDirectory(dir, dir));
   std::vector<TString> files;
   TRegexp re(glob, true);
   for (auto *obj : *sysDir->GetListOfFiles()) {
      TString name = obj->GetName();
      if (name.Index(re) == 0 && name.Length() == name(re).Length())
         files.push_back(name);
   }
   std::sort(files.begin(), files.end());
   for (auto &name : files) {
      TString chunk = "";
      auto at = name.Index(".c");
      if (at >= 0 && name.Length() > at + 4 && isdigit(name[at + 2]) && isdigit(name[at + 3]))
         chunk = name(at + 1, 3);
      std::unique_ptr<TFile> file(TFile::Open(dir + "/" + name));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> points(reader, "AtTpcPoint");
      TTreeReaderValue<TH1D> info(reader, "Info");
      while (reader.Next()) {
         double phi[2] = {NAN, NAN};
         for (int i = 0; i < points->GetEntriesFast(); ++i) {
            auto point = dynamic_cast<AtMCPoint *>(points->At(i));
            int frag = point->GetTrackID() - 2; // Track 1 is the beam
            if (frag >= 0 && frag < 2 && std::isnan(phi[frag]))
               phi[frag] = std::atan2(point->GetPy(), point->GetPx());
         }
         out << chunk << "," << reader.GetCurrentEntry() << "," << info->GetBinContent(6) << "," << phi[0] << ","
             << phi[1] << "\n";
      }
   }
   std::cout << "Wrote " << csv << " from " << files.size() << " sim files" << std::endl;
}
