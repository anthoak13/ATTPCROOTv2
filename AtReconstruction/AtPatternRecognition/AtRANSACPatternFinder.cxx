#include "AtRANSACPatternFinder.h"

#include "AtEvent.h"
#include "AtPatternEvent.h"
#include "AtSampleConsensus.h"

#include <memory>

AtPATTERN::AtRANSACPatternFinder::AtRANSACPatternFinder()
   : fSampleConsensus(std::make_unique<SampleConsensus::AtSampleConsensus>())
{
}

std::unique_ptr<AtPatternEvent> AtPATTERN::AtRANSACPatternFinder::FindTracks(AtEvent &event)
{
   return std::make_unique<AtPatternEvent>(fSampleConsensus->Solve(&event));
}

void AtPATTERN::AtRANSACPatternFinder::SetPatternType(AtPatterns::PatternType type)
{
   fSampleConsensus->SetPatternType(type);
}

void AtPATTERN::AtRANSACPatternFinder::SetEstimator(SampleConsensus::Estimators estimator)
{
   fSampleConsensus->SetEstimator(estimator);
}

void AtPATTERN::AtRANSACPatternFinder::SetNumIterations(int n)
{
   fSampleConsensus->SetNumIterations(n);
}

void AtPATTERN::AtRANSACPatternFinder::SetMinHitsPattern(int n)
{
   fSampleConsensus->SetMinHitsPattern(n);
}

void AtPATTERN::AtRANSACPatternFinder::SetDistanceThreshold(double mm)
{
   fSampleConsensus->SetDistanceThreshold(mm);
}

void AtPATTERN::AtRANSACPatternFinder::SetChargeThreshold(double q)
{
   fSampleConsensus->SetChargeThreshold(q);
}
