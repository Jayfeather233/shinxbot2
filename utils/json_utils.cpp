#include "utils.h"

#include <iostream>
#include <jsoncpp/json/json.h>

Json::Value string_to_json(std::string str){
    Json::CharReaderBuilder ReaderBuilder;
    ReaderBuilder["emitUTF8"] = true;//utf8支持,不加这句,utf8的中文字符会编程\uxxx
    std::unique_ptr<Json::CharReader> charread(ReaderBuilder.newCharReader());
    Json::Value root;
    std::string strerr;
    bool isok = charread->parse(str.c_str(),str.c_str()+str.size(),&root,&strerr);
    if(!isok || strerr.size() != 0){
        setlog(LOG::ERROR, "string to json failed: " + strerr);
        std::cerr<<str<<std::endl;
    }
    return root;
}