/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/defs.h	1.1.2.6"

/*      machine dependent include file for the Intel 386 */

# include <ctype.h>
# include <string.h>

/* For now, turn off everything */
#define IMPREGAL
#define IMPIL
/* #define IMPCOMTAIL */
#define P5
#define SW_PIPE
#define TSRET	2	/* used by optim on parse_com for TMPSRET */

/* Intel 386 opcodes */

#define LABEL	3
#define HLABEL	4
#define DHLABEL	5	/* hard label created for .def .line */
#define ASMS	6
#define LCMT	7

#define	CALL	21
#define	LCALL	22
#define	RET	23
#define	LRET	24
#define	JMP	25
#define	LJMP	26
#define	JA	27
#define	JAE	28
#define	JB	29
#define	JBE	30
#define	JC	31
#define	JCXZ	32
#define	JE	33
#define	JG	34
#define	JGE	35
#define	JL	36
#define	JLE	37
#define	JNA	38
#define	JNAE	39
#define	JNB	40
#define	JNBE	41
#define	JNC	42
#define	JNE	43
#define	JNG	44
#define	JNGE	45
#define	JNL	46
#define	JNLE	47
#define	JNO	48
#define	JNP	49
#define	JNS	50
#define	JNZ	51
#define	JO	52
#define	JP	53
#define	JPE	54
#define	JPO	55
#define	JS	56
#define	JZ	57
#define	LOOP	58
#define	LOOPE	59
#define	LOOPNE	60
#define	LOOPNZ	61
#define	LOOPZ	62
#define	REP	63
#define	REPNZ	64
#define	REPZ	65
#define	AAA	66
#define	AAD	67
#define	AAM	68
#define	AAS	69
#define	DAA	70
#define	DAS	71
#define	ADCB	72
#define	ADCW	73
#define	ADCL	74
#define	ADDB	75
#define	ADDW	76
#define	ADDL	77
#define	DECB	78
#define	DECW	79
#define	DECL	80
#define	DIVB	81
#define	DIVW	82
#define	DIVL	83
#define	IDIVB	84
#define	IDIVW	85
#define	IDIVL	86
#define	IMULB	87
#define	IMULW	88
#define	IMULL	89
#define	INCB	90
#define	INCW	91
#define	INCL	92
#define	MULB	93
#define	MULW	94
#define	MULL	95
#define	NEGB	96
#define	NEGW	97
#define	NEGL	98
#define	SBBB	99
#define	SBBW	100
#define	SBBL	101
#define	SUBB	102
#define	SUBW	103
#define	SUBL	104
#define	ANDB	105
#define	ANDW	106
#define	ANDL	107
#define	ORB	108
#define	ORW	109
#define	ORL	110
#define	XORB	111
#define	XORW	112
#define	XORL	113
#define	CLRB	114
#define	CLRW	115
#define	CLRL	116
#define	RCLB	117
#define	RCLW	118
#define	RCLL	119
#define	RCRB	120
#define	RCRW	121
#define	RCRL	122
#define	ROLB	123
#define	ROLW	124
#define	ROLL	125
#define	RORB	126
#define	RORW	127
#define	RORL	128
#define	SALB	129
#define	SALW	130
#define	SALL	131
#define	SARB	132
#define	SARW	133
#define	SARL	134
#define	SHLB	135
#define	SHLW	136
#define	SHLL	137
#define	SHRB	138
#define	SHRW	139
#define	SHRL	140
#define	SHLDW	141
#define	SHLDL	142
#define	SHRDW	143
#define	SHRDL	144
#define	CMPB	145
#define	CMPW	146
#define	CMPL	147
#define	TESTB	148
#define	TESTW	149
#define	TESTL	150
#define	CBTW	151
#define	CWTL	152
#define	CWTD	153
#define	CLTD	154
#define	LDS	155
#define	LEAW	156
#define	LEAL	157
#define	LES	158
#define	MOVB	159
#define	MOVW	160
#define	MOVL	161
#define	MOVSBW	162
#define	MOVSBL	163
#define	MOVSWL	164
#define	MOVZBW	165
#define	MOVZBL	166
#define	MOVZWL	167
#define	NOTB	168
#define	NOTW	169
#define	NOTL	170
#define	POPW	171
#define	POPL	172
#define	PUSHW	173
#define	PUSHL	174
#define	XCHGB	175
#define	XCHGW	176
#define	XCHGL	177
#define	XLAT	178
#define	CLC	179
#define	CLD	180
#define	CLI	181
#define	CMC	182
#define	LAHF	183
#define	POPF	184
#define	PUSHF	185
#define	SAHF	186
#define	STC	187
#define	STD	188
#define	STI	189
#define	SCAB	190
#define	SCAW	191
#define	SCAL	192
#define	SCMPB	193
#define	SCMPW	194
#define	SCMPL	195
#define	SLODB	196
#define	SLODW	197
#define	SLODL	198
#define	SMOVB	199
#define	SMOVW	200
#define	SMOVL	201
#define	SSTOB	202
#define	SSTOW	203
#define	SSTOL	204
#define	INB	205
#define	INW	206
#define	INL	207
#define	OUTB	208
#define	OUTW	209
#define	OUTL	210
#define	ESC	211
#define	HLT	212
#define	INT	213
#define	INTO	214
#define	IRET	215
#define	LOCK	216
#define	WAIT	217
#define	ENTER	218
#define	LEAVE	219
#define	PUSHA	220
#define	POPA	221
#define	INS	222
#define	OUTS	223
#define	BOUND	224
#define	CTS	225
#define	LGDT	226
#define	SGDT	227
#define	LIDT	228
#define	SIDT	229
#define	LLDT	230
#define	SLDT	231
#define	LTR	232
#define	STR	233
#define	LMSW	234
#define	SMSW	235
#define	LAR	236
#define	LSL	237
#define	ARPL	238
#define	VERR	239

