#include "utils.h"

#include <cctype>
#include <random>
using u32    = uint_least32_t;
using engine = std::mt19937;

std::random_device os_seed;
const u32 seed = os_seed();
engine generator = engine(seed);
std::uniform_int_distribution< u32 > uni_dis_0_65535 = std::uniform_int_distribution< u32 >(0, 65536);

int get_random(int maxi){
    return uni_dis_0_65535(generator) % maxi;
}