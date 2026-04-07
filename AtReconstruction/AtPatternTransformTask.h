#ifndef ATPATTERNTRANSFORMTASK_H
#define ATPATTERNTRANSFORMTASK_H

#include "AtPatternTransform.h"

#include <FairTask.h>

#include <TClonesArray.h>
#include <TString.h>

#include <memory>

class TBuffer;
class TClass;
class TMemberInspector;

/**
 * @brief FairTask that applies an AtPatternTransform to each event.
 *
 * Copies the input AtPatternEvent into a new output branch and calls
 * Transform() on the copy. Multiple instances can be chained in a FairRunAna
 * pipeline to apply clustering, ordering, selection, merging, seeding, etc.
 *
 * @see AtPATTERN::AtPatternTransform
 */
class AtPatternTransformTask : public FairTask {
public:
   explicit AtPatternTransformTask(std::unique_ptr<AtPATTERN::AtPatternTransform> transform);
   ~AtPatternTransformTask() = default;

   void SetInputBranch(TString name) { fInputBranchName = std::move(name); }
   void SetOutputBranch(TString name) { fOutputBranchName = std::move(name); }
   void SetPersistence(bool value) { fIsPersistence = value; }

   InitStatus Init() override;
   void Exec(Option_t *opt) override;

private:
   std::unique_ptr<AtPATTERN::AtPatternTransform> fTransform;
   TString fInputBranchName{"AtPatternEvent"};
   TString fOutputBranchName{"AtPatternEventTransformed"};
   TClonesArray *fInputArray{nullptr};
   TClonesArray fOutputArray;
   bool fIsPersistence{false};

   ClassDefOverride(AtPatternTransformTask, 1);
};

#endif // ATPATTERNTRANSFORMTASK_H
