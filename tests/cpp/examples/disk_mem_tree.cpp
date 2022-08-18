
#include <filesystem>

#include <spatial_index/index.hpp>

using namespace spatial_index;
constexpr auto FILENAME = "rtree.mem";


int main() {
    {
        std::cerr << "Starting..." << std::endl;
        auto tree = IndexTreeMemDisk<Sphere>::create(FILENAME, 1, true);
        std::cerr << "Created" << std::endl;
        auto tree2 = std::move(tree);
        tree2->insert(Sphere{{.0, .0}, 1.});
        tree2->insert(Sphere{{.0, .0}, 1.});
        tree2->insert(Sphere{{.0, .0}, 1.});
        std::cerr << "We inserted some elems! Cur count=" << tree2->size() << std::endl;
        tree2.close();
        auto tree3 = IndexTreeMemDisk<Sphere>::open(FILENAME);
        std::cerr << "We reopened the tree! Cur count=" << tree3->size() << std::endl;
        auto tree4 = std::move(tree3);
        std::cerr << "We reopened the tree in same context shrunk! Cur count=" << tree4->size()
                  << std::endl;
        if (tree4->size() != 3) {
            return 1;
        }
    }
    {
        auto tree = IndexTreeMemDisk<Sphere>::open(FILENAME);
        std::cerr << "We reopened the tree! Cur count=" << tree->size() << std::endl;
        tree.close();
        auto tree2 = IndexTreeMemDisk<Sphere>::open(FILENAME);
        std::cerr << "We reopened the tree! Cur count=" << tree2->size() << std::endl;
        if (tree2->size() != 3) {
            return 1;
        }

    }

    std::filesystem::remove_all(FILENAME);
    return 0;
}
