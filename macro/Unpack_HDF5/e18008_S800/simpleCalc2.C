void simpleCalc2(){

  Double_t      fWhmFocus = 0.5; //cm, FWHM of Gaussian
  Double_t      fDiv = 10.*1E-3; //radians
  Double_t      fZFocus = 50; //cm, focus distance from entrance

  auto c = new TCanvas("c","",600,600);
  c->Divide(2,2);
  TH1D *hPx = new TH1D("hPx","",100,-0.1,0.1);
  TH1D *hPy = new TH1D("hPy","",100,-0.1,0.1);
  TH1D *hPz = new TH1D("hPz","",100,6.67,6.68);
  TH1D *hDiffP = new TH1D("hDiffP","",100,0,0.02);

for (size_t i = 0; i < 1e4; i++) {



  Double_t x=0., y=0., xFocus=0., yFocus=0., theta=0., phi=0.;
  Double_t ptot=sqrt(pow(0,2) + pow(0,2) + pow(sqrt( pow(115. * 14. / 1000.0 + 14.008596359*931.494/1000.0,2) - pow(14.008596359*931.494/1000.0,2) ),2));

  //x is coordinates of beam particle at ATTPC entrance, xFocus is coordinates at focus.
  xFocus = gRandom->Gaus(0,fWhmFocus / 2.355);
  yFocus = gRandom->Gaus(0,fWhmFocus / 2.355);

  do{
    theta = gRandom->Uniform(-fDiv,fDiv);
    phi = gRandom->Uniform(-fDiv,fDiv);
    x = xFocus-fZFocus*tan(phi);
    y = yFocus-sqrt(pow(fZFocus,2)+pow(xFocus-x,2))*tan(theta);
  }
  while(sqrt(pow(x,2)+pow(y,2))>2. && sqrt(pow(tan(theta),2)+pow(tan(phi),2))>tan(fDiv));

  Double_t fVx   =x ;
  Double_t fVy   =y ;
  Double_t fVz   =0. ;

  Double_t fPx=ptot*cos(theta)*sin(phi);
  Double_t fPy=ptot*sin(theta);
  Double_t fPz=sqrt(ptot*ptot - fPx*fPx - fPy*fPy);

  hPx->Fill(fPx);
  hPy->Fill(fPy);
  hPz->Fill(fPz);
  hDiffP->Fill((ptot-fPz)/ptot*100.);//sqrt(fPx*fPx - fPy*fPy)/ptot
  // hDiffP->Fill(sqrt(fPx*fPx - fPy*fPy)/tan(sqrt(pow(tan(theta),2)+pow(tan(phi),2)))/ptot);//
  cout<<fPx<<" "<<fPy<<" "<<fPz<<" "<<ptot<<" "<<sqrt(fPx*fPx - fPy*fPy)/tan(sqrt(pow(tan(theta),2)+pow(tan(phi),2)))/ptot<<endl;
}//for event i

  c->cd(1);
  hPx->Draw();
  c->cd(2);
  hPy->Draw();
  c->cd(3);
  hPz->Draw();
  c->cd(4);
  hDiffP->Draw();
  return 0;

}
