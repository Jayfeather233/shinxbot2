#pragma once

#include "utils.h"
#include "processable.h"

#include <map>

class Cat
{
private:
    enum Place
    {
        LIVING_ROOM,
        KITCHEN,
        BEDROOM,
        BATHROOM
    };
    std::string generateRandomColor();
    std::string generateRandomPattern();
    std::string getLocationString(Place location);
    std::string get_random_text(const Json::Value &J);

    std::string name;
    std::string bodyColor;
    std::string colorPattern;
    int food;
    int water;
    int affection;
    time_t lastVisitTime;
    Place location;
    int64_t _id;
    std::string pat();
    std::string feed();
    std::string water_f();
    std::string play();
    std::string care();
    std::string move();

public:
    Cat(const std::string &name, int64_t user_id);
    Cat(int64_t user_id = -1);
    std::string getinfo();
    std::string get_humanread_info();
    std::string intro();
    std::string adopt();
    std::string process(const std::string &input);
    void save_cat();
};

class catmain : public processable {
private:
    static Json::Value cat_text;
    std::map<int64_t, Cat> cat_map;
    void save_map();
public:
    catmain();
    static Json::Value get_text();
    void process(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    bool check(std::string message, std::string message_type, int64_t user_id, int64_t group_id);
    std::string help();
};