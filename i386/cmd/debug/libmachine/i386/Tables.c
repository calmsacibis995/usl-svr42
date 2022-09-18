/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libmachine/i386/Tables.c	1.2"

#include	"dis.h"

/* Some opcodes are invalid, some represent several instructions
 * an must be further decoded
 */
#define		INVALID		{"",UNKNOWN,0}
#define		INDIRECT	{"",UNKNOWN,0}

/*
 *	In 16-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '16' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG16[8][2] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%ax",
/* 001  */		"%cl",		"%cx",
/* 010  */		"%dl",		"%dx",
/* 011	*/		"%bl",		"%bx",
/* 100	*/		"%ah",		"%sp",
/* 101	*/		"%ch",		"%bp",
/* 110	*/		"%dh",		"%si",
/* 111	*/		"%bh",		"%di",

};

/*
 *	In 32-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '32' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG32[8][2] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%eax",
/* 001  */		"%cl",		"%ecx",
/* 010  */		"%dl",		"%edx",
/* 011	*/		"%bl",		"%ebx",
/* 100	*/		"%ah",		"%esp",
/* 101	*/		"%ch",		"%ebp",
/* 110	*/		"%dh",		"%esi",
/* 111	*/		"%bh",		"%edi",

};


/*
 *	In 16-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the size of the displacement in each mode.
 */

const char dispsize16 [8][4] = {
/* mod		00	01	10	11 */
/* r/m */
/* 000 */	0,	1,	2,	0,
/* 001 */	0,	1,	2,	0,
/* 010 */	0,	1,	2,	0,
/* 011 */	0,	1,	2,	0,
/* 100 */	0,	1,	2,	0,
/* 101 */	0,	1,	2,	0,
/* 110 */	2,	1,	2,	0,
/* 111 */	0,	1,	2,	0
};


/*
 *	In 32-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the size of the displacement in this mode.
 */

const char dispsize32 [8][4] = {
/* mod		00	01	10	11 */
/* r/m */
/* 000 */	0,	1,	4,	0,
/* 001 */	0,	1,	4,	0,
/* 010 */	0,	1,	4,	0,
/* 011 */	0,	1,	4,	0,
/* 100 */	0,	1,	4,	0,
/* 101 */	4,	1,	4,	0,
/* 110 */	0,	1,	4,	0,
/* 111 */	0,	1,	4,	0
};


/*
 *	When data16 has been specified,
 * the following array specifies the registers for the different addressing modes.
 * Indexed first by mode, then by register number.
 */

const char *regname16[4][8] = {
/*reg  000        001        010        011        100    101   110     111 */
/*mod*/
/*00*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "",    "%bx",
/*01*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*10*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*11*/ "%ax",     "%cx",     "%dx",     "%bx",     "%sp", "%bp", "%si", "%di"
};


/*
 *	When data16 has not been specified,
 * 
 *	fields, to determine the addressing mode, and will also provide
 *	strings for printing.
 */

