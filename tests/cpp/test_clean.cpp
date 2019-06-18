
#include <spatial_index/index.hpp>


using namespace spatial_index;

int main() {
    // create the rtree using default constructor
    IndexTree rtree;

    // fill the spatial index
    printf("filling index with objects\n");
    Sphere spheres[] = {Sphere{{.0, .0}, 1.}, Sphere{{4., 0.}, 1.5}};
    unsigned long i = 0;
    for (auto const& x : spheres)  {
        rtree.insert(ISoma{i++, x});
    }
    rtree.insert(ISegment{i++, 1, {{2.,0.}, {4.,0.}, 1.}});

    Box3D query_box(Point3D(2, 0), Point3D(3, 1));
    std::vector<IndexEntry> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    printf("Num objects: %lu\n", result_s.size());
    printf("Selected object: %f\n", boost::get<ISoma>(result_s.front()).radius);

    index_dump(rtree, "myrtree.tree");

    {
        IndexTree t2(index_load("myrtree.tree"));
        result_s.clear();
        t2.query(bgi::intersects(query_box), std::back_inserter(result_s));
        printf("Num objects: %lu\n", result_s.size());
        printf("Selected object: %f\n", boost::get<ISoma>(result_s.front()).radius);
    }

    return 0;
}
