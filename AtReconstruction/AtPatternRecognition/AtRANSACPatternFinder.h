#ifndef ATRANSACPATTERNFINDER_H
#define ATRANSACPATTERNFINDER_H

#include "AtPatternFinder.h"

#include "AtEstimatorMethods.h"
#include "AtPatternTypes.h"
#include "AtSampleConsensus.h"

#include <memory>

class AtEvent;
class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Pattern finder using Sample Consensus (RANSAC) algorithms.
 *
 * Wraps AtSampleConsensus::Solve behind the AtPatternFinder interface.
 * Supports multiple estimators (RANSAC, LMedS, MLESAC, WRANSAC, Chi2, YRANSAC)
 * and multiple pattern types (Line, Circle2D, Y, Fission).
 *
 * @ingroup PatternFinders
 */
class AtRANSACPatternFinder : public AtPatternFinder {
public:
   AtRANSACPatternFinder();

   std::unique_ptr<AtPatternEvent> FindTracks(AtEvent &event) override;

   void SetPatternType(AtPatterns::PatternType type);
   void SetEstimator(SampleConsensus::Estimators estimator);
   void SetNumIterations(int n);
   void SetMinHitsPattern(int n);
   void SetDistanceThreshold(double mm);
   void SetChargeThreshold(double q);

private:
   std::unique_ptr<SampleConsensus::AtSampleConsensus> fSampleConsensus;
};

} // namespace AtPATTERN

#endif // ATRANSACPATTERNFINDER_H
