/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:util/symbols.c	1.6"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <util/ipl.h>
#include <svc/pic.h>
#include <proc/user.h>
#include <mem/immu.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <proc/proc.h>
#include <mem/vmparam.h>
#include <proc/reg.h>
#include <svc/bootinfo.h>
#include <util/fp.h>
#include <mem/faultcatch.h>
#include <svc/sysenvmt.h>
#ifdef VPIX
#include <vpix/v86.h>
#endif	/* VPIX */
#ifdef WEITEK
#include <util/weitek.h>
#endif	/* WEITEK */

#define offsetof(s, m)	(size_t)(&(((s *)0)->m))

	/* spl/ipl */
size_t __SYMBOL__IPLHI = IPLHI;

size_t __SYMBOL__SPL0 = SPL0;
size_t __SYMBOL__SPL1 = SPL1;
size_t __SYMBOL__SPL2 = SPL2;
size_t __SYMBOL__SPL3 = SPL3;
size_t __SYMBOL__SPL4 = SPL4;
size_t __SYMBOL__SPL5 = SPL5;
size_t __SYMBOL__SPL6 = SPL6;
size_t __SYMBOL__SPLSTR = SPLSTR;
size_t __SYMBOL__SPLVM = SPLVM;
size_t __SYMBOL__SPLIMP = SPLIMP;
size_t __SYMBOL__SPLTTY = SPLTTY;

size_t __SYMBOL__SPL7 = IPLHI;
size_t __SYMBOL__SPLHI = IPLHI;


	/* pic */
size_t __SYMBOL__PIC_NEEDICW4 = PIC_NEEDICW4;
size_t __SYMBOL__PIC_ICW1BASE = PIC_ICW1BASE;
size_t __SYMBOL__PIC_86MODE = PIC_86MODE;
size_t __SYMBOL__PIC_AUTOEOI = PIC_AUTOEOI;
size_t __SYMBOL__PIC_SLAVEBUF = PIC_SLAVEBUF;
size_t __SYMBOL__PIC_MASTERBUF = PIC_MASTERBUF;
size_t __SYMBOL__PIC_SPFMODE = PIC_SPFMODE;
size_t __SYMBOL__PIC_READISR = PIC_READISR;
size_t __SYMBOL__PIC_NSEOI = PIC_NSEOI;
size_t __SYMBOL__PIC_VECTBASE = PIC_VECTBASE;
size_t __SYMBOL__NPIC = NPIC;
size_t __SYMBOL__MCMD_PORT = MCMD_PORT;
size_t __SYMBOL__MIMR_PORT = MIMR_PORT;
size_t __SYMBOL__SCMD_PORT = SCMD_PORT;
size_t __SYMBOL__SIMR_PORT = SIMR_PORT;
size_t __SYMBOL__MASTERLINE = MASTERLINE;
size_t __SYMBOL__SLAVEBASE = SLAVEBASE;
size_t __SYMBOL__PICBUFFERED = PICBUFFERED;
size_t __SYMBOL__I82380 = I82380;

	/* user.h */
size_t __SYMBOL__PSCOMSIZ = PSCOMSIZ;
size_t __SYMBOL__KSTKSZ = KSTKSZ;

size_t __SYMBOL__u_callgate = offsetof(struct user, u_callgate);
size_t __SYMBOL__u_callgatep = offsetof(struct user, u_callgatep);
size_t __SYMBOL__u_debugon = offsetof(struct user, u_debugon);
size_t __SYMBOL__u_debugreg = offsetof(struct user, u_debugreg);
size_t __SYMBOL__u_fpintgate = offsetof(struct user, u_fpintgate);
size_t __SYMBOL__u_procp = offsetof(struct user, u_procp);
size_t __SYMBOL__u_sigfault = offsetof(struct user, u_sigfault);
size_t __SYMBOL__u_tss = offsetof(struct user, u_tss);
size_t __SYMBOL__u_weitek = offsetof(struct user, u_weitek);
size_t __SYMBOL__u_weitek_reg = offsetof(struct user, u_weitek_reg);
size_t __SYMBOL__u_fault_catch = offsetof(struct user, u_fault_catch);

	/* immu */
