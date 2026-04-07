#ifndef ATPRAUTILS_H
#define ATPRAUTILS_H

class AtTrack;

namespace AtPATTERN {

/// Returns true if the track's vertex-end cluster lies within the vertex region.
/// @param track       Track to test (must have at least one cluster).
/// @param vertexZ     Reference Z position (typically max front-cluster Z in event).
/// @param radiusXY    Maximum XY distance from beam axis [mm].
/// @param zTolerance  Maximum |Z - vertexZ| [mm]. Default preserves current behavior.
bool IsVertexTrack(AtTrack &track, double vertexZ, double radiusXY, double zTolerance = 50.0);

} // namespace AtPATTERN
#endif // ATPRAUTILS_H
