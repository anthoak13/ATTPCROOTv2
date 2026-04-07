#ifndef ATCLUSTERORDERER_H
#define ATCLUSTERORDERER_H

#include "AtPatternTransform.h"

class AtPatternEvent;
class AtTrack;

namespace AtPATTERN {

/**
 * @brief Orders clusters along each track using a nearest-neighbor walk.
 *
 * Starts at the cluster with the highest Z position (the vertex end) and
 * walks greedily to the nearest unvisited cluster.
 *
 * @ingroup PatternTransforms
 */
class AtClusterOrderer : public AtPatternTransform {
public:
   void Transform(AtPatternEvent &event) override;

   void OrderTrack(AtTrack &track) const;

private:
};

} // namespace AtPATTERN

#endif // ATCLUSTERORDERER_H
