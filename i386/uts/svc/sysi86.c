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

#ident	"@(#)uts-x86:svc/sysi86.c	1.14"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <acc/priv/privilege.h>
#include <mem/immu.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <svc/systm.h>
#include <svc/sysi86.h>
#include <svc/utsname.h>
#include <util/sysmacros.h>
#include <svc/uadmin.h>
#include <util/map.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/swap.h>
#include <util/var.h>
#include <mem/tuneable.h>
#include <io/rtc/rtc.h>
#include <util/fp.h>
#include <mem/seg.h>
#include <svc/systm.h>
#include <io/ioctl.h>  /* for ioctl_arg definition */
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <fs/fstyp.h>
#include <io/uio.h>
#include <proc/mman.h>
#include <svc/bootinfo.h>	/* for ID byte */
#include <mem/seg.h>
#include <mem/vm_hat.h>
#include <mem/as.h>
#include <proc/cred.h>
#include <acc/mac/mac.h>
#include <io/mkdev.h>
#include <svc/sysenvmt.h>	/* for BUS type */

#include <util/xdebug.h>
#ifdef WEITEK
#include <util/weitek.h>
#endif

#ifdef KPERF
#include <proc/disp.h>

int kpftraceflg;
STATIC int kpchildslp;
#endif /* KPERF */
#ifdef MERGE386
#include <mem/vmparam.h>
#endif

#if DEBUG
#define DEBUGF(x) printf x
/* usage DEBUGF(("format",arg[,arg ...]))  */
#else
#define DEBUGF(x)
#endif /* DEBUG */

extern int physmem;                     /* size of memory in clicks     */
time_t localtime_cor = 0;		/* Local time correction in secs */
int    dbug;				/* Some debug control */

#ifdef VPIX
int	vpixenable = 1;			/* Flags to enable or disable VP/ix */
#endif /* VPIX */
#ifdef MERGE386
int	merge386enable = 0;		/* and Merge386 for runtime checking */
#endif /* MERGE386 */

/*
 *      sysi86 System Call
 */

struct sysi86 {
	int	cmd;
	int	arg1;
	int	arg2;
	int	arg3;
};
struct sysi86a {
	short    cmd;           /* these can be just about anything */
	union ioctl_arg arg;    /* use this for all new commands!   */
	long     arg2, arg3;    /* backward compatability           */
};
#define arg1 arg.larg

#ifdef KPERF
asm int 
geteip()
{
	leal 0(%esp), %eax
}
#endif /* KPERF */

