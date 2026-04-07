#include "AtPatternFindingTask.h"

#include "AtEvent.h"
#include "AtPatternEvent.h"

#include <FairLogger.h>
#include <FairRootManager.h>

#include <TObject.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

ClassImp(AtPatternFindingTask);

AtPatternFindingTask::AtPatternFindingTask(std::unique_ptr<AtPATTERN::AtPatternFinder> finder)
   : FairTask("AtPatternFindingTask"), fFinder(std::move(finder)), fPatternEventArray("AtPatternEvent", 1)
{
}

InitStatus AtPatternFindingTask::Init()
{
   FairRootManager *ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(error) << "AtPatternFindingTask: cannot find RootManager!";
      return kERROR;
   }

   fEventArray = dynamic_cast<TClonesArray *>(ioMan->GetObject(fInputBranchName));
   if (fEventArray == nullptr) {
      LOG(error) << "AtPatternFindingTask: cannot find branch " << fInputBranchName;
      return kERROR;
   }

   ioMan->Register(fOutputBranchName, "AtTPC", &fPatternEventArray, fIsPersistence);
   return kSUCCESS;
}

void AtPatternFindingTask::Exec(Option_t * /*opt*/)
{
   fPatternEventArray.Delete();

   if (fEventArray->GetEntriesFast() == 0)
      return;

   auto &event = *dynamic_cast<AtEvent *>(fEventArray->At(0));
   auto &hitArray = event.GetHits();

   std::cout << "  -I- AtPatternFindingTask - Event: " << event.GetEventID() << "\n";

   try {
      if (static_cast<int>(hitArray.size()) > fMinNumHits &&
          static_cast<int>(hitArray.size()) < fMaxNumHits) {
         auto patternEvent = fFinder->FindTracks(event);
         if (patternEvent)
            new (fPatternEventArray[0]) AtPatternEvent(std::move(*patternEvent));
      }
   } catch (std::runtime_error &e) {
      std::cout << "AtPatternFindingTask: error: " << e.what() << "\n";
   }
}
