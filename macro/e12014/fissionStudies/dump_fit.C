#include <fstream>

#include "plot_fit.C"

/**
 * One CSV row per event of [fit <name>] in a study (all chunks), for speed_report.py: the file, entry,
 * whether the fitter produced a result, the true and fitted light-fragment Z, dZ = true - fitted (one value
 * per event; zHistDiff fills both fragments, +dZ and -dZ, so its mean is always 0), the best fit's
 * objective, cut = 1 if plot_fit's FillPlots (default cuts) would fill the event into zHistDiff and hObj, and
 * the true decay angle in the center-of-mass frame (deg), and the best fit's fragment-0 Z and the azimuths of
 * its two fragment directions (sim_truth.C gives the true ones, to match fragments), and the true vertex depth
 * (mm from the window). From the Y-pattern the fit starts from: the folding angle (deg, between the two
 * fragment directions), and the vertex depth (mm from the window); and the fitted beam energy at the vertex
 * (MeV). Events without a fit still get the true decay angle and depth.
 * study may be <remote>:<study>; results reads a local study's results files (see FitFiles).
 *
 *    root -l -b -q 'dump_fit.C("hpc:speed_fit_ref", "a", "a.csv")'
 */
void dump_fit(TString study, TString name, TString csv, bool results = false)
{
   std::ofstream out(csv.Data());
   out << "file,entry,fitted,zsim,zfit,dz,obj,cut,thcm,z0fit,ph0,ph1,vz,fold,vzfit,ebeam\n";
   for (auto &path : FitFiles(study, name, results)) {
      std::unique_ptr<TFile> file(TFile::Open(path));
      TTreeReader reader("cbmsim", file.get());
      TTreeReaderValue<TClonesArray> resultArray(reader, "AtMCResult");
      TTreeReaderValue<TH1D> simInfo(reader, "SimInfo");
      while (reader.Next()) {
         out << gSystem->BaseName(path) << "," << reader.GetCurrentEntry() << ",";
         auto result = dynamic_cast<MCFitter::AtMCResult *>(resultArray->At(0)); // null: skipped event
         // True decay angle in the CoM frame (deg) and vertex depth from the window (mm)
         double thcm = simInfo->GetBinContent(7) * TMath::RadToDeg();
         double vz = simInfo->GetBinContent(3);
         if (!result) {
            out << "0,,,,,," << thcm << ",,,," << vz << ",,,\n";
            continue;
         }
         int z0 = result->fParameters["Z0"], z1 = result->fParameters["Z1"];
         int zSim = simInfo->GetBinContent(6); // Fragment 0's Z. The partner is z0 + z1 - zSim.
         int zSimLight = std::min(zSim, z0 + z1 - zSim), zFitLight = std::min(z0, z1);
         auto &par = result->fParameters; // FillPlots' default cuts
         bool cut = ex(par["EBeam"]) > 0 && ex(par["EBeam"]) < 5000 && par["Amp"] < 1 && par["Amp"] > 0 &&
                    par["ObjQ"] < 1000 && par["ObjQ"] > 0 && par["ObjPos"] < 100;
         double th0 = par["th0"], th1 = par["th1"];
         double fold = std::acos(std::sin(th0) * std::sin(th1) * std::cos(par["ph0"] - par["ph1"]) +
                                 std::cos(th0) * std::cos(th1)) *
                       TMath::RadToDeg();
         // z0 and the azimuths are the fitted fragment 0's Z and the two fitted fragment directions
         out << "1," << zSimLight << "," << zFitLight << "," << zSimLight - zFitLight << "," << result->fObjective
             << "," << cut << "," << thcm << "," << z0 << "," << par["ph0"] << "," << par["ph1"] << "," << vz << ","
             << fold << "," << par["vZ"] << "," << ex(par["EBeam"]) << "\n";
      }
   }
}
