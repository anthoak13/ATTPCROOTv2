#include <unistd.h>

void read_simAnaFile_example()
{

	FairRunAna* run = new FairRunAna(); //Forcing a dummy run

	TString digiFileName = TString::Format("/mnt/analysis/e18008/rootAna/giraud/simulation/digi/attpcdigi_d2He_1000_ran_2.root");
	TFile* file = new TFile(digiFileName,"READ");
	TTree* tree = (TTree*) file -> Get("cbmsim");
	// Int_t nEvents = tree -> GetEntries();

	// TChain* inputdata = new TChain("cbmsim");
	// inputdata->Add(digiFileName);
	// inputdata->LoadTree(0);
	//inputdata->Print();
	//std::cout<<inputdata->GetListOfFiles()<<std::endl;
	// TTreeReader inputfilereader(inputdata);

	TTreeReader reader("cbmsim", file);
	// TTreeReaderValue<TClonesArray> ransacArray(reader, "ATRansac");
	TTreeReaderValue<TClonesArray> eventArray(reader, "ATEventH");
	// TTreeReaderValue<TClonesArray> eventArray(reader, "ATRawEvent");
	// TTreeReaderValue<TClonesArray> SimPointArray(reader, "AtTpcPoint");


	/// --------------------- Event loop -------------------------------------------
	for(Int_t i=0;i<100;i++){ reader.Next(); ATEvent* event = (ATEvent*) eventArray->At(0); std::cout<<event->GetNumHits()<<std::endl;}	
	// for(Int_t i=0;i<nEvents;i++){
	// for(Int_t i=0;i<100;i++){
	// 	reader.Next();
	//
	// 	ATEvent* event = (ATEvent*) eventArray->At(0);
	// 	Int_t nHitsEvent = event->GetNumHits();
	//
	// 	std::cout<<" "<<i<<"  "<<nHitsEvent<<std::endl;
	//
	// 	// get information from geant4
	// 	// Int_t Npoints = SimPointArray -> GetEntries();
	// 	// Double_t rmax[8]={0};
	// 	// Double_t zmax[8]={0};
	// 	// Double_t phimax[8]={0};
	// 	// Double_t thetamax[8]={0};
	// 	// // TVector3 vtxG4[8]={(0,0,0)};
	// 	//
	// 	// for(Int_t ipoint=0; ipoint<Npoints; ipoint++) {
	// 	// 	AtTpcPoint* point = (AtTpcPoint*) SimPointArray->At(ipoint);
	// 	// 	TString VolName=point->GetVolName();
	// 	// 	//std::cout<<" Volume Name : "<<VolName<<std::endl;
	// 	//
	// 	// 	Int_t trackID = point -> GetTrackID();
	// 	// 	//	std::cout<<" Track ID : "<<trackID<<std::endl;
	// 	// 	if(i%2!=0 && trackID>0 && trackID<8 && VolName=="drift_volume"){
	// 	// 		if(rmax[trackID]<sqrt(pow(point->GetXOut(),2) + pow(point->GetYOut(),2))) {
	// 	// 			rmax[trackID]=sqrt(pow(point->GetXOut(),2) + pow(point->GetYOut(),2));
	// 	// 			zmax[trackID]=100.-point->GetZIn()-4.;//-4 or a shift observed... wrong parameters probably
	// 	// 			thetamax[trackID]=atan( sqrt(pow(point->GetPxOut(),2) + pow(point->GetPyOut(),2))/(-point->GetPzOut()));
	// 	// 			phimax[trackID]=atan(point->GetXOut()/point->GetYOut());
	// 	// 			if(trackID==2)std::cout<<"TRACKS2 "<<trackID<<" "<<rmax[trackID]*10.<<" "<<phimax[trackID]<<" "<<thetamax[trackID]<<std::endl;
	// 	// 		}
	// 	// 	}
	// 	// }
	//
	//
	// 	// for(Int_t iHit=0; iHit<nHitsEvent; iHit++){
	// 	// 	ATHit hit = event->GetHit(iHit);
	// 	// 	TVector3 position = hit.GetPosition();
	// 	// 	Double_t distWlinePt = distPtLine(fitPar1,position);
	// 	// 	if(distWlinePt<30.0){
	// 	// 		if(range_p1>130 && (position-vertexMean).Mag()>0.66*range_p1){
	// 	// 			avgDistWline1+=distWlinePt;
	// 	// 			nDistWline1++;
	// 	// 			trackProfile->Fill(distWlinePt);
	// 	// 		}
	// 	// 	}
	// 	// }
	//
	//
	// } //end event loop
}
