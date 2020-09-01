#pragma once

#include <iomanip> // std::quoted
#include <ostream> // std::ostream
#include <string>  // std::string
#include <vector>  // std::vector

std::ostream &operator<<(std::ostream &os, const std::vector<std::string> &v) {
    if (v.empty())
        return os << "{}";

    os << '{';
    for (size_t i = 0; i < v.size() - 1; ++i) {
        os << ' ' << std::quoted(v[i]) << ',';
    }
    return os << ' ' << std::quoted(v.back()) << " }";
}

struct ByteFormatter {
    ByteFormatter(std::size_t size) : size(size) {}
    size_t size;
};

std::ostream &operator<<(std::ostream &os, ByteFormatter fmt) {
    if (fmt.size < 1024) {
        return os << fmt.size << " B";
    } else {
        double scalsize = fmt.size;
        unsigned i;
        for (i = 0; scalsize >= 1024; ++i)
            scalsize /= 1024;
        const char *units[] = {" KiB", " MiB", " GiB", " TiB", " PiB", " EiB"};
        auto prec = os.precision();
        auto flags = os.flags();
        os << std::setprecision(2) << std::fixed << scalsize << units[i - 1];
        os.precision(prec);
        os.flags(flags);
        return os;
    }
}

struct DetailedByteFormatter {
    DetailedByteFormatter(std::size_t size) : size(size) {}
    size_t size;
};

std::ostream &operator<<(std::ostream &os, DetailedByteFormatter fmt) {
    os << ByteFormatter(fmt.size);
    if (fmt.size >= 1024)
        os << " (" << fmt.size << " B)";
    return os;
}