#ifndef ATVERTEXTRACKSELECTOR_H
#define ATVERTEXTRACKSELECTOR_H

#include "AtPatternTransform.h"

class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Keeps only tracks that have an endpoint near the interaction vertex.
 *
 * Finds the maximum Z among all vertex-end clusters (proxy for the vertex Z).
 * Keeps tracks whose vertex end is within vertexRadiusXY mm of the beam axis
 * (XY origin) AND within 50 mm in Z of the maximum. Removes all other tracks.
 *
 * @ingroup PatternTransforms
 */
class AtVertexTrackSelector : public AtPatternTransform {
public:
   explicit AtVertexTrackSelector(double vertexRadiusXY = 80.0) : fVertexRadiusXY(vertexRadiusXY) {}

   void Transform(AtPatternEvent &event) override;

   void SetVertexRadiusXY(double mm) { fVertexRadiusXY = mm; }
   void SetVertexZTolerance(double mm) { fVertexZTolerance = mm; }

private:
   double fVertexRadiusXY{80.0};
   double fVertexZTolerance{50.0};
};

} // namespace AtPATTERN

#endif // ATVERTEXTRACKSELECTOR_H
