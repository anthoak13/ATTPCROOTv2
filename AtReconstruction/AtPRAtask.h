#ifndef AtPRATASK_H
#define AtPRATASK_H

#include <FairTask.h> // for FairTask, InitStatus

#include <Rtypes.h>       // for Int_t, Double_t, Bool_t, THashConsistencyH...
#include <TClonesArray.h> // for TClonesArray
#include <TString.h>

#include <cstddef> // for size_t
#include <memory>
#include <utility>

class AtDigiPar;
class TBuffer;
class TClass;
class TMemberInspector;
#include "AtTransformChain.h"

namespace AtPATTERN {
class AtTrackFinderTC;
class AtSmooth3DClusterer;
class AtClusterOrderer;
class AtCircleSeeder;
class AtTrackPruner;
class AtBeamTrackRejector;
class AtFragmentMerger;
class AtVertexTrackSelector;
} // namespace AtPATTERN

/**
 * @brief Task for pattern recognition using TriplClust.
 *
 * Uses AtTrackFinderTC for raw track finding, then applies the full
 * transform chain (clustering, ordering, beam rejection, fragment merging,
 * vertex selection, seeding) as individually-injectable AtPatternTransform steps.
 *
 * For fine-grained control over each step, use AtPatternFindingTask with
 * AtPATTERN::AtTrackFinderTC plus individual AtPatternTransformTask instances.
 */
class AtPRAtask : public FairTask {
private:
   TString fInputBranchName;
   TString fOutputBranchName;

   TClonesArray *fEventHArray{};
   TClonesArray fPatternEventArray;

   AtDigiPar *fPar;

   std::unique_ptr<AtPATTERN::AtTrackFinderTC> fPRA;

   Int_t fPRAlgorithm;

   Int_t fMinNumHits;
   Int_t fMaxNumHits;

   Bool_t kIsPersistence;

   // HC parameters
   float fHCs;
   size_t fHCk;
   size_t fHCn;
   size_t fHCm;
   float fHCr;
   float fHCa;
   float fHCt;
   size_t fHCpadding;

   // Prunning parameters
   Bool_t kSetPrunning;
   Int_t fKNN;             //<! Number of nearest neighbors kNN
   Double_t fStdDevMulkNN; //<! Std dev multiplier for kNN
   Double_t fkNNDist;      //<! Distance threshold for outlier rejection in kNN

   // Clustering parameters
   Double_t fClusterRadius{20.0};   // Overlapping clusters: radius > distance
   Double_t fClusterDistance{15.0}; // Gives 2.2% RMS (was r10 d20 → 3.8%)

   // Selection/merging parameters
   Double_t fMinLabTheta{10.0};    // BeamTrackRejector threshold (degrees)
   Double_t fVertexRadiusXY{80.0}; // VertexTrackSelector radius (mm)
   Double_t fMergeDist{30.0};      // FragmentMerger distance (mm)

   // Transform chain (built in Init(), applied in Exec())
   AtPATTERN::AtTransformChain fChain;

   std::unique_ptr<AtPATTERN::AtSmooth3DClusterer>    fClusterer;
   std::unique_ptr<AtPATTERN::AtClusterOrderer>       fOrderer;
   std::unique_ptr<AtPATTERN::AtBeamTrackRejector>    fBeamRejector;
   std::unique_ptr<AtPATTERN::AtFragmentMerger>       fFragmentMerger;
   std::unique_ptr<AtPATTERN::AtVertexTrackSelector>  fVertexSelector;
   std::unique_ptr<AtPATTERN::AtCircleSeeder>         fSeeder;
   std::unique_ptr<AtPATTERN::AtTrackPruner>          fPruner; // only when kSetPrunning

public:
   AtPRAtask();
   ~AtPRAtask();

   virtual InitStatus Init();
   virtual void Exec(Option_t *option);
   virtual void SetParContainers();
   virtual void Finish();

   void SetInputBranch(TString branchName) { fInputBranchName = std::move(branchName); }
   void SetOutputBranch(TString branchName) { fOutputBranchName = std::move(branchName); }

   void SetPersistence(Bool_t value = kTRUE);
   void SetPRAlgorithm(Int_t value = 0);

   void SetScluster(float s) { fHCs = s; }
   void SetKtriplet(size_t k) { fHCk = k; }
   void SetNtriplet(size_t n) { fHCn = n; }
   void SetMcluster(size_t m) { fHCm = m; }
   void SetRsmooth(float r) { fHCr = r; }
   void SetAtriplet(float a) { fHCa = a; }
   void SetTcluster(float t) { fHCt = t; }
   void SetPadding(size_t padding) { fHCpadding = padding; }

   void SetMaxNumHits(Int_t maxHits) { fMaxNumHits = maxHits; }
   void SetMinNumHits(Int_t minHits) { fMinNumHits = minHits; }

   void SetPrunning() { kSetPrunning = kTRUE; }
   void SetkNN(Double_t knn) { fKNN = knn; }
   void SetStdDevMulkNN(Double_t stdDevMul) { fStdDevMulkNN = stdDevMul; }
   void SetkNNDist(Double_t dist) { fkNNDist = dist; }

   void SetClusterRadius(Double_t clusterRadius) { fClusterRadius = clusterRadius; }
   void SetClusterDistance(Double_t clusterDistance) { fClusterDistance = clusterDistance; }

   void SetClusterer(std::unique_ptr<AtPATTERN::AtSmooth3DClusterer> clusterer);
   void SetOrderer(std::unique_ptr<AtPATTERN::AtClusterOrderer> orderer);
   void SetBeamRejector(std::unique_ptr<AtPATTERN::AtBeamTrackRejector> r);
   void SetFragmentMerger(std::unique_ptr<AtPATTERN::AtFragmentMerger> m);
   void SetVertexSelector(std::unique_ptr<AtPATTERN::AtVertexTrackSelector> s);
   void SetSeeder(std::unique_ptr<AtPATTERN::AtCircleSeeder> seeder);
   void SetPruner(std::unique_ptr<AtPATTERN::AtTrackPruner> pruner);

   void SetMinLabTheta(Double_t theta) { fMinLabTheta = theta; }
   void SetVertexRadiusXY(Double_t r) { fVertexRadiusXY = r; }
   void SetMergeDist(Double_t dist) { fMergeDist = dist; }

   void SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime);

   ClassDef(AtPRAtask, 1);
};

#endif
