/* The ppc LC_UNIXTHREAD state: 40 words (ABI fact). */
#ifndef SHIM_PPC_THREAD_STATUS_H
#define SHIM_PPC_THREAD_STATUS_H
#define PPC_THREAD_STATE 1
#define PPC_THREAD_STATE64 5
typedef struct { unsigned int __srr0, __srr1, __r0, __r1, __r2, __r3, __r4, __r5, __r6, __r7, __r8, __r9, __r10, __r11, __r12, __r13, __r14, __r15, __r16, __r17, __r18, __r19, __r20, __r21, __r22, __r23, __r24, __r25, __r26, __r27, __r28, __r29, __r30, __r31, __cr, __xer, __lr, __ctr, __mq, __vrsave; } ppc_thread_state_t;
#define PPC_THREAD_STATE_COUNT (sizeof(ppc_thread_state_t) / sizeof(int))
#pragma pack(4)
typedef struct { unsigned long long __srr0, __srr1, __r0, __r1, __r2, __r3, __r4, __r5, __r6, __r7, __r8, __r9, __r10, __r11, __r12, __r13, __r14, __r15, __r16, __r17, __r18, __r19, __r20, __r21, __r22, __r23, __r24, __r25, __r26, __r27, __r28, __r29, __r30, __r31; unsigned int __cr; unsigned long long __xer, __lr, __ctr; unsigned int __vrsave; } ppc_thread_state64_t;
#pragma pack()
#define PPC_THREAD_STATE64_COUNT (sizeof(ppc_thread_state64_t) / sizeof(int))
#endif
