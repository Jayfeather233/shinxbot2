#pragma once

#include <filesystem>
#include <map>
#include <vector>
#include <zip.h>

#ifndef ZIP_LENGTH_TO_END
#define ZIP_LENGTH_TO_END -1
#endif

namespace fs = std::filesystem;

class archivist {
private:
    std::map<std::string,
             std::vector<std::tuple<fs::path,
                                    fs::path, std::string>>>
        arc_list; // pair<file, rele_path, passwd>
    std::string default_pwd;

    bool archive_add_path(
        zip_t *archive, const fs::path &path,
        const std::string &passwd,
        const fs::path &rele_path = fs::path());

    bool archive_add_dir(
        zip_t *archive, const fs::path &path,
        const std::string &passwd,
        const fs::path &rele_path = fs::path());

    bool archive_add_file(
        zip_t *archive, const fs::path &path,
        const std::string &passwd,
        const fs::path &rele_path = fs::path());

public:
    /**
     * Please do not add files under ./config path,
     * since all files were automatically added
     */
    void
    add_path(const std::string &name, const fs::path &path,
             const fs::path &rele_path = fs::path(),
             const std::string &passwd = "");
    void remove_path(const std::string &name);
    bool make_archive(const fs::path &path);
    void set_default_pwd(const std::string &pwd);
};