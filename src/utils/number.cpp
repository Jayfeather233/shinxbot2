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

int64_t my_string2int64(const std::wstring &s)
{
    int64_t ans = 0;
    int64_t f = 1;
    bool is_digit = false;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == L'-') {
            f = -1;
            is_digit = true;
        }
        else if (L'0' <= s[i] && s[i] <= L'9') {
            ans = ans * 10 + s[i] - L'0';
            is_digit = true;
        } else if(is_digit){
            break;
        }
    }
    return ans * f;
}

int64_t my_string2int64(const std::string &s)
{
    int64_t ans = 0;
    int64_t f = 1;
    bool is_digit = false;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '-') {
            f = -1;
            is_digit = true;
        }
        else if ('0' <= s[i] && s[i] <= '9') {
            ans = ans * 10 + s[i] - '0';
            is_digit = true;
        } else if(is_digit){
            break;
        }
    }
    return ans * f;
}
