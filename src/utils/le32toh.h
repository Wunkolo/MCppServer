#ifndef LE32TOH_H
#define LE32TOH_H
#include <cstdint>

// Byte swap functions for different bit widths
inline uint16_t byteswap16(uint16_t x) {
    return (x >> 8) | (x << 8);
}

inline uint32_t byteswap32(uint32_t x) {
    return ((x & 0x000000FF) << 24) |
           ((x & 0x0000FF00) << 8) |
           ((x & 0x00FF0000) >> 8) |
           ((x & 0xFF000000) >> 24);
}

inline uint64_t byteswap64(uint64_t x) {
    return ((x & 0x00000000000000FFULL) << 56) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0xFF00000000000000ULL) >> 56);
}

// Function to convert 32-bit little-endian to host byte order
#ifdef _WIN32
inline uint32_t le32toh(uint32_t little_endian) {
    // Compile-time endianness detection
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        // Host is little-endian
        return little_endian;
    #elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        // Host is big-endian
        #if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap32(little_endian);
        #elif defined(_MSC_VER)
            return _byteswap_ulong(little_endian);
        #else
            return byteswap32(little_endian);
        #endif
    #else
        // Runtime endianness detection
        static const int num = 1;
        if (*reinterpret_cast<const char*>(&num) == 1) {
            // Little-endian
            return little_endian;
        } else {
            // Big-endian
            #if defined(__GNUC__) || defined(__clang__)
                return __builtin_bswap32(little_endian);
            #elif defined(_MSC_VER)
                return _byteswap_ulong(little_endian);
            #else
                return byteswap32(little_endian);
            #endif
        }
    #endif
}
#endif

#endif //LE32TOH_H
