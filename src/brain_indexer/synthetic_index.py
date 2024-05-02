import numpy as np
from brain_indexer import core, MorphIndex
from brain_indexer.util import factor


class UniformFactory:
    """A class to generate uniform indexes for testing purposes.
    Currently only supports Morphology indexes using the
    `morph_index` method.

    args:
        n_elements: number of elements in the index
        n_segments_per_section: number of segments per section
        boundary: the size of the cube in which the points are generated
    """

    def __init__(self, n_elements, boundary):
        self.n_elements = n_elements
        self.boundary = boundary

    def _uniform_morph(self, n_sections, n_segments_per_section, boundary, generator):
        n_segments = n_sections * n_segments_per_section

        points = np.empty((n_segments, 2, 3), dtype=np.float32)
        points[:, 0, :] = generator.uniform(
            low=-boundary,
            high=boundary,
            size=(n_segments, 3))
        points[:, 1, :] = points[:, 0, :] + \
            generator.uniform(low=-10.0, high=10.0, size=(n_segments, 3))

        points = np.reshape(points, (-1, 3))

        rtree = core.MorphIndex()

        radii = np.ones(2 * n_segments_per_section)
        section_types = np.zeros(2 * n_segments_per_section)
        offsets = np.arange(0, 2 * n_segments_per_section, 2)

        for i in range(n_sections):
            j = i * 2 * n_segments_per_section
            morph = points[j: j + 2 * n_segments_per_section]
            rtree._add_neuron(0, morph, radii, offsets, section_types, has_soma=False)
        index = MorphIndex(rtree)

        return index

    def morph_index(self,
                    n_sections=None,
                    n_segments_per_section=None,
                    random_state=None):
        """Create a uniform index with 'n_elements' or 'n_sections' *
        'n_segments_per_section' elements in the cube [-boundary, boundary]^3.

        If n_sections and n_segments_per_section are not provided, they are
        calculated from n_elements using `brain_indexer.util.factor`.
        If n_section and n_segments_per_section are provided, n_elements is ignored.

        The index created is a synthetic one so you shouldn't expect it
        to be biologically correct. It is created by generating random points in the cube
        [-boundary, boundary]^3 and then connecting them with a line segment.
        The lenght of every segment is never longer than 10 units.
        The points are then added to the index.

        All the GIDs are set to 0, all the IDs are non-unique and the sections
        are all of type 0 (undefined).

        Radius is set to 1.0 for all the segments.

        Both 'n_section' and 'n_segments_per_section' are unbounded but please keep
        in mind that selecting a large number of sections and segments
        will result in a very large index.

        e.g. an index with 10000 sections and 1000 segments per section will have
        10M segments and will take ~500MB of disk space.

        Using the `random_state` argument you can control the random number generator.
        By default, the random seed or generator is unspecified. If needed
        the user can specify a numeric seed or a numpy random generator.
        """
        if n_sections is None or n_segments_per_section is None:
            n_sections, n_segments_per_section = factor(self.n_elements, dims=2)

        if random_state is None:
            generator = np.random.default_rng()
        elif isinstance(random_state, np.random.Generator):
            generator = random_state
        else:
            generator = np.random.default_rng(random_state)
        return self._uniform_morph(n_sections,
                                   n_segments_per_section,
                                   self.boundary,
                                   generator)
