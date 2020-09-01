#pragma once

#include "matcher.hpp"
#include "sha1.hpp"
#include <chrono>
#include <map>

struct PathEntry {
    std::filesystem::path path;
    bool in_hash_map;
};

struct FileStats {
    size_t num_files = 0;
    size_t total_size = 0;
    size_t num_hashed = 0;
    size_t total_hashed_size = 0;
    std::chrono::nanoseconds hash_duration = {};
};

inline auto scan_folder(const std::filesystem::path &path,
                        const Matcher &matcher,
                        bool include_empty_files = false) {

    namespace fs = std::filesystem;
    using clock = std::chrono::high_resolution_clock;
    SHA1_Digester digester;
    FileStats stats;

    // Data structures for storing two levels of dictionaries:
    //
    // The first one uses the file size as the key, and as the
    // value, it stores the file path, and whether the entry is
    // included in the second dictionary or not.
    //
    // The second dictionary uses the MD5 hash as the key, and
    // a pointer to the corresponding entry in the first
    // dictionary as the value.
    using size_map_t = std::multimap<std::size_t, PathEntry>;
    size_map_t size_map;
    using hash_map_t =
        std::multimap<SHA1_Digester::hash_t, size_map_t::const_pointer>;
    hash_map_t hash_map;

    // Iterate over all files in the given folder recursively
    const auto opt = fs::directory_options::skip_permission_denied;
    for (auto direntry : fs::recursive_directory_iterator(path, opt)) {
        if (!direntry.is_regular_file() || direntry.is_symlink() ||
            !matcher(direntry.path()))
            continue;

        // Get the file size
        auto size = direntry.file_size();

        // Ignore empty files, unless the option "includ-empty-files"
        // was passed
        if (size == 0 && !include_empty_files)
            continue;

        ++stats.num_files;
        stats.total_size += size;

        // Use the file size as an index in the first map, search
        // it to see if a file with this size already exists
        const auto old_it = size_map.lower_bound(size);

        // If a file with the same size is already in the size map
        if (old_it != size_map.end() && old_it->first == size) {
            // Insert the new entry right before the old one.
            // Since there are now at least two entries with the
            // same size, we'll have to compute their hashes.
            // The second argument is "true" to indicate that the
            // hash is available for this entry
            const auto new_it = size_map.emplace_hint(
                old_it, size, PathEntry{direntry.path(), true});

            // If this is only the second file with this size,
            // (meaning that this is the first collision in the size map)
            // that means that the first file that had this size hasn't been
            // hashed yet, so do it now, and add it to the list
            const auto &[old_key, old_val] = *old_it;
            if (!old_val.in_hash_map) {
                auto t0 = clock::now();
                const auto old_hash = digester.digest_file(old_val.path);
                auto t1 = clock::now();
                digester.reset();
                hash_map.emplace(old_hash, &*old_it);
                ++stats.num_hashed;
                stats.total_hashed_size += size;
                stats.hash_duration += t1 - t0;
            }

            // Also hash the second file with the same size and add it
            // to the hash map
            auto t0 = clock::now();
            const auto new_hash = digester.digest_file(direntry.path());
            auto t1 = clock::now();
            digester.reset();
            hash_map.emplace(new_hash, &*new_it);
            ++stats.num_hashed;
            stats.total_hashed_size += size;
            stats.hash_duration += t1 - t0;
        }
        // If this is the first file with this particular size
        else {
            // Simply insert it into the size list without computing
            // the hash, so the second argument is "false" (no hash
            // available yet)
            size_map.emplace_hint(old_it, size,
                                  PathEntry{direntry.path(), false});
        }
    }

    return std::make_tuple(std::move(size_map), std::move(hash_map), stats);
}

#include "utilities.hpp"

std::ostream &operator<<(std::ostream &os, FileStats stats) {
    os << "Scanned " << stats.num_files << " files, totalling "
       << DetailedByteFormatter(stats.total_size) << " in size.\n";
    os << "Hashed " << stats.num_hashed << " files, totalling "
       << DetailedByteFormatter(stats.total_hashed_size)
       << " in size, at an average rate of "
       << ByteFormatter(std::round(1e9 * stats.total_hashed_size /
                                   stats.hash_duration.count()))
       << "/s.";
    return os;
}