#define BOUNDL	240
#define BOUNDW	241
#define BSFL	242
#define BSFW	243
#define BSRL	244
#define BSRW	245
#define BSWAP	246
#define BTCL	247
#define BTCW	248
#define BTL	249
#define BTRL	250
#define BTRW	251
#define BTSL	252
#define BTSW	253
#define BTW	254
#define CLTS	255
#define CMPSB	256
#define CMPSL	257
#define CMPSW	258
#define CMPXCHGB	259
#define CMPXCHGL	260
#define CMPXCHGW	261
#define INSB	262
#define INSL	263
#define INSW	264
#define INVD	265
#define INVLPG	266
#define LARL	267
#define LARW	268
#define LDSL	269
#define LDSW	270
#define LESL	271
#define LESW	272
#define LFSL	273
#define LFSW	274
#define LGSL	275
#define LGSW	276
#define LODSB	277
#define LODSL	278
#define LODSW	279
#define LSLL	280
#define LSLW	281
#define LSSL	282
#define LSSW	283
#define MOVSL	284
#define NOP	285
#define OUTSB	286
#define OUTSL	287
#define OUTSW	288
#define POPAL	289
#define POPAW	290
#define POPFL	291
#define POPFW	292
#define PUSHAL	293
#define PUSHAW	294
#define PUSHFL	295
#define PUSHFW	296
#define REPE	297
#define REPNE	298
#define SCASB	299
#define SCASL	300
#define SCASW	301
#define SETA	302
#define SETAE	303
#define SETB	304
#define SETBE	305
#define SETC	306
#define SETE	307
#define SETG	308
#define SETGE	309
#define SETL	310
#define SETLE	311
#define SETNA	312
#define SETNAE	313
#define SETNB	314
#define SETNBE	315
#define SETNC	316
#define SETNE	317
#define SETNG	318
#define SETNL	319
#define SETNLE	320
#define SETNO	321
#define SETNP	322
#define SETNS	323
#define SETNZ	324
#define SETO	325
#define SETP	326
#define SETPE	327
#define SETPO	328
#define SETS	329
#define SETZ	330
#define SSCAB	331
#define SSCAL	332
#define SSCAW	333
#define STOSB	334
#define STOSL	335
#define STOSW	336
#define VERW	337
#define WBINVD	338
#define XADDB	339
#define XADDL	340
#define XADDW	341

