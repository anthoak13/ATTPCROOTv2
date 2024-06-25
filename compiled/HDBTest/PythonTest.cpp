#include <TApplication.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TRootCanvas.h>
#include <TStyle.h>

#include "hdbscan.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/numpy.h>
#include <set>
using namespace std;
namespace py = pybind11;
using namespace pybind11::literals;

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

   // py::scoped_interpreter guard{}; // start the interpreter and keep it alive
   py::initialize_interpreter();
   py::module_ skcluster = py::module_::import("sklearn.cluster");
   py::module_ np = py::module_::import("numpy");

   const char *vmcWorkDir = std::getenv("VMCWORKDIR");
   std::string filename = "../HDBSCANDataset/event_" + std::to_string(eventNumber) + ".dat ";
   // std::string filename = std::string(vmcWorkDir) + "/compiled/HDBTest/HDBSCANDataset/event_1.dat ";
   //  Load in the CSV file with the data
   py::object np_loadtxt = np.attr("loadtxt");
   py::array_t<float> data = np_loadtxt("../HDBSCANDataset/event_1.dat", "dtype"_a = np.attr("float32"),
                                        "delimiter"_a = ' ', "skiprows"_a = 1);

   py::print(data);
   std::cout << "Loaded file\n";
   int len = data.shape(0);
   std::cout << "Length: " << len << "\n";

   // Create a numpy array with the same shape as the 'data' array
   py::array_t<double> cluster_data = np.attr("empty")("shape"_a = py::make_tuple(len, 3));
   std::cout << "Made cluster_data\n";

   // Copy the first three columns of 'data' into 'cluster_data'
   {
      auto cluster_data_rw = cluster_data.mutable_unchecked<2>();
      auto data_r = data.unchecked<2>(); // needs type since not specified in np_loadtxt
      for (int i = 0; i < len; i++) {
         for (int j = 0; j < 3; j++) {

            cluster_data_rw(i, j) = data_r(i, j);
         }
      }
   }

   py::print(cluster_data);
   int min_points = 5;
   int min_size = 5;
   double cluster_selection_epsilon = 0.0;

   auto clusterizer = skcluster.attr("HDBSCAN")("min_cluster_size"_a = min_size, "min_samples"_a = min_points,
                                                "allow_single_cluster"_a = true,
                                                "cluster_selection_epsilon"_a = cluster_selection_epsilon);

   auto fitted_clusters = clusterizer.attr("fit")(cluster_data);
   py::array_t<int> labels = np.attr("unique")(fitted_clusters.attr("labels_"));

   py::print(labels);

   // Create a ROOT graph
   std::map<int, TGraph2D *> graphsByLabel;
   auto dataset = cluster_data.unchecked<2>();
   py::array labels_ = fitted_clusters.attr("labels_");
   auto data_labels = labels_.unchecked<int, 1>();
   for (size_t i = 0; i < len; ++i) {
      double x = dataset(i, 0);
      double y = dataset(i, 1);
      double z = dataset(i, 2);
      int label = data_labels(i); // -1 is noise, otherwise 0 to num clusters-1

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

      graph->SetMarkerColor((label % 8) + 1);
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

   canvas->Modified();
   canvas->Update();
   TRootCanvas *rc = (TRootCanvas *)canvas->GetCanvasImp();
   rc->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
   app.Run();

   return 0;
}