/********************************************************************
 * AtTrackFinderTC (TriplClust)                                     *
 * Log: Class started 31-05-2021                                    *
 * Author: Y. Ayyad (yassid.ayyad@usc.es)                           *
 ********************************************************************/

#ifndef AtTRACKFINDERTC_H
#define AtTRACKFINDERTC_H

#ifdef _OPENMP
#include <omp.h>
#endif
#include "AtPatternFinder.h"

#include "cluster.h" // for Cluster
#include <stdio.h>   // for size_t

#include <memory> // for unique_ptr
#include <vector> // for vector

class PointCloud;
class AtEvent;
class AtPatternEvent;
class AtTrack;

namespace AtPATTERN {
struct hc_params {
   float s;
   size_t k;
   size_t n;
   size_t m;
   float r;
   float a;
   float t;
   float _padding;
};

/**
 * @brief TriplClust pattern finder.
 *
 * Assigns hits to raw track candidates using the TriplClust hierarchical
 * clustering algorithm (Dalitz et al.). Output tracks contain raw hits only;
 * no clustering, ordering, or geometric seeding is performed here.
 *
 * @ingroup PatternFinders
 */
class AtTrackFinderTC : public AtPatternFinder {
private:
   hc_params inputParams{.s = 0.3, .k = 19, .n = 2, .m = 15, .r = 2, .a = 0.03, .t = 4.0};

public:
   AtTrackFinderTC() = default;
   ~AtTrackFinderTC() = default;

   std::unique_ptr<AtPatternEvent> FindTracks(AtEvent &event) override;

   void SetScluster(float s) { inputParams.s = s; }
   void SetKtriplet(size_t k) { inputParams.k = k; }
   void SetNtriplet(size_t n) { inputParams.n = n; }
   void SetMcluster(size_t m) { inputParams.m = m; }
   void SetRsmooth(float r) { inputParams.r = r; }
   void SetAtriplet(float a) { inputParams.a = a; }
   void SetTcluster(float t) { inputParams.t = t; }
   void SetPadding(size_t padding) { inputParams._padding = padding; }

protected:
   void eventToClusters(AtEvent &event, PointCloud &cloud);
   void BuildRawTracksFromClusters(PointCloud &cloud, const std::vector<cluster_t> &clusters, AtEvent &event,
                                   std::vector<AtTrack> &tracks, PointCloud &noisePoints);
};

} // namespace AtPATTERN

#endif
