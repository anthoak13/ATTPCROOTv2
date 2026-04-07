#include "AtPRAtask.h"

#include "AtBeamTrackRejector.h"
#include "AtCircleSeeder.h"
#include "AtClusterOrderer.h"
#include "AtDigiPar.h"      // for AtDigiPar
#include "AtEvent.h"        // for AtEvent
#include "AtFragmentMerger.h"
#include "AtPatternEvent.h" // for AtPatternEvent
#include "AtSmooth3DClusterer.h"
#include "AtTrackFinderTC.h"
#include "AtTrackPruner.h"
#include "AtVertexTrackSelector.h"

#include <FairLogger.h>      // for LOG, FairLogger
#include <FairRootManager.h> // for FairRootManager
#include <FairRun.h>         // for FairRun
#include <FairRuntimeDb.h>   // for FairRuntimeDb

#include <TObject.h> // for TObject

#include <iostream>  // for operator<<, basic_ostream, cout, ostream
#include <memory>    // for unique_ptr<>::element_type, unique_ptr
#include <stdexcept> // for runtime_error
#include <utility>   // for move
#include <vector>    // for allocator, vector

AtPRAtask::AtPRAtask()
   : fInputBranchName("AtEventH"), fOutputBranchName("AtPatternEvent"), FairTask("AtPRAtask"),
     fPatternEventArray("AtPatternEvent", 1)
{

   LOG(debug) << "Default Constructor of AtPRAtask";
   fPar = nullptr;
   fPRAlgorithm = 0;
   kIsPersistence = kFALSE;
   fMinNumHits = 10;
   fMaxNumHits = 5000;

   fHCs = 0.3;
   fHCk = 19;
   fHCn = 2;
   fHCm = 15;
   fHCr = 2.0;
   fHCa = 0.03;
   fHCt = 4.0;
   fHCpadding = 0.0;

   kSetPrunning = kFALSE;
   fKNN = 5;
   fStdDevMulkNN = 0.0;
   fkNNDist = 10.0;
}

AtPRAtask::~AtPRAtask()
{
   LOG(debug) << "Destructor of AtPRAtask";
}

void AtPRAtask::SetPersistence(Bool_t value)
{
   kIsPersistence = value;
}
void AtPRAtask::SetPRAlgorithm(Int_t value)
{
   fPRAlgorithm = value;
}

void AtPRAtask::SetClusterer(std::unique_ptr<AtPATTERN::AtSmooth3DClusterer> clusterer)
{
   fClusterer = std::move(clusterer);
}

void AtPRAtask::SetOrderer(std::unique_ptr<AtPATTERN::AtClusterOrderer> orderer)
{
   fOrderer = std::move(orderer);
}

void AtPRAtask::SetBeamRejector(std::unique_ptr<AtPATTERN::AtBeamTrackRejector> r)
{
   fBeamRejector = std::move(r);
}

void AtPRAtask::SetFragmentMerger(std::unique_ptr<AtPATTERN::AtFragmentMerger> m)
{
   fFragmentMerger = std::move(m);
}

void AtPRAtask::SetVertexSelector(std::unique_ptr<AtPATTERN::AtVertexTrackSelector> s)
{
   fVertexSelector = std::move(s);
}

void AtPRAtask::SetSeeder(std::unique_ptr<AtPATTERN::AtCircleSeeder> seeder)
{
   fSeeder = std::move(seeder);
}

void AtPRAtask::SetPruner(std::unique_ptr<AtPATTERN::AtTrackPruner> pruner)
{
   fPruner = std::move(pruner);
}

void AtPRAtask::SetParContainers()
{
   LOG(debug) << "SetParContainers of AtPRAtask";

   FairRun *run = FairRun::Instance();
   if (!run)
      LOG(fatal) << "No analysis run!";

   FairRuntimeDb *db = run->GetRuntimeDb(); // NOLINT
   if (!db)
      LOG(fatal) << "No runtime database!";

   fPar = (AtDigiPar *)db->getContainer("AtDigiPar"); // NOLINT
   if (!fPar)
      LOG(fatal) << "AtDigiPar not found!!";
}

