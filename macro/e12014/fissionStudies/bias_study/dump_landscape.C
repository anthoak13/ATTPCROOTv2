/**
 * Every sampled point of every event of the fit results files matching `glob`: one CSV row per simulated
 * event of the fit (its chunk, entry, fragment-0 Z, objective and amplitude), plus per event the fixed
 * fragment azimuths (ph0, ph1) and the true fragment-0 Z from SimInfo. The fit's only free parameter is
 * the fragment Z (directions and vertex are fixed to the Y-pattern), so the rows of one event at one Z
 * are independent draws of the objective's noise at that Z.
 *
 *    root -l -b -q 'dump_landscape.C("/path/tune_fid90-base.fit.c*.results.root", "out.csv")'
 */
void dump_landscape(TString glob, TString csv)
{
   std::ofstream out(csv.Data());
   out << "chunk,entry,ztrue0,tvx,tvy,tvz,tve,ph0,ph1,th0,th1,vx,vy,vz,iter,z0,obj,amp,ebeam\n";
   TString dir = gSystem->GetDirName(glob);
   TString pat = gSystem->BaseName(glob);
   TRegexp re(pat, true);
   std::vector<TString> files;
   std::unique_ptr<TSystemDirectory> sysDir(new TSystemDirectory(dir, dir));
   for (auto *obj : *sysDir->GetListOfFiles()) {
      TString name = obj->GetName();
      if (name.Index(re) == 0 && name.Length() == name(re).Length())
         files.push_back(name);
   }
   std::sort(files.begin(), files.end());
   for (auto &name : files) {
      TString chunk = "";
      auto at = name.Index(".fit.c");
      if (at >= 0)
         chunk = name(at + 5, 3);
      std::unique_ptr<TFile> file(TFile::Open(dir + "/" + name));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> res(reader, "AtMCResult");
      TTreeReaderValue<TH1D> info(reader, "SimInfo");
      while (reader.Next()) {
         for (int i = 0; i < res->GetEntriesFast(); ++i) {
            auto r = dynamic_cast<MCFitter::AtMCResult *>(res->At(i));
            auto &p = r->fParameters;
            out << chunk << "," << reader.GetCurrentEntry() << "," << info->GetBinContent(6) << ","
                << info->GetBinContent(1) << "," << info->GetBinContent(2) << "," << info->GetBinContent(3) << ","
                << info->GetBinContent(4) << "," << p["ph0"] << "," << p["ph1"] << "," << p["th0"] << ","
                << p["th1"] << "," << p["vX"] << "," << p["vY"] << "," << p["vZ"] << "," << r->fIterNum << ","
                << p["Z0"] << "," << r->fObjective << "," << p["Amp"] << "," << p["EBeam"] << "\n";
         }
      }
   }
   std::cout << "Wrote " << csv << " from " << files.size() << " files" << std::endl;
}
