#include "plot_fit.C"

#include <fstream>

/**
 * One CSV row per event of [fit <name>] in a study (all chunks), for speed_report.py: the file, entry,
 * whether the fitter produced a result, the true and fitted light-fragment Z, dZ = true - fitted (one value
 * per event; zHistDiff fills both fragments, +dZ and -dZ, so its mean is always 0), the best fit's
 * objective, and cut = 1 if plot_fit's FillPlots (default cuts) would fill the event into zHistDiff and hObj.
 * study may be <remote>:<study>; results reads a local study's results files (see FitFiles).
 *
 *    root -l -b -q 'dump_fit.C("hpc:speed_fit_ref", "a", "a.csv")'
 */
void dump_fit(TString study, TString name, TString csv, bool results = false)
{
   std::ofstream out(csv.Data());
   out << "file,entry,fitted,zsim,zfit,dz,obj,cut\n";
   for (auto &path : FitFiles(study, name, results)) {
      std::unique_ptr<TFile> file(TFile::Open(path));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> resultArray(reader, "AtMCResult");
      TTreeReaderValue<TH1D> simInfo(reader, "SimInfo");
      while (reader.Next()) {
         out << gSystem->BaseName(path) << "," << reader.GetCurrentEntry() << ",";
         auto result = dynamic_cast<MCFitter::AtMCResult *>(resultArray->At(0)); // null: skipped event
         if (!result) {
            out << "0,,,,,\n";
            continue;
         }
         int z0 = result->fParameters["Z0"], z1 = result->fParameters["Z1"];
         int zSim = simInfo->GetBinContent(6); // Fragment 0's Z. The partner is z0 + z1 - zSim.
         int zSimLight = std::min(zSim, z0 + z1 - zSim), zFitLight = std::min(z0, z1);
         auto &par = result->fParameters; // FillPlots' default cuts
         bool cut = ex(par["EBeam"]) > 0 && ex(par["EBeam"]) < 5000 && par["Amp"] < 1 && par["Amp"] > 0 &&
                    par["ObjQ"] < 1000 && par["ObjQ"] > 0 && par["ObjPos"] < 100;
         out << "1," << zSimLight << "," << zFitLight << "," << zSimLight - zFitLight << "," << result->fObjective
             << "," << cut << "\n";
      }
   }
}
