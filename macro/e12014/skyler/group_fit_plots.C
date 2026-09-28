#include <TCanvas.h>
#include <TFile.h>
#include <TKey.h>
#include <TLegend.h>
#include <TSystem.h>

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

void group_fit_plots(std::vector<int> runNums, std::vector<double> parameterValues,
                     TString parameterName = "decayAngle", TString outputDir = "./plots_by_parameter")
{
   if (runNums.size() != parameterValues.size()) {
      Error("group_fit_plots", "runNums and parameterValues must have the same size");
      return;
   }

   gSystem->mkdir(outputDir, true);

   std::vector<TFile *> outputFiles;
   for (auto &plot : GetGroupedFitPlots())
      outputFiles.push_back(new TFile(outputDir + "/" + plot.name + ".root", "RECREATE"));

   for (size_t i = 0; i < runNums.size(); ++i) {
      if (tree) {
         delete tree;
         tree = nullptr;
      }

      plot_fit(runNums[i], false);
      TString label = TString::Format("%s_%g", parameterName.Data(), parameterValues[i]);
      auto plots = GetGroupedFitPlots();

      for (size_t plotIndex = 0; plotIndex < plots.size(); ++plotIndex) {
         outputFiles[plotIndex]->cd();
         auto copy = static_cast<TH1 *>(plots[plotIndex].hist->Clone(label));
         copy->SetTitle(TString::Format("%s, %s = %g", plots[plotIndex].hist->GetTitle(), parameterName.Data(),
                                        parameterValues[i]));
         copy->Write();
      }
   }

   if (tree) {
      delete tree;
      tree = nullptr;
   }

   for (auto file : outputFiles) {
      file->Write();
      file->Close();
      delete file;
   }
}

void group_fit_plots_by_angle(std::vector<std::vector<int>> runGroups, std::vector<double> anglesDeg,
                              TString outputDir = "./plots_by_angle");

void group_fit_plots_by_angle(std::vector<int> runNums, std::vector<double> anglesDeg,
                              TString outputDir = "./plots_by_angle")
{
   if (runNums.size() != anglesDeg.size()) {
      Error("group_fit_plots_by_angle", "runNums and anglesDeg must have the same size");
      return;
   }

   std::vector<std::vector<int>> runGroups;
   std::vector<double> uniqueAngles;
   std::map<double, size_t> angleToGroup;

   for (size_t i = 0; i < runNums.size(); ++i) {
      auto group = angleToGroup.find(anglesDeg[i]);
      if (group == angleToGroup.end()) {
         angleToGroup[anglesDeg[i]] = runGroups.size();
         runGroups.push_back({});
         uniqueAngles.push_back(anglesDeg[i]);
         group = angleToGroup.find(anglesDeg[i]);
      }
      runGroups[group->second].push_back(runNums[i]);
   }

   group_fit_plots_by_angle(runGroups, uniqueAngles, outputDir);
}

void group_fit_plots_by_angle(std::vector<std::vector<int>> runGroups, std::vector<double> anglesDeg, TString outputDir)
{
   if (runGroups.size() != anglesDeg.size()) {
      Error("group_fit_plots_by_angle", "runGroups and anglesDeg must have the same size");
      return;
   }

   gSystem->mkdir(outputDir, true);

   std::vector<TFile *> outputFiles;
   for (auto &plot : GetGroupedFitPlots())
      outputFiles.push_back(new TFile(outputDir + "/" + plot.name + ".root", "RECREATE"));

   for (size_t i = 0; i < runGroups.size(); ++i) {
      if (tree) {
         delete tree;
         tree = nullptr;
      }

      plot_fit(runGroups[i], false);
      TString label = TString::Format("angle_%gdeg", anglesDeg[i]);
      auto plots = GetGroupedFitPlots();

      for (size_t plotIndex = 0; plotIndex < plots.size(); ++plotIndex) {
         outputFiles[plotIndex]->cd();
         auto copy = static_cast<TH1 *>(plots[plotIndex].hist->Clone(label));
         copy->SetTitle(TString::Format("%s, decay angle = %g deg (runs combined)", plots[plotIndex].hist->GetTitle(),
                                        anglesDeg[i]));
         copy->Write();
      }
   }

   if (tree) {
      delete tree;
      tree = nullptr;
   }

   for (auto file : outputFiles) {
      file->Write();
      file->Close();
      delete file;
   }
}

void show_angles(TString plotName = "Z", TString outputDir = "./plots_by_angle", bool overlay = false,
                 int requestedColumns = 0)
{
   TFile *file = TFile::Open(outputDir + "/" + plotName + ".root", "READ");
   if (!file || file->IsZombie()) {
      Error("show_angles", "Could not open %s/%s.root", outputDir.Data(), plotName.Data());
      return;
   }

   std::vector<TH1 *> plots;
   TIter next(file->GetListOfKeys());
   while (auto key = static_cast<TKey *>(next())) {
      auto plot = dynamic_cast<TH1 *>(key->ReadObj());
      if (plot && TString(plot->GetName()).BeginsWith("angle_") && key->GetCycle() == 2)
         plots.push_back(plot);
   }
   std::cout << "Plots found for " << plotName << ": " << plots.size() << std::endl;
   if (plots.empty()) {
      Error("show_angles", "No angle plots found in %s/%s.root", outputDir.Data(), plotName.Data());
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
   auto canvas = new TCanvas(TString::Format("angleComparison_%s", plotName.Data()), "Decay angle comparison",
                             canvasWidth, canvasHeight);

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
