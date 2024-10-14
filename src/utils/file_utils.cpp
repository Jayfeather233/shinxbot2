#include "utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>

std::fstream openfile(const fs::path file_path,
                      const std::ios_base::openmode mode)
{
    if (!fs::exists(file_path)) {
        fs::path path(file_path);
        fs::create_directories(path.parent_path());
    }
    std::fstream file(file_path, mode);
    if (file.is_open()) {
        return file;
    }
    else {
        set_global_log(LOG::ERROR, "Cannot open file: " + file_path.string());
        throw(file_path.string() + ": open file failed").c_str();
    }
}

std::string readfile(const fs::path &file_path,
                     const std::string &default_content)
{
    std::ifstream afile;
    afile.open(file_path, std::ios::in);

    if (afile.is_open()) {
        std::string ans, line;
        while (!afile.eof()) {
            getline(afile, line);
            ans += line + "\n";
        }
        afile.close();
        return ans;
    }
    else {
        try {
            std::fstream ofile = openfile(file_path, std::ios::out);
            ofile << default_content;
            ofile.flush();
            ofile.close();
            return default_content;
        }
        catch (...) {
            return "";
        }
    }
}

void writefile(const fs::path file_path, const std::string &content,
               bool is_append)
{
    std::fstream ofile;
    try {
        ofile = openfile(file_path, is_append ? std::ios::app : std::ios::out);
    }
    catch (...) {
    }
    ofile << content;
    ofile.flush();
    ofile.close();
}

void command_download(const std::string &httpAddress,
                      const std::string &filePath, const std::string &fileName,
                      const bool proxy)
{
    fs::path p(filePath);
    if (!fs::exists(p)) {
        fs::create_directories(p);
    }
    p /= fileName;
    // std::cout<<p.string()<<std::endl;
    std::string command = "curl -o " + p.string() + " ";
    if (!proxy)
        command += "--noproxy '*' ";
    command += httpAddress + " > /dev/null 2>&1";
    int ret = system(command.c_str());
    if (ret) {
        std::ostringstream oss;
        oss << "download " << httpAddress << " to " << filePath << "/"
            << fileName << " Proxy=" << proxy << "failed.";
        set_global_log(LOG::ERROR, oss.str());
    }
}

void download(const std::string &httpAddress, const fs::path &filePath,
              const std::string &fileName, const bool proxy)
{
    try {
        auto ret = split_http_addr(httpAddress);
        std::string data = do_get(ret.first, ret.second, {}, proxy);
        std::fstream ofile;
        try {
            ofile =
                openfile(filePath / fileName, std::ios::out | std::ios::binary);
        }
        catch (...) {
            set_global_log(LOG::ERROR, "Cannot open file " +
                                           (filePath / fileName).string() +
                                           " for download.");
            return;
        }
        ofile << data;
        ofile.flush();
        ofile.close();
    }
    catch (const std::exception &e) {
        set_global_log(LOG::ERROR, "At download from" + httpAddress + " to " +
                                       (filePath / fileName).string() +
                                       ", Exception occurred: " + e.what());
    }
}