#include "utils.h"

#include <iomanip>
#include <string>

std::string to_human_string(const int64_t u)
{
    std::stringstream ss;
    if (u > 1000000000)
        ss << std::fixed << std::setprecision(2) << u / 1000000000.0 << "B";
    else if (u > 1000000)
        ss << std::fixed << std::setprecision(2) << u / 1000000.0 << "M";
    else if (u > 1000)
        ss << std::fixed << std::setprecision(2) << u / 1000.0 << "k";
    else
        ss << u;
    return ss.str();
}