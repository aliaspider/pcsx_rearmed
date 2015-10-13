#ifndef __ASSEM_ARM_H__
#define __ASSEM_ARM_H__

#define HOST_REGS 13
#define HOST_CCREG 10
#define HOST_BTREG 8
#define EXCLUDE_REG 11

#define HOST_IMM8 1
#define HAVE_CMOV_IMM 1
#define CORTEX_A8_BRANCH_PREDICTION_HACK 1
#define USE_MINI_HT 1
//#define REG_PREFETCH 1
#define HAVE_CONDITIONAL_CALL 1
#define DISABLE_TLB 1
//#define MUPEN64
#define FORCE32 1
#define DISABLE_COP1 1
#define PCSX 1
#define RAM_SIZE 0x200000

#ifndef __ARM_ARCH_7A__
//#undef CORTEX_A8_BRANCH_PREDICTION_HACK
//#undef USE_MINI_HT
#endif

#ifndef BASE_ADDR_FIXED
#define BASE_ADDR_FIXED 0
#endif

#ifdef FORCE32
#define REG_SHIFT 2
#else
#define REG_SHIFT 3
#endif

/* ARM calling convention:
   r0-r3, r12: caller-save
   r4-r11: callee-save */

#define ARG1_REG 0
#define ARG2_REG 1
#define ARG3_REG 2
#define ARG4_REG 3

/* GCC register naming convention:
   r10 = sl (base)
   r11 = fp (frame pointer)
   r12 = ip (scratch)
   r13 = sp (stack pointer)
   r14 = lr (link register)
   r15 = pc (program counter) */

#define FP 11
#define LR 14
#define HOST_TEMPREG 14

// Note: FP is set to &dynarec_local when executing generated code.
// Thus the local variables are actually global and not on the stack.

extern char *invc_ptr;

//#define TARGET_SIZE_2 24 // 2^24 = 16 megabytes
#define TARGET_SIZE_2 22
//#define TARGET_SIZE_2 19

// Code generator target address
#if BASE_ADDR_FIXED
// "round" address helpful for debug
#define BASE_ADDR 0x1000000
#else
#if defined(VITA)
extern char* translation_cache;
#else
extern char translation_cache[1 << TARGET_SIZE_2];
//extern char * translation_cache;
extern char translation_cache_w[1 << TARGET_SIZE_2];

extern char* translation_cache_ptr;
extern char* translation_cache_w_ptr;
extern char* translation_cache_ptr_max;
extern char* translation_cache_w_ptr_max;

extern u32 translation_cache_offset;



#define to_write_buffer_u32(X)   ((u32*)(((u32)(X) + translation_cache_offset)))
#define to_write_buffer_u8(X)     ((u8*)(((u32)(X) + translation_cache_offset)))

#define from_write_buffer_u32(X)   ((u32*)(((u32)(X) - translation_cache_offset)))
#define from_write_buffer_u8(X)     ((u8*)(((u32)(X) - translation_cache_offset)))

#endif
#define BASE_ADDR ((u_int)translation_cache_ptr)
#endif

#endif //__ASSEM_ARM_H__