int
sysi86(uap, rvp)
	register struct sysi86a *uap;
	rval_t *rvp;
{
	int		error = 0;
	register int	idx;
	register int	c;
	struct rtc_t	clkx;
	char		sysnamex[sizeof(utsname.sysname)];
	extern void	settime();
	struct bcd_tm {
		unsigned char unit_sec;
		unsigned char ten_sec;
		unsigned char unit_min;
		unsigned char ten_min;
		unsigned char unit_hr;
		unsigned char ten_hr;
		unsigned char unit_day;
		unsigned char ten_day;
		unsigned char unit_mon;
		unsigned char ten_mon;
		unsigned char unit_yr;
		unsigned char ten_yr;
		unsigned char llyr;
	};

	switch ((short)uap->cmd) {
#ifdef DEBUG
	case 23:
                switch ((short)uap->arg1) {
                        case 0:
                                break;
                        case 1:
                                dbug = uap->arg2;
                                break;
                        default:
                                error = EINVAL;
                }
                rvp->r_val1 = dbug;
                break;
#endif

	case SI86FSF:
	{

		switch (uap->arg1) {
		case 0:
		case 1:
			fsflush_control(0,uap->arg1);
			break;

		default:
			error = EINVAL;
		}
		break;
	}

	/*
	 * Return the real console device number
	 */
	case SI86GCON:
	{
		dev_t console_dev = makedevice(5,0);

		if(copyout((caddr_t)&console_dev,(caddr_t)uap->arg1,
			   sizeof(dev_t)))
			error = EFAULT;
		break;
	}

	/*
 	 * Copy a string from a given kernel address to a user buffer. Uses
	 * copyout() for each byte to trap bad kernel addresses while watching
	 * for the null terminator.
	 */
	case SI86KSTR :
	{
		register char	*src;
		register char	*dst;
		register char	*dstlim;

		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		src = uap->arg.cparg;                   /*????*/
		dstlim = (dst = (char *) uap->arg2) + (unsigned int) uap->arg3;
		do {
			if (dst == dstlim || copyout(src, dst++, 1) == -1) {
				error = EFAULT;
				break;
			}
		} while (*src++);
		break;
	}

	case RTODC:	/* read TODC */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (rtodc(&clkx))
			error = ENXIO;
		else
		if (copyout((caddr_t) &clkx, (caddr_t) uap->arg1, sizeof(clkx)))
			error = EFAULT;
		break;

	case STIME:	/* set internal time, not hardware clock */
		if (pm_denied(u.u_cred, P_SYSOPS))
			error = EPERM;
		else
			settime((time_t)uap->arg.larg);
		break;
 
	case SETNAME:	/* rename the system */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		for (idx = 0;
		  (c = fubyte((caddr_t) uap->arg.cparg + idx)) > 0
		    && idx < sizeof(sysnamex) - 1;
		  ++idx)
			sysnamex[idx] = c;
		if (c) {
			error = c < 0 ? EFAULT : EINVAL;
			break;
		}
		sysnamex[idx] = '\0';
		for ( idx = 0; idx < sizeof(sysnamex); idx++) 
		      utsname.sysname[idx] = utsname.nodename[idx] = sysnamex[idx];

		/* XENIX Support */
		for ( idx = 0; idx < sizeof(xutsname.sysname); idx++) 
		      xutsname.sysname[idx] = xutsname.nodename[idx] = sysnamex[idx];
		/* End XENIX Support */
		break;

	/*  return the size of memory */
	case SI86MEM:
		rvp->r_val1 = ctob( physmem );
		break;


	/*	This function is just here for compatibility.
	**      It is really just a part of SI86SWPI.  This
	**	new function should be used for all new
	**	applications.
	*/

	case SI86SWAP:
	{
		swpi_t	swpbuf;

		swpbuf.si_cmd   = SI_ADD;
		swpbuf.si_buf   = uap->arg.cparg;
		swpbuf.si_swplo = uap->arg2;
		swpbuf.si_nblks = uap->arg3;
		swapfunc(&swpbuf);
		break;
	}

	/*	General interface for adding, deleting, or
	**	finding out about swap files.  See swap.h
	**	for a description of the argument.
	**              sysi86(SI86SWPI, arg_ptr);
	**/

	case SI86SWPI:
	{
		swpi_t	swpbuf;

		if ((copyin(uap->arg.cparg, (caddr_t)&swpbuf, sizeof(swpi_t)) ) == -1)
			error = EFAULT;
		else
			swapfunc(&swpbuf);
		break;
	}

	/*      Transfer to software  debugger.  Debug levels
	**	may be set there and then we can return to run a
	**	test.
	*/

	case SI86TODEMON : {
		if (pm_denied(u.u_cred, P_SYSOPS))
			error = EPERM;
		else if (!si86_call_demon())
			error = EINVAL;
		break;
	}

	/*
	**      Tell a user what kind of Floating Point support we have.
	**      fp_kind (defined in fp.h) is returned in the low-order byte.
	**      If Weitek support is included, weitek_type (defined in
	**      weitek.h) is returned in the second byte.
	*/

	case SI86FPHW:
		c = fp_kind & 0xFF;
#ifdef WEITEK
		c |= ((weitek_kind & 0xFF) << 8);
#endif
		if ( suword( uap->arg.iparg, c )  == -1)
			error = EFAULT;
		break;

	/*
	 * Set a segment descriptor
	 */
	case SI86DSCR:
		error = setdscr(uap->arg.iparg);
		break;

	/*
	 * Read a user process u-block.
	 * XXX this interface should be moved someplace else.
	 */
	case RDUBLK:
	{
		register struct proc *p;
		caddr_t addr;
		int ocount;
		struct uio uio;
		struct iovec iov;

		if (uap->arg3 < 0) {
			error = EINVAL;
			break;
		}		
		if ((p = prfind(uap->arg1)) == NULL) {
			error = ESRCH;
			break;
		}

		/* check MAC access */
		if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid, p->p_cred->cr_lid)
		&&  pm_denied(u.u_cred, P_MACREAD)
		&&  pm_denied(u.u_cred, P_COMPAT)) {
			error = ESRCH;
			break;
		}

		if (p->p_stat == 0 || p->p_stat == SIDL
		  || p->p_stat == SZOMB) {
			error = EINVAL;
			break;
		}
		ocount = min(uap->arg3, ctob(USIZE));
		iov.iov_base = (caddr_t) uap->arg2;
		iov.iov_len = uio.uio_resid = ocount;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_fmode = 0;
		uio.uio_segflg = UIO_USERSPACE;

		addr = (caddr_t)KUSER(p->p_segu);
		error = uiomove(addr, uio.uio_resid, UIO_READ, &uio);
		rvp->r_val1 = ocount - uio.uio_resid;
		break;
	}

