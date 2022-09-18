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

#ident	"@(#)uts-x86:svc/main.c	1.20"
#ident	"$Header: $"

#include <fs/vnode.h>
#include <io/asyncsys.h>
#include <mem/as.h>
#include <mem/bootconf.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/seg_dummy.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <mem/vmsystm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/fp.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef WEITEK
#include <util/weitek.h>
#endif

/* well known processes */
proc_t *proc_sched;		/* memory scheduler */
proc_t *proc_init;		/* init */
proc_t *proc_pageout;		/* pageout daemon */
proc_t *proc_bdflush;		/* buffer cache flush daemon */
proc_t *proc_sd01dkd;		/* disk bad block handler daemon */
proc_t *proc_adtflush;		/* audit flush daemon */

vnode_t	*rootdir;
long	dudebug;

extern int	physmem;	/* physical mem in clicks (from startup.c) */
extern int	icode[];
extern int	szicode;
extern int	userstack[];
extern int	sanity_clk;
int		eisa_bus = 0;
unsigned int	eisa_brd_id = 0;

extern int	checksumOK;

/* workarounds for 80386 B1 stepping bugs */
extern int	do386b1_x87;	/* used for errata #10 (387 section) */
extern paddr_t	fp387cr3;	/* (errata #21) */

extern struct tss386 ktss, dftss;

extern struct bootobj swapfile;
extern struct bootobj dumpfile;
daddr_t nswap;  /* set to size of swapdev by driver when rootdev is opened */


/*
 * Initialization code.
 * fork - process 0 to schedule
 *      - process 1 execute bootstrap
 *
 * loop at low address in user mode -- /sbin/init
 * cannot be executed.
 */

