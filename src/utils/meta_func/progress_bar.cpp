#include "progress_bar.hpp"

#include <fmt/core.h>
#include <iomanip>
#include <mutex>
#include <ostream>

#include "utils.h"

static std::mutex m;

BarInfo::BarInfo(float pg, const std::string &dc) : progress(pg), desc(dc), f(nullptr), b(nullptr) {}
void BarInfo::setProgress(float pg) { progress = pg; }
void BarInfo::setDesc(const std::string &d) { this->desc = d; }
void BarInfo::setBar(float pg, const std::string &d)
{
    progress = pg;
    this->desc = d;
}
BarInfo::~BarInfo()
{
    std::lock_guard<std::mutex> lock(m);
    b->f = f;
    f->b = b;
}


progressBar::progressBar()
{
    root.b = &tail;
    tail.f = &root;
    root.f = tail.b = nullptr;
}

void progressBar::addBar(BarInfo *u)
{
    std::lock_guard<std::mutex> lock(m);
    if (u->f != nullptr) {
        set_global_log(LOG::INFO, fmt::format("A Progress Bar: {} registered twice!", u->desc));
        return;
    }
    u->f = tail.f;
    u->b = &tail;
    tail.f->b = u;
    tail.f = u;
}

std::string progressBar::desc()
{
    std::lock_guard<std::mutex> lock(m);
    std::ostringstream oss;
    BarInfo *p = root.b;
    size_t cnt = 0;
    while (p != &tail) {
        ++cnt;
        oss << p->desc << ' ' << std::setprecision(2) << p->progress * 100
            << '%' << std::endl;
    }
    return fmt::format("Total {} Tasks running.\n{}", cnt, oss.str());
}

progressBar::~progressBar()
{
    if (root.b != &tail || tail.f != &root) {
        set_global_log(LOG::ERROR, "progressBar list not properly released. May result in Segmentation fault.");
    }
}