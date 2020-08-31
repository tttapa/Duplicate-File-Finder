#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <openssl/evp.h>
#include <regex>
#include <vector>

namespace fs = std::filesystem;
namespace po = boost::program_options;

using vector_of_string = std::vector<std::string>;

std::ostream &operator<<(std::ostream &os, const vector_of_string &v) {
    if (v.empty())
        return os << "{}";

    os << '{';
    for (size_t i = 0; i < v.size() - 1; ++i) {
        os << ' ' << std::quoted(v[i]) << ',';
    }
    return os << ' ' << std::quoted(v.back()) << " }";
}

class Matcher {
  public:
    Matcher(vector_of_string incl, vector_of_string excl)
        : incl_re(incl.begin(), incl.end()), excl_re(excl.begin(), excl.end()) {
        if (!incl.empty())
            std::cout << "Include patterns: " << incl << std::endl;
        if (!excl.empty())
            std::cout << "Exclude patterns: " << excl << std::endl;
    }

    bool operator()(const std::string &s) const {
        for (const auto &ptrn : incl_re) {
            if (!std::regex_match(s, ptrn))
                return false;
        }
        for (const auto &ptrn : excl_re)
            if (std::regex_match(s, ptrn))
                return false;
        return true;
    }

  private:
    std::vector<std::regex> incl_re;
    std::vector<std::regex> excl_re;
};

class MD5_Digester {
  public:
    MD5_Digester() { EVP_DigestInit_ex(mdctx, md, NULL); }
    ~MD5_Digester() { EVP_MD_CTX_free(mdctx); }

    void reset() {
        EVP_MD_CTX_reset(mdctx);
        EVP_DigestInit_ex(mdctx, md, NULL);
    }

    void digest(const void *data, std::size_t length) {
        EVP_DigestUpdate(mdctx, data, length);
    }

    std::vector<unsigned char> get() const {
        std::vector<unsigned char> md_value(EVP_MD_size(md));
        EVP_DigestFinal(mdctx, md_value.data(), nullptr);
        return md_value;
    }

    std::vector<unsigned char> digest_file(const fs::path &path) {
        std::ifstream f(path);
        buff.resize(1024ul * EVP_MD_CTX_block_size(mdctx));
        while (f) {
            auto sz = f.read(buff.data(), buff.size()).gcount();
            digest(buff.data(), sz);
        }
        return get();
    }

  private:
    const EVP_MD *md = EVP_md5();
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    std::vector<char> buff;
};

int main(int argc, char *argv[]) {
    po::options_description desc("Backup Hash");
    po::positional_options_description pos;
    po::variables_map vm;
    desc.add_options()                         //
        ("help,h", "Print this help message.") //
        ("exclude,e", po::value<vector_of_string>(),
         "Exclude any files that match these patterns.") //
        ("include,i", po::value<vector_of_string>(),
         "Include only files that match these patterns.") //
        ("dir,d", po::value<std::string>(),
         "The directory to process.") //
        ;
    pos.add //
        ("dir", -1);

    {
        auto parser = po::command_line_parser(argc, argv);
        po::store(parser.options(desc).positional(pos).run(), vm);
        po::notify(vm);
    }

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    fs::path path = fs::current_path();
    if (vm.count("dir")) {
        path = fs::path(vm["dir"].as<std::string>());
        std::cout << "Path: " << path << std::endl;
    }

    const auto get_default = [](po::variable_value entry, auto def) {
        return entry.empty() ? def : entry.template as<decltype(def)>();
    };
    Matcher matcher(get_default(vm["include"], vector_of_string()),
                    get_default(vm["exclude"], vector_of_string()));

    MD5_Digester digester;

    struct PathEntry {
        fs::path path;
        bool in_hash_map;
    };

    using hash_t = std::vector<unsigned char>;
    std::multimap<std::size_t, PathEntry> size_map;
    std::multimap<hash_t, fs::path const *> hash_map;

    const auto opt = fs::directory_options::skip_permission_denied;
    for (auto direntry : fs::recursive_directory_iterator(path, opt)) {
        if (!direntry.is_regular_file() || !matcher(direntry.path()))
            continue;

        // Get the file size
        auto size = direntry.file_size();

        // Use the file size as an index in the first map, search
        // it to see if a file with this size already exists
        const auto old_it = size_map.lower_bound(size);

        // If a file with the same size is already in the size map
        if (old_it != size_map.end() && old_it->first == size) {
            // Insert the new entry right before the old one.
            // Since there are now at least two entries with the
            // same size, we'll have to compute their hashes.
            // The second argument is "true" to indicate that the
            // hash is available
            const auto new_it = size_map.emplace_hint(
                old_it, size, PathEntry{direntry.path(), true});

            // If this is only the second file with this size,
            // (i.e. the first collision in the size map) that means
            // that the first file that had this size hasn't been
            // hashed yet, so do it now, and add it to the list
            const auto &[old_key, old_val] = *old_it;
            if (!old_val.in_hash_map) {
                const auto old_hash = digester.digest_file(old_val.path);
                digester.reset();
                hash_map.emplace(old_hash, &old_val.path);
            }

            // Also hash the second file with the same size and add it
            // to the hash map
            const auto new_hash = digester.digest_file(direntry.path());
            digester.reset();
            hash_map.emplace(new_hash, &new_it->second.path);
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

    if (hash_map.size() > 0) {

        const bool sorted = true;
        using iter_t = decltype(hash_map)::const_iterator;
        std::pmr::monotonic_buffer_resource pmrbuff;
        std::pmr::vector<iter_t> sorted_its(&pmrbuff);

        puts("\nDuplicate files:");
        puts("----------------\n");
        auto it = hash_map.begin();
        auto prev = it;
        auto const end = hash_map.end();
        bool printed_first = false;
        for (++it; it != end; ++it) {
            if (prev->first == it->first) {
                if (!printed_first) {
                    if (!sorted)
                        puts(prev->second->c_str());
                    else
                        sorted_its.push_back(prev);
                }
                printed_first = true;
                if (!sorted)
                    puts(it->second->c_str());
            } else {
                if (printed_first)
                    if (!sorted)
                        puts("===");
                printed_first = false;
            }
            prev = it;
        }
        if (sorted) {
            const auto cmp = [](iter_t a, iter_t b) {
                return a->second->native() < b->second->native();
            };
            std::sort(sorted_its.begin(), sorted_its.end(), cmp);
            for (auto it : sorted_its) {
                for (auto hash_it = it; hash_it->first == it->first;
                     ++hash_it) {
                    puts(hash_it->second->c_str());
                }
                puts("===");
            }
        }
    } else {
        puts("\nNo duplicate files.");
    }
}