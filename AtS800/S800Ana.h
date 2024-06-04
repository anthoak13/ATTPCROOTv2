#ifndef S800Ana_H
#define S800Ana_H

#include <Rtypes.h>
#include <TObject.h>

#include <vector>

#include "S800InverseMap.h"

// AtTPCROOT classes

class FairLogger;
class S800Calc;
class TBuffer;
class TClass;
class TCutG;
class TMemberInspector;
class TString;

class S800Ana : public TObject {
private:
   FairLogger *fLogger;

   std::vector<Double_t> fParameters;
   std::vector<Double_t> fTofObjCorr;
   std::vector<Double_t> fMTDCObjRange;
   std::vector<Double_t> fMTDCXfRange;

   std::vector<TCutG *> fcutPID1;
   std::vector<TCutG *> fcutPID2;
   std::vector<TCutG *> fcutPID3;

   Double_t fXfObj_ToF;
   Double_t fObjCorr_ToF;
   Double_t fICSum_E;
   Double_t fX0;
   Double_t fX1;
   Double_t fY0{0};
   Double_t fY1{0};
   Double_t fAfp;
   Double_t fBfp{0};
   Double_t fAta;
   Double_t fBta;
   Double_t fYta;
   Double_t fDta;
   Double_t fThetaLab;
   Double_t fPhi;

   // Not used for now but could be.
   // If these correction coefs are needed at some point should code alsos setter/getter functions.
   Double_t fBtaCorr{1};

   S800InverseMap fInvMap;

public:
   S800Ana();
   ~S800Ana() = default;

   void SetPID1cut(std::vector<TString> file);
   void SetPID2cut(std::vector<TString> file);
   void SetPID3cut(std::vector<TString> file);
   void SetParameters(std::vector<Double_t> vec);
   void SetTofObjCorr(std::vector<Double_t> vec);
   void SetMTDCObjRange(std::vector<Double_t> vec);
   void SetMTDCXfRange(std::vector<Double_t> vec);
   void SetInverseMap(TString mapPath, Float_t zMin, Float_t zMax, Float_t zStep);

   std::vector<Double_t> GetParameters();
   std::vector<Double_t> GetTofObjCorr();
   std::vector<Double_t> GetMTDCObjRange();
   std::vector<Double_t> GetMTDCXfRange();
   Double_t GetXfObj_ToF();
   Double_t GetObjCorr_ToF();
   Double_t GetICSum_E();
   std::vector<Double_t> GetFpVariables();
   std::vector<Double_t> GetReconstTargetVariables();

   Bool_t isInPID(S800Calc *s800calc);
   void Calc(S800Calc *s800calc);
   std::vector<Double_t> CalcInverseMap(Float_t zta);

   // void InitStatus Init();
   // virtual void Exec(Option_t *opt);


   void Reset();

   ClassDef(S800Ana, 1);
};

#endif