#define	F2XM1	342
#define	FABS	343
#define	FCHS	344
#define	FCLEX	345
#define	FCOMPP	346
#define	FDECSTP	347
#define	FINCSTP	348
#define	FINIT	349
#define	FLD1	350
#define	FLDL2E	351
#define	FLDL2T	352
#define	FLDLG2	353
#define	FLDLN2	354
#define	FLDPI	355
#define	FLDZ	356
#define	FNCLEX	357
#define	FNINIT	358
#define	FNOP	359
#define	FPATAN	360
#define	FPREM	361
#define	FPTAN	362
#define	FRNDINT	363
#define	FSCALE	364
#define	FSETPM	365
#define	FSQRT	366
#define	FTST	367
#define	FWAIT	368
#define	FXAM	369
#define	FXTRACT	370
#define	FYL2X	371
#define	FYL2XP1	372
#define	FLDCW	373
#define	FSTCW	374
#define	FNSTCW	375
#define	FSTSW	376
#define	FNSTSW	377
#define	FSTENV	378
#define	FNSTENV	379
#define	FLDENV	380
#define	FSAVE	381
#define	FNSAVE	382
#define	FRSTOR	383
#define	FBLD	384
#define	FBSTP	385
#define	FIADD	386
#define	FIADDL	387
#define	FICOM	388
#define	FICOML	389
#define	FICOMP	390
#define	FICOMPL	391
#define	FIDIV	392
#define	FIDIVL	393
#define	FIDIVR	394
#define	FIDIVRL	395
#define	FILD	396
#define	FILDL	397
#define	FILDLL	398
#define	FIMUL	399
#define	FIMULL	400
#define	FIST	401
#define	FISTL	402
#define	FISTP	403
#define	FISTPL	404
#define	FISTPLL	405
#define	FISUB	406
#define	FISUBL	407
#define	FISUBR	408
#define	FISUBRL	409
#define	FADD	410
#define	FADDS	411
#define	FADDL	412
#define	FADDP	413
#define	FCOM	414
#define	FCOMS	415
#define	FCOML	416
#define	FCOMP	417
#define	FCOMPS	418
#define	FCOMPL	419
#define	FDIV	420
#define	FDIVS	421
#define	FDIVL	422
#define	FDIVP	423
#define	FDIVR	424
#define	FDIVRS	425
#define	FDIVRL	426
#define	FDIVRP	427
#define	FFREE	428
#define	FLD	429
#define	FLDS	430
#define	FLDL	431
#define	FLDT	432
#define	FMUL	433
#define	FMULS	434
#define	FMULL	435
#define	FMULP	436
#define	FST	437
#define	FSTS	438
#define	FSTL	439
#define	FSTP	440
#define	FSTPS	441
#define	FSTPL	442
#define	FSTPT	443
#define	FSUB	444
#define	FSUBS	445
#define	FSUBL	446
#define	FSUBP	447
#define	FSUBR	448
#define	FSUBRS	449
#define	FSUBRL	450
#define	FSUBRP	451
#define	FXCH	452
#define FCOS	453
#define FPREM1	454
#define FSIN	455
#define FSINCOS	456
#define FUCOM	457
#define FUCOMP	458
#define FUCOMPP	459
#define	OTHER	461
#define SAFE_ASM	500 /* SAFE_ASM must be greater than any other opcode */

#define is_safe_asm(p) ((p->op >= SAFE_ASM) || (p->sasm == SAFE_ASM))

/* pseudo ops */

enum psops { /* arranged alphabetically by their actual spellings */
	TWOBYTE,	/* .2byte */
	FOURBYTE,	/* .4byte */
	ALIGN,
	BCD,
	BSS,
	BYTE,
	COMM,
	DATA,
	DOUBLE,
	EVEN,
	EXT,
	FIL,
	FLOAT,
	GLOBL,
	IDENT,
	LCOMM,
	LOCAL,
	LONG,
	PREVIOUS,
	SECTION,
	SET,
	SIZE,
	STRING,
	TEXT,
	TYPE,
	VALUE,
	VERSION,
	WEAK,
	WORD,
	ZERO,
	POTHER /* gives required dimension of string table */
};

# define CC '/' /* begin comment character */

# define ASMEND	"/ASMEND"

/* Control sections */

enum Section {CSbss,CSdata,CSdebug,CSline,CStext,CSrodata,CSdata1,CSother} ;

/* predicates and functions */

#define NEWSIZE	(11+1+4+1+4+1+1+1+1)	/* for "ddddddddddd(%exx,%exx,8)\0" */
#define ADDLSIZE (1+10+1)		/* for "$2147483647\0" */
#define LABELSIZE	13			/* for ".L2147483637\0" */

# define islabel(p) \
	(p != NULL && (p->op == LABEL || p->op == HLABEL || p->op == DHLABEL))
# define ishl(p) (p->opcode[0] != '.' || (p->opcode[0] == '.' && p->opcode[1] == '.' ) || p->op == HLABEL || p->op == DHLABEL)
#define is_hard_label(s) (s[0] != '.' || (s[0] == '.' && s[1] == '.' ))
# define is_debug_label(p) (p->opcode[0] == '.' && p->opcode[1] == '.' )
# define is_label_text(s) (s[0] ==  '.' && (s[1] == '.' || isalpha(s[1])))
# define isuncbr(p) (p->op >= RET && p->op <= LJMP)
# define iscbr(p) (p->op >= JA && p->op <= JZ)
# define isbr(p) (p->op >= RET && p->op <= LOOPZ)
# define ishb(p) (p->op == RET || p->op == LRET || p->op == LJMP)
#define FindWhite(p)    while(!isspace(*p) && *(p) != '\0') p++;
#define SkipWhite(p)    while(isspace(*p)) p++;
#define strlength(p)	(strlen(p) + 1)	/* length of string including '\0' */
#define new_sets_uses(p)	{ p->uses = uses(p); p->sets = sets(p); }

