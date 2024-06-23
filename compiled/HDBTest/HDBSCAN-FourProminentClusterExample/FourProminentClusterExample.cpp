#include <TApplication.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TStyle.h>

#include "hdbscan.hpp"

#include <cstdio>
#include <iostream>
#include <map>
#include <set>
using namespace std;

int main(int argc, char **argv)
{
   TApplication app("app", &argc, argv);

   // Load in the CSV file with the data
   Hdbscan hdbscan("../HDBSCANDataset/FourProminentClusterDataset.csv");

   // Arguments are the number of coordintes and if there is a header line to skip
   auto ret = hdbscan.loadCsv(2, true);
   if (ret == 0) {
      cout << "Error loading csv file" << endl;
      return 0;
   }

   auto dataset = hdbscan.dataset; // This is the list of points loaded

   // execute(int minPoints, int minClusterSize, string distanceMetric);
   // Do not yet know which parameter corresponds to min_cluster_size and min_samples
   hdbscan.execute(5, 5, "Euclidean");
   hdbscan.displayResult();
   cout << "You can access other fields like cluster labels, membership probabilities and outlier scores." << endl;

   // Create a ROOT graph
   std::map<int, TGraph *> graphsByLabel;
   for (size_t i = 0; i < dataset.size(); ++i) {
      double x = dataset[i][0];
      double y = dataset[i][1];
      int label = hdbscan.normalizedLabels_[i]; // -1 is noise, otherwise 1- num clusters

      if (graphsByLabel.find(label) == graphsByLabel.end()) {
         graphsByLabel[label] = new TGraph();
      }
      graphsByLabel[label]->SetPoint(graphsByLabel[label]->GetN(), x, y);
   }

   std::vector<int> colors{kRed, kBlue, kGreen, kCyan};

   for (auto &[label, graph] : graphsByLabel) {
      graph->SetMarkerStyle(20); // Just an example style
      if (label == -1) {
         graph->SetMarkerColor(kBlack); // Noise is black
         continue;
      }
      graph->SetMarkerColor(colors.back());
      colors.pop_back();
   }

   // Draw the graph
   TCanvas *canvas = new TCanvas("canvas", "Four Prominent Clusters", 800, 600);
   canvas->DrawFrame(-10, -10, 10, 10); // Set bounds of the frame

   for (auto &[label, graph] : graphsByLabel) {
      graph->Draw("P same");
      cout << "Label " << label << " has " << graph->GetN() << " points" << " and color " << graph->GetMarkerColor()
           << endl;
   }

   app.Run();

   return 0;
}