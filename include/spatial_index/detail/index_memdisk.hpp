#include "index.hpp"

namespace spatial_index {


template <typename T>
MemDiskPtr<T> MemDiskPtr<T>::create(const std::string& filename,
                                    size_t size_mb,
                                    bool close_shrink) {
    auto status = std::remove(filename.c_str());
    if (status != 0 && errno != ENOENT) {
        std::cerr << "File deletion failed: " << std::strerror(errno) << '\n';
        throw std::runtime_error("Could not delete existing file: " + filename);
    }
    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_or_create,
                                                                  filename.c_str(),
                                                                  size_mb * 1024 * 1024);
    return MemDiskPtr(std::move(mapped_file), close_shrink? filename : "");
}


template <typename T>
MemDiskPtr<T> MemDiskPtr<T>::open(const std::string& filename) {
    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_only, filename.c_str());
    return MemDiskPtr(std::move(mapped_file));
}


template <typename T>
MemDiskPtr<T>::MemDiskPtr(std::unique_ptr<bip::managed_mapped_file>&& mapped_file,
                          const std::string& close_shrink_fname)
    : mapped_file_(std::move(mapped_file))
    , close_shrink_fname_(close_shrink_fname)
    , tree_(mapped_file_->find_or_construct<T>("object")(
        MemDiskAllocator<typename T::value_type>(mapped_file_->get_segment_manager())))
{ }


template <typename T>
void MemDiskPtr<T>::close() {
    if (!mapped_file_) {  // was moved
        return;
    }
    mapped_file_->flush();
    mapped_file_.reset();
    if (!close_shrink_fname_.empty()) {
        std::cout << "[MemDiskPtr] Shrinking managed mapped file" << std::endl;
        bip::managed_mapped_file::shrink_to_fit(close_shrink_fname_.c_str());
    }
}


}  // namespace spatial_index