#if     defined(VPIX)
	/* The SI86V86 subsystem call of the SYSI86 system call has     */
	/* sub system calls defined for it v86.h. The SI86V86 call      */
	/* is processed in the module v86.c.                            */

	case SI86V86: {
		v86syscall();                   /* Process V86 system call */
		break;
		}
#endif /* VPIX */

	/*
	**  Set the local time correction in secs (includes Daylight savings)
	*/

	case SI86SLTIME:
		if (!pm_denied(u.u_cred, P_SYSOPS))
			localtime_cor = (time_t)(uap->arg1);
		else
			error = EPERM;
		break;

	/*
	 * NFA system entry hook
	 * nfa_sys finds its own way around the u struct. added for 
	 * opennet nfa
	 */
	case SI86NFA:
		nfa_sys(uap,rvp);
		break;

	/* XENIX Support */
	case SI86BADVISE:

		/*
	 	 * Specify XENIX variant behavior.
	 	 */
		switch(uap->arg.iarg & 0xFF00) {
			case SI86B_GET:
				/* Return badvise bits */
				if (BADVISE_PRE_SV)
					rvp->r_val1 |= SI86B_PRE_SV;
				if (BADVISE_XOUT)
					rvp->r_val1 |= SI86B_XOUT;
				if (BADVISE_XSDSWTCH)
					rvp->r_val1 |= SI86B_XSDSWTCH;
				break;
			case SI86B_SET:
				/* Set badvise bits.
				 * We assume that if pre-System V behavior
				 * is specified, then the x.out behavior is
				 * implied (i.e., the caller gets the same
				 * behavior by specifying SI86B_PRE_SV alone
				 * as they get by specifying both SI86B_PRE_SV
				 * and SI86B_XOUT).
				 */ 
				if (uap->arg.iarg & SI86B_PRE_SV) 
					u.u_renv |= (UB_PRE_SV | UB_XOUT);
				else
				if (uap->arg.iarg & SI86B_XOUT) {
					u.u_renv |= UB_XOUT;
					u.u_renv &= ~UB_PRE_SV;
				}
				else
					u.u_renv &= ~(UB_PRE_SV | UB_XOUT);	
				if (uap->arg.iarg & SI86B_XSDSWTCH) {
					/* copy the "real" sd to the 286 copy */
					xsdswtch(1);
					u.u_renv |= UB_XSDSWTCH;
				}
				else {
					/* copy the 286 sd to the "real" copy */
					xsdswtch(0);
					u.u_renv &= ~UB_XSDSWTCH;
				}
				break;
			default:
				error = EINVAL;
				break;
		}
		break;
	case SI86SHRGN:
	{

		extern	xsd86shrgn();
		/*
		 * Enable/disable XENIX small model shared data context    
		 * switching support.  The 'cparg' argument is a pointer   
		 * to an xsdbuf struct which contains the 386 start addr
		 * for the sd seg and the 286 start addr for the sd seg.
		 * When a proc that has requested shared data copying (via
		 * SI86BADVISE) is switched
		 * to, the kernel copies the sd seg from the 386 addr to 
		 * the 286 addr.  When the proc is switched from, the kernel 
		 * copies the sd seg from the 286 addr to the 386 addr. 
		 * Note that if the 286 addr is NULL, the   
		 * shared data segment's context switching support is 
		 * disabled.
		 */


		xsd86shrgn(uap->arg.cparg);
		break;
	}
	case SI86SHFIL:
		error = mapfile(uap->arg.sparg, rvp);
		break;

	case SI86PCHRGN:
		error = chmfile(uap->arg.sparg, rvp);
		break;

	case SI86CHIDT:
		error = error = chidt(uap->arg.cparg);
		break;

	/* End XENIX Support */

	/* remove 286 emulator special read access */
	case SI86EMULRDA:
		u.u_renv &= ~UE_EMUL;
		break;

