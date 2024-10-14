#include "backup.h"
#include "utils.h"
#include <zip.h>

void archivist::add_path(const std::string &name,
                         const fs::path &path,
                         const fs::path &rele_path,
                         const std::string &passwd)
{
    this->arc_list[name].emplace_back(path, rele_path, passwd);
}

void archivist::remove_path(const std::string &name)
{
    this->arc_list.erase(name);
}

bool archivist::archive_add_file(zip_t *archive,
                                 const fs::path &path,
                                 const std::string &passwd,
                                 const fs::path &rele_path)
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
        if (ret < 0) {
            set_global_log(LOG::ERROR, "backup file enc error");
            return false;
        }
    }
    else {
        int ret = zip_file_set_encryption(archive, ind, ZIP_EM_AES_256, NULL);
        if (ret < 0) {
            set_global_log(LOG::ERROR, "backup file enc error");
            return false;
        }
    }
    return true;
}
bool archivist::archive_add_dir(zip_t *archive,
                                const fs::path &path,
                                const std::string &passwd,
                                const fs::path &rele_path)
{
    for (const auto &entry : fs::directory_iterator(path)) {
        if (!archive_add_path(archive, entry.path(), passwd,
                              rele_path / path.filename())) {
            return false;
        }
    }
    return true;
}
bool archivist::archive_add_path(zip_t *archive,
                                 const fs::path &path,
                                 const std::string &passwd,
                                 const fs::path &rele_path)
{
    if (fs::is_directory(path)) {
        return this->archive_add_dir(archive, path, passwd, rele_path);
    }
    else if (fs::is_regular_file(path)) {
        return this->archive_add_file(archive, path, passwd, rele_path);
    }
    else {
        return true; // symbolic link?
    }
}

bool archivist::make_archive(const fs::path &path)
{
    zip_t *archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
    if (archive == NULL) {
        set_global_log(LOG::ERROR, "backup zip create error");
        return false;
    }
    else {
        zip_set_default_password(archive, default_pwd.c_str());
        for (const auto &f : this->arc_list) {
            for (const auto &[path, rele_path, passwd] : f.second) {
                if (!this->archive_add_path(archive, path, passwd, rele_path)) {
                    zip_close(archive);
                    return false;
                }
            }
        }
        zip_close(archive);
        return true;
    }
}

void archivist::set_default_pwd(const std::string &pwd)
{
    this->default_pwd = pwd;
}