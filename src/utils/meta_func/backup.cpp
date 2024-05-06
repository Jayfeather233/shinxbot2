#include "backup.h"
#include "utils.h"
#include <zip.h>

void archivist::add_path(const std::filesystem::path &path,
                         const std::filesystem::path &rele_path,
                         const std::string passwd)
{
    this->arc_list.emplace_back(path, rele_path, passwd);
}

bool archivist::archive_add_file(zip_t *archive,
                                 const std::filesystem::path &path,
                                 const std::string &passwd,
                                 const std::filesystem::path &rele_path)
{
    zip_source_t *source =
        zip_source_file(archive, path.c_str(), 0, ZIP_LENGTH_TO_END);
    if (source == NULL) {
        set_global_log(LOG::ERROR, "backup file open error");
        return false;
    }
    zip_int64_t ind =
        zip_file_add(archive, (rele_path / (path.filename())).c_str(), source,
                     ZIP_FL_ENC_GUESS);
    if (ind < 0) {
        set_global_log(LOG::ERROR, "backup add file error");
        zip_source_free(source);
        return false;
    }
    if (!passwd.empty()) {
        int ret = zip_file_set_encryption(archive, ind, ZIP_EM_AES_256,
                                          passwd.c_str());
        set_global_log(LOG::ERROR, "backup file enc error");
        return false;
    }
    return true;
}
bool archivist::archive_add_dir(zip_t *archive,
                                const std::filesystem::path &path,
                                const std::string &passwd,
                                const std::filesystem::path &rele_path)
{
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if(!archive_add_path(archive, entry.path(), passwd, rele_path/path.filename())){
            return false;
        }
    }
}
bool archivist::archive_add_path(zip_t *archive,
                                 const std::filesystem::path &path,
                                 const std::string &passwd,
                                 const std::filesystem::path &rele_path)
{
    if (std::filesystem::is_directory(path)) {
        return this->archive_add_dir(archive, path, passwd, rele_path);
    }
    else if (std::filesystem::is_regular_file(path)) {
        return this->archive_add_file(archive, path, passwd, rele_path);
    }
    else {
        return true; // symbolic link?
    }
}

bool archivist::make_archive(const std::filesystem::path &path)
{
    zip_t *archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
    if (archive == NULL) {
        set_global_log(LOG::ERROR, "backup zip create error");
        return false;
    }
    else {
        for (const auto &[path, rele_path, passwd] : this->arc_list) {
            if (!this->archive_add_path(archive, path, passwd, rele_path)) {
                zip_close(archive);
                return false;
            }
        }
        zip_close(archive);
        return true;
    }
}