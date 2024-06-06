
//#include "AtPattern.h"

static Double_t proton_mass = 1.0078250322 * 931.494 - 0.511;
static Double_t proj_mass = 14.008596359 * 931.494 - 0.511 * 8.;
static Double_t target_mass = 2.01410177812 * 931.494;
static Double_t recoil_mass = 14.00307400443 * 931.494 - 0.511 * 7.;
static Double_t he2_mass = 2.0 * proton_mass;
static Double_t Ekin_proj = 105.16 * 14.008596359; // 100.0
// static Double_t decay_frag_mass = 14.00307400443*931.494/1000;//GeV/c^2
// static Double_t decay_frag_mass = 12.*931.494/1000;//GeV/c^2
static Double_t decay_frag_mass = 13.0033548352 * 931.494 / 1000; // GeV/c^2 13C
// static Double_t decay_frag_mass = 13.005738609*931.494/1000;//GeV/c^2 13N
// static Double_t decay_frag_mass = 10.012936862*931.494/1000;//GeV/c^2

static Int_t nbTracksPerVtx = 2;

TSpline3 *splineEloss;
using XYZVector = ROOT::Math::XYZVector;
using XYZPoint = ROOT::Math::XYZPoint;
Double_t aDVel[300];

std::pair<Double_t, Double_t> GetThetaPhi(AtTrack track, XYZVector vertex, XYZVector maxPos, Int_t zdir)
{
   std::pair<Double_t, Double_t> thetaPhi;
   std::vector<Double_t> par;
   par = track.GetPattern()->GetPatternPar();
   XYZVector vp(TMath::Sign(1, maxPos.X()) * fabs(par[3]), TMath::Sign(1, maxPos.Y()) * fabs(par[4]),
                zdir * TMath::Sign(1, (maxPos.Z() - vertex.Z())) * fabs(par[5])); // works with simu
   thetaPhi.first = vp.Theta();
   thetaPhi.second = vp.Phi();
   return thetaPhi;
}

std::pair<Double_t, Double_t> GetThetaPhi(XYZVector vertex, XYZVector maxPos)
{
   std::pair<Double_t, Double_t> thetaPhi;
   XYZVector vp = maxPos - vertex;
   thetaPhi.first = vp.Theta();
   thetaPhi.second = vp.Phi();
   return thetaPhi;
}

Double_t FindAngleBetweenTracks(XYZVector vec1, XYZVector vec2)
{
   Double_t ang = acos(vec1.Dot(vec2) / (sqrt(vec1.Mag2()) * sqrt(vec2.Mag2())));
   return ang;
}

// returns the projection of a point on a parametric line
// dir is the direction of the parametric line, posOn is a point of the line, posOut is the point that will be projected
XYZVector ptOnLine(std::vector<Double_t> par, XYZVector posOut)
{
   XYZVector result(-999, -999, -999);
   XYZVector posOn(par[0], par[1], par[2]);
   XYZVector dir(par[3], par[4], par[5]);
   XYZVector vop1 = ((dir.Cross(posOut - posOn)).Cross(dir)).Unit();
   Double_t paraVar1 = posOut.Dot(dir.Unit()) - posOn.Dot(dir.Unit());
   Double_t paraVar2 = posOn.Dot(vop1) - posOut.Dot(vop1);
   XYZVector vInter1 = posOn + dir.Unit() * paraVar1;
   XYZVector vInter2 = posOut + vop1 * paraVar2;
   if ((vInter1 - vInter2).Mag2() < 1e-10)
      result = vInter1;
   return result;
}

void SetDVelArray()
{
   TString fileName = "utils/drift_vel_cal_vtxZ_FermiFit.txt";
   ifstream fDVel(fileName);
   Int_t l1 = 0;
   Double_t l2 = 0;
   for (string line; getline(fDVel, line);) {
      stringstream parse_die(line);
      parse_die >> l1 >> l2;
      aDVel[l1] = l2;
   }
   fDVel.close();
}