#ifdef MERGE386
	/*
	**  VM86 command for Merge 386 functions 
	*/
	case SI86VM86:
		if (merge386enable)
			error = vm86(uap->arg.iparg, u.u_cred, rvp);
		else
			error = EINVAL;
		break;
#endif /* MERGE386 */
#if MERGE386 || VPIX
	case SI86VMENABLE:
		if (pm_denied(u.u_cred, P_SYSOPS)) { /* not sure if SYSOPS is correct */
			error = EPERM;
			break;
		}
		switch (uap->arg1) {
		case 0:
#ifdef VPIX
			vpixenable = 1;
#endif /* VPIX */
#ifdef MERGE386
			merge386enable = 0;
#endif /* MERGE386 */
			break;
		case 1:
#ifdef VPIX
			vpixenable = 0;
#endif /* VPIX */
#ifdef MERGE386
			merge386enable = 1;
#endif /* MERGE386 */
			break;
		default:
			error = EINVAL;
			break;
		}
		break;
#endif	/* MERGE386 || VPIX */

#ifdef	KPERF
	/* synchronization between parent and child  
	*/
	case KPFCHILDSLP:
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (kpchildslp == 0)
			sleep((caddr_t) &kpchildslp, PPIPE);
		break;

	/*	Turn the kernel performance measurement code on 
	*/

	case KPFTRON:
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		takephase = 0;
		putphase = 0;
		numrccount = 0;
		outbuf = 0;
		pre_trace = 1;
		kpftraceflg = 0;
		kpchildslp = 0;
		/* DEBUG */
		break;

	/*	Wait for a buffer of kernel perf statistics to fill, and return
	**	the data to the user.  First record is number of records
	**	to be returned (maximum is NUMRC).
	**	Usage: sysi86( KPFTRON2, &buffer, sizeof(buffer))
	**	the following logic is used:
	**	1. kpftraceflg on  A. takephase = putphase, sleep waiting for a 
	**			   buffer to fill
	**			B. takephase !=putphase, go copy records
	**			   and takephase = takephase +1 %NUMPHASE
	** 	always check for abnormal conditions
	**      2. kpftraceflg off A. takephase = putphase, copy numrccount
	**			   of records, ( buffer may not be full)
	**			B. takephase != putphase , go copy records
	**			   and takephase = takephase +1 % NUMPHASE
	*/

	case KPFTRON2: 
		{
		register int *buffer = (int*)(uap->arg1);
		register int size = uap->arg2;
		int *dataddr;
		int numrc;


		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}


		if ( size != (1+NUMRC)*sizeof(kernperf_t) ) {
			/* buffer too small even for size */
			cmn_err(CE_CONT, "sysi86, kpftron2, Buffer too small\n");
			u.u_error = EINVAL;
			break;
		}

		numrc = NUMRC;
		dataddr = (int*)&kpft[takephase*NUMRC];
		if (kpftraceflg == 1)  
			if (takephase == putphase )  {
				kpchildslp = 1;
				wakeup((caddr_t) &kpchildslp);
				sleep((caddr_t) &kpft[takephase*NUMRC],PPIPE);
			}
		 /* full buffer i.e. abnormal termination */
		if (outbuf == 1) {
			u.u_error = EINVAL;
			break;
		}
	
		/* tracing is off, here under normal conditions */
		if ((kpftraceflg  == 0 ) && (takephase == putphase))
			numrc = numrccount;
		copyrecords(dataddr,buffer,numrc);
		takephase = (takephase + 1) % NUMPHASE;
		break;
		}

	/*	Turn the kernel performance measurement off.
	*/

	case KPFTROFF: 
	{
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		Kpc = geteip();
		kperf_write(KPT_END,Kpc,curproc);
		pre_trace = 0;
		kpftraceflg = 0;
		wakeup((caddr_t) &kpft[takephase*NUMRC]);
		kpchildslp = 0;
		break;
	}
