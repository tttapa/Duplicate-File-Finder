#pragma once

#include "utilities.hpp"
#include <iostream>
#include <regex>

/// Regex matcher to match filenames (either include or exclude them).
class Matcher {
  public:
    Matcher(const std::vector<std::string> &incl,
            const std::vector<std::string> &excl)
        : incl_re(incl.begin(), incl.end()), excl_re(excl.begin(), excl.end()) {
        if (!incl.empty())
            std::cout << "Include patterns: " << incl << std::endl;
        if (!excl.empty())
            std::cout << "Exclude patterns: " << excl << std::endl;
    }

    bool operator()(const std::string &s) const {
        const auto match = [&s](std::regex ptrn) {
            return std::regex_match(s, ptrn);
        };
        return (incl_re.empty() ||
                std::any_of(incl_re.begin(), incl_re.end(), match)) &&
               !std::any_of(excl_re.begin(), excl_re.end(), match);
    }

  private:
    std::vector<std::regex> incl_re;
    std::vector<std::regex> excl_re;
};