const char *regname32[4][8] = {
/*reg   000       001       010       011       100       101       110       111 */
/*mod*/
/*00 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "",     "%esi", "%edi",
/*01 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*10 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*11 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"
};

/*
 *	If r/m==100 then the following byte (the s-i-b byte) must be decoded
 */

const char *scale_factor[4] = {
	"1",
	"2",
	"4",
	"8"
};

const char *indexname[8] = {
	",%eax",
	",%ecx",
	",%edx",
	",%ebx",
	"",
	",%ebp",
	",%esi",
	",%edi"
};

/* For communication to locsympr */
char **regname;

/*
 *	Segment registers are selected by a two or three bit field.
 */

const char	*SEGREG[6] = {

/* 000 */	"%es",
/* 001 */	"%cs",
/* 010 */	"%ss",
/* 011 */	"%ds",
/* 100 */	"%fs",
/* 101 */	"%gs",

};

/*
 * Special Registers
 */

const char *DEBUGREG[8] = {
	"%db0", "%db1", "%db2", "%db3", "%db4", "%db5", "%db6", "%db7"
};

const char *CONTROLREG[8] = {
	"%cr0", "%cr1", "%cr2", "%cr3", "%cr4", "%cr5?", "%cr6?", "%cr7?"
};

const char *TESTREG[8] = {
	"%tr0?", "%tr1?", "%tr2?", "%tr3", "%tr4", "%tr5", "%tr6", "%tr7"
};

/*
 *	Decode table for 0x0F00 opcodes
 */

const struct instable op0F00[8] = {

/*  [0]  */	{"sldt",M,0},	{"str",M,0},	{"lldt",M,0},	{"ltr",M,0},
/*  [4]  */	{"verr",M,0},	{"verw",M,0},	INVALID,	INVALID,
};


/*
 *	Decode table for 0x0F01 opcodes
 */

const struct instable op0F01[8] = {

/*  [0]  */	{"sgdt",M,0},	{"sidt",M,0},	{"lgdt",M,0},	{"lidt",M,0},
/*  [4]  */	{"smsw",M,0},	INVALID,		{"lmsw",M,0},	{"invlpg", M, 0 },
};

/*
 *	Decode table for 0x0FC8 opcode -- i486 bswap instruction
 *
 *	bit pattern: 0000 1111 1100 1reg
 */
const struct instable op0FC8[4] = {
/*  [0]  */	{"bswap",R,0},	INVALID,	INVALID,	INVALID,
};

/*
 *	Decode table for 0x0FBA opcodes
 */

const struct instable op0FBA[8] = {

/*  [0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4]  */	{"bt",MIb,1},	{"bts",MIb,1},	{"btr",MIb,1},	{"btc",MIb,1},
};


/*
 *	Decode table for 0x0F opcodes
 *	Invalid entries 0x30 - 0x7F are deleted to save space
 */

const struct instable op0F[9][16] = {

/*  [00]  */	{INDIRECT,	INDIRECT,	{"lar",MR,0},	{"lsl",MR,0},
/*  [04]  */	INVALID,		INVALID,		{"clts",GO_ON,0},	INVALID,
/*  [08]  */	{"invd",GO_ON,0},	{"wbinvd",GO_ON,0},	INVALID,	INVALID,
/*  [0C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [10]  */	{INVALID,		INVALID,		INVALID,		INVALID,
/*  [14]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [18]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [1C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [20]  */	{{"mov",SREG,1},	{"mov",SREG,1},	{"mov",SREG,1},	{"mov",SREG,1},
/*  [24]  */	{"mov",SREG,1},	INVALID,		{"mov",SREG,1},	INVALID,
/*  [28]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [30]  */	{{"wrmsr",GO_ON,0},	{"rdtsc",GO_ON,0},	{"rdmsr",GO_ON,0},	INVALID,

/*  [34]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [38]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [3C]  */	INVALID,		INVALID,		INVALID,		INVALID},
/* Invalid entries 0x40 - 0x7F are deleted to save space */
/*  [80]  */	{{"jo",D,0},	{"jno",D,0},	{"jb",D,0},	{"jae",D,0},
/*  [84]  */	{"je",D,0},	{"jne",D,0},	{"jbe",D,0},	{"ja",D,0},
/*  [88]  */	{"js",D,0},	{"jns",D,0},	{"jp",D,0},	{"jnp",D,0},
/*  [8C]  */	{"jl",D,0},	{"jge",D,0},	{"jle",D,0},	{"jg",D,0}},

/*  [90]  */	{{"seto",M,0},	{"setno",M,0},	{"setb",M,0},	{"setae",M,0},
/*  [94]  */	{"sete",M,0},	{"setne",M,0},	{"setbe",M,0},	{"seta",M,0},
/*  [98]  */	{"sets",M,0},	{"setns",M,0},	{"setp",M,0},	{"setnp",M,0},
/*  [9C]  */	{"setl",M,0},	{"setge",M,0},	{"setle",M,0},	{"setg",M,0}},

/*  [A0]  */	{{"push",LSEG,1},	{"pop",LSEG,1},	{"cpuid",GO_ON,	0},	{"bt",RMw,1},
/*  [A4]  */	{"shld",DSHIFT,1},	{"shld",DSHIFTcl,1},	INVALID,	INVALID,
/*  [A8]  */	{"push",LSEG,1},	{"pop",LSEG,1},	{"rsm",GO_ON,0},		{"bts",RMw,1},
/*  [AC]  */	{"shrd",DSHIFT,1},	{"shrd",DSHIFTcl,1},	INVALID,		{"imul",MRw,1}},

/*  [B0]  */	{{"cmpxchgb",RMw,0},	{"cmpxchg",RMw,1},	{"lss",MR,0},	{"btr",RMw,1},
/*  [B4]  */	{"lfs",MR,0},	{"lgs",MR,0},	{"movzb",MOVZ,1},	{"movzwl",MOVZ,0},
/*  [B8]  */	INVALID,		INVALID,		INDIRECT,	{"btc",RMw,1},
/*  [BC]  */	{"bsf",MRw,1},	{"bsr",MRw,1},	{"movsb",MOVZ,1},	{"movswl",MOVZ,0}},
/*  [C0]  */	{{"xaddb",RMw,0},	{"xadd",RMw,1},	INVALID, INVALID,
/*  [C4]  */	INVALID,	INVALID,	INVALID,	{"cmpxchg8b",GO_ON,0},
/*  [C8]  */	INVALID,	INVALID,	INVALID,	INVALID,
/*  [CC]  */	INVALID,	INVALID,	INVALID,	INVALID, }
};


/*
 *	Decode table for 0x80 opcodes
 */

const struct instable op80[8] = {

/*  [0]  */	{"addb",IMlw,0},	{"orb",IMlw,0},	{"adcb",IMlw,0},	{"sbbb",IMlw,0},
/*  [4]  */	{"andb",IMlw,0},	{"subb",IMlw,0},	{"xorb",IMlw,0},	{"cmpb",IMlw,0},
};


/*
 *	Decode table for 0x81 opcodes.
 */

const struct instable op81[8] = {

/*  [0]  */	{"add",IMlw,1},	{"or",IMlw,1},	{"adc",IMlw,1},	{"sbb",IMlw,1},
/*  [4]  */	{"and",IMlw,1},	{"sub",IMlw,1},	{"xor",IMlw,1},	{"cmp",IMlw,1},
};


/*
 *	Decode table for 0x82 opcodes.
 */

const struct instable op82[8] = {

/*  [0]  */	{"addb",IMlw,0},	INVALID,		{"adcb",IMlw,0},	{"sbbb",IMlw,0},
/*  [4]  */	INVALID,		{"subb",IMlw,0},	INVALID,		{"cmpb",IMlw,0},
};
/*
 *	Decode table for 0x83 opcodes.
 */

const struct instable op83[8] = {

/*  [0]  */	{"add",IMlw,1},	{"or",IMlw,1},	{"adc",IMlw,1},	{"sbb",IMlw,1},
/*  [4]  */	{"and",IMlw,1},	{"sub",IMlw,1},	{"xor",IMlw,1},		{"cmp",IMlw,1},
};

/*
 *	Decode table for 0xC0 opcodes.
 */

const struct instable opC0[8] = {

/*  [0]  */	{"rolb",MvI,0},	{"rorb",MvI,0},	{"rclb",MvI,0},	{"rcrb",MvI,0},
/*  [4]  */	{"shlb",MvI,0},	{"shrb",MvI,0},	INVALID,		{"sarb",MvI,0},
};

/*
 *	Decode table for 0xD0 opcodes.
 */

const struct instable opD0[8] = {

/*  [0]  */	{"rolb",Mv,0},	{"rorb",Mv,0},	{"rclb",Mv,0},	{"rcrb",Mv,0},
/*  [4]  */	{"shlb",Mv,0},	{"shrb",Mv,0},	INVALID,		{"sarb",Mv,0},
};

/*
 *	Decode table for 0xC1 opcodes.
 *	186 instruction set
 */

const struct instable opC1[8] = {

/*  [0]  */	{"rol",MvI,1},	{"ror",MvI,1},	{"rcl",MvI,1},	{"rcr",MvI,1},
/*  [4]  */	{"shl",MvI,1},	{"shr",MvI,1},	INVALID,		{"sar",MvI,1},
};

/*
 *	Decode table for 0xD1 opcodes.
 */

const struct instable opD1[8] = {

/*  [0]  */	{"rol",Mv,1},	{"ror",Mv,1},	{"rcl",Mv,1},	{"rcr",Mv,1},
/*  [4]  */	{"shl",Mv,1},	{"shr",Mv,1},	INVALID,		{"sar",Mv,1},
};


/*
 *	Decode table for 0xD2 opcodes.
 */

const struct instable opD2[8] = {

/*  [0]  */	{"rolb",Mv,0},	{"rorb",Mv,0},	{"rclb",Mv,0},	{"rcrb",Mv,0},
/*  [4]  */	{"shlb",Mv,0},	{"shrb",Mv,0},	INVALID,		{"sarb",Mv,0},
};
/*
 *	Decode table for 0xD3 opcodes.
 */

const struct instable opD3[8] = {

/*  [0]  */	{"rol",Mv,1},	{"ror",Mv,1},	{"rcl",Mv,1},	{"rcr",Mv,1},
/*  [4]  */	{"shl",Mv,1},	{"shr",Mv,1},	INVALID,		{"sar",Mv,1},
};


/*
 *	Decode table for 0xF6 opcodes.
 */

const struct instable opF6[8] = {

/*  [0]  */	{"testb",IMw,0},	INVALID,		{"notb",Mw,0},	{"negb",Mw,0},
/*  [4]  */	{"mulb",MA,0},	{"imulb",MA,0},	{"divb",MA,0},	{"idivb",MA,0},
};


/*
 *	Decode table for 0xF7 opcodes.
 */

const struct instable opF7[8] = {

/*  [0]  */	{"test",IMw,1},	INVALID,		{"not",Mw,1},	{"neg",Mw,1},
/*  [4]  */	{"mul",MA,1},	{"imul",MA,1},	{"div",MA,1},	{"idiv",MA,1},
};


/*
 *	Decode table for 0xFE opcodes.
 */

const struct instable opFE[8] = {

/*  [0]  */	{"incb",Mw,0},	{"decb",Mw,0},	INVALID,		INVALID,
/*  [4]  */	INVALID,		INVALID,		INVALID,		INVALID,
};
/*
 *	Decode table for 0xFF opcodes.
 */

const struct instable opFF[8] = {

/*  [0]  */	{"inc",Mw,1},	{"dec",Mw,1},	{"call",INM,0},	{"lcall",INM,0},
/*  [4]  */	{"jmp",INM,0},	{"ljmp",INM,0},	{"push",M,1},	INVALID,
};

/* for 287 instructions, which are a mess to decode */

const struct instable opFP1n2[8][8] = {
/* bit pattern:	1101 1xxx MODxx xR/M */
/*  [0,0] */	{{"fadds",M,0},	{"fmuls",M,0},	{"fcoms",M,0},	{"fcomps",M,0},
/*  [0,4] */	{"fsubs",M,0},	{"fsubrs",M,0},	{"fdivs",M,0},	{"fdivrs",M,0}},
/*  [1,0]  */	{{"flds",M,0},	INVALID,		{"fsts",M,0},	{"fstps",M,0},
/*  [1,4]  */	{"fldenv",M,0},	{"fldcw",M,0},	{"fnstenv",M,0},	{"fnstcw",M,0}},
/*  [2,0]  */	{{"fiaddl",M,0},	{"fimull",M,0},	{"ficoml",M,0},	{"ficompl",M,0},
/*  [2,4]  */	{"fisubl",M,0},	{"fisubrl",M,0},	{"fidivl",M,0},	{"fidivrl",M,0}},
/*  [3,0]  */	{{"fildl",M,0},	INVALID,		{"fistl",M,0},	{"fistpl",M,0},
/*  [3,4]  */	INVALID,		{"fldt",M,0},	INVALID,		{"fstpt",M,0}},
/*  [4,0]  */	{{"faddl",M,0},	{"fmull",M,0},	{"fcoml",M,0},	{"fcompl",M,0},
/*  [4,1]  */	{"fsubl",M,0},	{"fsubrl",M,0},	{"fdivl",M,0},	{"fdivrl",M,0}},
/*  [5,0]  */	{{"fldl",M,0},	INVALID,		{"fstl",M,0},	{"fstpl",M,0},
/*  [5,4]  */	{"frstor",M,0},	INVALID,		{"fnsave",M,0},	{"fnstsw",M,0}},
/*  [6,0]  */	{{"fiadd",M,0},	{"fimul",M,0},	{"ficom",M,0},	{"ficomp",M,0},
/*  [6,4]  */	{"fisub",M,0},	{"fisubr",M,0},	{"fidiv",M,0},	{"fidivr",M,0}},
/*  [7,0]  */	{{"fild",M,0},	INVALID,		{"fist",M,0},	{"fistp",M,0},
/*  [7,4]  */	{"fbld",M,0},	{"fildll",M,0},	{"fbstp",M,0},	{"fistpll",M,0}}
};

const struct instable opFP3[8][8] = {
/* bit  pattern:	1101 1xxx 11xx xREG */
/*  [0,0]  */	{{"fadd",FF,0},	{"fmul",FF,0},	{"fcom",F,0},	{"fcomp",F,0},
/*  [0,4]  */	{"fsub",FF,0},	{"fsubr",FF,0},	{"fdiv",FF,0},	{"fdivr",FF,0}},
/*  [1,0]  */	{{"fld",F,0},	{"fxch",F,0},	{"fnop",GO_ON,0},	{"fstp",F,0},
/*  [1,4]  */	INVALID,		INVALID,		INVALID,		INVALID},
/*  [2,0]  */	{INVALID,		INVALID,		INVALID,		INVALID,
/*  [2,4]  */	INVALID,		{"fucompp",GO_ON,0},INVALID,		INVALID},
/*  [3,0]  */	{INVALID,		INVALID,		INVALID,		INVALID,
/*  [3,4]  */	INVALID,		INVALID,		INVALID,		INVALID},
/*  [4,0]  */	{{"fadd",FF,0},	{"fmul",FF,0},	{"fcom",F,0},	{"fcomp",F,0},
/*  [4,4]  */	{"fsub",FF,0},	{"fsubr",FF,0},	{"fdiv",FF,0},	{"fdivr",FF,0}},
/*  [5,0]  */	{{"ffree",F,0},	{"fxch",F,0},	{"fst",F,0},	{"fstp",F,0},
/*  [5,4]  */	{"fucom",F,0},	{"fucomp",F,0},	INVALID,		INVALID},
/*  [6,0]  */	{{"faddp",FF,0},	{"fmulp",FF,0},	{"fcomp",F,0},	{"fcompp",GO_ON,0},
/*  [6,4]  */	{"fsubp",FF,0},	{"fsubrp",FF,0},	{"fdivp",FF,0},	{"fdivrp",FF,0}},
/*  [7,0]  */	{{"ffree",F,0},	{"fxch",F,0},	{"fstp",F,0},	{"fstp",F,0},
/*  [7,4]  */	{"fstsw",M,0},	INVALID,		INVALID,		INVALID}
};

const struct instable opFP4[4][8] = {
/* bit pattern:	1101 1001 111x xxxx */
/*  [0,0]  */	{{"fchs",GO_ON,0},	{"fabs",GO_ON,0},	INVALID, INVALID,
/*  [0,4]  */	{"ftst",GO_ON,0},	{"fxam",GO_ON,0},	INVALID, INVALID},
/*  [1,0]  */	{{"fld1",GO_ON,0},	{"fldl2t",GO_ON,0},	{"fldl2e",GO_ON,0},	{"fldpi",GO_ON,0},
/*  [1,4]  */	{"fldlg2",GO_ON,0},	{"fldln2",GO_ON,0},	{"fldz",GO_ON,0}, INVALID},
/*  [2,0]  */	{{"f2xm1",GO_ON,0},	{"fyl2x",GO_ON,0},	{"fptan",GO_ON,0},	{"fpatan", GO_ON, 0},
/*  [2,4]  */	{"fxtract",GO_ON,0}, {"fprem1",GO_ON,0}, {"fdecstp",GO_ON,0},{"fincstp",GO_ON,0}},
/*  [3,0]  */	{{"fprem",GO_ON,0},	{"fyl2xp1",GO_ON,0},{"fsqrt",GO_ON,0}, {"fsincos",GO_ON,0},
/*  [3,4]  */	{"frndint",GO_ON,0},{"fscale",GO_ON,0},{"fsin",GO_ON,0}, {"fcos", GO_ON, 0}}
};

const struct instable opFP5[8] = {
/* bit pattern:	1101 1011 1110 0xxx */
/*  [0]  */	INVALID,		INVALID,		{"fnclex",GO_ON,0},{"fninit",GO_ON,0},
/*  [4]  */	{"fsetpm",GO_ON,0},	INVALID,		INVALID,		INVALID,
};

/*
 *	Main decode table for the op codes.  The first two nibbles
 *	will be used as an index into the table.  If there is a
 *	a need to further decode an instruction, the array to be
 *	referenced is indicated with the other two entries being
 *	empty.
 */

const struct instable distable[16][16] = {

/* [0,0] */	{{"addb",RMw,0},	{"add",RMw,1},	{"addb",MRw,0},	{"add",MRw,1},
/* [0,4] */	{"addb",IA,0},	{"add",IA,1},	{"push",SEG,1},	{"pop",SEG,1},
/* [0,8] */	{"orb",RMw,0},	{"or",RMw,1},	{"orb",MRw,0},	{"or",MRw,1},
/* [0,C] */	{"orb",IA,0},	{"or",IA,1},	{"push",SEG,1},	INDIRECT},

/* [1,0] */	{{"adcb",RMw,0},	{"adc",RMw,1},	{"adcb",MRw,0},	{"adc",MRw,1},
/* [1,4] */	{"adcb",IA,0},	{"adc",IA,1},	{"push",SEG,1},	{"pop",SEG,1},
/* [1,8] */	{"sbbb",RMw,0},	{"sbb",RMw,1},	{"sbbb",MRw,0},	{"sbb",MRw,1},
/* [1,C] */	{"sbbb",IA,0},	{"sbb",IA,1},	{"push",SEG,1},	{"pop",SEG,1}},

/* [2,0] */	{{"andb",RMw,0},	{"and",RMw,1},	{"andb",MRw,0},	{"and",MRw,1},
/* [2,4] */	{"andb",IA,0},	{"and",IA,1},	{"%es:",OVERRIDE,0},{"daa",GO_ON,0},
/* [2,8] */	{"subb",RMw,0},	{"sub",RMw,1},	{"subb",MRw,0},	{"sub",MRw,1},
/* [2,C] */	{"subb",IA,0},	{"sub",IA,1},	{"%cs:",OVERRIDE,0},{"das",GO_ON,0}},

/* [3,0] */	{{"xorb",RMw,0},	{"xor",RMw,1},	{"xorb",MRw,0},	{"xor",MRw,1},
/* [3,4] */	{"xorb",IA,0},	{"xor",IA,1},	{"%ss:",OVERRIDE,0},{"aaa",GO_ON,0},
/* [3,8] */	{"cmpb",RMw,0},	{"cmp",RMw,1},	{"cmpb",MRw,0},	{"cmp",MRw,1},
/* [3,C] */	{"cmpb",IA,0},	{"cmp",IA,1},	{"%ds:",OVERRIDE,0},{"aas",GO_ON,0}},

/* [4,0] */	{{"inc",R,1},	{"inc",R,1},	{"inc",R,1},	{"inc",R,1},
/* [4,4] */	{"inc",R,1},	{"inc",R,1},	{"inc",R,1},	{"inc",R,1},
/* [4,8] */	{"dec",R,1},	{"dec",R,1},	{"dec",R,1},	{"dec",R,1},
/* [4,C] */	{"dec",R,1},	{"dec",R,1},	{"dec",R,1},	{"dec",R,1}},

/* [5,0] */	{{"push",R,1},	{"push",R,1},	{"push",R,1},	{"push",R,1},
/* [5,4] */	{"push",R,1},	{"push",R,1},	{"push",R,1},	{"push",R,1},
/* [5,8] */	{"pop",R,1},	{"pop",R,1},	{"pop",R,1},	{"pop",R,1},
/* [5,C] */	{"pop",R,1},	{"pop",R,1},	{"pop",R,1},	{"pop",R,1}},

/* [6,0] */	{{"pusha",GO_ON,1},	{"popa",GO_ON,1},	{"bound",MR,1},	{"arpl",RMw,0},
/* [6,4] */	{"%fs:",OVERRIDE,0},{"%gs:",OVERRIDE,0},{"data16",DM,0},	{"addr16",AM,0},
/* [6,8] */	{"push",I,1},	{"imul",IMUL,1},	{"push",Ib,1},	{"imul",IMUL,1},
/* [6,C] */	{"insb",GO_ON,0},	{"ins",GO_ON,1},	{"outsb",GO_ON,0},	{"outs",GO_ON,1}},

/* [7,0] */	{{"jo",BD,0},	{"jno",BD,0},	{"jb",BD,0},	{"jae",BD,0},
/* [7,4] */	{"je",BD,0},	{"jne",BD,0},	{"jbe",BD,0},	{"ja",BD,0},
/* [7,8] */	{"js",BD,0},	{"jns",BD,0},	{"jp",BD,0},	{"jnp",BD,0},
/* [7,C] */	{"jl",BD,0},	{"jge",BD,0},	{"jle",BD,0},	{"jg",BD,0}},

/* [8,0] */	{INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [8,4] */	{"testb",MRw,0},	{"test",MRw,1},	{"xchgb",MRw,0},	{"xchg",MRw,1},
/* [8,8] */	{"movb",RMw,0},	{"mov",RMw,1},	{"movb",MRw,0},	{"mov",MRw,1},
/* [8,C] */	{"mov",SM,1},	{"lea",MR,1},	{"mov",MS,1},	{"pop",M,1}},

/* [9,0] */	{{"nop",GO_ON,0},	{"xchg",RA,1},	{"xchg",RA,1},	{"xchg",RA,1},
/* [9,4] */	{"xchg",RA,1},	{"xchg",RA,1},	{"xchg",RA,1},	{"xchg",RA,1},
/* [9,8] */	{"",CBW,0},	{"",CWD,0},	{"lcall",SO,0},	{"fwait",GO_ON,0},
/* [9,C] */	{"pushf",GO_ON,1},	{"popf",GO_ON,1},	{"sahf",GO_ON,0},	{"lahf",GO_ON,0}},

/* [A,0] */	{{"movb",OA,0},	{"mov",OA,1},	{"movb",AO,0},	{"mov",AO,1},
/* [A,4] */	{"movsb",SD,0},	{"movs",SD,1},	{"cmpsb",SD,0},	{"cmps",SD,1},
/* [A,8] */	{"testb",IA,0},	{"test",IA,1},	{"stosb",AD,0},	{"stos",AD,1},
/* [A,C] */	{"lodsb",SA,0},	{"lods",SA,1},	{"scasb",AD,0},	{"scas",AD,1}},

/* [B,0] */	{{"movb",IR,0},	{"movb",IR,0},	{"movb",IR,0},	{"movb",IR,0},
/* [B,4] */	{"movb",IR,0},	{"movb",IR,0},	{"movb",IR,0},	{"movb",IR,0},
/* [B,8] */	{"mov",IR,1},	{"mov",IR,1},	{"mov",IR,1},	{"mov",IR,1},
/* [B,C] */	{"mov",IR,1},	{"mov",IR,1},	{"mov",IR,1},	{"mov",IR,1}},

/* [C,0] */	{INDIRECT,	INDIRECT,	{"ret",RET,0},	{"ret",GO_ON,0},
/* [C,4] */	{"les",MR,0},	{"lds",MR,0},	{"movb",IMw,0},	{"mov",IMw,1},
/* [C,8] */	{"enter",ENTER,0},	{"leave",GO_ON,0},	{"lret",RET,0},	{"lret",GO_ON,0},
/* [C,C] */	{"int",INT3,0},	{"int",Ib,0},	{"into",GO_ON,0},	{"iret",GO_ON,0}},

/* [D,0] */	{INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [D,4] */	{"aam",U,0},	{"aad",U,0},	{"falc",GO_ON,0},	{"xlat",GO_ON,0},

/* 287 instructions.  */
/* [D,8] */	INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [D,C] */	INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT},

/* [E,0] */	{{"loopnz",BD,0},	{"loopz",BD,0},	{"loop",BD,0},	{"jcxz",BD,0},
/* [E,4] */	{"inb",P,0},	{"in",P,1},	{"outb",P,0},	{"out",P,1},
/* [E,8] */	{"call",D,0},	{"jmp",D,0},	{"ljmp",SO,0},	{"jmp",BD,0},
/* [E,C] */	{"inb",V,0},	{"in",V,1},	{"outb",V,0},	{"out",V,1}},

/* [F,0] */	{{"lock ",PREFIX,0},	{"",JTAB,0},	{"repnz ",PREFIX,0},	{"repz ",PREFIX,0},
/* [F,4] */	{"hlt",GO_ON,0},	{"cmc",GO_ON,0},	INDIRECT,	INDIRECT,
/* [F,8] */	{"clc",GO_ON,0},	{"stc",GO_ON,0},	{"cli",GO_ON,0},	{"sti",GO_ON,0},
/* [F,C] */	{"cld",GO_ON,0},	{"std",GO_ON,0},	INDIRECT,	INDIRECT}
};
