void run_pra_sim_integration(Int_t nEvents = 8, TString outputDir = "")
{
   FairLogger::GetLogger()->SetLogScreenLevel("ERROR");

   TString dir = getenv("VMCWORKDIR");
   if (dir.IsNull()) {
      std::cerr << "VMCWORKDIR is not set. Source build/config.sh first." << std::endl;
      gSystem->Exit(1);
      return;
   }

   if (outputDir.IsNull())
      outputDir = dir + "/macro/tests/AT-TPC/data/pra-sim-integration";

   gSystem->mkdir(outputDir, kTRUE);

   TString simFile = outputDir + "/attpcsim.root";
   TString parFile = outputDir + "/attpcpar.root";
   TString geoFileInputName = dir + "/geometry/ATTPC_H300torr.root";
   FairRunSim *run = new FairRunSim();
   run->SetName("TGeant4");
   run->SetSink(new FairRootFileSink(simFile));
   FairRuntimeDb *rtdb = run->GetRuntimeDb();

   run->SetMaterials("media.geo");

   FairModule *cave = new AtCave("CAVE");
   cave->SetGeometryFileName("cave.geo");
   run->AddModule(cave);

   FairDetector *attpc = new AtTpc("ATTPC", kTRUE);
   attpc->SetGeometryFileName(geoFileInputName);
   run->AddModule(attpc);

   auto *fMagField = new AtConstField();
   fMagField->SetField(0., 0., 28.5);
   fMagField->SetFieldRegion(-50, 50, -50, 50, -10, 230);
   run->SetField(fMagField);

   FairPrimaryGenerator *primGen = new FairPrimaryGenerator();

   Int_t z = 6;
   Int_t a = 16;
   Int_t q = 0;
   Int_t m = 1;
   Double_t px = 0.000 / a;
   Double_t py = 0.000 / a;
   Double_t pz = 2.297 / a;
   Double_t beamExcEner = 0.0;
   Double_t beamMass = 16.014701;
   Double_t nominalEnergy = 8;

   auto *ionGen = new AtTPCIonGenerator("Ion", z, a, q, m, px, py, pz, beamExcEner, beamMass, nominalEnergy);
   ionGen->SetSpotRadius(0, -100, 0);
   primGen->AddGenerator(ionGen);

   std::vector<Int_t> Zp;
   std::vector<Int_t> Ap;
   std::vector<Int_t> Qp;
   std::vector<Double_t> Pxp;
   std::vector<Double_t> Pyp;
   std::vector<Double_t> Pzp;
   std::vector<Double_t> Mass;
   std::vector<Double_t> ExE;

   Int_t mult = 4;
   Double_t residualEnergy = 40.0;

   Zp.push_back(z);
   Ap.push_back(a);
   Qp.push_back(q);
   Pxp.push_back(px);
   Pyp.push_back(py);
   Pzp.push_back(pz);
   Mass.push_back(16.014701);
   ExE.push_back(beamExcEner);

   Zp.push_back(1);
   Ap.push_back(1);
   Qp.push_back(0);
   Pxp.push_back(0.0);
   Pyp.push_back(0.0);
   Pzp.push_back(0.0);
   Mass.push_back(1.0078250322);
   ExE.push_back(0.0);

   Zp.push_back(6);
   Ap.push_back(16);
   Qp.push_back(0);
   Pxp.push_back(0.0);
   Pyp.push_back(0.0);
   Pzp.push_back(0.0);
   Mass.push_back(16.014701);
   ExE.push_back(0.0);

   Zp.push_back(1);
   Ap.push_back(1);
   Qp.push_back(0);
   Pxp.push_back(0.0);
   Pyp.push_back(0.0);
   Pzp.push_back(0.0);
   Mass.push_back(1.0078250322);
   ExE.push_back(0.0);

   Double_t thetaMinCms = 20.0;
   Double_t thetaMaxCms = 90.0;

   auto *twoBody = new AtTPC2Body("TwoBody", &Zp, &Ap, &Qp, mult, &Pxp, &Pyp, &Pzp, &Mass, &ExE, residualEnergy,
                                  thetaMinCms, thetaMaxCms);
   primGen->AddGenerator(twoBody);

   run->SetGenerator(primGen);
   run->SetStoreTraj(kFALSE);

   Bool_t parameterMerged = kTRUE;
   FairParRootFileIo *parOut = new FairParRootFileIo(parameterMerged);
   parOut->open(parFile.Data());
   rtdb->setOutput(parOut);

   run->Init();
   run->Run(nEvents);

   rtdb->saveOutput();
   parOut->close();

   std::cout << "Simulation-based PRA integration MC output: " << simFile << std::endl;
}
