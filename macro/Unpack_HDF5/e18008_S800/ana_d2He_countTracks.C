
//#include "AtPattern.h"


static Double_t proton_mass = 1.0078250322 * 931.494 - 0.511;
static Double_t proj_mass = 14.008596359 * 931.494 - 0.511*8.;
static Double_t target_mass = 2.01410177812 * 931.494;
static Double_t recoil_mass = 14.00307400443 * 931.494 - 0.511*7.;
static Double_t he2_mass = 2.0 * proton_mass;
static Double_t Ekin_proj = 105.16 * 14.008596359;//100.0
// static Double_t decay_frag_mass = 14.00307400443*931.494/1000;//GeV/c^2
//static Double_t decay_frag_mass = 12.*931.494/1000;//GeV/c^2
static Double_t decay_frag_mass = 13.0033548352*931.494/1000;//GeV/c^2 13C
//static Double_t decay_frag_mass = 13.005738609*931.494/1000;//GeV/c^2 13N
//static Double_t decay_frag_mass = 10.012936862*931.494/1000;//GeV/c^2

static Int_t nbTracksPerVtx=2;


TSpline3 *splineEloss;
using XYZVector = ROOT::Math::XYZVector;
using XYZPoint = ROOT::Math::XYZPoint;
Double_t aDVel[300];

std::pair<Double_t,Double_t> GetThetaPhi(AtTrack track, XYZVector vertex, XYZVector maxPos, Int_t zdir)
{
	std::pair<Double_t,Double_t> thetaPhi;
  std::vector<Double_t> par;
  par = track.GetPattern()->GetPatternPar();
  XYZVector vp(TMath::Sign(1,maxPos.X())*fabs(par[3]), TMath::Sign(1,maxPos.Y())*fabs(par[4]), zdir*TMath::Sign(1,(maxPos.Z()-vertex.Z()))*fabs(par[5]));//works with simu
	thetaPhi.first = vp.Theta();
	thetaPhi.second = vp.Phi();
	return thetaPhi;
}

std::pair<Double_t,Double_t> GetThetaPhi(XYZVector vertex, XYZVector maxPos)
{
	std::pair<Double_t,Double_t> thetaPhi;
  XYZVector vp=maxPos-vertex;
	thetaPhi.first = vp.Theta();
	thetaPhi.second = vp.Phi();
	return thetaPhi;
}

Double_t FindAngleBetweenTracks(XYZVector vec1, XYZVector vec2)
{
  Double_t ang = acos(vec1.Dot(vec2)/(sqrt(vec1.Mag2())*sqrt(vec2.Mag2())));
  return ang;
}

//returns the projection of a point on a parametric line
//dir is the direction of the parametric line, posOn is a point of the line, posOut is the point that will be projected
XYZVector ptOnLine(std::vector<Double_t> par, XYZVector posOut)
{
        XYZVector result(-999,-999,-999);
				XYZVector posOn(par[0],par[1],par[2]);
				XYZVector dir(par[3],par[4],par[5]);
        XYZVector vop1 = ((dir.Cross(posOut-posOn)).Cross(dir)).Unit();
        Double_t paraVar1 = posOut.Dot(dir.Unit())-posOn.Dot(dir.Unit());
        Double_t paraVar2 = posOn.Dot(vop1)-posOut.Dot(vop1);
        XYZVector vInter1 = posOn + dir.Unit()*paraVar1;
        XYZVector vInter2 = posOut + vop1*paraVar2;
        if((vInter1-vInter2).Mag2()<1e-10) result=vInter1;
        return result;
}

void SetDVelArray(){
	TString fileName = "utils/drift_vel_cal_vtxZ_FermiFit.txt";
	ifstream fDVel(fileName);
	Int_t l1=0;
	Double_t l2=0;
	for (string line; getline(fDVel, line);) {
	  stringstream parse_die(line);
	  parse_die >> l1 >> l2;
	  aDVel[l1] = l2;
	}
	fDVel.close();
}


