#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H

#include <stdint.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  REG_R8 = 0,
# define REG_R8		REG_R8
  REG_R9,
# define REG_R9		REG_R9
  REG_R10,
# define REG_R10	REG_R10
  REG_R11,
# define REG_R11	REG_R11
  REG_R12,
# define REG_R12	REG_R12
  REG_R13,
# define REG_R13	REG_R13
  REG_R14,
# define REG_R14	REG_R14
  REG_R15,
# define REG_R15	REG_R15
  REG_RDI,
# define REG_RDI	REG_RDI
  REG_RSI,
# define REG_RSI	REG_RSI
  REG_RBP,
# define REG_RBP	REG_RBP
  REG_RBX,
# define REG_RBX	REG_RBX
  REG_RDX,
# define REG_RDX	REG_RDX
  REG_RAX,
# define REG_RAX	REG_RAX
  REG_RCX,
# define REG_RCX	REG_RCX
  REG_RSP,
# define REG_RSP	REG_RSP
  REG_RIP,
# define REG_RIP	REG_RIP
  REG_EFL,
# define REG_EFL	REG_EFL
  REG_CSGSFS,		/* Actually short cs, gs, fs, __pad0.  */
# define REG_CSGSFS	REG_CSGSFS
  REG_ERR,
# define REG_ERR	REG_ERR
  REG_TRAPNO,
# define REG_TRAPNO	REG_TRAPNO
  REG_OLDMASK,
# define REG_OLDMASK	REG_OLDMASK
  REG_CR2
# define REG_CR2	REG_CR2
};

// #define __ctx(fld) fld

// typedef long long int greg_t;

// #define __NGREG 23

// typedef greg_t gregset_t[__NGREG];

// struct _libc_fpxreg
// {
//   unsigned short int __ctx(significand)[4];
//   unsigned short int __ctx(exponent);
//   unsigned short int __glibc_reserved1[3];
// };

// struct _libc_xmmreg
// {
//   uint32_t    __ctx(element)[4];
// };

// struct _libc_fpstate
// {
//   /* 64-bit FXSAVE format.  */
//   uint16_t            __ctx(cwd);
//   uint16_t            __ctx(swd);
//   uint16_t            __ctx(ftw);
//   uint16_t            __ctx(fop);
//   uint64_t            __ctx(rip);
//   uint64_t            __ctx(rdp);
//   uint32_t            __ctx(mxcsr);
//   uint32_t            __ctx(mxcr_mask);
//   struct _libc_fpxreg   _st[8];
//   struct _libc_xmmreg   _xmm[16];
//   uint32_t            __glibc_reserved1[24];
// };

// /* Structure to describe FPU registers.  */
// typedef struct _libc_fpstate *fpregset_t;

// /* Context to describe whole processor state.  */
// typedef struct {
// 	gregset_t __ctx(gregs);
// 	/* Note that fpregs is a pointer.  */
// 	fpregset_t __ctx(fpregs);
// 	unsigned long long __reserved1 [8];
// } mcontext_t;

// typedef struct {
// 	unsigned long int __ctx(uc_flags);
// 	struct ucontext_t *uc_link;
// 	stack_t uc_stack;
// 	mcontext_t uc_mcontext;
// 	sigset_t uc_sigmask;
// 	struct _libc_fpstate __fpregs_mem;
// 	unsigned long long int __ssp[4];
// } ucontext_t;

#ifdef __cplusplus
}
#endif

#endif // _SYS_UCONTEXT_H
