#include "utils.h"

#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>
#include <filesystem>

std::fstream openfile(const std::string file_path, const std::ios_base::openmode mode){
    if(!std::filesystem::exists(file_path)){
        std::filesystem::path path(file_path);
        std::filesystem::create_directories(path.parent_path());
    }
    std::fstream file(file_path, mode);
    if(file.is_open()){
        return file;
    } else {
        std::cerr << " Cannot open file: " << file_path <<std::endl;
        throw (file_path + ": open file failed").c_str();
    }
}

std::string readfile(const std::string &file_path, const std::string &default_content){
    std::ifstream afile;
    afile.open(file_path, std::ios::in);

    if(afile.is_open()){
        std::string ans, line;
        while (!afile.eof()) {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();
        return ans;
    } else {
        try{
            std::fstream ofile = openfile(file_path, std::ios::out);
            ofile << default_content;
            ofile.flush();
            ofile.close();
            return default_content;
        } catch(...) {
            return "";
        }
    }
}

void writefile(const std::string file_path, const std::string &content, bool is_append){
    std::fstream ofile;
    try{
        ofile = openfile(file_path, is_append ? std::ios::app : std::ios::out);
    } catch (...){}
    ofile << content;
    ofile.flush();
    ofile.close();
}