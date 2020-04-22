//Code to simulate fission event

#include "FairEventManager.h"
#include "FairMCTracks.h"
#include "FairMCPointDraw.h"

void eventDisplay(TString fileName="PbFission_sim")
{

  TString dir = getenv("VMCWORKDIR");

  //Form file names
  TString  InputFile = TString::Format("./data/%s.root", fileName.Data());
  TString  ParFile   = TString::Format("./data/%sPar.root", fileName.Data());
  TString  OutFile   = TString::Format("./data/%sOut.root", fileName.Data());

  //For some reason this line needs to be here for ROOT to load in all of the required libraries?
  ATVertexPropagator* vertex_prop = new ATVertexPropagator();

  // -----   Create Reconstruction run   ----------------------------------------
  FairRunAna* fRun = new FairRunAna();
  fRun->SetInputFile(InputFile.Data());
  fRun->SetOutputFile(OutFile.Data());


  FairRuntimeDb* rtdb = fRun->GetRuntimeDb();
  FairParRootFileIo* parInput = new FairParRootFileIo();
  parInput->open(ParFile.Data());
  rtdb->setOutput(parInput);

  
  FairEventManager *fMan= new FairEventManager();

  //----------------------Traks and points -------------------------------------
  FairMCTracks    *Track     = new FairMCTracks("Monte-Carlo Tracks");
  FairMCPointDraw *AtTpcPoints = new FairMCPointDraw("AtTpcPoint", kBlue, kFullSquare);

  fMan->AddTask(Track);
  fMan->AddTask(AtTpcPoints);


  fMan->Init();

}
