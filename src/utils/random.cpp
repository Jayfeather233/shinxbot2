#include "utils.h"

#include <cctype>
#include <random>
using u32 = uint32_t;
using engine = std::mt19937;

engine& rng()
{
    thread_local static engine gen(std::random_device{}());
    return gen;
}

int get_random(int maxi)
{
    if (maxi <= 0) return 0;
    return std::uniform_int_distribution<int>(0, maxi - 1)(rng());
}

int get_random(int mini, int maxi)
{
    if (maxi <= mini) return mini;
    return std::uniform_int_distribution<int>(mini, maxi - 1)(rng());
}

float get_random_f(float mini, float maxi)
{
    return std::uniform_real_distribution<float>(mini, maxi)(rng());
}