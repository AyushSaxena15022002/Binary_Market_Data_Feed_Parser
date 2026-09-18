#pragma once

#include <cstdint>

namespace itch {
namespace utils {

inline uint16_t ntoh16(uint16_t net) {
    return __builtin_bswap16(net);
}

inline uint32_t ntoh32(uint32_t net) {
    return __builtin_bswap32(net);
}

inline uint64_t ntoh64(uint64_t net) {
    return __builtin_bswap64(net);
}

inline uint64_t ntoh48(const uint8_t* bytes) {
    return (static_cast<uint64_t>(bytes[0]) << 40) |
           (static_cast<uint64_t>(bytes[1]) << 32) |
           (static_cast<uint64_t>(bytes[2]) << 24) |
           (static_cast<uint64_t>(bytes[3]) << 16) |
           (static_cast<uint64_t>(bytes[4]) << 8)  |
           (static_cast<uint64_t>(bytes[5]));
}

}
}
