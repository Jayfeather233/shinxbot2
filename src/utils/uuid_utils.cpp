#include "utils.h"
#include "uuid_v4.h"
#include "endianness.h"

static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
std::string generate_uuid() {
    return uuidGenerator.getUUID().str();
}