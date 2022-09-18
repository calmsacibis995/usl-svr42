/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)uts-x86:util/cmn_err.c	1.9"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <util/cmn_err.h>
#include <fs/vnode.h>
#include <svc/resource.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/exec.h>
#include <io/tty.h>
#include <proc/reg.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <mem/seg.h>
#include <util/inline.h>
#include <svc/bootinfo.h>
#include <util/xdebug.h>
#include <io/iobuf.h>
#include <io/strlog.h>
#include <util/debug.h>
#include <svc/sysi86.h>
#include <io/stream.h>
#include <io/log/log.h>

void sysdump();

STATIC void printn();

extern	int	cpu_family;
int panic_level = 0; /* Used outside this module by kdb */

/*
 * A delay is required when outputting many lines to the console
 * if it is a DMD 5620 terminal.  The value of 0x1000 was
 * chosen empirically.  If your console is a normal terminal, set
 * the delay to 0.  Note that dmd_delay can be set to 0 on the
 * running system with a kernel debugger.
 */

#define	DMD_DELAY	0

STATIC int dmd_delay = DMD_DELAY;

void
dodmddelay()
{
	int	delay;

	if (dmd_delay) {
		delay = dmd_delay;
		while (delay--) ;
	}
}

/*
 * Save output in a buffer where we can look at it with a kernel debugger
 * or crash.  If the message begins with a '!', then only put
 * it in the buffer, not out to the console.
 */

extern	int	conslog_set();

STATIC	short	prt_where = PRW_CONS;
static	short	not_cmn;	
static	int	cmntype;
STATIC	char	consbuf[256];
STATIC	int	conspos;

#define	output(c) {						\
	if (prt_where & PRW_CONS) {				\
		if (conslog_set(CONSLOG_STAT) == CONSLOG_ENA)	\
			consbuf[conspos++ % 256] = c;		\
		else						\
			putchar(c);				\
	}							\
	if (prt_where & PRW_BUF) {				\
		if (putbufwpos >= putbufsz)			\
			putbufwpos = 0;				\
		putbuf[putbufwpos++] = c;			\
		putbufrpos = putbufwpos;			\
		NVRAMPUTCHAR(c);				\
	}							\
}

char   *panicstr;		/* pointer to panic message */
int	putbufrpos = 0;		/* next byte to read in system putbuf */
int	putbufwpos = 0;		/* next byte to write in system putbuf */
int	in_demon = 0;

/*
 * Configure the destination of kernel messages, return the previous state
 */
short
prfconfig(where)
short where;
{
	short rval = prt_where;

	prt_where = where;

	return(rval);
}

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d %o %x %c %b are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much suspended.
 * Printf should not be used for chit-chat.
 */

STATIC void
xprintf(fmtp)
	char **fmtp;
{
	VA_LIST ap;
	register char *fmt;
	register char	c;
	register char	*s;
	register int	opri;
	int width;

	fmt = *fmtp;
	ap = (char *)(fmtp + 1);

	opri = splhi();
	if (in_demon)
		conslog_set(CONSLOG_DIS);
	else {
		bzero(consbuf, 256);
		conspos = 0;
	}

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0') {
			if (not_cmn)	/* cmn_err not used */
				cmntype = 0;

			/*
			 * If message was 257 bytes long, conspos will be 0
			 * and no message will be output.  Too bad.
			 */
			if (conspos > 0)
			    (void)strlog(0, 0, 0, SL_CONSOLE|cmntype, 
				consbuf, 0);
			if (in_demon)
				conslog_set(CONSLOG_ENA);
			splx(opri);
			return;
		}
		output(c);
	}
	width = 0;
	if (*fmt == '.') {
		while (*++fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt - '0');
	}
	if ((c = *fmt++) == 'l')
		c = *fmt++;
	if (c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	switch (c) {
	case 'd':
	case 'u':
		printn(VA_ARG(ap, long), 10, (c != 'u'), width, 0);
		break;
	case 'o':
		printn(VA_ARG(ap, long), 8, 0, width, 11);
		break;
	case 'x':
		printn(VA_ARG(ap, long), 16, 0, width, 8);
		break;
	case 'b':
		printn((long)(VA_ARG(ap, int) & 0xFF), 16, 0, width, 2);
		break;
	case 's':
		s = VA_ARG(ap, char *);
		while ((c = *s++) != 0)
			output(c);
		break;
	case 'c':
		c = (VA_ARG(ap, int) & 0xFF);
		output(c);
		break;
	default:
		/* unknown format -- print it */
		output(c);
	}
	goto loop;
}