#endif	/* KPERF */

	case SI86LIMUSER:
		switch (uap->arg1) {
			case EUA_GET_LIM:
			case EUA_GET_CUR:
				break;
			case EUA_ADD_USER:
			case EUA_UUCP:
				if (pm_denied(u.u_cred, P_SYSOPS))
					return(EPERM);
				break;
			default:
				return EINVAL;
		}
		rvp->r_val1 = enable_user_alloc(uap->arg1);
		break;


	case SI86RDID:	/* Read the ROM BIOS ID bit */
		if ((bootinfo.id[0] == 'I') && (bootinfo.id[1] == 'D') &&
		    (bootinfo.id[2] == 'N') && (bootinfo.id[3] == 'O'))
			rvp->r_val1 = bootinfo.id[4];
		else
			rvp->r_val1 = 0;
		break;

	case SI86RDBOOT: /* Bootable Non-SCSI Hard Disk? */
		if (bootinfo.hdparams[0].hdp_ncyl == 0)
			rvp->r_val1 = 0;
		else
			rvp->r_val1 = 1;
		break;

	case SI86BUSTYPE: /* what bus type is this machine */
		rvp->r_val1 = EISA_BUS;
		if (sysenvmtp->machflags&MC_BUS)
			rvp->r_val1 = MCA_BUS;
		if (sysenvmtp->machflags&AT_BUS)
			rvp->r_val1 = ISA_BUS;
		break;

	default:
		error = EINVAL;
	}
	return(error);
}

#ifndef NODEBUGGER
int demon_call_type = -1;
	/*
	 * The demon_call_type flag is checked by the kernel trap code
	 * for an Int 3 (breakpoint) trap; if it's not -1, then it's
	 * used as the type code for a debugger call, and the return EIP
	 * is adjusted past the Int 3 instruction.
	 * Referenced only from os/trap.c .
	 */
#endif

si86_call_demon()
{
#ifndef	NODEBUGGER
	extern void (*cdebugger)(), nullsys();

	if (cdebugger != nullsys) {
		/*
		 * Set a flag and generate a trap into the debugger.
		 * This is done, rather than calling the debugger directly,
		 * to get a trap frame saved.
		 */
		demon_call_type = DR_SECURE_USER;
		asm(" int $3");	/* Force a debug trap */
		return(1);
	}
#endif
	DEBUGF(("Debugger not installed\n"));
	return(0);
}

void
call_demon()
{
#ifndef NODEBUGGER
	if (cdebugger != nullsys) {
		/*
		 * Set a flag and generate a trap into the debugger.
		 * This is done, rather than calling the debugger directly,
		 * to get a trap frame saved.
		 */
		demon_call_type = DR_OTHER;
		asm(" int $3");	/* Force a debug trap */
	}
#endif
}


/*
 *  SI86CHIDT:
 *  Revector int 0xf0 to int 0xff to a user routine.  In xenix 286
 *  binaries, these precede each floating point instruction.
 */
