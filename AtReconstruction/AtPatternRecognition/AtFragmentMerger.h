#ifndef ATFRAGMENTMERGER_H
#define ATFRAGMENTMERGER_H

#include "AtPatternTransform.h"

class AtPatternEvent;

namespace AtPATTERN {

class AtSmooth3DClusterer;
class AtClusterOrderer;

/**
 * @brief Merges track fragments into nearby primary tracks.
 *
 * Identifies primary tracks (front cluster within vertexRadiusXY of the beam
 * axis and within 50 mm in Z of the maximum front-cluster Z). For each primary,
 * searches for non-primary tracks whose nearest endpoint is within mergeDist mm
 * of the primary track's far end. Matching fragments have their hits appended to
 * the primary and are removed from the event.
 *
 * If a clusterer has been set via SetClusterer(), the merged primary track is
 * re-clustered and re-ordered after each individual merge so that the far-end
 * position used for the next fragment search is up to date. Without a clusterer,
 * the merged track's cluster array is cleared and the caller is responsible for
 * re-clustering (original standalone behavior).
 *
 * Non-merged non-primary tracks are left in the event; use AtVertexTrackSelector
 * after this transform to remove them.
 *
 * Merging repeats until no further merges are possible.
 *
 * @ingroup PatternTransforms
 */
class AtFragmentMerger : public AtPatternTransform {
public:
   explicit AtFragmentMerger(double mergeDist = 30.0, double vertexRadiusXY = 80.0)
      : fMergeDist(mergeDist), fVertexRadiusXY(vertexRadiusXY)
   {
   }

   void Transform(AtPatternEvent &event) override;

   void SetMergeDist(double mm) { fMergeDist = mm; }
   void SetVertexRadiusXY(double mm) { fVertexRadiusXY = mm; }
   void SetVertexZTolerance(double mm) { fVertexZTolerance = mm; }

   /// Set a non-owning pointer to the clusterer used for per-merge re-clustering.
   /// The caller (e.g. AtPRAtask) must ensure the clusterer outlives this object.
   /// If nullptr (default), cluster arrays are cleared but not rebuilt after each merge.
   void SetClusterer(AtPATTERN::AtSmooth3DClusterer *p) { fClusterer = p; }

   /// Set a non-owning pointer to the orderer used after each merge.
   /// Caller must ensure the orderer outlives this object.
   /// If nullptr (default), ordering is skipped after merge.
   void SetOrderer(AtPATTERN::AtClusterOrderer *o) { fOrderer = o; }

private:
   double fMergeDist{30.0};
   double fVertexRadiusXY{80.0};
   double fVertexZTolerance{50.0};
   AtPATTERN::AtSmooth3DClusterer *fClusterer{nullptr}; // non-owning
   AtPATTERN::AtClusterOrderer *fOrderer{nullptr};       // non-owning
};

} // namespace AtPATTERN

#endif // ATFRAGMENTMERGER_H
