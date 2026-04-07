#ifndef ATPATTERNFINDINGTASK_H
#define ATPATTERNFINDINGTASK_H

#include "AtPatternFinder.h"

#include <FairTask.h>

#include <TClonesArray.h>
#include <TString.h>

#include <memory>

class TBuffer;
class TClass;
class TMemberInspector;

/**
 * @brief FairTask that runs an AtPatternFinder on each event.
 *
 * Reads an AtEvent from the input branch and writes an AtPatternEvent to the
 * output branch. The algorithm is injected via the constructor.
 *
 * @see AtPATTERN::AtPatternFinder
 */
class AtPatternFindingTask : public FairTask {
public:
   explicit AtPatternFindingTask(std::unique_ptr<AtPATTERN::AtPatternFinder> finder);
   ~AtPatternFindingTask() = default;

   void SetInputBranch(TString name) { fInputBranchName = std::move(name); }
   void SetOutputBranch(TString name) { fOutputBranchName = std::move(name); }
   void SetPersistence(bool value) { fIsPersistence = value; }
   void SetMinNumHits(int n) { fMinNumHits = n; }
   void SetMaxNumHits(int n) { fMaxNumHits = n; }

   InitStatus Init() override;
   void Exec(Option_t *opt) override;

private:
   std::unique_ptr<AtPATTERN::AtPatternFinder> fFinder;
   TString fInputBranchName{"AtEventH"};
   TString fOutputBranchName{"AtPatternEvent"};
   TClonesArray *fEventArray{nullptr};
   TClonesArray fPatternEventArray;
   bool fIsPersistence{false};
   int fMinNumHits{10};
   int fMaxNumHits{5000};

   ClassDefOverride(AtPatternFindingTask, 1);
};

#endif // ATPATTERNFINDINGTASK_H
