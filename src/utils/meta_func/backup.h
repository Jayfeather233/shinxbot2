#pragma once

#include <filesystem>
#include <vector>
#include <zip.h>

#ifndef ZIP_LENGTH_TO_END
#define ZIP_LENGTH_TO_END -1
#endif

class archivist {
private:
    std::vector<
        std::tuple<std::filesystem::path, std::filesystem::path, std::string>>
        arc_list; // pair<file, rele_path, passwd>

    bool archive_add_path(
        zip_t *archive, const std::filesystem::path &path,
        const std::string &passwd,
        const std::filesystem::path &rele_path = std::filesystem::path());

    bool archive_add_dir(
        zip_t *archive, const std::filesystem::path &path,
        const std::string &passwd,
        const std::filesystem::path &rele_path = std::filesystem::path());

    bool archive_add_file(
        zip_t *archive, const std::filesystem::path &path,
        const std::string &passwd,
        const std::filesystem::path &rele_path = std::filesystem::path());

public:
    /**
     * Please do not add files under ./config path,
     * since all files were automatically added
     */
    void
    add_path(const std::filesystem::path &path,
             const std::filesystem::path &rele_path = std::filesystem::path(),
             const std::string passwd = "");
    bool make_archive(const std::filesystem::path &path);
};