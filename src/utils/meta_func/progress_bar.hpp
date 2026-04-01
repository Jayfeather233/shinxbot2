#pragma once

#include <cstdint>
#include <string>

class BarInfo {
    float progress;
    std::string desc;
    BarInfo *f, *b;

public:
    explicit BarInfo(float pg = 0, const std::string &dc = "");
    void setProgress(float pg);
    void setDesc(const std::string &d);
    void setBar(float pg, const std::string &d);
    ~BarInfo();

    friend class progressBar;
};

class progressBar {
    static const int barWidth = 20;
    BarInfo root, tail;

public:
    progressBar();
    void addBar(BarInfo *u);
    std::string desc() const;
    ~progressBar();
};