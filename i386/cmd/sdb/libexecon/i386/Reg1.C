#ident	"@(#)sdb:libexecon/i386/Reg1.C	1.5"

// Reg1.C -- register names and attributes, machine specific data (i386)

#include "Reg.h"
#include "Itype.h"

RegAttrs regs[] = {
//
//	ref		name		size		flags	stype	offset
//
	REG_EAX,	"%eax",		4,		0,	Suint4,	EAX,
	REG_ECX,	"%ecx",		4,		0,	Suint4, ECX,
	REG_EDX,	"%edx",		4,		0,      Suint4, EDX,
	REG_EBX,	"%ebx",		4,		0,	Suint4, EBX,
//
//	Stack registers
//
	REG_ESP,	"%esp",		4,		0,	Suint4, UESP,
	REG_EBP,	"%ebp",		4,		0,	Suint4, EBP,
	REG_ESI,	"%esi",		4,		0,	Suint4, ESI,
	REG_EDI,	"%edi",		4,		0,	Suint4, EDI,
//
//	Instruction Pointer register
//
	REG_EIP,	"%eip",		4,		0,	Suint4, EIP,
//
//	Flags register
//
	REG_EFLAGS,	"%eflags",	4,		0,	Suint4, EFL,
	REG_TRAPNO,	"%trapno",	4,		0,	Suint4, TRAPNO,
//
//	floating point stack
//
	FP_ST0,		"%st(0)",	0,		1,	Sdfloat, 0,
	FP_ST1,		"%st(1)",	0,		1,	Sdfloat, 0,
	FP_ST2,		"%st(2)",	0,		1,	Sdfloat, 0,
	FP_ST3,		"%st(3)",	0,		1,	Sdfloat, 0,
	FP_ST4,		"%st(4)",	0,		1,	Sdfloat, 0,
	FP_ST5,		"%st(5)",	0,		1,	Sdfloat, 0,
	FP_ST6,		"%st(6)",	0,		1,	Sdfloat, 0,
	FP_ST7,		"%st(7)",	0,		1,	Sdfloat, 0,
//
//	floating point special registers
//
	FP_CW,		"%fpcw",	4,		0,	Suint4, 0,
	FP_SW,		"%fpsw",	4,		0,	Suint4, 0,
	FP_TW,		"%fptw",	4,		0,	Suint4, 0,
	FP_IP,		"%fpip",	4,		0,	Suint4, 0,
	FP_DP,		"%fpdp",	4,		0,	Suint4, 0,
//
// end marker
//
	REG_UNK,	0,		0,		0,	0,	0
};
