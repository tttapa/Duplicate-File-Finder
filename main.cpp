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
#include <regex>
#include <vector>
#include <fmt/format.h>

#include "scanner.hpp"

namespace fs = std::filesystem;
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
    po::options_description desc("Allowed options");
    po::positional_options_description pos;
    po::variables_map vm;
    desc.add_options()                         //
        ("help,h", "Print this help message.") //
        ("dir,d", po::value<std::string>()->value_name("<directory>"),
         "The directory to process.") //
        ("include,i",
         po::value<std::vector<std::string>>()->value_name("<regex>"),
         "Include only files that match these patterns.") //
        ("exclude,e",
         po::value<std::vector<std::string>>()->value_name("<regex>"),
         "Exclude any files that match these patterns.")    //
        ("sort-size,s", "Sort the output by file size.")    //
        ("include-empty-files", "Include all empty files.") //
        ;
    pos.add //
        ("dir", -1);

    { // Parse command line arguments:
        auto parser = po::command_line_parser(argc, argv);
        po::store(parser.options(desc).positional(pos).run(), vm);
        po::notify(vm);
        if (vm.count("help")) {
            fmt::print(stderr, R"(Find duplicate files.

Usage: {} [options] [directory]

If no directory is provided, the current working directory is used.

)",
                       argv[0]);
            std::cerr << desc << "\n";
            return 0;
        }
    }

    fs::path path = fs::current_path();
    if (vm.count("dir")) {
        path = fs::path(vm["dir"].as<std::string>());
        std::cout << "Path: " << path << std::endl;
    }

    bool include_empty_files = vm.count("include-empty-files");
    bool sort_by_size = vm.count("sort-size");

    const auto get_default = [](po::variable_value entry, auto def) {
        return entry.empty() ? def : entry.template as<decltype(def)>();
    };
    Matcher matcher(get_default(vm["include"], std::vector<std::string>()),
                    get_default(vm["exclude"], std::vector<std::string>()));

    const auto &[size_map, hash_map, stats] =
        scan_folder(path, matcher, include_empty_files);

    // Print the stats
    std::cout << stats << std::endl;

    if (hash_map.size() > 0) {
        using iter_t = decltype(hash_map)::const_iterator;
        std::pmr::monotonic_buffer_resource pmrbuff;
        std::pmr::vector<iter_t> sorted_its(&pmrbuff);

        // Find the first element of each block of multiple files
        // with the same hash. Ignore hashes that only occur once.
        // Insert an iterator to the first element of such a block
        // in a vector.
        auto it = hash_map.begin();
        auto const end = hash_map.end();
        while (it != end) {
            const auto prev = it;
            if (++it != end && prev->first == it->first) {
                sorted_its.push_back(prev);
                while (++it != end && prev->first == it->first)
                    ;
            }
        }

        // Sort that vector either by file path or by file size.
        const auto cmp_path = +[](iter_t a, iter_t b) {
            return a->second->second.path < b->second->second.path;
        };
        const auto cmp_size = +[](iter_t a, iter_t b) {
            return a->second->first > b->second->first;
        };
        const auto cmp = sort_by_size ? cmp_size : cmp_path;
        std::sort(sorted_its.begin(), sorted_its.end(), cmp);

        // Print the sorted results
        puts("\nDuplicate files:");
        puts("----------------\n");
        for (const auto it : sorted_its) {
            std::cout << DetailedByteFormatter(it->second->first) << '\n';
            for (auto hash_it = it; hash_it->first == it->first; ++hash_it)
                puts(hash_it->second->second.path.c_str());
            puts("");
        }

    } else {
        puts("\nNo duplicate files.");
    }
}