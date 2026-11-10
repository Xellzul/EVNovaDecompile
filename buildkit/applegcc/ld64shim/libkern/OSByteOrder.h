/* The OSByteOrder subset FileAbstraction.hpp uses, for a little-endian Linux host. */
#ifndef SHIM_OSBYTEORDER_H
#define SHIM_OSBYTEORDER_H
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#define OSSwapBigToHostInt32(x) bswap_32(x)
#define OSSwapBigToHostInt64(x) bswap_64(x)
#define OSSwapHostToBigInt32(x) bswap_32(x)
#define OSSwapInt16(x) bswap_16(x)
#define OSSwapInt32(x) bswap_32(x)
#define OSSwapInt64(x) bswap_64(x)
static inline uint16_t OSReadLittleInt16(const void *p, uintptr_t o) { uint16_t v; memcpy(&v, (const char *)p + o, 2); return v; }
static inline uint32_t OSReadLittleInt32(const void *p, uintptr_t o) { uint32_t v; memcpy(&v, (const char *)p + o, 4); return v; }
static inline uint64_t OSReadLittleInt64(const void *p, uintptr_t o) { uint64_t v; memcpy(&v, (const char *)p + o, 8); return v; }
static inline void OSWriteLittleInt16(void *p, uintptr_t o, uint16_t v) { memcpy((char *)p + o, &v, 2); }
static inline void OSWriteLittleInt32(void *p, uintptr_t o, uint32_t v) { memcpy((char *)p + o, &v, 4); }
static inline void OSWriteLittleInt64(void *p, uintptr_t o, uint64_t v) { memcpy((char *)p + o, &v, 8); }
static inline uint16_t OSReadBigInt16(const void *p, uintptr_t o) { return bswap_16(OSReadLittleInt16(p, o)); }
static inline uint32_t OSReadBigInt32(const void *p, uintptr_t o) { return bswap_32(OSReadLittleInt32(p, o)); }
static inline uint64_t OSReadBigInt64(const void *p, uintptr_t o) { return bswap_64(OSReadLittleInt64(p, o)); }
static inline void OSWriteBigInt16(void *p, uintptr_t o, uint16_t v) { OSWriteLittleInt16(p, o, bswap_16(v)); }
static inline void OSWriteBigInt32(void *p, uintptr_t o, uint32_t v) { OSWriteLittleInt32(p, o, bswap_32(v)); }
static inline void OSWriteBigInt64(void *p, uintptr_t o, uint64_t v) { OSWriteLittleInt64(p, o, bswap_64(v)); }
#endif