InitStatus AtPRAtask::Init()
{
   LOG(debug) << "Initilization of AtPRAtask";

   if (fPRAlgorithm == 0) {
      LOG(info) << "Using Track Finder TriplClust algorithm";

      fPRA = std::make_unique<AtPATTERN::AtTrackFinderTC>();
      fPRA->SetTcluster(fHCt);
      fPRA->SetScluster(fHCs);
      fPRA->SetKtriplet(fHCk);
      fPRA->SetNtriplet(fHCn);
      fPRA->SetMcluster(fHCm);
      fPRA->SetRsmooth(fHCr);
      fPRA->SetAtriplet(fHCa);
      fPRA->SetPadding(fHCpadding);

      std::cout << " Track Finder TriplClust parameters (see Dalitz et al.) "
                << "\n";
      std::cout << " T Cluster : " << fHCt << "\n";
      std::cout << " S Cluster : " << fHCs << "\n";
      std::cout << " K Triplet : " << fHCk << "\n";
      std::cout << " N Triplet : " << fHCn << "\n";
      std::cout << " M Cluster : " << fHCm << "\n";
      std::cout << " R Smooth  : " << fHCr << "\n";
      std::cout << " A Triplet : " << fHCa << "\n";

   } else if (fPRAlgorithm == 1) {
      LOG(info) << "Using RANSAC algorithm";

   } else if (fPRAlgorithm == 2) {
      LOG(info) << "Using Hough transform algorithm";
   }

   // Build the post-finding transform chain only when not user-provided
   if (!fClusterer)
      fClusterer = std::make_unique<AtPATTERN::AtSmooth3DClusterer>(fClusterRadius, fClusterDistance);

   if (fPar) {
      double tbTime = fPar->GetTBTime() * 1e-3; // ns → us
      fClusterer->SetDiffusionParams(fPar->GetCoefDiffusionTrans(), fPar->GetCoefDiffusionLong(),
                                     fPar->GetDriftVelocity(), tbTime);
      LOG(info) << "AtPRAtask: diffusion params from AtDigiPar — CoefT=" << fPar->GetCoefDiffusionTrans()
                << " CoefL=" << fPar->GetCoefDiffusionLong() << " DriftVel=" << fPar->GetDriftVelocity()
                << " TBTime=" << tbTime << " us";
   }

   if (!fOrderer)
      fOrderer = std::make_unique<AtPATTERN::AtClusterOrderer>();

   if (!fBeamRejector)
      fBeamRejector = std::make_unique<AtPATTERN::AtBeamTrackRejector>(fMinLabTheta);

   if (!fFragmentMerger)
      fFragmentMerger = std::make_unique<AtPATTERN::AtFragmentMerger>(fMergeDist, fVertexRadiusXY);

   if (!fVertexSelector)
      fVertexSelector = std::make_unique<AtPATTERN::AtVertexTrackSelector>(fVertexRadiusXY);

   if (!fSeeder)
      fSeeder = std::make_unique<AtPATTERN::AtCircleSeeder>();

   if (kSetPrunning && !fPruner) {
      fPruner = std::make_unique<AtPATTERN::AtTrackPruner>();
      fPruner->SetKNN(fKNN);
      fPruner->SetStdDevMul(fStdDevMulkNN);
      fPruner->SetKNNDist(fkNNDist);
   }

   // Wire the shared clusterer and orderer into the fragment merger for per-merge
   // re-clustering and re-ordering. Both are owned by this task and share the same lifetime.
   fFragmentMerger->SetClusterer(fClusterer.get());
   fFragmentMerger->SetOrderer(fOrderer.get());

   // Build the post-finding transform chain. Pipeline order matches prior Exec() sequence.
   // Null-safe: Add() silently skips nullptr (e.g. fPruner when pruning is disabled).
   fChain.Clear();
   fChain.Add(fClusterer.get());
   fChain.Add(fPruner.get());
   fChain.Add(fOrderer.get());
   fChain.Add(fBeamRejector.get());
   fChain.Add(fFragmentMerger.get());
   fChain.Add(fVertexSelector.get());
   fChain.Add(fSeeder.get());

   // Get a handle from the IO manager
   FairRootManager *ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(error) << "Cannot find RootManager!";
      return kERROR;
   }

   fEventHArray = dynamic_cast<TClonesArray *>(ioMan->GetObject(fInputBranchName));
   if (fEventHArray == nullptr) {
      LOG(error) << "Cannot find AtEvent array!";
      return kERROR;
   }

   ioMan->Register(fOutputBranchName, "AtTPC", &fPatternEventArray, kIsPersistence);

   return kSUCCESS;
}

void AtPRAtask::Exec(Option_t *option)
{
   LOG(debug) << "Exec of AtPRAtask";

   fPatternEventArray.Delete();

   if (fEventHArray->GetEntriesFast() == 0)
      return;

   AtEvent &event = *(dynamic_cast<AtEvent *>(fEventHArray->At(0))); // TODO: Make sure we are not copying
   auto &hitArray = event.GetHits();

   std::cout << "  -I- AtPRAtask -  Event Number :  " << event.GetEventID() << "\n";

   try {

      if (hitArray.size() > fMinNumHits && hitArray.size() < fMaxNumHits) {
         auto patternEvent = fPRA->FindTracks(event);
         if (patternEvent) {
            fChain.Transform(*patternEvent);
            new (fPatternEventArray[0]) AtPatternEvent(std::move(*patternEvent));
         }
      }

   } catch (std::runtime_error e) {
      std::cout << "Analyzation failed! Error: " << e.what() << std::endl;
   }
}

void AtPRAtask::SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime)
{
   if (fClusterer)
      fClusterer->SetDiffusionParams(coefT, coefL, driftVel, tbTime);
}

void AtPRAtask::Finish()
{
   LOG(debug) << "Finish of AtPRAtask";
}