/* predicates for safe asm ops */

#define sa_islabel(p) \
  	(p && (((int) p->op == LABEL + SAFE_ASM) \
  		|| ((int) p->op == HLABEL + SAFE_ASM) \
  		|| ((int) p->op == DHLABEL + SAFE_ASM)))

#define sa_isuncbr(p) ((int) p->op >= RET + SAFE_ASM \
  	&& (int) p->op <= LJMP + SAFE_ASM)
#define sa_iscbr(p) ((int) p->op >= JA + SAFE_ASM \
  	&& (int) p->op <= JZ + SAFE_ASM)
#define sa_isbr(p) (((int) p->op >= (RET +SAFE_ASM)) \
  	&& ((int) p->op <= (LOOPZ + SAFE_ASM)))

#define is_any_label(p)	(islabel(p) || sa_islabel(p))
#define is_any_uncbr(p)	(isuncbr(p) || sa_isuncbr(p))
#define is_any_cbr(p)	(iscbr(p) || sa_iscbr(p))
#define is_any_br(p)	(isbr(p) || sa_isbr(p))

/*
 * The second test in the isrev is extra checking so that
 * jump indirects do not get converted to jCC indirects which
 * are illegal on the 386.
 */
# define isrev(p) (p->op >= JA && p->op <= JZ && \
		   !(p->forw != NULL && \
		     p->forw->op == JMP && \
		     p->forw->op1[0] == '*') \
		  )

# define isret(p) (p->op == RET || p->op == LRET)
# define iscompare(p) (p->op == CMPL || p->op == CMPB || p->op == CMPW)
# define setlab(p) (p->op  = LABEL)
# define setbr(p,l) {(p)->op = JMP; (p)->opcode = "jmp"; \
	(p)->op1 = (l);}
# define bboptim(f,l) 0
# define mvlivecc(p) (p->back->nlive = (p->back->nlive & ~CONCODES) | (p->nlive & CONCODES))
# define swplivecc(p,q) { int x; x=(p->nlive & CONCODES); mvlivecc(q); q->nlive = (q->nlive & ~CONCODES) | x; }

/* maximum number of operands */

# define MAXOPS 4

/* The live/dead analysis information */

# define LIVEDEAD	32

/* live dead bits for physical registers. For each of %eax, %ebx, %ecx, %edx 
   there are 3 separate live-dead bits: Consider %eax, the 3 live-dead bits
   correspond to the following names:
	1. %ah
	2. %al
	3. %eax or %ax
*/

/* temps */
#define	Eax	0x00000001
#define	Edx	0x00000002
#define	Ecx	0x00000004

#define	FP0	0x00000008
#define	FP1	0x00000010

/* register variables */
#define	Ebx		0x00000020
#define	Esi		0x00000040
#define	Edi		0x00000080

#define FP2		0x00000100
#define FP3		FP2
#define FP4		FP2
#define FP5		FP2
#define FP6		FP2
#define FP7		FP2

#define Ebi		0x00000200
#define BI		0x00000400
#define EBI		(Ebi|BI)

#define Ebp     0x00000800
#define BP      0x00001000
#define EBP     (Ebp | BP)

#define	ESP		0x00008000
/* condition codes */
# define CONCODES 	0x00010000

#ifdef NOSPLIT
/* all register names for a physical register map to the same live_dead bit */
#define	AH		Eax
#define	AL		Eax
#define	BH		Ebx
#define	BL		Ebx
#define	CH		Ecx
#define	CL		Ecx
#define	DH		Edx
#define	DL		Edx
#else
/* separate live-dead bits for same physical register */
#define	AH		0x00020000
#define	AL		0x00040000
#define	BH		0x00080000
#define	BL		0x00100000
#define	CH		0x00200000
#define	CL		0x00400000
#define	DH		0x00800000
#define	DL		0x01000000
#endif

#define Ax		0x02000000   /*16 bit registers*/
#define Dx		0x04000000
#define Bx		0x08000000
#define Cx		0x10000000
#define SI		0x20000000
#define DI		0x40000000

#define MEM		(unsigned) 0x80000000

/* everything */
#define	REGS		0x7FFFFFFF


