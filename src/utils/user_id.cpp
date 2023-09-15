#include "utils.h"

int64_t get_userid(const std::wstring &s)
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
int64_t get_userid(const std::string &s)
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
