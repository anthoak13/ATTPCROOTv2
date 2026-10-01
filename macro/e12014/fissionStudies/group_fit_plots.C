#include <TCanvas.h>
#include <TFile.h>
#include <TKey.h>
#include <TLegend.h>
#include <TSystem.h>

#include "RunConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>

#include "plot_fit.C"

struct GroupedFitPlot {
   TString name;
   TH1 *hist;
};

std::vector<GroupedFitPlot> GetGroupedFitPlots()
{
   return {{"Z", zHist},
           {"A", aHist},
           {"Amp", hAmp},
           {"Obj", hObj},
           {"ObjPos", hObjPos},
           {"ObjQ", hObjQ},
           {"MR", hMR},
           {"Beam", hBeam},
           {"ZvsObj", hZvsObj},
           {"ZvsAmp", hZvsAmp},
           {"AmpvsPosObj", hAmpvsPosObj},
           {"AmpvsObj", hAmpvsObj},
           {"AmpvsLoc", hAmpvsLoc},
           {"ZvsObjSim", hZvsObjSim},
           {"ZSim", zHistSim},
           {"ZDiff", zHistDiff}};
}

/// Section name of an .ini line, or "" if it isn't a section header. Matches fission.py's configparser:
/// a ; or # after whitespace starts a comment, and the header runs to the last ].
TString SectionName(const std::string &line)
{
   std::string text = line;
   for (size_t i = 1; i < text.size(); ++i)
      if ((text[i] == ';' || text[i] == '#') && isspace(text[i - 1])) {
         text.resize(i);
         break;
      }
   TString trimmed = TString(text).Strip(TString::kBoth);
   auto close = trimmed.Last(']');
   if (!trimmed.BeginsWith("[") || close < 2)
      return "";
   return trimmed(1, close - 1);
}

/// Run names of a study, from the [run <name>] sections of studies/<study>.ini.
std::vector<TString> StudyRuns(TString study)
{
   std::vector<TString> runs;
   std::ifstream file(("studies/" + study + ".ini").Data());
   if (!file) {
      Error("StudyRuns", "Cannot open studies/%s.ini (start ROOT from the fissionStudies directory)", study.Data());
      return runs;
   }
   std::string line;
   while (std::getline(file, line)) {
      auto section = SectionName(line);
      if (section.BeginsWith("run "))
         runs.push_back(TString(section(4, section.Length())).Strip(TString::kBoth));
   }
   return runs;
}

/**
 * Group the runs of a study by the value of one setting and write each plot, one histogram per value,
 * to <outputDir>/<plot>.root. Values are read from the settings recorded in every fit file, which
 * include the macro defaults, so they are what was actually run. Runs (and chunks) sharing a value
 * are combined. Only runs listed in studies/<study>.ini are used, so files of removed or renamed runs
 * are ignored.
 *
 *    group_fit_plots("angle_scan", "sim.decayAngle")
 *    show_groups("Z", "./groups_angle_scan")
 */
void group_fit_plots(TString study, TString key, TString outputDir = "")
{
   if (outputDir.IsNull())
      outputDir = "./groups_" + study;

   // Collect fit files of the study's current runs by the value of key
   auto runs = StudyRuns(study);
   if (runs.empty())
      return;
   std::map<std::string, std::vector<TString>> groups;
   auto dataDir = RunConfig::DataDir();
   auto dir = gSystem->OpenDirectory(dataDir);
   while (auto entry = gSystem->GetDirEntry(dir)) {
      TString name = entry;
      if (std::none_of(runs.begin(), runs.end(), [&](const TString &run) { return IsFitFile(name, study, run); }))
         continue;
      if (!IsFinished(dataDir + "/" + name))
         continue;
      auto cfg = RunConfig::FromOutput(dataDir + "/" + name);
      auto value = cfg.GetStr(key.Data(), "");
      if (value.empty()) {
         Warning("group_fit_plots", "%s has no %s, skipping", name.Data(), key.Data());
         continue;
      }
      groups[value].push_back(dataDir + "/" + name);
   }
   gSystem->FreeDirectory(dir);

   if (groups.empty()) {
      Error("group_fit_plots", "No fit files for study %s in %s", study.Data(), dataDir.Data());
      return;
   }

   gSystem->mkdir(outputDir, true);
   std::vector<TFile *> outputFiles;
   for (auto &plot : GetGroupedFitPlots())
      outputFiles.push_back(new TFile(outputDir + "/" + plot.name + ".root", "RECREATE"));

   for (auto &[value, files] : groups) {
      gROOT->cd(); // Keep plot_fit's histograms out of the output files
      plot_fit(files, false);
      TString label = TString::Format("%s_%s", key.Data(), value.c_str());
      auto plots = GetGroupedFitPlots();

      for (size_t plotIndex = 0; plotIndex < plots.size(); ++plotIndex) {
         outputFiles[plotIndex]->cd();
         auto copy = static_cast<TH1 *>(plots[plotIndex].hist->Clone(label));
         copy->SetTitle(TString::Format("%s, %s = %s", plots[plotIndex].hist->GetTitle(), key.Data(), value.c_str()));
         copy->Write();
      }
   }

   for (auto file : outputFiles) {
      file->Close();
      delete file;
   }
}

/// Draw every group of one plot written by group_fit_plots, side by side or overlaid.
void show_groups(TString plotName, TString outputDir, bool overlay = false, int requestedColumns = 0)
{
   TFile *file = TFile::Open(outputDir + "/" + plotName + ".root", "READ");
   if (!file || file->IsZombie()) {
      Error("show_groups", "Could not open %s/%s.root", outputDir.Data(), plotName.Data());
      return;
   }

   std::vector<TH1 *> plots;
   TIter next(file->GetListOfKeys());
   while (auto key = static_cast<TKey *>(next())) {
      auto plot = dynamic_cast<TH1 *>(key->ReadObj());
      if (plot)
         plots.push_back(plot);
   }
   std::cout << "Plots found for " << plotName << ": " << plots.size() << std::endl;
   if (plots.empty()) {
      Error("show_groups", "No plots found in %s/%s.root", outputDir.Data(), plotName.Data());
      file->Close();
      delete file;
      return;
   }

   requestedColumns = requestedColumns > 0 ? requestedColumns : std::sqrt(plots.size()) + 1;

   const bool isTwoDimensional = plots.front()->InheritsFrom(TH2::Class());
   const bool usePads = isTwoDimensional || !overlay;
   const int columns = usePads ? std::max(1, std::min(requestedColumns, static_cast<int>(plots.size()))) : 1;
   const int rows = (plots.size() + columns - 1) / columns;
   const int canvasWidth = usePads ? columns * 700 : 1200;
   const int canvasHeight = usePads ? rows * 500 : 700;
   auto canvas =
      new TCanvas(TString::Format("groupComparison_%s", plotName.Data()), plotName, canvasWidth, canvasHeight);

   if (usePads) {
      canvas->Divide(columns, rows, 0.01, 0.01);
      for (size_t i = 0; i < plots.size(); ++i) {
         canvas->cd(i + 1);
         plots[i]->Draw(isTwoDimensional ? "colz" : "hist");
      }
   } else {
      auto legend = new TLegend(0.72, 0.7, 0.9, 0.9);
      for (size_t i = 0; i < plots.size(); ++i) {
         plots[i]->SetLineColor(static_cast<int>(i) + 1);
         plots[i]->SetLineWidth(2);
         plots[i]->Draw(i == 0 ? "hist" : "hist same");
         legend->AddEntry(plots[i], plots[i]->GetName(), "l");
      }
      legend->Draw();
   }

   canvas->Update();
}
