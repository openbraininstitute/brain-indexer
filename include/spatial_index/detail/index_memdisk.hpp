#include "index.hpp"

namespace spatial_index {

namespace detail {

struct file_versioning {
    const unsigned struct_version = SPATIAL_INDEX_STRUCT_VERSION;
    const unsigned boost_version = BOOST_VERSION;
};

} // namespace detail


template <typename T>
MemDiskPtr<T> MemDiskPtr<T>::create(const std::string& index_path,
                                    size_t size_mb,
                                    bool close_shrink) {

    util::ensure_valid_output_directory(index_path);
    std::string heavy_data_relpath = "index.bin";

    auto element_type = value_to_element_type<typename T::value_type>();
    auto meta_data = create_basic_meta_data(element_type);
    meta_data[MetaDataConstants::memory_mapped_key] = {
        // The heavy data, i.e. the relative path of the memory mapped files:
        {"heavy_data_path", heavy_data_relpath}
    };
    write_meta_data(default_meta_data_path(index_path), meta_data);

    auto filename = join_path(index_path, heavy_data_relpath);
    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_or_create,
                                                                  filename.c_str(),
                                                                  size_mb * 1024 * 1024);
    mapped_file->construct<detail::file_versioning>(".version")();
    return MemDiskPtr(std::move(mapped_file), close_shrink? filename : "");
}


template <typename T>
MemDiskPtr<T> MemDiskPtr<T>::open(const std::string& path) {
    auto filename = resolve_heavy_data_path(path, MetaDataConstants::memory_mapped_key);

    auto mapped_file = std::make_unique<bip::managed_mapped_file>(bip::open_only, filename.c_str());
    auto versions = mapped_file->find<detail::file_versioning>(".version").first;

    if (versions->struct_version != SPATIAL_INDEX_STRUCT_VERSION) {
        throw std::runtime_error(
            (boost::format("Memory mapped file structs mismatch. Expected: %d, Given: %d")
                % SPATIAL_INDEX_STRUCT_VERSION
                % versions->struct_version
            ).str()
        );
    }
    if (versions->boost_version != BOOST_VERSION) {
        std::cerr << "[Warning] Boost versions mismatch! Expected: " << BOOST_VERSION
                 << " Created with: " << versions->boost_version << '\n'
                 << " -> To ensure compatibility load a Spatial Index built with the same Boost.\n";
    }
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
