#ifndef ATPATTERNTRANSFORM_H
#define ATPATTERNTRANSFORM_H

class AtPatternEvent;

namespace AtPATTERN {

/**
 * @brief Interface for transforming a pattern event.
 *
 * Implementations operate on an AtPatternEvent in place. They may modify
 * tracks (clustering, ordering, seeding), add/remove tracks (selection,
 * merging), or anything else that transforms the event.
 *
 * @defgroup PatternTransforms Pattern Transforms
 */
class AtPatternTransform {
public:
   virtual ~AtPatternTransform() = default;

   /// Transform a pattern event in place.
   virtual void Transform(AtPatternEvent &event) = 0;
};

} // namespace AtPATTERN

#endif // ATPATTERNTRANSFORM_H