main(argc, argv)
int	argc;
char	*argv[];
{
	register int	(**initptr)();
	struct		vnode *swapvp;
	extern void	pm_init();
	extern struct vnode *makespecvp();
	register int    i;
	extern int	(*io_init[])();
	extern int	(*init_tbl[])();
	extern int	(*io_start[])();
	extern int	sched();
	extern int	pageout();
	extern int	fsflush();
	extern int	sd01dkd();
	extern int	kmem_freepool();
	int		error = 0;
	extern void 	modinit();
#ifdef ATT_EISA
	char		id;
	unsigned char	byte;
	char		portval = (char)0xFF;
	int		eisa_port = 0xC80;
	int		eisa_id_port = 0xC82;
#endif

	inituname();
 	cred_init();
 	dnlc_init();

	/*
	 * Set up credentials.
	 */
	sys_cred = crget();

	/*
	 * set the credential structure for this process to
	 * the system credential structure.
	 */
	u.u_cred = sys_cred;
	crhold(sys_cred);	/* increment the reference count on sys_cred */

	/* Now finish the 80386 B1 stepping (errata #21) workaround */
	u.u_tss->t_cr3 |= fp387cr3;
	ktss.t_cr3 |= fp387cr3;
	dftss.t_cr3 |= fp387cr3;
	prfconfig(PRW_CONS);

#ifdef ATT_EISA
	/* set eisa_bus variable before driver inits */
	if ((bootinfo.id[0] == 'I') && (bootinfo.id[1] == 'D') &&
           (bootinfo.id[2] == 'N') && (bootinfo.id[3] == 'O'))
		id = bootinfo.id[4];

	if ((id == C2) || (id == C3) || (id == C4))
		eisa_bus = 0;
	else {
              	outb(eisa_port, portval); 
		portval = inb(eisa_port);
		if (portval == (char)0xFF)
                        eisa_bus = 0;
		else {
                    	eisa_bus = 1;
			byte = inb(eisa_id_port); /* grab 0xc82 */
			eisa_brd_id = byte;
			byte = inb(eisa_id_port + 1); /* get 0xc83 */
			eisa_brd_id += byte << 8;
		}
	}
#else
	eisa_bus = 0;
#endif

	/*
	 * general hook for things which need to run prior to turning
	 * on interrupts.
	 */
	oem_pre_clk_init();

	clkstart();

	cmn_err(CE_CONT, "^\n");	/* Need a newline for alternate console */
	cmn_err(CE_CONT, "!total real memory        = %d\n", ctob(physmem));
	cmn_err(CE_CONT, "!total available memory   = %d\n\n", ctob(epages - pages));


	cmn_err(CE_CONT, "USL UNIX System V Release %s Version %s for the Intel386 Family\n",
					utsname.release, utsname.version);
	cmn_err(CE_CONT, "\nCopyright (c) 1992 UNIX System Laboratories, Inc.\n");
	cmn_err(CE_CONT, "All Rights Reserved\n\n");
	cmn_err(CE_CONT, "The system is coming up.  Please wait.\n\n");

	/* was the bootinfo structure OK when recieved from boot? */

	if ( checksumOK != 1 )
		cmn_err(CE_CONT, "Bootinfo checksum incorrect.\n\n");

	/* Assert: (kernel stack + floating point stuff) is equal to 1 page.
	 */

	if (((char *)&u + NBPP) != (char *) &u.u_tss)
		cmn_err(CE_PANIC,"main: Invalid Kernel Stack Size in U block\n");

 	/*
 	 * Call all system initialization functions.
 	 */

 	for (initptr= &io_init[0]; *initptr; initptr++)
		(**initptr)();

	picinit();      /* initialize PICs, but do not enable interrupts */

	spl0();         /* enable interrupts */

#ifdef ATT_EISA
        if (eisa_bus && sanity_clk)
		sanity_init();	/* start up sanity clock */
#endif

 	for (initptr= &init_tbl[0]; *initptr; initptr++) 
		(**initptr)();
 	for (initptr= &io_start[0]; *initptr; initptr++) 
		(**initptr)();

#ifdef DEBUG
	/*
	 *  Debug only - print the page mapping structure contents for
	 *  all memory chunks.
	*/
	print_pagepool();
#endif
        /*
         * Set the scan rate and other parameters of the paging subsystem.
 	 */

 	setupclock();

 	u.u_error = 0;		/* XXX kludge for SCSI driver */
 	vfs_mountroot();	/* Mount the root file system */

	/*
	 * Initialize the kernel-based privilege table.
	 * This should be done immediately after mounting
	 * the root file system in case anything done after
	 * requires privilege.  If so, it will have it.
	 *
	 * Anything done before this is assumed to not require
	 * privilege to perform its tasks.
	 */
	pm_init();




	u.u_start = hrestime.tv_sec;
 
	/*
	 *  pull in the floating point emulator.
	 */

	fpeinit();

 	/*
	 * This call to swapconf must come after 
	 * root has been mounted.
	 */

	swapfile.bo_size = nswap;
	swapconf();

	if(dumpdev != swapdev){
		dumpfile.bo_vp = (struct vnode *)makespecvp(dumpdev, VBLK);
		VOP_OPEN(&dumpfile.bo_vp, FWRITE, u.u_cred);
	}

	modinit(argv[0]);

	/*
	 * Initialize file descriptor info in uarea.
	 * NB:  getf() in fio.c expects u.u_nofiles >= NFPCHUNK
	 */
	u.u_nofiles = NFPCHUNK;

        schedpaging();
 
	/*
 	 * Make init process; enter scheduling loop with system process.
	 */

	spl0();

 	if (newproc(NP_INIT, NULL, &error)) {
		register struct dscr *ldt, *ldta;
		register struct gate_desc *gldt;
		extern struct gate_desc scall_dscr, sigret_dscr;

 		register proc_t *p = u.u_procp;
		proc_init = p;

		/*
		 * Prevent an ill-timed reboot signal from killing init
		 * before it has a chance to set up its signal dispositions.
		 * The console driver in some implementations will send
		 * such a signal if the user hits CTRL-ALT-DEL.
		 */
		setsigact(SIGFPE, SIG_IGN, (k_sigset_t)0, 0);

		/* 
		 * we will start the user level init
		 * clear the special flags set to get
		 * past the first context switch 
		 */
		p->p_flag &= ~(SSYS | SLOCK);  
 		p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;

 		/*
 		 * Set up the text region to do an exec
 		 * of /sbin/init.  The "icode" is in misc.s.
 		 */

 		/*
 		 * Allocate user address space.
 		 */

 		p->p_as = as_alloc();
		if (p->p_as == NULL) {
			cmn_err(CE_PANIC,"main: as_alloc returned null\n");
		}

 		/*
 		 * Make a text segment for icode
 		 */

 		(void) as_map(p->p_as, UVTEXT,
 		    szicode, segvn_create, zfod_argsp);

 		if (copyout((caddr_t)icode, (caddr_t)(UVTEXT), szicode))
			cmn_err(CE_PANIC, "main - copyout of icode failed");

 		/*
 		 * Allocate a stack segment
 		 */

 		(void) as_map(p->p_as, userstack,
 		    ctob(SSIZE), segvn_create, zfod_argsp);

		/*
		 * 80386 B1 Errata #10 -- reserve page at 0x80000000
		 * to prevent bug from occuring.
		 */

		if (do386b1_x87) {
			(void) as_map(p->p_as, 0x80000000, ctob(1),
					segdummy_create, NULL);
		}

		/*
		 * set up LDT. We use gldt to set up the syscall
		 * and sigret call gates, and ldt/ldta for the
		 * code/data segments.
		 */
		gldt = (struct gate_desc *)u.u_procp->p_ldt;
		gldt[seltoi(USER_SCALL)] = scall_dscr;
		gldt[seltoi(USER_SIGCALL)] = sigret_dscr;

		ldt = (struct dscr *)u.u_procp->p_ldt;
		ldt += seltoi(USER_CS);
		ldt->a_base0015 = 0;
		ldt->a_base1623 = 0;
		ldt->a_base2431 = 0;
		ldt->a_lim0015 = (ushort)btoct(MAXUVADR-1);
		ldt->a_lim1619 = ((unsigned char)(btoct(MAXUVADR-1) >> 16)) & 0x0F;
		ldt->a_acc0007 = UTEXT_ACC1;
		ldt->a_acc0811 = TEXT_ACC2;

		ldta = (struct dscr *)u.u_procp->p_ldt;
		ldta += seltoi(USER_DS);
		*ldta = *ldt;
		ldta->a_acc0007 = UDATA_ACC1;
#ifdef WEITEK
		ldta->a_lim0015 = (ushort)btoct(WEITEK_MAXADDR);
		ldta->a_lim1619 = ((unsigned char)(btoct(WEITEK_MAXADDR) >> 16)) & 0x0F;
#endif
		/*
		 * set up LDT entries for floating point emulation.
		 * 2 entries: one for a 32-bit alias to the user's stack,
		 *   and one for a window into the fp save area in the
		 *   user structure.
		 */
		ldt = (struct dscr *)u.u_procp->p_ldt;
		ldt += seltoi(USER_FP);
		setdscrbase(ldt, &u.u_fpvalid);
		i = (int)(&u.u_fps) - (int)(&u.u_fpvalid) +
						sizeof(u.u_fps);
#ifdef WEITEK
		i += sizeof(u.u_weitek_reg);
#endif
		setdscrlim(ldt, i);
		ldt->a_acc0007 = UDATA_ACC1;
		ldt->a_acc0811 = DATA_ACC2_S;

		ldt = (struct dscr *)u.u_procp->p_ldt;
		ldt += seltoi(USER_FPSTK);
		*ldt = *ldta;

 		return UVTEXT;
	}

 	if (newproc(NP_SYSPROC, NULL, &error)) {
		register proc_t *p = u.u_procp;
		proc_pageout = p;
		p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;
		U_COMM("pageout");
		pageout();
		cmn_err(CE_PANIC, "main: return from pageout()");
	}

 	if (newproc(NP_SYSPROC, NULL, &error)) {
		register proc_t *p = u.u_procp;
		proc_bdflush = p;
		p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;
		U_COMM("fsflush");
		fsflush();
		cmn_err(CE_PANIC, "main: return from fsflush()");
	}

	if (aio_config() && aiodmn_spawn() != 0) {
		aiodaemon();
		cmn_err(CE_PANIC, "main: return from aiodaemon()");
	}

 	if (newproc(NP_SYSPROC, NULL, &error)) {
 		/*
 		 * use "kmdaemon" rather than "kmem_freepool"
 		 * will be more intelligble for ps
 		 */
 		u.u_procp->p_cstime = u.u_procp->p_stime = 0;
		u.u_procp->p_cutime = u.u_procp->p_utime = 0;
		U_COMM("kmdaemon");
		kmem_freepool();
		cmn_err(CE_PANIC, "main: return from kmem_freepool()");
 	}
 
 	if (newproc(NP_SYSPROC, NULL, &error)) {
		register proc_t *p = u.u_procp;
		proc_sd01dkd = p;
		p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;
		U_COMM("sd01dkd");
		sd01dkd();
		cmn_err(CE_PANIC, "main: return from sd01dkd()");
	}

	if ( adt_installed() ) {
		if (newproc(NP_SYSPROC, NULL, &error)) {
			register proc_t *p = u.u_procp;
			proc_adtflush = p;
			p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;
			p->p_cred = crcopy(u.u_cred);
			U_COMM("adtflush");
			adtflush();
			cmn_err(CE_PANIC, "main: return from adtflush()");
		}
	}

	pid_setmin();

	U_COMM("sched");

 	return (int)sched;
}
