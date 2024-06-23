#include <TApplication.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TGraph2D.h>
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

   // Get the cluster parameters from the command line
   int minPoints = 0;
   int minClusterSize = 0;
   int eventNumber = 1;
   if (argc >= 3) {
      minPoints = atoi(argv[1]);
      minClusterSize = atoi(argv[2]);
      if (argc == 4) {
         eventNumber = atoi(argv[3]);
      }
   } else {
      cout << "Please provide min_points and min_cluster_size as command line arguments" << endl;
      return 0;
   }

   // Load in the CSV file with the data
   Hdbscan hdbscan("../HDBSCANDataset/event_" + to_string(eventNumber) + ".dat");

   // Arguments are the number of coordintes and if there is a header line to skip
   auto ret = hdbscan.loadCsv(3, true, ' ');
   if (ret == 0) {
      cout << "Error loading csv file" << endl;
      return 0;
   }

   auto dataset = hdbscan.dataset; // This is the list of points loaded

   // execute(int minPoints, int minClusterSize, string distanceMetric);
   // Do not yet know which parameter corresponds to min_cluster_size and min_samples
   hdbscan.execute(minPoints, minClusterSize, "Euclidean");
   // hdbscan.displayResult();
   // cout << "You can access other fields like cluster labels, membership probabilities and outlier scores." << endl;

   // Create a ROOT graph
   std::map<int, TGraph2D *> graphsByLabel;
   for (size_t i = 0; i < dataset.size(); ++i) {
      double x = dataset[i][0];
      double y = dataset[i][1];
      double z = dataset[i][2];
      int label = hdbscan.normalizedLabels_[i]; // -1 is noise, otherwise 1- num clusters

      if (graphsByLabel.find(label) == graphsByLabel.end()) {
         graphsByLabel[label] = new TGraph2D();
      }
      graphsByLabel[label]->SetPoint(graphsByLabel[label]->GetN(), x, y, z);
   }

      for (auto &[label, graph] : graphsByLabel) {
      graph->SetMarkerStyle(20); // Just an example style
      if (label == -1) {
         graph->SetMarkerColor(kBlack); // Noise is black
         continue;
      }
      if (label >= 8)
         graph->SetMarkerColor(kOrange);
      else {
         graph->SetMarkerColor(label + 1);
      }
   }

   // Draw the graph
   TCanvas *canvas = new TCanvas("canvas", ("Event " + to_string(eventNumber)).data(), 800, 600);
   // canvas->DrawFrame(-250, -250, 0, 250, 250, 1000); // Set bounds of the frame
   bool first = true;
   for (auto &[label, graph] : graphsByLabel) {
      if (first) {
         graph->Draw("P0");
         first = false;
      } else
         graph->Draw("P0 same");

      cout << "Label " << label << " has " << graph->GetN() << " points" << " and color " << graph->GetMarkerColor()
           << endl;
   }

   app.Run();

   return 0;
}