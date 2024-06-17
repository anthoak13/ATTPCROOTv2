/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef AtTPC_H
#define AtTPC_H

#include <FairDetector.h>

#include <Rtypes.h>
#include <TLorentzVector.h>
#include <TString.h>
#include <TVector3.h>

#include <string>
#include <utility>

class AtMCPoint;
class FairVolume;
class TClonesArray;
class TBuffer;
class TClass;
class TList;
class TMemberInspector;

class AtTpc : public FairDetector {
private:
   /** Track information to be stored until the track leaves the
   active volume.
   */
   Int_t fTrackID{-1};             //!  track index
   Int_t fVolumeID{-1};            //!  volume id
   Int_t fDetCopyID{};             //!  Det volume id  // added by Marc
   TLorentzVector fPosIn, fPosOut; //!  position
   TLorentzVector fMomIn, fMomOut; //!  momentum
   Double32_t fTime{-1};           //!  time
   Double32_t fLength{-1};         //!  length
   Double32_t fELoss{-1};          //!  energy loss
   TString fVolName{""};
   Double32_t fELossAcc{-1};

   /** container for data points */

   TClonesArray *fAtTpcPointCollection; //!

public:
   /**      @param Name Detector Name
    *       @param Active ProcessHits() will be called if true
    */
   AtTpc(const char *Name, Bool_t Active);
   AtTpc();
   virtual ~AtTpc();

   /** From FairDetector **/
   virtual Bool_t ProcessHits(FairVolume *v = 0) override;
   virtual void Register() override;
   virtual TClonesArray *GetCollection(Int_t iColl) const override;
   virtual void Reset() override;
   virtual void Print(Option_t *option = "") const override;
   virtual void EndOfEvent() override;

   /** From FairModule **/
   virtual void ConstructGeometry() override;
   virtual Bool_t CheckIfSensitive(std::string name) override;

   AtMCPoint *
   AddHit(Int_t trackID, Int_t detID, TVector3 pos, TVector3 mom, Double_t time, Double_t length, Double_t eLoss);

   AtMCPoint *AddHit(Int_t trackID, Int_t detID, TString VolName, Int_t detCopyID, TVector3 pos, TVector3 mom,
                     Double_t time, Double_t length, Double_t eLoss, Double_t EIni, Double_t AIni, Int_t A, Int_t Z);

private:
   std::pair<Int_t, Int_t> DecodePdG(Int_t PdG_Code);

   void trackEnteringVolume();
   void getTrackParametersFromMC();
   void getTrackParametersWhileExiting();
   void resetVertex();
   void addHit();
   bool reactionOccursHere();
   void startReactionEvent();

   AtTpc(const AtTpc &) = delete;
   AtTpc &operator=(const AtTpc &) = delete;

   ClassDefOverride(AtTpc, 3)
};

#endif // NEWDETECTOR_H
