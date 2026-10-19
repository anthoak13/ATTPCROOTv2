/**
 * Like sim_truth.C, with the polar angles and kinetic energies of both fragments: one CSV row per simulated
 * event of the sim files matching `glob` (a full path): chunk, entry, fragment-0 Z, and per fragment the azimuth
 * and polar angle (rad, of the momentum at its first step) and its kinetic energy (MeV, at its first step).
 * Tracks of an event are numbered from 1: the beam, then fragment 0, then fragment 1.
 *
 *    root -l -b -q 'truth_dirs.C("/path/tune_data90-fine.sim.c0*.root", "truth.csv")'
 */
void truth_dirs(TString glob, TString csv)
{
   std::ofstream out(csv.Data());
   out << "chunk,entry,z0,phi0,phi1,th0,th1,ke0,ke1\n";
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
      TString chunk = "";
      auto at = name.Index(".sim.c");
      if (at >= 0)
         chunk = name(at + 5, 3);
      std::unique_ptr<TFile> file(TFile::Open(dir + "/" + name));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> points(reader, "AtTpcPoint");
      TTreeReaderValue<TH1D> info(reader, "Info");
      while (reader.Next()) {
         double phi[2] = {NAN, NAN}, th[2] = {NAN, NAN}, ke[2] = {NAN, NAN};
         for (int i = 0; i < points->GetEntriesFast(); ++i) {
            auto point = dynamic_cast<AtMCPoint *>(points->At(i));
            int frag = point->GetTrackID() - 2;
            if (frag >= 0 && frag < 2 && std::isnan(phi[frag])) {
               ROOT::Math::XYZVector p(point->GetPx(), point->GetPy(), point->GetPz());
               phi[frag] = p.Phi();
               th[frag] = p.Theta();
               double A = frag == 0 ? info->GetBinContent(5) : 204 - info->GetBinContent(5);
               double m = A * 939.0 / 1000.; // GeV, the sim's fragment mass
               ke[frag] = (std::sqrt(p.Mag2() + m * m) - m) * 1000. + point->GetEnergyLoss() * 1000.;
            }
         }
         out << chunk << "," << reader.GetCurrentEntry() << "," << info->GetBinContent(6) << "," << phi[0] << ","
             << phi[1] << "," << th[0] << "," << th[1] << "," << ke[0] << "," << ke[1] << "\n";
      }
   }
}
