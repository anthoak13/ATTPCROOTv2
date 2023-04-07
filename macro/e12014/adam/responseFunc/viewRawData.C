#ifndef __CLING__

#include "AtTabMain.h"
#include "AtTabPads.h"
#include "AtTpcMap.h"
#include "AtViewerManager.h"

#include <FairFileSource.h>
#include <FairLogger.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>

#include <TString.h>

#include <iostream>
#endif // __CINT__

void viewRawData(TString OutputDataFile = "./data/output.reco_display.root")
{
   TString InputDataFile = "/mnt/analysis/e12014/huntc/newFork/ATTPCROOTv2/code/unpacking/data/pulser/run_0036.root";
   std::cout << "Opening: " << InputDataFile << std::endl;

   TString dir = getenv("VMCWORKDIR");
   TString geoFile = "ATTPC_v1.1_geomanager.root";
   TString mapFile = "e12014_pad_mapping.xml";

   TString InputDataPath = InputDataFile;
   TString OutputDataPath = OutputDataFile;
   TString GeoDataPath = dir + "/geometry/" + geoFile;
   TString mapDir = dir + "/scripts/" + mapFile;

   FairRunAna *fRun = new FairRunAna();
   FairRootFileSink *sink = new FairRootFileSink(OutputDataFile);
   FairFileSource *source = new FairFileSource(InputDataFile);
   fRun->SetSource(source);
   fRun->SetSink(sink);
   fRun->SetGeomFile(GeoDataPath);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParRootFileIo *parIo1 = new FairParRootFileIo();
   // parIo1->open("param.dummy.root");
   rtdb->setFirstInput(parIo1);

   auto fMap = std::make_shared<AtTpcMap>();
   fMap->ParseXMLMap(mapDir.Data());
   AtViewerManager *eveMan = new AtViewerManager(fMap);

   auto tabMain = std::make_unique<AtTabMain>();
   tabMain->SetMultiHit(100); // Set the maximum number of multihits in the visualization
   eveMan->AddTab(std::move(tabMain));
   auto tabPads = std::make_unique<AtTabPad>(1, 2);
   tabPads->DrawRawADC(0, 0);
   tabPads->DrawFPN(0, 1);
   eveMan->AddTab(std::move(tabPads));

   eveMan->Init();

   std::cout << "Finished init" << std::endl;
   // eveMan->RunEvent(27);
}
