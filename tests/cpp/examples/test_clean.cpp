#include <filesystem>

#include <brain_indexer/index.hpp>


using namespace brain_indexer;

int main() {
    // create the rtree using default constructor
    IndexTree<MorphoEntry> rtree;

    // fill the spatial index
    printf("filling index with objects\n");
    Sphere spheres[] = {Sphere{{.0, .0}, 1.}, Sphere{{4., 0.}, 1.5}};
    unsigned long i = 1;
    for (auto const& x: spheres) {
        rtree.insert(Soma{i++, x});
    }
    rtree.insert(Segment{i++, 1u, 1u, Point3D{2., 0.}, Point3D{4., 0.}, 1.f, SectionType::undefined});

    Box3D query_box(Point3D(2, 0), Point3D(3, 1));

    std::vector<gid_segm_t> result_s;

    rtree.query(bgi::intersects(query_box), iter_gid_segm_getter(result_s));

    printf("Num objects: %lu\n", result_s.size());
    printf("Selected gid: %lu\n", result_s.front().gid);

    std::string index_path = "myrtree.tree";
    rtree.dump(index_path);

    {
        IndexTree<MorphoEntry> t2(index_path);
        std::vector<identifier_t> gids;
        t2.query(bgi::intersects(query_box), iter_ids_getter(gids));
        printf("Num objects: %lu\n", gids.size());
        printf("Selected gid: %lu\n", gids.front());
    }

    std::filesystem::remove_all(index_path);

    return 0;
}
