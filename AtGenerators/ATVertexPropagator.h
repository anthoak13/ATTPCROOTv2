#ifndef ATVertexPropagator_H
#define ATVertexPropagator_H

#include "TObject.h"

#include <iostream>
#include <map>

class ATVertexPropagator;


class ATVertexPropagator : public TObject
{

  public:


    ATVertexPropagator();
    virtual ~ATVertexPropagator();

    Bool_t Test();

    ClassDef(ATVertexPropagator,1)


   void SetVertex(Double_t vx,Double_t vy,Double_t vz,Double_t invx,Double_t invy,Double_t invz,Double_t px,Double_t py, Double_t pz, Double_t E);
   void SetBeamMass(Double_t m);
   void SetRecoilE(Double_t val);
   void SetRecoilA(Double_t val);
   void SetScatterE(Double_t val);
   void SetScatterA(Double_t val);
   void SetBURes1E(Double_t val); //Recoil(Scatt) breaks up. Residual 1
   void SetBURes1A(Double_t val);
   void SetBURes2E(Double_t val); //Recoil(Scatt) breaks up. Residual 2
   void SetBURes2A(Double_t val);
   void SetRndELoss(Double_t eloss);
   void SetBeamNomE(Double_t ener);
   void ResetVertex();
   void SetMassNum(Int_t mnum);
   void SetAtomicNum(Int_t anum);

   Double_t GetBeamMass();
   Double_t GetVx();
   Double_t GetVy();
   Double_t GetVz();
   Double_t GetInVx();
   Double_t GetInVy();
   Double_t GetInVz();
   Double_t GetPx();
   Double_t GetPy();
   Double_t GetPz();
   Double_t GetEnergy();
   Double_t GetRndELoss();
   Double_t GetBeamNomE();
   Double_t GetRecoilE();
   Double_t GetRecoilA();
   Double_t GetScatterE();
   Double_t GetScatterA();
   Double_t GetBURes1E(); //Recoil(Scatt) breaks up. Residual 1
   Double_t GetBURes1A();
   Double_t GetBURes2E(); //Recoil(Scatt) breaks up. Residual 2
   Double_t GetBURes2A();
   Int_t GetMassNum();
   Int_t GetAtomicNum();

   //Tracking of event counters

   Bool_t fBeamEvt = false;
   Bool_t IsBeamEvt() { return fBeamEvt; }
   void FlipBeamEvt()  { fBeamEvt = !fBeamEvt; }

   Bool_t fDecayEvt = true;
   Bool_t IsDecayEvt() { return fDecayEvt; }
   void FlipDecayEvt()  { fDecayEvt = !fDecayEvt; }


   Int_t GetDecayEvtCnt();
   void IncDecayEvtCnt();

   Int_t fDecayEvtCnt;

   void SetValidKine(Bool_t val);
   Bool_t GetValidKine();

   Double_t fVx;
   Double_t fVy;
   Double_t fVz;
   Double_t fPx;
   Double_t fPy;
   Double_t fPz;
   Double_t fE;
   Double_t fBeamMass;
   Double_t fRndELoss;
   Double_t fBeamNomE;
   Double_t fInVx;
   Double_t fInVy;
   Double_t fInVz;
   Double_t fRecoilE;
   Double_t fRecoilA;
   Double_t fScatterE;
   Double_t fScatterA;
   Double_t fBURes1E;
   Double_t fBURes1A;
   Double_t fBURes2E;
   Double_t fBURes2A;
   Bool_t fIsValidKine;
   Int_t fAiso;
   Int_t fZiso;

};

extern ATVertexPropagator *gATVP; // global


#endif
