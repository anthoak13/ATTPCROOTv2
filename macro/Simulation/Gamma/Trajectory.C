void Trajectory()
{
   //-----User Settings:-----------------------------------------------
   TString InputFile = "data/PxCT_test.root";
   TString ParFile = "data/PxCTpar.root";
   TString OutFile = "data/PxCT_out.root";
TString unpackDir = "Simulation/Gamma/";
	TString dir = getenv("VMCWORKDIR");
   TString geoFile = "PxCT_geomanager.root";
TString GeoDataPath = dir + "/geometry/" + geoFile;
TString InputDataPath = dir + "/macro/" + unpackDir + InputFile;
   TString OutputDataPath = dir + "/macro/" + unpackDir + OutFile;
   // -----   Reconstruction run   -------------------------------------------
  FairRunAna *fRun = new FairRunAna();
   FairRootFileSink *sink = new FairRootFileSink(OutputDataPath);
   FairFileSource *source = new FairFileSource(InputDataPath);
   fRun->SetSource(source);
   fRun->SetSink(sink);
   fRun->SetGeomFile(GeoDataPath);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParRootFileIo *parInput1 = new FairParRootFileIo();
   parInput1->open(ParFile.Data());
   rtdb->setFirstInput(parInput1);

   FairEventManager *fMan = new FairEventManager();

   //----------------------Traks and points -------------------------------------
   //FairMCTracks *Track = new FairMCTracks("Monte-Carlo Tracks");
   FairMCPointDraw *AtPxCTPoints = new FairMCPointDraw("AtMCPoint", kRed, kFullSquare);

   //fMan->AddTask(Track);
   fMan->AddTask(AtPxCTPoints);

   fMan->Init();
}
