#ifndef ATPATTERNFINDER_H
#define ATPATTERNFINDER_H

#include <memory>

class AtEvent;
class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Interface for finding track candidates in a hit cloud.
 *
 * Implementations assign hits to tracks and return an AtPatternEvent
 * containing raw track candidates and noise hits. Tracks contain only
 * raw hits -- no clustering, ordering, or geometric parameters.
 *
 * @defgroup PatternFinders Pattern Finders
 */
class AtPatternFinder {
public:
   virtual ~AtPatternFinder() = default;

   /// Find track candidates in a hit cloud.
   virtual std::unique_ptr<AtPatternEvent> FindTracks(AtEvent &event) = 0;
};

} // namespace AtPATTERN

#endif // ATPATTERNFINDER_H
