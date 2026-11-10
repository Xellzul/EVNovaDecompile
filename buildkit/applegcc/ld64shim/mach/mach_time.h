/* -v statistics only. */
#ifndef SHIM_MACH_TIME_H
#define SHIM_MACH_TIME_H
#include <stdint.h>
#include <mach/machine/vm_types.h>
#define KERN_SUCCESS 0
struct mach_timebase_info { uint32_t numer, denom; };
static inline uint64_t mach_absolute_time(void) { return 0; }
static inline kern_return_t mach_timebase_info(struct mach_timebase_info *i) { i->numer = i->denom = 1; return 0; }
#endif
