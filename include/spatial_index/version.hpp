#pragma once

/** \brief The version of SpatialIndex structs.
 *
 * This enables checking if a serialized tree is compatible with this version of
 * the SI.
 *
 * Note: Bump this constant every time a breaking change was made to the data
 * structures of the indexed elements.
 *
 * The versions are:
 *   * 0, 1 : Very early pre-release. Almost certainly bugged, always recreate
 *     such indexes.
 *   * 2: Uses the first radius as the radius.
 *   * 3: Uses the average of the two radii at either endpoint.
 *   * 4: First version with version information on all objects.
 *   * 5: Adds neurite type to morphology indexes.
 *
 * Note, that due to a bug, indexes up to and including version 3 are not
 * versioned.  Hence, any `IndexTree<T, A>` with version `<= 3` has unknown
 * version information. It is therefore unsafe to open such indexes.
 */
#define SPATIAL_INDEX_STRUCT_VERSION 5
