#include "utils.h"

int64_t get_userid(const std::wstring &s){
    int64_t ans = 0;
    for(size_t i = 0; i < s.length(); i++){
        if(L'0'<=s[i] && s[i]<=L'9'){
            ans = ans * 10 + s[i] - L'0';
        }
    }
    return ans;
}
int64_t get_userid(const std::string &s){
    int64_t ans = 0;
    for(size_t i = 0; i < s.length(); i++){
        if('0'<=s[i] && s[i]<='9'){
            ans = ans * 10 + s[i] - '0';
        }
    }
    return ans;
}