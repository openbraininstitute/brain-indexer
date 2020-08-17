#pragma once

#include "grid_common.hpp"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <fstream>
#include <string>
#include <vector>

namespace spatial_index {

namespace detail {


/// \brief A helper to translate a voxel id to a filename string
inline std::string voxel2name(const voxel_id_type& key) {
    return (boost::format("VX_%d_%d_%d.rtree") % key[0] % key[1] % key[2]).str();
}


/// \brief A helper to build a voxel_id from a filename string
inline voxel_id_type filename2id(const std::string& filename) {
    voxel_id_type id;
    if (std::sscanf(filename.c_str(), "VX_%d_%d_%d.rtree", &id[0], &id[1], &id[2])
            != 3) {
        throw std::runtime_error("SpatialIndex: Invalid rtree filename - " + filename);
    }
    return id;
}


/// \brief The mode how to open a MultiIndex
enum class OpenMode {
    Read,
    ReadWrite,
    WriteTruncate
};


/// \brief A disk-based set of spatial indexes
template <typename IndexT>
class IndexDB {

public:
    typedef IndexT index_type;

    /** \brief Open or create a disk-based set of spatial-indexes
     *
     * \param path: The path to the IndexDB
     * \param m: The open-mode (OpenMode::ReadOnly, ReadWrite, WriteTruncate)
     * \param voxel_size: When creating, specify the voxel size
     */
    inline explicit IndexDB(const std::string& path, OpenMode m=OpenMode::Read, int voxel_length=-1);

    index_type load(const voxel_id_type& key) const noexcept {
        return index_type(path_ + "/" + voxel2name(key));
    }
    void store(const voxel_id_type& key, const index_type& index) const {
        index.dump(path_ + "/" + voxel2name(key));
        std::ofstream{meta_path(), std::ios::app} << voxel2name(key) << std::endl;
    }
    void load_into(const voxel_id_type& key, std::vector<index_type>& vec) const {
        vec.emplace_back(path_ + "/" + voxel2name(key));
    }
    const auto& voxels_avail() const noexcept {
        return voxels_avail_;
    }
    operator bool() const noexcept {
        return bool(path_);
    }

private:
    template <typename>
    friend class ::spatial_index::MultiIndex;

    std::string meta_path() const noexcept {
        return path_ + "/_meta.txt";
    }
    std::string path_;
    int voxel_length_;
    std::vector<voxel_id_type> voxels_avail_;

};


template <typename IndexT>
inline IndexDB<IndexT>::IndexDB(const std::string& path, OpenMode m, int voxel_length)
    : path_(path)
    , voxel_length_(voxel_length)
{
    namespace fs = boost::filesystem;
    const auto meta_file = meta_path();

    if (m == OpenMode::WriteTruncate && voxel_length == -1) {
        throw std::invalid_argument("IndexDB voxel_length must be set for WriteTruncate");
    }

    if (m == OpenMode::ReadWrite || m == OpenMode::WriteTruncate) {
        fs::create_directories(path_);
        if (m == OpenMode::WriteTruncate) {
            std::ofstream f{meta_file, std::ios::trunc};
            f << "length=" << voxel_length_ << std::endl;
            return;
        }
        else std::ofstream{meta_file};  // create if non existing
    }

    if (!fs::exists(meta_file)) {
        throw std::runtime_error("MultiIndex source doesnt exist: " + path_);
    }

    // mode is Read or ReadWrite
    std::ifstream f{meta_file};
    std::string line;

    // Retrieve voxel length from 1st line
    std::getline(f, line);
    line.erase(0ul, line.find('=') + 1);
    voxel_length_ = std::stoi(line);

      // Then the names of available indexes
    while (std::getline(f, line)) {
        voxels_avail_.push_back(filename2id(line));
    }
}


}  // namespace grid_db

}  // namespace spatial_index
