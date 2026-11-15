#pragma once
#include <array>
#include <algorithm>
#include <climits>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

namespace snake {
using Records = std::array<int, 3>;
inline Records readRecords(std::istream& input) {
    Records result{};
    const std::array<std::string,3> keys{{"classic", "maze", "speed"}};
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string key, extra;
        long long value;
        if (!(row >> key)) continue;
        auto found = std::find(keys.begin(), keys.end(), key);
        if (found == keys.end()) continue;
        int& target = result[static_cast<std::size_t>(found-keys.begin())];
        target = (row >> value) && value >= 0 && value <= INT_MAX && !(row >> extra) ? static_cast<int>(value) : 0;
    }
    return result;
}
inline void writeRecords(std::ostream& output, const Records& records) {
    output << "classic " << records[0] << "\nmaze " << records[1] << "\nspeed " << records[2] << '\n';
}
}
