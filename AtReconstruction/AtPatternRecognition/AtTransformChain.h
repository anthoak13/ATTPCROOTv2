#ifndef ATTRANSFORMCHAIN_H
#define ATTRANSFORMCHAIN_H

#include "AtPatternTransform.h"

#include <vector>

class AtPatternEvent;

namespace AtPATTERN {

/// Non-owning composite AtPatternTransform. Applies steps in insertion order.
/// Callers retain ownership of all added transforms.
/// Add() silently skips null pointers; optional steps need no special handling.
class AtTransformChain : public AtPatternTransform {
public:
   void Add(AtPatternTransform *step)
   {
      if (step)
         fSteps.push_back(step);
   }
   void Clear() { fSteps.clear(); }
   void Transform(AtPatternEvent &event) override
   {
      for (auto *step : fSteps)
         step->Transform(event);
   }

private:
   std::vector<AtPatternTransform *> fSteps;
};

} // namespace AtPATTERN
#endif // ATTRANSFORMCHAIN_H
