#ifndef ATTRACKPRUNER_H
#define ATTRACKPRUNER_H

#include "AtPatternTransform.h"

class AtPatternEvent;
class AtTrack;

namespace AtPATTERN {

/**
 * @brief Removes outlier hits from each track using k-nearest-neighbor distances.
 *
 * For each hit, computes the mean distance to its k nearest neighbors within
 * the track. If (mean + stdDev * stdDevMul) exceeds kNNDist, the hit is
 * flagged as an outlier and removed.
 *
 * @ingroup PatternTransforms
 */
class AtTrackPruner : public AtPatternTransform {
public:
   void Transform(AtPatternEvent &event) override;

   void SetKNN(int k) { fKNN = k; }
   void SetStdDevMul(double mul) { fStdDevMul = mul; }
   void SetKNNDist(double dist) { fKNNDist = dist; }

private:
   bool IsOutlier(const AtTrack &track, int hitIdx) const;

   int fKNN{5};
   double fStdDevMul{0.0};
   double fKNNDist{10.0};
};

} // namespace AtPATTERN

#endif // ATTRACKPRUNER_H
