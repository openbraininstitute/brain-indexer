#include "index.hpp"

namespace spatial_index {


template <typename T>
IndexTreeMemDisk<T> IndexTreeMemDisk<T>::open_or_create(const std::string& filename,
                                                        size_t size_mb,
                                                        bool truncate,
                                                        bool close_shrink) {
    if (truncate) {
        std::remove(filename.c_str());
    }
    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_or_create,
                                                                  filename.c_str(),
                                                                  size_mb * 1024 * 1024);
    return IndexTreeMemDisk(filename, std::move(mapped_file), close_shrink);
}


template <typename T>
IndexTreeMemDisk<T> IndexTreeMemDisk<T>::open(const std::string& filename){
    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_only, filename.c_str());
    return IndexTreeMemDisk(filename, std::move(mapped_file));
}


template <typename T>
IndexTreeMemDisk<T>::IndexTreeMemDisk(const std::string& fname,
                                      std::unique_ptr<bip::managed_mapped_file>&& mapped_file,
                                      bool close_shrink)
    : super(std::move(*mapped_file->find_or_construct<super>("rtree")(
          MemDiskAllocator<T>(mapped_file->get_segment_manager()))))
    , filename_(fname)
    , mapped_file_(std::move(mapped_file))
    , close_shrink_(close_shrink)
{ }


template <typename T>
void IndexTreeMemDisk<T>::close() {
    if (!mapped_file_) {
        return;
    }
    auto obj = mapped_file_->find<super>("rtree").first;
    *obj = static_cast<super&&>(*this);  // sync object to mem mapped file, moving

    // Deinitialize remaining ones, so out condition is met
    mapped_file_->flush();
    mapped_file_.reset();

    if (close_shrink_) {
        bip::managed_mapped_file::shrink_to_fit(filename_.c_str());
    }
}


}  // namespace spatial_index