void SetERtable()
{ // fit of the GEANT4 E vs R obtained from the simulation with the function model given by LISE++
   ifstream fER("eLossTables/p_in_d_530torr_SRIM.txt"); // from SRIM++,
   Double_t l1 = 0, l2 = 0;
   vector<vector<Double_t>> Energy_Range;

   for (string line; getline(fER, line);) {
      stringstream parse_die(line);
      vector<Double_t> iRE;
      parse_die >> l1 >> l2;
      iRE.push_back(l1); // E in MeV
      iRE.push_back(l2); // mm
      Energy_Range.push_back(iRE);
   }
   fER.close();
   Int_t v_size = Energy_Range.size();
   Double_t X[v_size];
   Double_t Y[v_size];
   for (Int_t i = 0; i < v_size; i++) {
      X[i] = Energy_Range.at(i).at(0) * 1.; // 0.98
      Y[i] = Energy_Range.at(i).at(1) * 1.;
      // cout<<X[i]<<" "<<Y[i]<<endl;
   }
   // splineEloss = new TGraph(v_size,Y,X);
   splineEloss = new TSpline3("ElossRange", Y, X, v_size);
}

XYZVector ClosestPoint2Lines(std::vector<Double_t> par1, std::vector<Double_t> par2, Int_t nHits1, Int_t nHits2)
{
   XYZVector p1(par1[0], par1[1], par1[2]); // p1
   XYZVector e1(par1[3], par1[4], par1[5]); // d1
   XYZVector p2(par2[0], par2[1], par2[2]); // p2
   XYZVector e2(par2[3], par2[4], par2[5]); // d2
   XYZVector n1 = e1.Cross(e2.Cross(e1));
   XYZVector n2 = e2.Cross(e1.Cross(e2));
   double t1 = (p2 - p1).Dot(n2) / (e1.Dot(n2));
   double t2 = (p1 - p2).Dot(n1) / (e2.Dot(n1));
   XYZVector c1 = p1 + t1 * e1;
   XYZVector c2 = p2 + t2 * e2;
   Double_t w1 = (Double_t)nHits1 / (nHits1 + nHits2);
   Double_t w2 = (Double_t)nHits2 / (nHits1 + nHits2);
   XYZVector meanpoint;
   XYZVector meanpoint1 = w1 * c1 + w2 * c2;
   XYZVector meanpoint2 = 0.5 * (c1 + c2);
   if ((nHits1 > 8 && nHits2 > 8) && (nHits1 < 50 || nHits2 < 50))
      meanpoint = meanpoint1; // if sufficient number of hits use the not weighted average
   else
      meanpoint = meanpoint2;
   return meanpoint;
}