int
chidt(fun)
char *fun;
{
	register struct gdscr * idte;
	extern struct gdscr def_intf0;
	extern struct gate_desc idt[];

	idte = (struct gdscr *) u.u_fpintgate;

	if (fun == (char *) 0) {
		*idte = def_intf0;
	}
	/*
	 * verify the validity of the address in user space.  For now,
	 * we ignore the type of segment: text, data, ....
	 */
	else if ((as_segat(u.u_procp->p_as, fun)) != NULL) {
		idte->gd_off0015 = (ushort) fun;
		idte->gd_selector = (short) USER_CS;
		idte->gd_unused = 0;
		idte->gd_acc0007 = GATE_UACC|GATE_386TRP;
		idte->gd_off1631 = ((int) fun >> 16) & 0xFFFF;
	}
	else {
		return(EFAULT);
	}

	/*
	 * since int 0xf0, ... are embedded in user text,
	 * and occur synchronously, this is ok.
	 */
	idte = (struct gdscr *) idt + 0xf0;
	for (; idte <= (struct gdscr *) idt + 0xff; idte++) {
		*idte = * (struct gdscr *) u.u_fpintgate;
	}
	return(0);
}
/* #endif */


#ifdef	KPERF
/*	Copy kernel perf statistics from kernel buffer to
**	user buffer.  First record is number of records
**	copied.
*/

copyrecords(dataddr,buffer,numrcx)
int *dataddr, *buffer, numrcx;
{
	register int datalen;
	kernperf_t *bufx;

	datalen = numrcx * sizeof( kernperf_t);
	if (suword(buffer,numrcx) == -1) {
		u.u_error = EFAULT;
		return;
	}

	bufx = ( kernperf_t *) buffer;
	if ((copyout(dataddr,bufx+1,datalen)) == -1)
		u.u_error = EFAULT;
}
#endif	/* KPERF */

/*
 *  SI86DSCR:
 *  Set a segment or gate descriptor.
 *  The following are accepted:
 *      executable and data segments in the LDT at DPL 3
 *      a call gate in the GDT at DPL 3 that points to a segment in the LDT
 *  The request structure is declared in sysi86.h.
 *  MERGE386 added the following support for Standard Mode Windows. S008
 *	-executable and data segments in the LDT at DPL 1,2,3
 *	-limit checking on the descriptors since page protection is gone if
 *	user runs in ring < 3.
*/

/* call gate structure */
struct cg {
	unsigned short off1; /* low order word of offset */
	unsigned short sel;  /* descriptor selector */
	unsigned char  cnt;  /* word count */
	unsigned char  acc1; /* access byte */
	unsigned short off2; /* high order word of offset */
};

extern struct seg_desc gdt[];

