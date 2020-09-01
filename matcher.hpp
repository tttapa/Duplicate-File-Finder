#pragma once

#include "utilities.hpp"
#include <iostream>
#include <regex>

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