#include "utils.h"

long get_userid(std::wstring s){
    long ans = 0;
    for(int i = 0; i < s.length(); i++){
        if(L'0'<=s[i] && s[i]<=L'9'){
            ans = ans * 10 + s[i] - L'0';
        }
    }
    return ans;
}
long get_userid(std::string s){
    long ans = 0;
    for(int i = 0; i < s.length(); i++){
        if('0'<=s[i] && s[i]<='9'){
            ans = ans * 10 + s[i] - '0';
        }
    }
    return ans;
}