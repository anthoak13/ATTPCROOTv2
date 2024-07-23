/**
 * This is an example script for reading in a simulated file and plotting the relevant simulation parameters
 * It will create and display a series of histograms for all the observables in the simInfo TH1D object that is recorded
 * when using the run_simp_fiss script.
 */

void plot_sim(int runNum = 3)
{
   TString inOutDir = "./data/"; // Directory to save the output file
   TString simFile = inOutDir + TString::Format("simFission%02d.root", runNum);

   TChain tree("cbmsim");
   tree.Add(simFile); // Load in the data

   // Create histograms to fill
   // Arguments are: internalName, Display name, number of bins, min value, max value
   TH1D *hXpos = new TH1D("hXpos", "Vertex X Position", 100, -100, 100);
   TH1D *hYpos = new TH1D("hYpos", "Vertex Y Position", 100, -100, 100);
   TH1D *hZpos = new TH1D("hZpos", "Vertex Z Position", 100, 0, 1000);
   TH1D *hEner = new TH1D("hEner", "Beam energy at vertex", 100, 0, 3500);
   TH1D *hFragA = new TH1D("hFragA", "Fragment Mass", 40, 100, 140);
   TH1D *hFragZ = new TH1D("hFragZ", "Fragment Charge", 20, 40, 60);
   TH1D *hAng = new TH1D("hAng", "Decay Angle (CoM)", 100, 0, TMath::Pi());
   TH1D *hAngDeg = new TH1D("hAngDeg", "Decay Angle (CoM) Deg", 100, 0, 180);

   tree.Draw("Info->GetBinContent(1)>>hXpos");
   tree.Draw("Info->GetBinContent(2)>>hYpos");
   tree.Draw("Info->GetBinContent(3)>>hZpos");
   tree.Draw("Info->GetBinContent(4)>>hEner");
   tree.Draw("Info->GetBinContent(5)>>hFragA");
   // tree.Draw("204 - Info->GetBinContent(5)>>hFragA");
   tree.Draw("Info->GetBinContent(6)>>hFragZ");
   // tree.Draw("85 - Info->GetBinContent(6)>>hFragZ"); //Plot the second fragment
   tree.Draw("Info->GetBinContent(7)>>hAng");
   tree.Draw("Info->GetBinContent(7)*TMath::RadToDeg()>>hAngDeg"); // You can also call other functions and do math in
                                                                   // Draw commands

   // Ater running this script, all the histograms are availibe in the terminal. To display a particular histogram you
   // can run the following command: `hEner->Draw()` where you replace `hEner` with the name of whatever histogram you
   // want to draw.
}