/* references to EAX or AX references all 3 live-dead bits, similary for EBX
   ECX and EDX
*/
#define AX	(Ax|AH|AL)
#define BX	(Bx|BH|BL)
#define CX	(Cx|CH|CL)
#define DX	(Dx|DH|DL)
#define EAX	(Eax|Ax|AH|AL)
#define EBX	(Ebx|Bx|BH|BL)
#define ECX	(Ecx|Cx|CH|CL)
#define EDX	(Edx|Dx|DH|DL)
#define ESI (Esi|SI)
#define EDI (Edi|DI)
#define R16MSB (Eax|Ebx|Ecx|Edx|Esi|Edi|Ebi)
#define R24MSB (R16MSB|Ax|Bx|Cx|Dx|AH|BH|CH|DH)
#define L2W(r) (r & ~R16MSB) /* Convert 4 to 2 byte reg */
#define L2B(r) (r & ~R24MSB) /* Convert 4 to 1 */
/* maximum return registers */
#define	MXRETREG	0x0000001F

/* always live registers */
#define	LIVEREGS	(pic_flag?(EBX|ESP):(ESP))
#define SAVEDREGS	(EBX|ESI|EDI|EBI|EBP)

# define isdeadcc(p) ((p->nlive & CONCODES) == 0)

/* integer size for various assumptions in IMPREGAL, and IMPIL */
#define INTSIZE 4

#ifdef	IMPRETVAL
extern int RETREG;
#else	/* IMPRETVAL */
#define RETREG		0x0000001F
#endif	/* IMPRETVAL */

/* options */

# define MEMFCN
# define COMTAIL
# define PEEPHOLE

/* line number stuff */

# define IDTYPE int
# define IDVAL 0

#define spflg(i) ( (i) == 'K' || (i) == 'y' || (i) == 'X' || \
   (i) == 'Q' || (i) == '_' || (i) == '3' || (i) == '4' || (i) == '5')
			/* indicate flags with suboptions
			   K: -Ksd, -Ksz ( speed vs. size )
			      -KPIC,-Kpic ( position indep code )
			      -Kieee,-Knoieee (whether ieee or not )
			   y: -yu, -ys, -y<num> (inline growth)
			   X: -Xt, -Xa, -Xc (ansi stuff)
			   _: -_r, -_e suppress reg_alloc, enter_leave
			   3: -386 (turn off 486 optimizations which hurt
					386 performance)
			   4: -486 (default: turn on 486 optimizations)
			   5: -586 (turn on 586 optimizations)          */

/* States of optimization mode */
#define	ODEFAULT	1
#define OSPEED		1
#define OSIZE		2

extern int optmode;

/* Macro to add new instruction:
**	opn	op code number of new instruction
**	opst	op code string of new instruction
**	opn1	operand 1 for new instruction
**	opn2	operand 2 for new instruction
*/
#define addi(pn,opn,opst,opn1,opn2) \
	{ \
		(pn) = insert( (pn) );		/* get new node */ \
		chgop((pn),(opn),(opst));	/* put in opcode num, str */ \
		(pn)->op1 = (opn1);		/* put in operands */ \
		(pn)->op2 = (opn2);	\
	}

#define opm 	ops[MAXOPS+1]

/* Macro to check for profiling code:
**	pn	pointer to first node
*/
#define isprof(pn) ( (pn)->op == MOVL \
	&& (pn)->forw->op == CALL \
	&& strcmp( (pn)->forw->op1, "_mcount" ) == 0 ) \

#define isgetaddr(pn) \
	   (pn->op == POPL \
		&& samereg("%eax",pn->op1) \
		&& pn->forw->op == XCHGL \
		&& samereg("%eax",pn->forw->op1) \
		&& strcmp("0(%esp)",pn->forw->op2) == 0)


/* (Initial) size of line buffers */
#define LINELEN BUFSIZ

/* Max size of string needed to represent any address that does not */
/* contain a symbolic portion  - "dddddddddd(%exx,%exx,8)\0" */
#define NONSYMADDRSZ	(10+1+4+1+4+1+1+1+1)

enum CC_Mode {Transition, Ansi, Conform};


/* Macros to handle volatile operands */
#define USERDATA
#define USERTYPE int	/* defines the type of the userdata field of NODE */
			/* We use it to hold bits indicating whether a */
			/* given operand of the node is volatile. */
#define USERINITVAL 0


#define mark_vol_opnd(node,opnd) (node)->userdata |= (1 << (opnd))
#define is_vol_opnd(node,opnd) ((node)->userdata & (1 << (opnd)))
#ifdef DEBUGGING
#include "debugging.h"
#endif

/* constants to be passed as parameters to drivers of optimizations */

#define ZERO_PROP	3
#define COPY_PROP	4
