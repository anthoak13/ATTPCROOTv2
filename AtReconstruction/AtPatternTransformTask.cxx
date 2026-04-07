#include "AtPatternTransformTask.h"

#include "AtPatternEvent.h"

#include <FairLogger.h>
#include <FairRootManager.h>

#include <TObject.h>

#include <utility>

ClassImp(AtPatternTransformTask);

AtPatternTransformTask::AtPatternTransformTask(std::unique_ptr<AtPATTERN::AtPatternTransform> transform)
   : FairTask("AtPatternTransformTask"), fTransform(std::move(transform)), fOutputArray("AtPatternEvent", 1)
{
}

InitStatus AtPatternTransformTask::Init()
{
   FairRootManager *ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(error) << "AtPatternTransformTask: cannot find RootManager!";
      return kERROR;
   }

   fInputArray = dynamic_cast<TClonesArray *>(ioMan->GetObject(fInputBranchName));
   if (fInputArray == nullptr) {
      LOG(error) << "AtPatternTransformTask: cannot find branch " << fInputBranchName;
      return kERROR;
   }

   ioMan->Register(fOutputBranchName, "AtTPC", &fOutputArray, fIsPersistence);
   return kSUCCESS;
}

void AtPatternTransformTask::Exec(Option_t * /*opt*/)
{
   fOutputArray.Delete();

   if (fInputArray->GetEntriesFast() == 0)
      return;

   auto *input = dynamic_cast<AtPatternEvent *>(fInputArray->At(0));
   if (input == nullptr)
      return;

   // Copy the input event into the output array, then transform the copy
   auto *output = new (fOutputArray[0]) AtPatternEvent(*input);
   fTransform->Transform(*output);
}