void ana_d2He(Int_t runNumber)
{

   SetERtable();
   SetDVelArray();

   FairRunAna *run = new FairRunAna(); // Forcing a dummy run
   // ATd2HeAnalysis *d2heana = new ATd2HeAnalysis ();

   // TString digiFileName = "/mnt/analysis/e18008/rootMerg/giraud/run_2271_0271_test15.root";
   TString digiFileName =
      TString::Format("/mnt/analysis/e18008/rootMerg/giraud/run_2%03d_%04d_test15.root", runNumber, runNumber);
   TFile *file = new TFile(digiFileName, "READ");
   TTree *tree = (TTree *)file->Get("cbmsim");
   Int_t nEvents = tree->GetEntries();
   std::cout << " Number of events : " << nEvents << std::endl;

   S800Calc *s800cal = new S800Calc();
   TBranch *bS800cal = tree->GetBranch("s800cal");
   bS800cal->SetAddress(&s800cal);

   TTreeReader reader("cbmsim", file);
   TTreeReaderValue<TClonesArray> patternArray(reader, "AtPatternEvent");
   TTreeReaderValue<TClonesArray> eventArray(reader, "AtEventH");

   TFile *outfile;
   TString outFileNameHead = TString::Format("ana_d2He_test_%04d_14N_test.root", runNumber);
   outfile = TFile::Open(outFileNameHead, "recreate");

   S800Ana s800Ana;

   //-----
   Int_t ivt = 0, irun = runNumber, NVtxEvt = 0, NTracksVtx = 0;
   Float_t range_p1 = 0., range_p2 = 0., charge1 = 0., charge2 = 0.;
   Float_t eLoss_p1_reco = 0.0, eLoss_p2_reco = 0.0;
   Float_t epsilon_pp = -999;
   Float_t theta1 = 0., theta2 = 0., phi1 = 0., phi2 = 0., angle12 = 0.;
   Float_t mom1_norm_reco = 0., mom2_norm_reco = 0.;
   Float_t E_tot_he2 = 0., he2_mass_ex = 0.;
   Float_t kin_He2 = 0., theta_He2 = 0., kin_He2_same = 0., theta_He2_same = 0., phi_He2 = 0.;
   Float_t theta_cm = 0., Ex4 = 0., Ex_reco_same = 0.;
   Float_t lastX1 = 0., lastX2 = 0., lastY1 = 0., lastY2 = 0., lastZ1 = 0., lastZ2 = 0., vertexX = 0., vertexY = 0.,
           vertexZ = 0.;
   Float_t MaxR1, MaxR2, MaxZ1, MaxZ2;
   Double_t theta_lab = 0;
   Double_t Eje_ata = -999, Eje_bta = -999, Eje_dta = -999;
   Double_t brho = -999;
   Double_t holeAcc = 0, dtaAcc = 0;

   XYZVector mom_proton1_reco, mom_proton2_reco, mom_He2_reco;

   ULong64_t S800_timeStamp = 0;
   Double_t S800_timeRf = 0., S800_x0 = 0., S800_x1 = 0., S800_y0 = 0., S800_y1 = 0., S800_E1up = 0., S800_E1down = 0.,
            S800_hodoSum = 0., S800_afp = 0., S800_bfp = 0., S800_ata = 0., S800_bta = 0., S800_yta = 0., S800_dta = 0.,
            S800_thetaLab = 0., S800_phi = 0., S800_E1up_ToF = 0., S800_E1down_ToF = 0., S800_E1_ToF = 0.,
            S800_XfObj_ToF = 0., S800_ObjCorr_ToF = 0., S800_Obj_ToF = 0., S800_XfObjCorr_ToF = 0., S800_ICSum_dE = 0.;

   XYZVector beamDir(-0.00915252, -0.00221017, 0.9999557);

   //---- Set S800Ana -------------------------------------------------------------
   TString mapPath = "/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/invMap/invmap_14N";
   s800Ana.SetInverseMap(mapPath, -0.5, 1, 0.1);
   
   vector<TString> fcutPID1File;
   vector<TString> fcutPID2File;
   vector<TString> fcutPID3File;

   // fcutPID1File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/XfObjObj_run115.root");
   // fcutPID2File.push_back("/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/ICSumObj_run115.root");
   fcutPID1File.push_back(
      "/projects/ceclub/giraud/git/ATTPCROOTv2/macro/Unpack_HDF5/e18008_S800/rootPID/XfObj14O.root");
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

   TTree *anatree = new TTree("anatree", "new TTree");

   anatree->Branch("ivt", &ivt);
   anatree->Branch("irun", &irun);
   anatree->Branch("NVtxEvt", &NVtxEvt);
   anatree->Branch("NTracksVtx", &NTracksVtx);
   anatree->Branch("range_p1", &range_p1);
   anatree->Branch("range_p2", &range_p2);
   anatree->Branch("charge1", &charge1);
   anatree->Branch("charge2", &charge2);
   anatree->Branch("theta1", &theta1);
   anatree->Branch("theta2", &theta2);
   anatree->Branch("phi1", &phi1);
   anatree->Branch("phi2", &phi2);
   anatree->Branch("lastX1", &lastX1);
   anatree->Branch("lastY1", &lastY1);
   anatree->Branch("lastZ1", &lastZ1);
   anatree->Branch("lastX2", &lastX2);
   anatree->Branch("lastY2", &lastY2);
   anatree->Branch("lastZ2", &lastZ2);
   anatree->Branch("vertexX", &vertexX);
   anatree->Branch("vertexY", &vertexY);
   anatree->Branch("vertexZ", &vertexZ);
   anatree->Branch("angle12", &angle12);
   anatree->Branch("eLoss_p1_reco", &eLoss_p1_reco);
   anatree->Branch("eLoss_p2_reco", &eLoss_p2_reco);
   anatree->Branch("kin_He2", &kin_He2);
   anatree->Branch("theta_He2", &theta_He2);
   anatree->Branch("E_tot_he2", &E_tot_he2);
   anatree->Branch("he2_mass_ex", &he2_mass_ex);
   anatree->Branch("theta_cm", &theta_cm);
   anatree->Branch("theta_lab", &theta_lab);
   anatree->Branch("Ex4", &Ex4);
   anatree->Branch("epsilon_pp", &epsilon_pp);
   anatree->Branch("mom1_norm_reco", &mom1_norm_reco);
   anatree->Branch("mom2_norm_reco", &mom2_norm_reco);
   anatree->Branch("Eje_ata", &Eje_ata);
   anatree->Branch("Eje_bta", &Eje_bta);
   anatree->Branch("Eje_dta", &Eje_dta);
   anatree->Branch("brho", &brho);
   anatree->Branch("holeAcc", &holeAcc);
   anatree->Branch("dtaAcc", &dtaAcc);

   anatree->Branch("S800_XfObj_ToF", &S800_XfObj_ToF);
   anatree->Branch("S800_ObjCorr_ToF", &S800_ObjCorr_ToF);
   anatree->Branch("S800_ICSum_dE", &S800_ICSum_dE);
   anatree->Branch("S800_CRDC0_x",&S800_x0);
   anatree->Branch("S800_CRDC1_x",&S800_x1);
   anatree->Branch("S800_CRDC0_y",&S800_y0);
   anatree->Branch("S800_CRDC1_y",&S800_y1);
   anatree->Branch("S800_afp",&S800_afp);
	anatree->Branch("S800_bfp",&S800_bfp);

   ///============================= Event loop ====================================
   std::cout << " nEvents : " << nEvents << "\n";
   for (Int_t i = 0; i < nEvents; i++) {
      s800cal->Clear();
      bS800cal->GetEntry(i);
      reader.Next();

      Bool_t isInPIDGates = kFALSE;
      isInPIDGates = s800Ana.isInPID(s800cal);

      if (!isInPIDGates)
         continue;

      std::cout << " Event Number : " << i << "\n";

      //---- Get S800 data -----------------------------------------------------------
      S800_XfObj_ToF = s800Ana.GetXfObj_ToF();
      S800_ObjCorr_ToF = s800Ana.GetObjCorr_ToF();
      S800_ICSum_dE = s800Ana.GetICSum_E();
      std::vector<Double_t> S800_fpVar; //[0]=fX0 | [1]=fX1 | [2]=fY0 | [3]=fY1 | [4]=fAfp | [5]=fBfp
      S800_fpVar = s800Ana.GetFpVariables();
      S800_x0 = S800_fpVar.at(0);
      S800_x1 = S800_fpVar.at(1);
      S800_y0 = S800_fpVar.at(2);
      S800_y1 = S800_fpVar.at(3);
      S800_afp = S800_fpVar.at(4);
      S800_bfp = S800_fpVar.at(5);
      //------------------------------------------------------------------------------

      AtPatternEvent *patternEvent = (AtPatternEvent *)patternArray->At(0);

      if (patternEvent) {

         std::vector<AtTrack> &patternTrackCand = patternEvent->GetTrackCand();

         AtFindVertex findVtx(9);
         findVtx.FindVertex(patternTrackCand, nbTracksPerVtx);
         std::vector<tracksFromVertex> tv;
         tv = findVtx.GetTracksVertex();
         NVtxEvt =
            tv.size();   // number of clusters of tracks forming a vertex (could have ex: 2 vertexes with 2 tracks each)
         NTracksVtx = 0; // number of tracks for each vertex

         for (size_t ive = 0; ive < NVtxEvt; ive++) {
            std::cout << "ive " << ive << " " << tv.at(ive).vertex.X() << " " << tv.at(ive).vertex.Y() << " "
                      << tv.at(ive).vertex.Z() << " " << tv.at(ive).tracks.at(0).GetGeoQEnergy() << " "
                      << tv.at(ive).tracks.at(1).GetGeoQEnergy() << std::endl;

            NTracksVtx = tv.at(ive).tracks.size();
            if (NTracksVtx != 2)
               continue; // don't analyze event with other than 2 tracks per vertex

            theta1 = 0.;
            theta2 = 0.;
            phi1 = 0.;
            phi2 = 0.;
            range_p1 = 0.;
            range_p2 = 0.;
            eLoss_p1_reco = 0.;
            eLoss_p2_reco = 0.;
            mom1_norm_reco = 0.;
            mom2_norm_reco = 0.;
            E_tot_he2 = 0.;
            he2_mass_ex = 0.;
            kin_He2 = 0.;
            theta_He2 = 0.;
            phi_He2 = 0.;
            theta_cm = 0.;
            Ex4 = 0.;
            MaxR1 = 0.;
            MaxR2 = 0.;
            MaxZ1 = 0.;
            MaxZ2 = 0.;
            theta_lab = 0;
            charge1 = 0;
            charge2 = 0;

            XYZVector vertexMean = tv.at(ive).vertex;
            XYZVector lastPoint1 = (XYZVector)tv.at(ive).tracks.at(0).GetLastPoint();
            XYZVector lastPoint2 = (XYZVector)tv.at(ive).tracks.at(1).GetLastPoint();
            MaxR1 = lastPoint1.Rho();
            MaxR2 = lastPoint2.Rho();
            MaxZ1 = lastPoint1.Z();
            MaxZ2 = lastPoint2.Z();

            charge1 = tv.at(ive).tracks.at(0).GetGeoQEnergy();
            charge2 = tv.at(ive).tracks.at(1).GetGeoQEnergy();

            // std::cout<<charge1<<" "<<charge2<<std::endl;

            // tracks must stop within the chamber
            if (charge1 < 5e3 || charge2 < 5e3 || MaxR1 > 245. || MaxR2 > 245. || MaxR1 < 35. || MaxR2 < 35. ||
                MaxZ1 > 975. || MaxZ2 > 975. || MaxZ1 < 25. || MaxZ2 < 25. || vertexMean.Z() < 25. ||
                vertexMean.Z() > 975.)
               continue;

            //------------------------------------------------------------------------------
            // refit only the last part of the tracks and do small adjustement for the drift vel.
            std::vector<AtTrack> patternTrackCandReFit;
            for (size_t itv = 0; itv < NTracksVtx; itv++) {
               AtTrack trackToReFit;
               std::vector<AtHit> hitArray = tv.at(ive).tracks.at(itv).GetHitArray();
               for (Int_t iHit = 0; iHit < hitArray.size(); iHit++) {
                  AtHit hit = hitArray.at(iHit);
                  XYZPoint position = hit.GetPosition();
                  // position = position.SetXYZ(position.X(), position.Y(), (1000. / aDVel[irun]) * (position.Z()));
                  position = position.SetXYZ(position.X(), position.Y(), position.Z());
                  hit.SetPosition(position);
                  if (sqrt(pow(position.X(), 2) + pow(position.Y(), 2)) > 25.) { //35
                     trackToReFit.AddHit(hit);
                  }
               }

               auto patternType = AtPatterns::PatternType::kLine;
               auto pattern = AtPatterns::CreatePattern(patternType);
               pattern->FitPattern(trackToReFit.GetHitArray(),
                                   100); // 100 is qThreshold, defined in the unpack macro as well
               trackToReFit.SetPattern(pattern->Clone());
               patternTrackCandReFit.push_back(trackToReFit);
            }

            // find new vertex
            std::vector<tracksFromVertex> tvReFit;
            if (patternTrackCandReFit.size() >= nbTracksPerVtx) {
               AtFindVertex findVtxReFit(9);
               findVtxReFit.FindVertex(patternTrackCandReFit, nbTracksPerVtx);
               tvReFit = findVtxReFit.GetTracksVertex();
            }
            std::cout << "tvReFit.size() "
                      << " " << tvReFit.size() << " " << nbTracksPerVtx << " " << patternTrackCandReFit.size()
                      << std::endl;
            if (tvReFit.size() < 1)
               continue; // tvReFit size should be 1

            vertexMean = (XYZPoint)tvReFit.at(0).vertex;
            // test ----
            auto ransacLine1 = dynamic_cast<const AtPatterns::AtPatternLine *>(tvReFit.at(0).tracks.at(0).GetPattern());
            auto ransacLine2 = dynamic_cast<const AtPatterns::AtPatternLine *>(tvReFit.at(0).tracks.at(1).GetPattern());
            // XYZVector newVertexMean =
            // ClosestPoint2Lines(ransacLine1->GetPatternPar(),ransacLine2->GetPatternPar(),tvReFit.at(0).tracks.at(0).GetHitArray().size(),tvReFit.at(0).tracks.at(1).GetHitArray().size());//weighted
            // vertex vertexMean = newVertexMean;
            //--- test
            lastPoint1 = tvReFit.at(0).tracks.at(0).GetLastPoint();
            lastPoint2 = tvReFit.at(0).tracks.at(1).GetLastPoint();
            XYZVector lastPoint1proj =
               ptOnLine(ransacLine1->GetPatternPar(),
                        lastPoint1); // projection of the last point of the track on the parametric line
            XYZVector lastPoint2proj = ptOnLine(ransacLine2->GetPatternPar(), lastPoint2);
            MaxR1 = lastPoint1proj.Rho();
            MaxR2 = lastPoint2proj.Rho();
            MaxZ1 = lastPoint1proj.Z();
            MaxZ2 = lastPoint2proj.Z();

            // charge1 = tvReFit.at(0).tracks.at(0).GetGeoQEnergy();
            // charge2 = tvReFit.at(0).tracks.at(1).GetGeoQEnergy();

            // for (size_t itvReFit = 0; itvReFit < tvReFit.size(); itvReFit++)
            // std::cout<<"itvReFit "<<itvReFit<<" "<<tvReFit.at(itvReFit).vertex.X()<<"
            // "<<tvReFit.at(itvReFit).vertex.Y()<<" "<<tvReFit.at(itvReFit).vertex.Z()<<" "<<std::endl;
            //	XYZPoint vertexMeanReFit = (XYZPoint)tvReFit.at(0).vertex;

            std::cout << "itvReFit "
                      << " " << vertexMean.X() << " " << vertexMean.Y() << " " << vertexMean.Z() << " " << MaxZ1 << " "
                      << MaxZ2 << " " << std::endl;

            //------------------------------------------------------------------------------

            lastX1 = lastPoint1proj.X();
            lastY1 = lastPoint1proj.Y();
            lastZ1 = lastPoint1proj.Z();
            lastX2 = lastPoint2proj.X();
            lastY2 = lastPoint2proj.Y();
            lastZ2 = lastPoint2proj.Z();
            vertexX = vertexMean.X();
            vertexY = vertexMean.Y();
            vertexZ = vertexMean.Z();

            // theta1 = GetThetaPhi(tvReFit.at(0).tracks.at(0), vertexMean, lastPoint1,1).first;//GetThetaPhi(..,..,-1)
            // for simu;
            // theta2 = GetThetaPhi(tvReFit.at(0).tracks.at(1), vertexMean, lastPoint2,1).first;
            // phi1 = GetThetaPhi(tvReFit.at(0).tracks.at(0), vertexMean, lastPoint1,1).second;
            // phi2 = GetThetaPhi(tvReFit.at(0).tracks.at(1), vertexMean, lastPoint2,1).second;
            theta1 = GetThetaPhi(vertexMean, lastPoint1proj).first; // GetThetaPhi(..,..,-1) for simu;
            theta2 = GetThetaPhi(vertexMean, lastPoint2proj).first;
            phi1 = GetThetaPhi(vertexMean, lastPoint1proj).second;
            phi2 = GetThetaPhi(vertexMean, lastPoint2proj).second;

            // std::vector<Double_t> fitPar1 = tvReFit.at(0).tracks.at(0).GetPattern()->GetPatternPar();
            // std::vector<Double_t> fitPar2 = tvReFit.at(0).tracks.at(1).GetPattern()->GetPatternPar();

            // XYZVector
            // vp1(TMath::Sign(1,lastX1)*fabs(fitPar1[3]),TMath::Sign(1,lastY1)*fabs(fitPar1[4]),TMath::Sign(1,(lastZ1-vertexZ))*fabs(fitPar1[5]));
            // XYZVector
            // vp2(TMath::Sign(1,lastX2)*fabs(fitPar2[3]),TMath::Sign(1,lastY2)*fabs(fitPar2[4]),TMath::Sign(1,(lastZ2-vertexZ))*fabs(fitPar2[5]));
            XYZVector vp1 = lastPoint1proj - vertexMean;
            XYZVector vp2 = lastPoint2proj - vertexMean;
            angle12 = FindAngleBetweenTracks(vp1, vp2);

            // std::cout<<i<<" "<<" protons 1 2 theta : "<<theta1*TMath::RadToDeg()<<"
            // "<<theta2*TMath::RadToDeg()<<"\n"; std::cout<<i<<" protons 1 2 phi : "<<phi1*TMath::RadToDeg()<<"
            // "<<phi2*TMath::RadToDeg()<<"\n";

            range_p1 = tvReFit.at(0).tracks.at(0).GetLinearRange((XYZPoint)vertexMean, (XYZPoint)lastPoint1proj);
            range_p2 = tvReFit.at(0).tracks.at(1).GetLinearRange((XYZPoint)vertexMean, (XYZPoint)lastPoint2proj);

            if (charge1 < 5e3 || charge2 < 5e3 || MaxR1 > 245. || MaxR2 > 245. || MaxR1 < 35. || MaxR2 < 35. ||
                MaxZ1 > 975. || MaxZ2 > 975. || MaxZ1 < 25. || MaxZ2 < 25. || vertexMean.Z() < 25. ||
                vertexMean.Z() > 975.)
               continue;
            //==============================================================================
            // get fragment parameters at vertex location from S800 inverse map

            Double_t zta = 1.066-vertexMean.Z()/1000;
            std::vector<Double_t> invMapVars;
            invMapVars = s800Ana.CalcInverseMap(zta);
            std::cout<<"invMap results, ata, bta, yta, dta, thetaLab, phi: "<<invMapVars.at(0)<<" "<<invMapVars.at(1)<<" "<<invMapVars.at(2)
            <<" "<<invMapVars.at(3)<<" "<<invMapVars.at(4)<<" "<<invMapVars.at(5)<<" "<<std::endl;
            Eje_ata = invMapVars.at(0);
            Eje_bta = invMapVars.at(1);
            Eje_dta = invMapVars.at(3);
            //==============================================================================

            //==============================================================================
            // methods to get the proton eloss

            // eLoss_p1_reco = eloss_approx(range_p1);
            // eLoss_p2_reco = eloss_approx(range_p2);

            eLoss_p1_reco = splineEloss->Eval(range_p1);
            eLoss_p2_reco = splineEloss->Eval(range_p2);

            // std::cout<<i<<" vertex : "<<vertexZ<<"\n";
            // std::cout<<i<<" range p1 p2 : "<<range_p1<<" "<<range_p2<<"\n";
            // std::cout<<i<<" eloss reco : "<<eLoss_p1_reco<<" "<<eLoss_p2_reco<<" "<<"\n";

            //==============================================================================

            epsilon_pp =
               0.5 * (eLoss_p1_reco + eLoss_p2_reco - 2 * sqrt(eLoss_p1_reco * eLoss_p2_reco) * TMath::Cos(angle12));

            // reconstruction of 2He
            mom1_norm_reco = TMath::Sqrt(eLoss_p1_reco * eLoss_p1_reco + 2.0 * eLoss_p1_reco * proton_mass);
            mom_proton1_reco.SetX(mom1_norm_reco * TMath::Sin(theta1) * TMath::Cos(phi1));
            mom_proton1_reco.SetY(mom1_norm_reco * TMath::Sin(theta1) * TMath::Sin(phi1));
            mom_proton1_reco.SetZ(mom1_norm_reco * TMath::Cos(theta1));

            mom2_norm_reco = TMath::Sqrt(eLoss_p2_reco * eLoss_p2_reco + 2.0 * eLoss_p2_reco * proton_mass);
            mom_proton2_reco.SetX(mom2_norm_reco * TMath::Sin(theta2) * TMath::Cos(phi2));
            mom_proton2_reco.SetY(mom2_norm_reco * TMath::Sin(theta2) * TMath::Sin(phi2));
            mom_proton2_reco.SetZ(mom2_norm_reco * TMath::Cos(theta2));
            // std::cout<<i<<" mom1 : "<<mom_proton1_reco.Mag()<<"\n";
            // std::cout<<i<<" mom2 : "<<mom_proton2_reco.Mag()<<"\n";

            mom_He2_reco = mom_proton1_reco + mom_proton2_reco;

            E_tot_he2 = (proton_mass + eLoss_p1_reco) + (proton_mass + eLoss_p2_reco);
            he2_mass_ex = TMath::Sqrt(E_tot_he2 * E_tot_he2 - mom_He2_reco.Mag2());
            // ex_he2_reco->Fill (he2_mass_ex - he2_mass);
            kin_He2 = TMath::Sqrt(mom_He2_reco.Mag2() + he2_mass_ex * he2_mass_ex) - he2_mass_ex;
            theta_He2 = mom_He2_reco.Theta() * TMath::RadToDeg();

            Double_t mom_beam = sqrt(pow(Ekin_proj + proj_mass, 2) - pow(proj_mass, 2));
            Double_t missing_mom;
            missing_mom = sqrt((mom_beam * beamDir - mom_He2_reco).Mag2());
            Double_t missing_energy = (Ekin_proj + proj_mass + target_mass - (kin_He2 + epsilon_pp + he2_mass + 0.511));
            Double_t missing_mass = sqrt(pow(missing_energy, 2) - pow(missing_mom, 2));

            phi_He2 = mom_He2_reco.Phi() * TMath::RadToDeg();
            // theta_r_he2_reco->Fill (theta_He2);
            // phi_r_he2_reco->Fill (phi_He2);
            // kin_r_he2_reco->Fill (kin_He2);
            // theta_kin_he2_reco->Fill (theta_He2, kin_He2);

            Ex4 = missing_mass - recoil_mass;

            Double_t sInv = pow(target_mass + proj_mass, 2) + 2. * target_mass * Ekin_proj;
            Double_t momCMScat = sqrt((pow(sInv - pow(he2_mass + epsilon_pp, 2) - pow(Ex4 + recoil_mass, 2), 2) -
                                       4. * pow(he2_mass + epsilon_pp, 2) * pow(Ex4 + recoil_mass, 2)) /
                                      (4. * sInv));

            // //------- rotation of track vectors so that Theta and Phi are in the beam frame
            TVector3 momBuff; // dirty trick to use Rotate functions (not available with XYZvector)
            momBuff.SetXYZ(mom_He2_reco.X(), mom_He2_reco.Y(), mom_He2_reco.Z());
            Double_t aRX = TMath::ATan2(beamDir.Y(), beamDir.Z());
            Double_t aRY = TMath::ATan2(-beamDir.X(), beamDir.Z());
            momBuff.RotateX(aRX); // rotate in trigo sens, y to z
            momBuff.RotateY(aRY); // rotate in trigo sens, z to x
            mom_He2_reco.SetXYZ(momBuff.X(), momBuff.Y(), momBuff.Z());

            theta_He2 = mom_He2_reco.Theta() * TMath::RadToDeg();
            Double_t thetaCMScat = asin(sqrt(mom_He2_reco.Mag2()) / momCMScat * sin(theta_He2 * TMath::DegToRad()));
            theta_lab = atan(sin(theta_cm * TMath::DegToRad()) /
                             (cos(theta_cm * TMath::DegToRad()) + recoil_mass / target_mass));
            theta_cm = thetaCMScat * TMath::RadToDeg();

            std::cout << " check values, Ex  " << Ex4 << " theta_cm " << theta_cm << " epsilon_pp " << epsilon_pp << " "
                      << std::endl;

            // thetacm_he2_reco->Fill (theta_cm);
            // Ex_reco->Fill (Ex4);
            // thetacm_Ex_he2_reco->Fill (theta_cm, Ex4);
            // thetacm_epsilon_pp_reco->Fill(theta_cm,epsilon_pp);

            ivt = i;
            anatree->Fill();
         } // for tv size (ive)
      }    // RANSAC null pointer
   }       // Event loop

   /// --------------------- End event loop ---------------------------------------
   outfile->cd();
   anatree->Write();

   //	tracks_z_r->Write ();
   //	tracks_x_y->Write ();
   // theta_r_he2_reco->Write ();
   // phi_r_he2_reco->Write ();
   // kin_r_he2_reco->Write ();
   // theta_kin_he2_reco->Write ();
   // thetacm_he2_reco->Write ();
   // Ex_reco->Write ();
   // ex_he2_reco->Write ();
   // thetacm_Ex_he2_reco->Write ();
   // thetacm_epsilon_pp_reco->Write();
   // epsilon_pp_reco->Write();

   outfile->Close();

} // end main
