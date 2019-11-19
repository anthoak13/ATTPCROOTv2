/* Root macro to generate a tree of possible fission decays in 
 * the CoM frame.
 * 
 * Output: data/fissionEvents.root and data/ionList.txt
 * data/fissionEvents.root contains tree "events" and histogram of mass distro
 * TTree "events"
 *  -> nTracks /I
 *  -> A[nTracks] /I
 *  -> Z[nTracks] /I
 *  -> P[nTracks] /TLorentzVector
 * TH1I "hMassDistro"

 * data/ionList.txt contains a text list of all ions used
 * 
 * Adam Anthony 11/181/19
 */

// Get the total kinetic energy of the fission fragmetns in MeV assuming
// Viola systematics (DOI: 10.1103/PhysRevC.31.1550)
double violaEn(int A, int Z)
{
  return 0.1189*Z*Z/TMath::Power(A, 1.0/3.0); + 7.3;
}

// Generates fission distro based on the assumtion that it's two gaussians
// seperated by massFrac with stdDev massDev
void genFissionProducts(int nEvents, int Z, int A, float massFrac, float massDev,
			unsigned int seed = 4357)
{
  TRandom3 rand(seed);
  
  //Name of the output files
  TString outName("data/fissionEvents.root");
  TString ionList("data/ionList.txt");

  //Create files
  ofstream ionFile(ionList.Data());
  TFile *outFile = new TFile(outName, "RECREATE");

  //Create map to hold the list of ions created
  std::set<TString> ionSet;
  
  //Create tree and histogram to fill
  outFile->cd();
  TH1I *hMass = new TH1I("hMassDistro", "Mass distribution", A*3/4-A/4, A/4, A*3/4);
  TH1F *hTh = new TH1F("hTh", "Theta distro", 100, 0, TMath::Pi());
  TH1F *hPhi = new TH1F("hPhi", "Phi distro", 100, -TMath::Pi(), TMath::Pi());
  TTree *tree = new TTree("trEvents", "Fission Events");

  //Add branches to tree
  Int_t nTracks;
  Int_t Aout[100];
  Int_t Zout[100];
  Double_t pX[100];
  Double_t pY[100];
  Double_t pZ[100];
  Double_t pT[100];
  std::vector<TLorentzVector> p;

  tree->Branch("nTracks", &nTracks, "nTracks/I");
  tree->Branch("Aout", Aout, "Aout[nTracks]/I");
  tree->Branch("Zout", Zout, "Zout[nTracks]/I");
  tree->Branch("pX", pX, "PX[nTracks]/D");
  tree->Branch("pY", pY, "PY[nTracks]/D");
  tree->Branch("pZ", pZ, "PZ[nTracks]/D");
  tree->Branch("pT", pT, "PT[nTracks]/D");
  
  // Loop through and generate events
  TVector3 *dir = new TVector3();
  for(int i = 0; i < nEvents; ++i)
  {
    nTracks = 2;

    //Generate the particle id and fill the mass distro histogram
    Aout[0] = rand.Gaus(A*massFrac, massDev);
    Aout[1] = A - Aout[0];
    Zout[0] = (float)Z/A * Aout[0];
    Zout[1] = (float)Z/A * Aout[1];

    //Add ions to set
    ionSet.emplace(Form("%d %d", Zout[0], Aout[0]));
    ionSet.emplace(Form("%d %d", Zout[1], Aout[1]));
    
    hMass->Fill(Aout[0]);
    hMass->Fill(Aout[1]);

    //Pick a direction for the 1st particle to decay to
    dir->SetMagThetaPhi(1, rand.Uniform(TMath::Pi()), rand.Uniform(2*TMath::Pi()) );
    hTh->Fill(dir->Theta());
    hPhi->Fill(dir->Phi());
    
    //Calculate the  mass and kinetic energy of the two particles
    double AtoE = 939.0; // MeV/u
    double m[2] = {AtoE*Aout[0], AtoE*Aout[1]};
    double KE = violaEn(A,Z);

    //Get the energies of the two particles
    double E[2];
    E[1] = m[0]*m[1] + m[1]*m[1] + KE*(m[0]+m[1]) + KE*KE/2.0;
    E[1] /= m[0]+m[1]+KE;
    E[0] = TMath::Sqrt(E[1]*E[1] + m[0]*m[0] - m[1]*m[1]);

    //Get the magnitude of the momentum and set the tree
    double pMag = TMath::Sqrt(E[0]*E[0]-m[0]*m[0]);
    //std::cout << Aout[0] << ": " << KE << " " << pMag << std::endl;

    pX[0] = pMag * dir->X();
    pY[0] = pMag * dir->Y();
    pZ[0] = pMag * dir->Z();
    pT[0] = E[0];

    pX[1] = pMag * -dir->X();
    pY[1] = pMag * -dir->Y();
    pZ[1] = pMag * -dir->Z();
    pT[1] = E[1];

    tree->Fill();
  } //end loop over events

  outFile->Write();
  outFile->Close();

  //Print out the ionlist
  ionFile << "#Z  A" << std::endl;
  for(auto elem : ionSet)
  {
    ionFile << elem << std::endl;
  }
  ionFile.close();
  
}
