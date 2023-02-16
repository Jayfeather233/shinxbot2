#include "utils.h"

int64_t get_userid(std::wstring s){
    int64_t ans = 0;
    for(int i = 0; i < s.length(); i++){
        if(L'0'<=s[i] && s[i]<=L'9'){
            ans = ans * 10 + s[i] - L'0';
        }
    }
    return ans;
}
int64_t get_userid(std::string s){
    int64_t ans = 0;
    for(int i = 0; i < s.length(); i++){
        if('0'<=s[i] && s[i]<='9'){
            ans = ans * 10 + s[i] - '0';
        }
    }
    return ans;
}