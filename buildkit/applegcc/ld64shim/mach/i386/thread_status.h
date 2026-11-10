/* The i386 LC_UNIXTHREAD state: 16 words (ABI fact). */
#ifndef SHIM_I386_THREAD_STATUS_H
#define SHIM_I386_THREAD_STATUS_H
#define i386_THREAD_STATE 1
typedef struct { unsigned int __eax, __ebx, __ecx, __edx, __edi, __esi, __ebp, __esp, __ss, __eflags, __eip, __cs, __ds, __es, __fs, __gs; } i386_thread_state_t;
#define i386_THREAD_STATE_COUNT (sizeof(i386_thread_state_t) / sizeof(int))
#define x86_THREAD_STATE64 4
typedef struct { unsigned long long __rax, __rbx, __rcx, __rdx, __rdi, __rsi, __rbp, __rsp, __r8, __r9, __r10, __r11, __r12, __r13, __r14, __r15, __rip, __rflags, __cs, __fs, __gs; } x86_thread_state64_t;
#define x86_THREAD_STATE64_COUNT (sizeof(x86_thread_state64_t) / sizeof(int))
#endif