size_t __SYMBOL__NPGPT = NPGPT;
size_t __SYMBOL__NBPP = NBPP;
size_t __SYMBOL__NBPPT = NBPPT;
size_t __SYMBOL__BPTSHFT = BPTSHFT;
size_t __SYMBOL__NPTPP = NPTPP;
size_t __SYMBOL__NPTPPSHFT = NPTPPSHFT;
size_t __SYMBOL__NUPP = NUPP;
size_t __SYMBOL__UPPSHFT = UPPSHFT;
size_t __SYMBOL__PNUMSHFT = PNUMSHFT;
size_t __SYMBOL__PNUMMASK = PNUMMASK;
size_t __SYMBOL__POFFMASK = POFFMASK;
size_t __SYMBOL__PTOFFMASK = PTOFFMASK;
size_t __SYMBOL__PNDXMASK = PNDXMASK;
size_t __SYMBOL__PGFNMASK = PGFNMASK;
size_t __SYMBOL__PTNUMSHFT = PTNUMSHFT;
size_t __SYMBOL__PTSIZE = PTSIZE;
size_t __SYMBOL__PTMASK = PTMASK;
size_t __SYMBOL__VPTSIZE = VPTSIZE;
size_t __SYMBOL__PT_ADDR = PT_ADDR;
size_t __SYMBOL__PG_ADDR = PG_ADDR;
#ifdef DEBUG
size_t __SYMBOL__PG_LOCKCNT = PG_LOCKCNT;
#endif
size_t __SYMBOL__PG_M = PG_M;
size_t __SYMBOL__PG_REF = PG_REF;
size_t __SYMBOL__PG_US = PG_US;
size_t __SYMBOL__PG_RW = PG_RW;
size_t __SYMBOL__PG_V = PG_V;
size_t __SYMBOL__PG_P = PG_P;
size_t __SYMBOL__PTE_RW = PTE_RW;
size_t __SYMBOL__PTE_PROTMASK = PTE_PROTMASK;
size_t __SYMBOL__O_PG_LOCK = O_PG_LOCK;
size_t __SYMBOL__PG_LOCK = PG_LOCK;
size_t __SYMBOL__XMEM_BIT = XMEM_BIT;
size_t __SYMBOL__SOFFMASK = SOFFMASK;
size_t __SYMBOL__SGENDMASK = SGENDMASK;
size_t __SYMBOL__PHYSCONTIG = PHYSCONTIG;
size_t __SYMBOL__NOSLEEP = NOSLEEP;
size_t __SYMBOL__SEL_RPL = SEL_RPL;

	/* proc/seg.h */

size_t __SYMBOL__UDATA_ACC1 = UDATA_ACC1;
size_t __SYMBOL__KDATA_ACC1 = KDATA_ACC1;
size_t __SYMBOL__DATA_ACC2 = DATA_ACC2;
size_t __SYMBOL__DATA_ACC2_S = DATA_ACC2_S;
size_t __SYMBOL__UTEXT_ACC1 = UTEXT_ACC1;
size_t __SYMBOL__KTEXT_ACC1 = KTEXT_ACC1;
size_t __SYMBOL__TEXT_ACC2 = TEXT_ACC2;
size_t __SYMBOL__TEXT_ACC2_S = TEXT_ACC2_S;
size_t __SYMBOL__LDT_UACC1 = LDT_UACC1;
size_t __SYMBOL__LDT_KACC1 = LDT_KACC1;
size_t __SYMBOL__LDT_ACC2 = LDT_ACC2;
size_t __SYMBOL__TSS3_KACC1 = TSS3_KACC1;
size_t __SYMBOL__TSS3_KBACC1 = TSS3_KBACC1;
size_t __SYMBOL__TSS2_KACC1 = TSS2_KACC1;
size_t __SYMBOL__TSS3_UACC1 = TSS3_UACC1;
size_t __SYMBOL__TGATE_UACC1 = TGATE_UACC1;
size_t __SYMBOL__TSS2_UACC1 = TSS2_UACC1;
size_t __SYMBOL__TSS_ACC2 = TSS_ACC2;
size_t __SYMBOL__SEG_CONFORM = SEG_CONFORM;
size_t __SYMBOL__LDTSEL = LDTSEL;
size_t __SYMBOL__UTSSSEL = UTSSSEL;
size_t __SYMBOL__KTSSSEL = KTSSSEL;
size_t __SYMBOL__KCSSEL = KCSSEL;
size_t __SYMBOL__KDSSEL = KDSSEL;
size_t __SYMBOL__DFTSSSEL = DFTSSSEL;
size_t __SYMBOL__JTSSSEL = JTSSSEL;
size_t __SYMBOL__MON1SEL = MON1SEL;
size_t __SYMBOL__MON3SEL = MON3SEL;
size_t __SYMBOL__FPESEL = FPESEL;
size_t __SYMBOL__XTSSSEL = XTSSSEL;
size_t __SYMBOL__GRANBIT = GRANBIT;
size_t __SYMBOL__USER_CS = USER_CS;
size_t __SYMBOL__USER_DS = USER_DS;
size_t __SYMBOL__USER_SCALL = USER_SCALL;
size_t __SYMBOL__USER_SIGCALL = USER_SIGCALL;
size_t __SYMBOL__USER_FPSTK = USER_FPSTK;
size_t __SYMBOL__USER_FP = USER_FP;
size_t __SYMBOL__CSALIAS_SEL = CSALIAS_SEL;
size_t __SYMBOL__IDTSZ = IDTSZ;
size_t __SYMBOL__MONIDTSZ = MONIDTSZ;
size_t __SYMBOL__MINLDTSZ = MINLDTSZ;
size_t __SYMBOL__MAXLDTSZ = MAXLDTSZ;
size_t __SYMBOL__GDTSZ = GDTSZ;
size_t __SYMBOL__SEL_LDT = SEL_LDT;
size_t __SYMBOL__KTBASE = KTBASE;
size_t __SYMBOL__KDBASE = KDBASE;
size_t __SYMBOL__GATE_UACC = GATE_UACC;
size_t __SYMBOL__GATE_KACC = GATE_KACC;
size_t __SYMBOL__GATE_386CALL = GATE_386CALL;
size_t __SYMBOL__GATE_386INT = GATE_386INT;
size_t __SYMBOL__GATE_386TRP = GATE_386TRP;
size_t __SYMBOL__GATE_TSS = GATE_TSS;

	/* proc/tss.h */