void SetERtable(){//fit of the GEANT4 E vs R obtained from the simulation with the function model given by LISE++
                ifstream fER("eLossTables/p_in_d_530torr_SRIM.txt");//from SRIM++,
                Double_t l1=0, l2=0;
                vector <vector<Double_t>> Energy_Range;

                for (string line; getline(fER, line);) {
                        stringstream parse_die(line);
                        vector<Double_t> iRE;
                        parse_die >> l1 >> l2 ;
                        iRE.push_back(l1);//E in MeV
                        iRE.push_back(l2);//mm
                        Energy_Range.push_back(iRE);
                }
                fER.close();
                Int_t v_size = Energy_Range.size();
                Double_t X[v_size];
                Double_t Y[v_size];
                for(Int_t i=0; i<v_size; i++){
                        X[i]=Energy_Range.at(i).at(0)*1.;//0.98
                        Y[i]=Energy_Range.at(i).at(1)*1.;
                        //cout<<X[i]<<" "<<Y[i]<<endl;
                }
                //splineEloss = new TGraph(v_size,Y,X);
		splineEloss = new TSpline3("ElossRange", Y, X, v_size);
}

XYZVector ClosestPoint2Lines(std::vector<Double_t> par1, std::vector<Double_t> par2, Int_t nHits1, Int_t nHits2)
{
	XYZVector p1(par1[0], par1[1], par1[2] );//p1
	XYZVector e1(par1[3], par1[4], par1[5] );//d1
	XYZVector p2(par2[0], par2[1], par2[2] );//p2
	XYZVector e2(par2[3], par2[4], par2[5] );//d2
  XYZVector n1 = e1.Cross(e2.Cross(e1));
  XYZVector n2 = e2.Cross(e1.Cross(e2));
  double t1 = (p2-p1).Dot(n2)/(e1.Dot(n2));
  double t2 = (p1-p2).Dot(n1)/(e2.Dot(n1));
  XYZVector c1 = p1 + t1*e1;
  XYZVector c2 = p2 + t2*e2;
	Double_t w1 = (Double_t)nHits1/(nHits1+nHits2);
	Double_t w2 = (Double_t)nHits2/(nHits1+nHits2);
  XYZVector meanpoint;
	XYZVector meanpoint1 = w1*c1+w2*c2;
	XYZVector meanpoint2 = 0.5*(c1+c2);
	if((nHits1>8 && nHits2>8) && (nHits1<50 || nHits2<50)) meanpoint = meanpoint1;//if sufficient number of hits use the not weighted average
	else meanpoint = meanpoint2;
  return meanpoint;

}

