/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: 	NetBSD: mknative-gcc,v 1.71 2013/06/28 08:30:10 mrg Exp  */
/* Generated from: NetBSD: mknative.common,v 1.9 2007/02/05 18:26:01 apb Exp  */

#ifndef GCC_TM_H
#define GCC_TM_H
#define TARGET_CPU_DEFAULT (TARGET_CPU_arm1176jzfs)
#ifndef NETBSD_ENABLE_PTHREADS
# define NETBSD_ENABLE_PTHREADS
#endif
#ifndef TARGET_BIG_ENDIAN_DEFAULT
# define TARGET_BIG_ENDIAN_DEFAULT 1
#endif
#ifndef TARGET_DEFAULT_FLOAT_ABI
# define TARGET_DEFAULT_FLOAT_ABI ARM_FLOAT_ABI_HARD
#endif
#ifdef IN_GCC
# include "options.h"
# include "config/dbxelf.h"
# include "config/elfos.h"
# include "config/netbsd.h"
# include "config/netbsd-elf.h"
# include "config/arm/elf.h"
# include "config/arm/aout.h"
# include "config/arm/arm.h"
# include "config/arm/bpabi.h"
# include "config/arm/netbsd-elf.h"
# include "config/arm/netbsd-eabi.h"
# include "defaults.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE && !defined USED_FOR_TARGET
# include "insn-constants.h"
# include "insn-flags.h"
#endif
#endif /* GCC_TM_H */