size_t __SYMBOL__t_link = offsetof(struct tss386, t_link);
size_t __SYMBOL__t_esp0 = offsetof(struct tss386, t_esp0);
size_t __SYMBOL__t_ss0 = offsetof(struct tss386, t_ss0);
size_t __SYMBOL__t_esp1 = offsetof(struct tss386, t_esp1);
size_t __SYMBOL__t_ss1 = offsetof(struct tss386, t_ss1);
size_t __SYMBOL__t_esp2 = offsetof(struct tss386, t_esp2);
size_t __SYMBOL__t_ss2 = offsetof(struct tss386, t_ss2);
size_t __SYMBOL__t_cr3 = offsetof(struct tss386, t_cr3);
size_t __SYMBOL__t_eip = offsetof(struct tss386, t_eip);
size_t __SYMBOL__t_eflags = offsetof(struct tss386, t_eflags);
size_t __SYMBOL__t_eax = offsetof(struct tss386, t_eax);
size_t __SYMBOL__t_ecx = offsetof(struct tss386, t_ecx);
size_t __SYMBOL__t_edx = offsetof(struct tss386, t_edx);
size_t __SYMBOL__t_ebx = offsetof(struct tss386, t_ebx);
size_t __SYMBOL__t_esp = offsetof(struct tss386, t_esp);
size_t __SYMBOL__t_ebp = offsetof(struct tss386, t_ebp);
size_t __SYMBOL__t_esi = offsetof(struct tss386, t_esi);
size_t __SYMBOL__t_edi = offsetof(struct tss386, t_edi);
size_t __SYMBOL__t_es = offsetof(struct tss386, t_es);
size_t __SYMBOL__t_cs = offsetof(struct tss386, t_cs);
size_t __SYMBOL__t_ss = offsetof(struct tss386, t_ss);
size_t __SYMBOL__t_ds = offsetof(struct tss386, t_ds);
size_t __SYMBOL__t_fs = offsetof(struct tss386, t_fs);
size_t __SYMBOL__t_gs = offsetof(struct tss386, t_gs);
size_t __SYMBOL__t_ldt = offsetof(struct tss386, t_ldt);
size_t __SYMBOL__t_bitmapbase = offsetof(struct tss386, t_bitmapbase);

	/* proc/proc.h */

size_t __SYMBOL__p_ubptbl = offsetof(struct proc, p_ubptbl);
size_t __SYMBOL__p_usize = offsetof(struct proc, p_usize);

	/* mem/vmparam.h */

size_t __SYMBOL__UVBASE = UVBASE;
size_t __SYMBOL__UVSTACK = UVSTACK;
size_t __SYMBOL__UVSHM = UVSHM;
size_t __SYMBOL__KVBASE = KVBASE;
size_t __SYMBOL__KVXBASE = KVXBASE;
size_t __SYMBOL__KVSBASE = KVSBASE;
size_t __SYMBOL__UVUBLK = UVUBLK;
size_t __SYMBOL__UVTEXT = UVTEXT;
size_t __SYMBOL__UVEND = UVEND;
size_t __SYMBOL__MINUVADR = MINUVADR;
size_t __SYMBOL__MAXUVADR = MAXUVADR;
size_t __SYMBOL__MINKVADR = MINKVADR;
size_t __SYMBOL__MAXKVADR = MAXKVADR;

	/* proc/reg.h */