STATIC void
xpanic(msgp)
	char **msgp;
{
	extern void sync();
	int	x;

	/* Save panic string (needed for routines elsewhere). */
	panicstr = *msgp;

	/* Get execution level. */
	x = splhi();
	splx(x);

#if NOTYET
	/* "sync" if at spl0 */
	if (x == 0)
		sync();
#endif

#ifndef NODEBUGGER
	(*cdebugger)(DR_PANIC, NO_FRAME);
#endif

	panic_act(panicstr);	/* call to rmc driver */

	/* Dump memory to disk and then reboot */
	sysdump();
}

void
sysdump()
{
	extern struct dscr gdt[];

	/* Force a task switch from the current task (to the current task)
		so our state gets saved in the uarea */
	setdscrbase(&gdt[seltoi(JTSSSEL)], (uint)u.u_tss);
	gdt[seltoi(JTSSSEL)].a_acc0007 = TSS3_KACC1;
	asm("ljmp $0x170,$0");	/* 0x170 = JTSSSEL */


	gdt[seltoi(JTSSSEL)].a_acc0007 = TSS3_KACC1;
	asm("ljmp $0x170,$0");	/* 0x170 = JTSSSEL */

	oemsysdump();

	rtnfirm();	/* oemreboot() should do this, but just in case... */
	/* NOTREACHED */
}

static void
xxcmn_err(level, fmtp)
	register int	level;
	char		**fmtp;
{
	register int	i;
	register int	x;
	char buf[512];
	char *p = buf;
	int prt_save = prt_where;

	/*
	 * Set up to print to putbuf, console, or both
	 * as indicated by the first character of the
	 * format.
	 */

	bzero((caddr_t)buf, sizeof(buf));

	not_cmn = 0;

	if (**fmtp == '!') {
		prt_save = prfconfig(PRW_BUF);
		(*fmtp)++;
	} else if (**fmtp == '^') {
		prt_save = prfconfig(PRW_CONS);
		(*fmtp)++;
	} else
		prt_save = prfconfig(PRW_BUF | PRW_CONS);

	switch (level) {
		case CE_CONT:
			cmntype = 0;
			xprintf(fmtp);
			break;

		case CE_NOTE:
			cmntype = SL_NOTE;
			strcpy(buf, "\nNOTICE: ");
			strcat(p, *fmtp);
			strcat(p, "\n");
			*fmtp = (char *)&buf;
			xprintf(fmtp);
			break;

		case CE_WARN:
			if (prt_where & PRW_CONS)
				warn_alm(fmtp);	/* call to rmc driver */
			cmntype = SL_WARN;
			strcpy(buf, "\nWARNING: ");
			strcat(p, *fmtp);
			strcat(p, "\n");
			*fmtp = (char *)&buf;
			xprintf(fmtp);
			break;

		case CE_PANIC: {
			switch (panic_level) {
			case 0: 
				x = splhi();

				/*
				 * Processes logging console messages
				 * will never run.  Force message to
				 * go to console.
				 */
				conslog_set(CONSLOG_DIS);

				prt_where = PRW_CONS | PRW_BUF;
				panic_level = 1;
				printf("\nPANIC: ");
				xprintf(fmtp);
				printf("\n");
				splx(x);

				xpanic(fmtp);
				/* NOTREACHED */

			case 1:
				panic_level = 2;
				prt_where = PRW_CONS | PRW_BUF;
				printf("\nDOUBLE PANIC: ");
				xprintf(fmtp);
				printf("\n");
#ifndef NODEBUGGER
				(*cdebugger)(DR_PANIC, NO_FRAME);
#endif
				sysdump();
				/* NOTREACHED */

			default:
				panic_level = 3;
				sysdump();
				/* NOTREACHED */
			}
		}

		default:
			cmn_err(CE_PANIC,
	  		  "unknown level: cmn_err(level=%d, msg=\"%s\")",
			   level, *fmtp);
	}

	not_cmn = 1;
	prfconfig(prt_save);
}

STATIC void
xcmn_err(level, fmtp)
	register int	level;
	char		**fmtp;
{
	xxcmn_err(level, fmtp);
}

/*PRINTFLIKE1*/
void 
#ifdef __STDC__
printf(char *fmt, ...)
#else
printf(fmt)
	char *fmt;
#endif
{
	xprintf(&fmt);
}

STATIC void
printn(n, b, sflag, width, zero)
	long n;
	register int b;
	int sflag;   /* 0: unsigned print, 1: signed print */
	int width;   /* 0: variable width, else pad w/spaces to this width */
	int zero;    /* if != 0, zero-fill width */
{
	register unsigned long nn = n;
	register int i;
	char d[11];  /* 32 bits in octal needs 11 digits */

	if (sflag && n < 0) {
		output('-');
		nn = -nn;
	}
	for (i=0;;) { /* output at least one digit (for 0) */
		d[i++] = nn % b;
		nn = nn / b;
		if (nn == 0)
			break;
	}

	while (width-- > (zero > i ? zero : i))
		output(' ');
	while (zero-- > i)
		output('0');
	while (i-- > 0)
		output("0123456789ABCDEF"[d[i]]);
}

