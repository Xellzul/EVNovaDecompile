/* -statistics only. */
#ifndef SHIM_VM_STATISTICS_H
#define SHIM_VM_STATISTICS_H
#include <mach/machine/vm_types.h>
typedef struct { natural_t pageins, pageouts, faults; } vm_statistics_data_t;
typedef natural_t mach_msg_type_number_t;
typedef void *host_info_t;
#define HOST_VM_INFO 2
#define vm_page_size 4096
static inline int mach_host_self(void) { return 0; }
static inline kern_return_t host_statistics(int h, int f, host_info_t i, mach_msg_type_number_t *c) { return 1; }
#endif