void ana_d2He_countTracks(Int_t runNumber)
{

  SetERtable();
	SetDVelArray();

  FairRunAna* run = new FairRunAna(); //Forcing a dummy run
  //ATd2HeAnalysis *d2heana = new ATd2HeAnalysis ();

  //TString digiFileName = "/mnt/analysis/e18008/rootMerg/giraud/run_2271_0271_test15.root";
	TString digiFileName = TString::Format("/mnt/analysis/e18008/rootMerg/giraud/run_2%03d_%04d_test15.root",runNumber, runNumber);
  TFile* file = new TFile(digiFileName,"READ");
  TTree* tree = (TTree*) file -> Get("cbmsim");
  Int_t nEvents = tree -> GetEntries();
  std::cout << " Number of events : " << nEvents << std::endl;

	S800Calc *s800cal = new S800Calc();
	TBranch *bS800cal = tree->GetBranch("s800cal");
	bS800cal->SetAddress(&s800cal);

  TTreeReader reader("cbmsim", file);
  TTreeReaderValue<TClonesArray> patternArray(reader, "AtPatternEvent");
	TTreeReaderValue<TClonesArray> eventArray(reader, "AtEventH");

  TFile* outfile;
  //TString  outFileNameHead = "ana_d2He_test_271.root";
	TString  outFileNameHead = TString::Format("ana_d2He_test_%04d_14N_nbTracks.root",runNumber);
  outfile   = TFile::Open(outFileNameHead,"recreate");

	S800Ana s800Ana;


  //-----
  Int_t ivt = 0, irun=runNumber, NVtxEvt=0, NTracksVtx=0, stopInside=0;
  vector<Float_t> theta, phi, angle12, range_p, charge;
  vector<Float_t> MaxR, MaxZ, lastX, lastY, lastZ;
	Float_t vertexX, vertexY, vertexZ;

	ULong64_t S800_timeStamp=0;
	Double_t S800_timeRf=0.,S800_x0=0.,S800_x1=0.,S800_y0=0.,S800_y1=0.,S800_E1up=0.,S800_E1down=0.,
  S800_hodoSum=0.,S800_afp=0.,S800_bfp=0.,S800_ata=0.,S800_bta=0.,S800_yta=0.,S800_dta=0.,
	S800_thetaLab=0.,S800_phi=0.,S800_E1up_ToF=0.,S800_E1down_ToF=0.,S800_E1_ToF=0.,S800_XfObj_ToF=0.,
	S800_ObjCorr_ToF=0., S800_Obj_ToF=0., S800_XfObjCorr_ToF=0., S800_ICSum_dE=0.;

  XYZVector beamDir(-0.00915252,-0.00221017,0.9999557);

//---- Set S800Ana -------------------------------------------------------------
	vector<TString> fcutPID1File;
	vector<TString> fcutPID2File;
	vector<TString> fcutPID3File;

	// fcutPID1File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/XfObjObj_run115.root");
	// fcutPID2File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/ICSumObj_run115.root");
	fcutPID1File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/XfObj14O.root");
	fcutPID2File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/afpx.root");
	// fcutPID3File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/13N.root");
	// fcutPID3File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/13C.root");
	 fcutPID3File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/14N.root");
	 // fcutPID3File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/12C.root");
	 // fcutPID3File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/10B.root");
	 std::vector<Double_t> S800MTDCObjCorr;
	 S800MTDCObjCorr.push_back(70.);
	 S800MTDCObjCorr.push_back(0.0085);
	 std::vector<Double_t> S800MTDCObjRange;
	 S800MTDCObjRange.push_back(-120);
	 S800MTDCObjRange.push_back(-20);
	 std::vector<Double_t> S800MTDCXfRange;
	 S800MTDCXfRange.push_back(160);
	 S800MTDCXfRange.push_back(240);

	s800Ana.SetPID1cut(fcutPID1File);
	s800Ana.SetPID2cut(fcutPID2File);
	s800Ana.SetPID3cut(fcutPID3File);
	s800Ana.SetMTDCXfRange(S800MTDCXfRange);
	s800Ana.SetMTDCObjRange(S800MTDCObjRange);
	s800Ana.SetTofObjCorr(S800MTDCObjCorr);
	//----------------------------------------------------------------------------

  TTree *anatree = new TTree("anatree","new TTree");

	anatree->Branch("ivt",&ivt);
	anatree->Branch("irun",&irun);
	anatree->Branch("NVtxEvt",&NVtxEvt);
	anatree->Branch("NTracksVtx",&NTracksVtx);
  anatree->Branch("range_p",&range_p);
	anatree->Branch("charge",&charge);
  anatree->Branch("theta",&theta);
  anatree->Branch("phi",&phi);
  anatree->Branch("lastX",&lastX);
  anatree->Branch("lastY",&lastY);
  anatree->Branch("lastZ",&lastZ);
  anatree->Branch("vertexX",&vertexX);
  anatree->Branch("vertexY",&vertexY);
  anatree->Branch("vertexZ",&vertexZ);
  anatree->Branch("angle12",&angle12);
  anatree->Branch("stopInside",&stopInside);

	anatree->Branch("S800_XfObj_ToF",&S800_XfObj_ToF);
	anatree->Branch("S800_ObjCorr_ToF",&S800_ObjCorr_ToF);
	anatree->Branch("S800_ICSum_dE",&S800_ICSum_dE);


///============================= Event loop ====================================
    std::cout << " nEvents : " << nEvents << "\n";
  for(Int_t i=0;i<nEvents;i++){
		s800cal->Clear();
		bS800cal->GetEntry(i);
    reader.Next();

		// Bool_t isInPIDGates = kFALSE;
 	 	// isInPIDGates = s800Ana.isInPID(s800cal);

	//	if(!isInPIDGates) continue;

    std::cout << " Event Number : " << i << "\n";


//---- Get S800 data -----------------------------------------------------------
		s800Ana.Calc(s800cal);
		S800_XfObj_ToF = s800Ana.GetXfObj_ToF();
		S800_ObjCorr_ToF = s800Ana.GetObjCorr_ToF();
		S800_ICSum_dE = s800Ana.GetICSum_E();
		std::vector<Double_t> S800_fpVar;//[0]=fX0 | [1]=fX1 | [2]=fY0 | [3]=fY1 | [4]=fAfp | [5]=fBfp
		S800_fpVar = s800Ana.GetFpVariables();

//------------------------------------------------------------------------------


    AtPatternEvent *patternEvent = (AtPatternEvent *)patternArray->At(0);

    if (patternEvent) {

       std::vector<AtTrack> &patternTrackCand = patternEvent->GetTrackCand();
       //std::vector<std::vector<Float_t>> lines;
			 /*
       std::cout << " Number of pattern tracks " << patternTrackCand.size() << "\n";
       for (auto track : patternTrackCand) {
         auto ransacLine = dynamic_cast<const AtPatterns::AtPatternLine *>(track.GetPattern());
         std::vector<Float_t> patternPar;
         patternPar = ransacLine->GetPatternPar();
         for (auto par : patternPar) {std::cout << " ini pattern par. : " << par << "\n";}
         //lines.push_back(patternPar);
       }
			 */

       AtFindVertex findVtx(9);
       findVtx.FindVertex(patternTrackCand,nbTracksPerVtx);
       std::vector<tracksFromVertex> tv;
       tv = findVtx.GetTracksVertex();
			 NVtxEvt = tv.size();//number of clusters of tracks forming a vertex (could have ex: 2 vertexes with 2 tracks each)
			 NTracksVtx = 0;//number of tracks for each vertex

       for (size_t ive = 0; ive < NVtxEvt; ive++) {
        std::cout<<"ive "<<ive<<" "<<tv.at(ive).vertex.X()<<" "<<tv.at(ive).vertex.Y()<<" "<<tv.at(ive).vertex.Z()<<" "<<tv.at(ive).tracks.at(0).GetGeoQEnergy()<<" "<<tv.at(ive).tracks.at(1).GetGeoQEnergy()<<std::endl;

				NTracksVtx = tv.at(ive).tracks.size();
				// if(NTracksVtx!=2) continue; //don't analyze event with other than 2 tracks per vertex

				theta.clear(); phi.clear(); range_p.clear(); //reset variables
    		MaxR.clear(); MaxZ.clear(); charge.clear();
				lastX.clear(); lastY.clear(); lastZ.clear();
				stopInside = 0;

				XYZVector vertexMean = tv.at(ive).vertex;
				vertexX = vertexMean.X();
				vertexY = vertexMean.Y();
				vertexZ = vertexMean.Z();
				vector<XYZVector> lastPoint;

				Int_t bCharge = 0, bMaxR = 0, bMaxZ = 0, bVertexMean = 0;

				for (size_t itv = 0; itv < NTracksVtx; itv++) {
			    lastPoint.push_back((XYZVector)tv.at(ive).tracks.at(itv).GetLastPoint());
			    MaxR.push_back(lastPoint.at(itv).Rho());
			    MaxZ.push_back(lastPoint.at(itv).Z());
					lastX.push_back(lastPoint.at(itv).X());
					lastY.push_back(lastPoint.at(itv).Y());
					lastZ.push_back(lastPoint.at(itv).Z());
					charge.push_back(tv.at(ive).tracks.at(itv).GetGeoQEnergy());
					theta.push_back(GetThetaPhi(vertexMean, lastPoint.at(itv)).first);
			    phi.push_back(GetThetaPhi(vertexMean, lastPoint.at(itv)).second);
    			range_p.push_back(tv.at(ive).tracks.at(itv).GetLinearRange((XYZPoint)vertexMean,(XYZPoint)lastPoint.at(itv)));

					if(MaxR.back()<245 && MaxR.back()>35)
						bMaxR += 1;
					if(charge.back()>5e3)
						bCharge += 1;
					if(MaxZ.back()<975 && MaxZ.back()>25)
						bMaxZ += 1;
					if(vertexZ<975 && vertexZ>25)
						bVertexMean += 1;
		// if(charge1<5e3 || charge2<5e3 || MaxR1>245. || MaxR2>245. || MaxR1<35. || MaxR2<35. || MaxZ1>975. || MaxZ2>975. || MaxZ1<25. || MaxZ2<25. || vertexMean.Z()<25. || vertexMean.Z()>975.) continue;
				}

		if ( bCharge==NTracksVtx && bMaxR==NTracksVtx && bMaxZ==NTracksVtx && bVertexMean==NTracksVtx )
			stopInside = 1;

    ivt=i;
    anatree->Fill();
  } //for tv size (ive)
}//RANSAC null pointer
}// Event loop


/// --------------------- End event loop ---------------------------------------
outfile->cd();
anatree->Write();

//	tracks_z_r->Write ();


outfile->Close();

} //end main
