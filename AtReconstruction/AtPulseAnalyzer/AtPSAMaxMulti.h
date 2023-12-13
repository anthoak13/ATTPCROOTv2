#ifndef AtPSAMAXMulti_H
#define AtPSAMAXMulti_H

#include "AtPSAMax.h"

#include <Rtypes.h> // for Bool_t, THashConsistencyHolder, ClassDefOverride

#include <array>  // for array
#include <memory> // for make_unique, unique_ptr

class AtPad;
class TBuffer;
class TClass;
class TMemberInspector;

/**
 * @brief Simple max finding PSA method.
 *
 *
 *
 */
class AtPSAMaxMulti : public AtPSAMax {

private:
   Int_t fSubtractWindow{5}; //Number of timebuckets to subtract around the peak
public:

   virtual HitVector AnalyzePad(AtPad *pad) override;
   std::unique_ptr<AtPSA> Clone() override { return std::make_unique<AtPSAMaxMulti>(*this); }
   void SetSubtractionWindow(int i) { fSubtractWindow = i;}

   ClassDefOverride(AtPSAMaxMulti, 1)
};

#endif