/*
 * Generic console putchar routine
 */
putchar(c)
register int c;
{
	extern struct conssw conssw;

	if(c == '\n')
		(*conssw.co)('\r',conssw.co_dev);

	(*conssw.co)(c,conssw.co_dev);
}

/*
 * Generic console getchar routine
 */
getchar()
{
	extern struct conssw conssw;

	return (*conssw.ci)(conssw.co_dev);
}

/*
 * Panic is called on unresolvable fatal errors.
 */

/*PRINTFLIKE1*/
void
#ifdef __STDC__
panic(char *msg, ...)
#else
panic(msg)
	char *msg;
#endif
{
	xcmn_err(CE_PANIC, &msg);
}

/*
 * prdev prints a warning message.
 * dev is a block special device argument.
 */

void
prdev(str, dev)
	char *str;
	dev_t dev;
{
	register major_t maj;

	maj = getmajor(dev);
	if (maj >= bdevcnt) {
		cmn_err(CE_WARN, "%s on bad dev 0x%x\n", str, dev);
		return;
	}
	if (*bdevsw[maj].d_flag & D_OLD)
		(*bdevsw[maj].d_print)(cmpdev(dev), str);
	else
		(*bdevsw[maj].d_print)(dev, str);
}

/*
 * Deverr prints a diagnostic from a device driver.
 * It prints: error on device name (major/minor), block number,
 * and two arguments, usually error status.
 */
deverr(dp, o1, o2, dn)
	struct iobuf *dp;
	char *dn;		/* device name */
{
	register struct buf *bp;

	bp = dp->b_actf;
	cmn_err(CE_WARN, "error on dev %s (%u/%u)", 
		dn,major(bp->b_dev),minor(bp->b_dev));
	cmn_err(CE_WARN, ", block=%D cmd=%x status=%x\n", bp->b_blkno, o1, o2);
}

/*
 * Seterror sets u.u_error to the parameter value. Used by
 * loadable device drivers.
 */
seterror(errno)
	char errno;
{
	u.u_error = errno;
}

/*PRINTFLIKE2*/
void
#ifdef __STDC__
cmn_err(int level, char *fmt, ...)
#else
cmn_err(level, fmt)
	int level;
	char *fmt;
#endif
{

	xcmn_err(level, &fmt);
}

/*
 * The following is an interface routine for the drivers.
 */

/*PRINTFLIKE1*/
void
#ifdef __STDC__
dri_printf(char *fmt, ...)
#else
dri_printf(fmt)
	char *fmt;
#endif
{
	xcmn_err(CE_CONT, &fmt);
}

#ifdef DEBUG
STATIC int aask, aok;
#endif

/*
 * Called by the ASSERT macro in debug.h when an assertion fails.
 */

int
assfail(a, f, l)
	register char *a;
	register char *f;
	int l;
{
	/*
	 * Again, force message to go to console, not processes.
	 */
	conslog_set(CONSLOG_DIS);

#ifdef DEBUG
	if (aask)  {
		cmn_err(CE_NOTE, "ASSERTION CAUGHT: %s, file: %s, line: %d",
		a, f, l);
		call_demon();
	}
	if (aok)
		return 0;	
#endif

	cmn_err(CE_PANIC, "assertion failed: %s, file: %s, line: %d", a, f, l);
	/* NOTREACHED */

}

void
nomemmsg(func, count, contflg, lockflg)
	register char	*func;
	register int	count;
	register int	contflg;
	register int	lockflg;
{
	cmn_err(CE_NOTE,
		"%s - Insufficient memory to%s %d%s page%s - %s",
		func, (lockflg ? " lock" : " allocate"),
		count, (contflg ? " contiguous" : ""),
		(count == 1 ? "" : "s"),
		"system call failed");
}

#ifdef DEBUG
sysin() {}
sysout() {}
sysok() {}
sysoops() {}

/* 
 ** set a breakpoint here to get back to demon at regular intervals.
 */
catchmenow() {}

printputbuf()
{
	register int		i;
	register int		cc;
	register int		opl;

	opl = splhi();
	i = putbufrpos;
	while (i != putbufwpos) {
		if (i < putbufsz) {
			if ((cc = putbuf[i]) != 0) {
				putchar(cc);
				if (cc == '\n')
					dodmddelay();
			}
			i++;
		} else {
			i = 0;
		}
	}
	splx(opl);
}

#endif


/*
** Print a register dump at most once in a panic.
** Record values of the registers for crash-dump analysis.
** Waits for slower terminals to catch up.
*/
STATIC int *snap_regptr;

