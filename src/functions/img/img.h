#pragma once

#include "processable.h"
#include <jsoncpp/json/json.h>
#include <map>
#include <set>

template <typename T1, typename T2> class bi_map {
private:
    std::map<T1, std::vector<T2>> mp1;
    std::map<T2, std::vector<T1>> mp2;

public:
    void insert(const T1 &a, const T2 &b)
    {
        mp1[a].push_back(b);
        mp2[b].push_back(a);
    }
    void erase_by_first(const T1 &a)
    {
        auto it = mp1.find(a);
        if (it != mp1.end()) {
            for (const auto &b : it->second) {
                auto &vec = mp2[b];
                vec.erase(std::remove(vec.begin(), vec.end(), a), vec.end());
                if (vec.empty()) {
                    mp2.erase(b);
                }
            }
            mp1.erase(it);
        }
    }
    void erase_by_second(const T2 &b)
    {
        auto it = mp2.find(b);
        if (it != mp2.end()) {
            for (const auto &a : it->second) {
                auto &vec = mp1[a];
                vec.erase(std::remove(vec.begin(), vec.end(), b), vec.end());
                if (vec.empty()) {
                    mp1.erase(a);
                }
            }
            mp2.erase(it);
        }
    }
    // get all possible T1 elements
    std::vector<T1> get_by_first() const
    {
        std::vector<T1> keys;
        for (const auto &pair : mp1) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    const std::vector<T2> &get_by_first(const T1 &a) const
    {
        static const std::vector<T2> empty;
        auto it = mp1.find(a);
        if (it != mp1.end()) {
            return it->second;
        }
        else {
            return empty;
        }
    }
    // get all possible T2 elements
    std::vector<T2> get_by_second() const
    {
        std::vector<T2> keys;
        for (const auto &pair : mp2) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    const std::vector<T1> &get_by_second(const T2 &b) const
    {
        static const std::vector<T1> empty;
        auto it = mp2.find(b);
        if (it != mp2.end()) {
            return it->second;
        }
        else {
            return empty;
        }
    }
};

class img : public processable {
private:
    bi_map<std::wstring, std::string> name_uuid; // name <-> uuid
    bi_map<std::string, groupid_t> uuid_groupid; // uuid -> groupids

    std::map<std::string, size_t> image_size; // uuid->count
    std::vector<std::string> image_list;      // all uuids
    std::vector<std::string> default_image_list;

    std::map<userid_t, std::tuple<bool, groupid_t, std::wstring>> is_adding;
    std::map<userid_t, std::tuple<bool, groupid_t, std::wstring>> is_deling;

    bool process_command(std::string message, const msg_meta &conf);
    void add_images(const std::wstring &message, const std::string &name,
                    const groupid_t &groupid, const msg_meta &conf);

    void del_images(const std::wstring &name, const groupid_t &groupid,
                    const std::string &index, const msg_meta &conf);
    bool process_add_images(const std::wstring &msg, const std::string &name,
                            const groupid_t &groupid, const msg_meta &conf);
    bool process_del_images(const std::wstring &name, const groupid_t &groupid,
                            const std::string &index, const msg_meta &conf);

public:
    img();
    void save();

    void process(std::string message, const msg_meta &conf);
    bool check(std::string message, const msg_meta &conf);
    std::string help();
    void set_backup_files(archivist *p, const std::string &name);
};

DECLARE_FACTORY_FUNCTIONS_HEADER