#include "utils.h"

#include <locale>
#include <codecvt>

std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;

std::wstring string_to_wstring(std::string u){
    return converter.from_bytes(u);
}
std::string wstring_to_string(std::wstring u){
    return converter.to_bytes(u);
}

std::string whitespaces = "\t\r\n ";
std::wstring w_whitespaces = L"\t\r\n ";

std::string trim(std::string u){
    size_t fir = u.find_first_not_of(whitespaces);
    if(fir == std::string::npos){
        return "";
    }
    size_t las = u.find_last_not_of(whitespaces);
    return u.substr(fir, las-fir+1);
}
std::wstring trim(std::wstring u){
    size_t fir = u.find_first_not_of(w_whitespaces);
    if(fir == std::wstring::npos){
        return L"";
    }
    size_t las = u.find_last_not_of(w_whitespaces);
    return u.substr(fir, las-fir+1);
}