snap(r0ptr,title)
int *r0ptr;
char *title;
{
	int i;
	int x;
	
	if (snap_regptr)
		return;
	snap_regptr = r0ptr;

	printf("%s:\n",title);
	printf("cr0 0x%.8x     cr2  0x%.8x     cr3 0x%.8x     tlb  0x%.8x\n",
	       _cr0(), _cr2(), _cr3(), querytlb() );
	if (cpu_family == 5)
		printf("cr4 0x%.8x\n", _cr4() );

	drv_usecwait(1000000);
	x = splhi();
	splx(x);
	printf("ss  0x%.8x     uesp 0x%.8x     efl 0x%.8x     ipl  0x%.8x\n",
	       0xffff&r0ptr[SS], r0ptr[UESP], r0ptr[EFL], x);
	drv_usecwait(1000000);
	printf("cs  0x%.8x     eip  0x%.8x     err 0x%.8x     trap 0x%.8x\n",
	       0xffff&r0ptr[CS], r0ptr[EIP], r0ptr[ERR], r0ptr[TRAPNO]);
	drv_usecwait(1000000);
	printf("eax 0x%.8x     ecx  0x%.8x     edx 0x%.8x     ebx  0x%.8x\n",
	       r0ptr[EAX], r0ptr[ECX], r0ptr[EDX], r0ptr[EBX]);
	drv_usecwait(1000000);
	printf("esp 0x%.8x     ebp  0x%.8x     esi 0x%.8x     edi  0x%.8x\n",
	       r0ptr[ESP], r0ptr[EBP], r0ptr[ESI], r0ptr[EDI]);
	drv_usecwait(1000000);
	printf("ds  0x%.8x     es   0x%.8x     fs  0x%.8x     gs   0x%.8x\n",
	       0xffff&r0ptr[DS], 0xffff&r0ptr[ES], 0xffff&r0ptr[FS], 0xffff&r0ptr[GS]);
}

/*
**
**	1) The value read from tr6 on the i386/i486 processors is the
**	same value as written on line 3 (0x00000801h).  The P5 is
**	the first processor to actually update tr6 when performing
**	a TLB lookup.
**
**	The return value of the modified function corresponds to the following
**	diagram:
**	+---------------------------------------------------------+
**	|physical address |PCD|PWT|LRU2|LRU1|LRU0|0|0|PL|REP|PS|CD|
**	+---------------------------------------------------------+ 
**  bits:       20          1   1   1    1    1   1 1  1  2   1  1
**
**	The PCD,PWT,LRU, PS and CD bits are zero on the i386 processor.  The
**	PS and CD bits are zero on the i486 processor.
**
**
**
**
** Use the test registers to poke at the tlb
** and guess at where cr2 is really.
*/
querytlb()
{
	asm("movl	%cr2,%eax");
	asm("andl	$0xfffff000,%eax");
	asm("orl	$0x00000801,%eax");
	asm("cmpl	$5,cpu_family");
	asm("jz		P5_lookup");

	/* Do the lookup for i386, i486 cpus */
	asm("movl	%eax,%tr6");
	asm("movl	%tr7,%eax");
	asm("cmpl	$4, cpu_family");
	asm("jz		mask_486");
	asm("andl	$0xfffff01c, %eax");	/* mask i386 cpu defined bits */
	asm("jmp	tlbvalid");

	/* Mask i486 cpu bits */
	asm("mask_486:");
	asm("andl	$0xffffff9c,%eax");	/* mask i486 cpu defined bits */
	asm("jmp	tlbvalid");

	/* Do the lookup for P5 */
	asm("P5_lookup:");
	asm("pushl	%ecx");			/* save ecx value */
	asm("movl	$8,%ecx");		/* model spec. register tr6 */
	asm("movl	$0,%edx");		/* clear upper 32 bits */
	asm("	wrmsr ");			/* write to tr6 */
	asm("	rdmsr ");			/* read tr6 value */
	asm("andl	$0x00000006,%eax");	/* mask P5 defined bits:  */
						/* PS - page size */
						/* CD - cache (data vs code) */
	asm("shrl	$1,%eax");		/* shift these 2 bits over */
	asm("pushl	%eax");			/* save this tr6 result on stack */
	asm("incl	%ecx");			/* model spec. register tr7 */
	asm("rdmsr");				/* read tr7 */
	asm("popl	%edx");			/* get tr6 result in edx */
	asm("popl	%ecx");			/* get saved ecx off stack */
	asm("orl	%edx,%eax");		/* 'or' two results together */
	asm("andl	$0xffffff9f,%eax");	/* mask off any undefined bits */
	asm("tlbvalid:");
}

