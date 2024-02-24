#include "utils.h"

#include <codecvt>
#include <locale>

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

std::wstring string_to_wstring(const std::string &u)
{
    return converter.from_bytes(u);
}
std::string wstring_to_string(const std::wstring &u)
{
    return converter.to_bytes(u);
}

static std::string whitespaces = "\t\r\n ";
static std::wstring w_whitespaces = L"\t\r\n ";

std::string trim(const std::string &u)
{
    size_t fir = u.find_first_not_of(whitespaces);
    if (fir == std::string::npos) {
        return "";
    }
    size_t las = u.find_last_not_of(whitespaces);
    return u.substr(fir, las - fir + 1);
}

std::wstring trim(const std::wstring &u)
{
    size_t fir = u.find_first_not_of(w_whitespaces);
    if (fir == std::wstring::npos) {
        return L"";
    }
    size_t las = u.find_last_not_of(w_whitespaces);
    return u.substr(fir, las - fir + 1);
}

std::string my_replace(const std::string &s, const char old, const char ne)
{
    std::string ans;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == old) {
            ans += ne;
        }
        else {
            ans += s[i];
        }
    }
    return ans;
}