size_t __SYMBOL__SS = SS;
size_t __SYMBOL__UESP = UESP;
size_t __SYMBOL__EFL = EFL;
size_t __SYMBOL__CS = CS;
size_t __SYMBOL__EIP = EIP;
size_t __SYMBOL__ERR = ERR;
size_t __SYMBOL__TRAPNO = TRAPNO;
size_t __SYMBOL__EAX = EAX;
size_t __SYMBOL__ECX = ECX;
size_t __SYMBOL__EDX = EDX;
size_t __SYMBOL__EBX = EBX;
size_t __SYMBOL__ESP = ESP;
size_t __SYMBOL__EBP = EBP;
size_t __SYMBOL__ESI = ESI;
size_t __SYMBOL__EDI = EDI;
size_t __SYMBOL__DS = DS;
size_t __SYMBOL__ES = ES;
size_t __SYMBOL__FS = FS;
size_t __SYMBOL__GS = GS;

	/* svc/bootinfo.h */

size_t __SYMBOL__BKI_MAGIC = BKI_MAGIC;
size_t __SYMBOL__BOOTINFO_LOC = BOOTINFO_LOC;
size_t __SYMBOL__KPTBL_LOC = KPTBL_LOC;
size_t __SYMBOL__memusedcnt = offsetof(struct bootinfo, memusedcnt);
size_t __SYMBOL__memused = offsetof(struct bootinfo, memused);

	/* svc/sysenvmt.h */

size_t __SYMBOL__MC_BUS = MC_BUS;
size_t __SYMBOL__machflags = offsetof(struct sysenvmt, machflags);

	/* util/fp.h */

size_t __SYMBOL__FP_NO = FP_NO;
size_t __SYMBOL__FP_SW = FP_SW;
size_t __SYMBOL__FP_HW = FP_HW;
size_t __SYMBOL__FP_287 = FP_287;
size_t __SYMBOL__FP_387 = FP_387;

	/* mem/faultcatch.h */
size_t __SYMBOL__fc_flags = offsetof(struct fault_catch, fc_flags);
size_t __SYMBOL__fc_errno = offsetof(struct fault_catch, fc_errno);
size_t __SYMBOL__fc_func = offsetof(struct fault_catch, fc_func);
size_t __SYMBOL__fc_jmp = offsetof(struct fault_catch, fc_jmp);
size_t __SYMBOL__CATCH_UFAULT = CATCH_UFAULT;
size_t __SYMBOL__CATCH_SEGMAP_FAULT = CATCH_SEGMAP_FAULT;
size_t __SYMBOL__CATCH_SEGU_FAULT = CATCH_SEGU_FAULT;
size_t __SYMBOL__CATCH_BUS_TIMEOUT = CATCH_BUS_TIMEOUT;
size_t __SYMBOL__CATCH_ALL_FAULTS = CATCH_ALL_FAULTS;
size_t __SYMBOL__CATCH_KERNEL_FAULTS = CATCH_KERNEL_FAULTS;

#ifdef VPIX
	/* vpix/v86.h */
size_t __SYMBOL__XTSSADDR = (size_t)XTSSADDR;
size_t __SYMBOL__xt_vflptr = offsetof(struct v86xtss, xt_vflptr);
size_t __SYMBOL__xt_imaskbits = offsetof(struct v86xtss, xt_imaskbits);
size_t __SYMBOL__xt_intr_pin = offsetof(struct v86xtss, xt_intr_pin);
size_t __SYMBOL__xt_magicstat = offsetof(struct v86xtss, xt_magicstat);
size_t __SYMBOL__xt_magictrap = offsetof(struct v86xtss, xt_magictrap);
size_t __SYMBOL__xt_op_emul = offsetof(struct v86xtss, xt_op_emul);
#endif	/* VPIX */

#ifdef WEITEK
	/* util/weitek.h */
size_t __SYMBOL__WEITEK_HW = WEITEK_HW;
size_t __SYMBOL__WEITEK_NO = WEITEK_NO;
size_t __SYMBOL__WEITEK_20MHz = WEITEK_20MHz;
#endif	/* WEITEK */