int
setdscr(ap)
int *ap;
{
	struct ssd ssd;         /* request structure buffer */
	u_short seli;  		/* selector index */
	struct dscr *dscrp;     /* descriptor pointer */
	struct cg *cgp;         /* call gate pointer */
	unsigned int	newsz;

	if ((copyin((caddr_t)ap, (caddr_t)&ssd, sizeof(struct ssd)) ) < 0 ) {
		return(EFAULT);
	}

	/* LDT segments: executable and data at DPL 3 only */

	if (ssd.sel & 4) {              /* test TI bit */
		/* check the selector index */
		seli = seltoi(ssd.sel);
		if ((seli <= (u_short)seltoi(USER_DS)) || (seli >= MAXLDTSZ))
			goto bad;
		if ((u_int)seli > u.u_ldtlimit) {
			newsz = btoc(u.u_procp->p_ldt - (addr_t)PTOU(u.u_procp) + (seli + 1) * sizeof(struct dscr));
		     	if (!segu_expand(newsz)) {
				return(ENOMEM);
			}
			u.u_ldtlimit = min(MAXLDTSZ,
				(ctob(u.u_procp->p_usize) -
				 (u.u_procp->p_ldt - (addr_t)PTOU(u.u_procp))) /
						sizeof(struct seg_desc)) - 1;

			setdscrlim(&u.u_ldt_desc,
				(u.u_ldtlimit+1)*sizeof(struct seg_desc) - 1);
			gdt[seltoi(LDTSEL)] = u.u_ldt_desc;

			loadldt(LDTSEL);
		}
		ASSERT(seli <= u.u_ldtlimit);
		dscrp = (struct dscr *)(u.u_procp->p_ldt) + seli;
		/* if acc1 is zero, clear the descriptor */
		if (! ssd.acc1) {
			((unsigned int *)dscrp)[0] = 0;
			((unsigned int *)dscrp)[1] = 0;
			return(0);
		}
		/* check segment type */
#ifdef MERGE386
		if ((ssd.acc1 & 0xF0) == 0x90) /* Don't allow ring 0 */
#else
		if ((ssd.acc1 & 0xF0) != 0xF0)
#endif
			goto bad;
#ifdef MERGE386						/* S008 */
		/*
		  The cases are combinations of normal and expand down
		  with the G, granularity bit 0 or 1.
		  Check G bit, adjust limit, then do check on up/down.
		  Expand down are segment types 4-7, 01xxb in acc1.
		  All quantities are unsigned. The upper address limit and
		  the lower address limit both need to be less than the top
		  of the user's virtual address space and upper>lower.  
		  The upper limit of an expand down segment is the 
		  base + maximum segment size.
		*/
		#define	GRAN	(1<<3)		/* granularity */
		#define	DFLT	(1<<2)		/* default size */
		{ /* brace, code not indented */
		unsigned long bottom, top;

		if ((ssd.acc1 & 0x1c) == 0x14) { 
			/* expand down */
			bottom = ssd.bo + 1 + ((ssd.acc2 & GRAN) ? 
				(ssd.ls<<12) : ssd.ls);
			top = ssd.bo + ((ssd.acc2 & DFLT) ?
					0xffffffff : 0xffff);
		} else { 
			/* expand up */
			bottom = ssd.bo;
			top = ssd.bo + ((ssd.acc2 & GRAN) ?
				(ssd.ls<<12) | 0xfff : ssd.ls);
		}
		if (bottom > top || top >= MAXUVADR)
			goto bad;

		} /* brace, code not indented */
#endif
		/* set up the descriptor */
		setdscrbase(dscrp, ssd.bo);
		setdscrlim(dscrp, ssd.ls);
		setdscracc1(dscrp, ssd.acc1);
		setdscracc2(dscrp, ssd.acc2);
		/* flag the process as having a modified LDT */
		u.u_ldtmodified = 1;
	}

	/* GDT segment: call gate into LDT at DPL 3 only */

	else {
		seli = seltoi(ssd.sel);
		if (seli <= 25 || seli >= GDTSZ)
			goto bad;
		switch(seli) {
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 50:
			goto bad;
		default:
			break;
		}

		/* if acc1 is zero, clear the descriptor and U-struct */
		cgp = (struct cg *)gdt + seli;
		if (! ssd.acc1) {
			((unsigned int *)cgp)[0] = 0;
			((unsigned int *)cgp)[1] = 0;
			u.u_callgatep = 0;
			u.u_callgate[0] = 0;
			u.u_callgate[1] = 0;
			return(0);
		}

		/* check that a call gate does not already exist */
		if (u.u_callgatep != 0)
			goto bad;

		/* check that call gate points to an LDT descriptor */
		if (((ssd.acc1 & 0xF7) != 0xE4) || (! (ssd.ls & 4)))           /* LDT */
			goto bad;

		cgp->off1 = ssd.bo;
		cgp->sel  = ssd.ls;
		cgp->cnt  = ssd.acc2;
		cgp->acc1 = ssd.acc1;
		cgp->off2 = ((unsigned short *)&ssd.bo)[1];

		/* copy call gate and its pointer into the user structure */
		u.u_callgatep = (int *)cgp;
		u.u_callgate[0] = ((int *)cgp)[0];
		u.u_callgate[1] = ((int *)cgp)[1];
	}
	return(0);

bad:
	return(EINVAL);
}

