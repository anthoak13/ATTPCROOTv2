/**
 * Y-pattern vertex against the simulated one, for digi (or fit results) files matching `glob`: one CSV row
 * per event with the true vertex (SimInfo, mm from the window) and the pattern's (converted the way the fit
 * converts it, 1000 - z), and the pattern's two fragment polar angles (rad).
 *
 *    root -l -b -q 'vertex_check.C("/path/study-name.digi.c0*.root", "out.csv")'
 */
void vertex_check(TString glob, TString csv)
{
   std::ofstream out(csv.Data());
   out << "file,entry,tvx,tvy,tvz,vx,vy,vz,th0,th1,ph0,ph1\n";
   TString dir = gSystem->GetDirName(glob);
   TRegexp re(gSystem->BaseName(glob), true);
   std::vector<TString> files;
   std::unique_ptr<TSystemDirectory> sysDir(new TSystemDirectory(dir, dir));
   for (auto *obj : *sysDir->GetListOfFiles()) {
      TString name = obj->GetName();
      if (name.Index(re) == 0 && name.Length() == name(re).Length())
         files.push_back(name);
   }
   std::sort(files.begin(), files.end());
   for (auto &name : files) {
      std::unique_ptr<TFile> file(TFile::Open(dir + "/" + name));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> fission(reader, "AtFissionEvent");
      TTreeReaderValue<TH1D> info(reader, "SimInfo");
      while (reader.Next()) {
         auto *ev = fission->GetEntriesFast() ? dynamic_cast<AtFissionEvent *>(fission->At(0)) : nullptr;
         auto pat = ev ? ev->GetYPattern() : nullptr;
         out << name << "," << reader.GetCurrentEntry() << "," << info->GetBinContent(1) << "," << info->GetBinContent(2)
             << "," << info->GetBinContent(3);
         if (!pat) {
            out << ",,,,,,,\n";
            continue;
         }
         auto v = pat->GetVertex();
         auto d0 = pat->GetFragmentDirection(0).Unit(), d1 = pat->GetFragmentDirection(1).Unit();
         if (d0.Z() < 0)
            d0.SetZ(-d0.Z());
         if (d1.Z() < 0)
            d1.SetZ(-d1.Z());
         out << "," << v.X() << "," << v.Y() << "," << 1000 - v.Z() << "," << d0.Theta() << "," << d1.Theta() << ","
             << d0.Phi() << "," << d1.Phi() << "\n";
      }
   }
}
