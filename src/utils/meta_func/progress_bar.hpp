#pragma once

#include <cstdint>
#include <string>

class BarInfo {
    float progress;
    std::string desc;
    BarInfo *f, *b;
public:
    BarInfo(float pg = 0, const std::string &dc = "");
    void setProgress(float pg);
    void setDesc(const std::string &d);
    void setBar(float pg, const std::string &d);
    ~BarInfo();

    friend class progressBar;
};

class progressBar {
    BarInfo root, tail;
public:
    progressBar();
    void addBar(BarInfo *u);
    std::string desc();
    ~progressBar();
};