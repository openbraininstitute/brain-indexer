import argparse

from spatial_index.synthetic_index import UniformFactory


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a uniform index with N points")
    parser.add_argument("n_elements", type=int, default=1000,
                        help="Number of elements to generate. Default:1000. If the number of section AND the number of segments per section are provided, the number of elements will be ignored.")
    parser.add_argument("--output", type=str,
                        default="uniform_index", help="Output folder. Default: uniform_index")
    parser.add_argument("--n_sections", type=int, default=None,
                        help="Number of sections to generate. Needs to be provided together with the number of segments per section or will be ignored.")
    parser.add_argument("--segments_per_section", type=int, default=None,
                        help="Number of segments per section. Needs to be provided together with the number of sections or will be ignored.")
    parser.add_argument("--boundary", type=float, default=1000,
                        help="Boundary of the index. The index will be a cube of size 2*boundary. Default: 1000")
    args = parser.parse_args()
    index = UniformFactory(args.n_elements, args.boundary).morph_index(
        args.n_sections, args.segments_per_section)
    index.write(args.output)
