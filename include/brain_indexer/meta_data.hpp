#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>

#include <brain_indexer/util.hpp>


namespace brain_indexer {

    struct MetaDataConstants {
        /**
         * \brief Version of the meta data format.
         *
         *  This does not version the actual heavy files. Those might
         *  depend on other constants such as `SPATIAL_INDEX_STRUCT_VERSION`,
         *  e.g. when using boost serialization.
         * 
         *  Again this version number is used only to control which meta data
         *  reader should be used to interpret the JSON file.
         */
        static constexpr int version = 0;

        static constexpr auto memory_mapped_key = "memory_mapped";
        static constexpr auto in_memory_key = "in_memory";
        static constexpr auto multi_index_key = "multi_index";
    };

    /** \brief Join two paths safely.
     *
     * The important edges cases are:
     *   * join_path("/foo", "/bar") == "/bar"
     *   * join_path("", "bar") == "bar"; unlike `"" + "/" + "bar".
     */
    inline std::string join_path(const std::filesystem::path& stem, const std::filesystem::path& tail) {
        return (stem / tail).string();
    }


    /// \brief Create the common parts of the meta data file.
    inline nlohmann::json create_basic_meta_data(const std::string& element_type) {
        return nlohmann::json{
            {"version", MetaDataConstants::version},
            {"element_type", element_type}
        };
    }

    /// \brief Writes the meta data file to disk.
    inline void write_meta_data(const std::string& filename, const nlohmann::json& json) {
        auto os = util::open_ofstream(filename);
        os << std::setw(2) << json << std::endl;
    }

    /// \brief The file name of the meta data file.
    inline std::string default_meta_data_relpath() {
        return "meta_data.json";
    }

    /// \brief The default path of `meta_data.json`.
    inline std::string default_meta_data_path(const std::string& index_path) {
        return join_path(index_path, default_meta_data_relpath());
    }

    /** \brief Deduces the path of the directory containing the `meta_data.json`.
     *
     *  Note that relative paths inside `meta_data.json` are relative to
     *  this directory.
     *
     *  The argument `path` can either be the path of the `meta_data.json`
     *  or the index path. Therefore, this function is idempotent:
     *
     *     auto index_path = canonicalize_index_path(path);
     *     assert(index_path == canonicalize_index_path(index_path));
     */
    inline std::string canonicalize_index_path(const std::string& path) {
        auto abs_path = std::filesystem::canonical(path);

        // If the canonical path is a regular file, it's the
        // meta data file. However, the paths are still relative
        // `path` not the equivalent of `realpath ${path}`.
        if(std::filesystem::is_regular_file(abs_path)) {
            return std::filesystem::path(path).parent_path();
        }

        return path;
    }

    /// \brief Deduces the path of the `meta_data.json`.
    inline std::string deduce_meta_data_path(const std::string& path) {
        // We need to canonicalize to avoid checking if a symlink
        // is a directory/file, which it isn't but we'd only care
        // about what the target is.
        auto abs_path = std::filesystem::canonical(path);

        if(std::filesystem::is_regular_file(abs_path)) {
            return abs_path;
        }
        else if(std::filesystem::is_directory(abs_path)) {
            auto filename = default_meta_data_path(abs_path);
            if(!std::filesystem::is_regular_file(filename)) {
                throw std::runtime_error("No such file: " + filename);
            }

            return filename;
        }
        else {
            throw std::runtime_error("Can't deduce meta data file from path: " + path);
        }
    }

    /// \brief Loads the `meta_data.json` from disk.
    inline nlohmann::json read_meta_data(const std::string& meta_data_path) {
        auto filename = deduce_meta_data_path(meta_data_path);
        auto is = brain_indexer::util::open_ifstream(filename, std::ios_base::in);
        auto meta_data = nlohmann::json{};
        is >> meta_data;

        return meta_data;
    }

    /** \brief Resolves relative paths stored in the `meta_data.json`.
      *
      * The `base_path` is either the path of `meta_data.json` or it path of the
      * directory its stored in.
      */
    inline std::string
    resolve_meta_data_path(const std::string& base_path, const std::string& relpath) {
        auto index_path = canonicalize_index_path(base_path);
        return join_path(index_path, relpath);
    }

    /// \brief Resolves the `heavy_data_path` in `subsection`.
    inline std::string
    resolve_heavy_data_path(const std::string& path, const std::string& subsection) {
        auto meta_data = read_meta_data(path);
        return resolve_meta_data_path(path, meta_data[subsection]["heavy_data_path"]);
    }
}
