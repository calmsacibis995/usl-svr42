/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/boot.c	1.13"
#ident	"$Header: $"

#include "util/types.h"
#include "svc/bootinfo.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "mem/immu.h"
#include "io/cram/cram.h"
#include "svc/bootcntl.h"

#include "boot/boot.h"
#include "boot/bootlink.h"
#include "boot/libfm.h"
#include "boot/sip.h"
#include "boot/error.h"

extern int	printf(), putchar(),
		strlen(), memcmp(), 
		bgets(), ischar(), doint(), shomem(), memwrap();

extern	char *memcpy(), *memset(), *strcat(), *strcpy(), *strncpy(), *strtok();

extern	unchar	getchar(), CMOSread();

extern	off_t	boot_delta;
extern	off_t	root_delta;
extern	bfstyp_t boot_fs_type;
extern	bfstyp_t root_fs_type;

extern	void	goany();
extern	void	update_bootcntl();

/*	Initalize the function pointers for the utility routines
 *	used by the boot supportive programs. Note changes here
 *	must be reflected in the initprog/initprog.h file and in
 *	bootlink.h.
 */

struct	bootfuncs	bf = {
		printf, strcpy, strncpy, strcat, strlen, memcpy, memcmp,
		getchar, putchar,
		bgets, ischar, doint, goany, CMOSread, memset, shomem, memwrap
	};

#define DFLT_BOOT_PATH   "/unix"
#define DFLT_BOOT_MSG    "Booting the UNIX System..."

char 	bootprompt[B_STRSIZ] =	"Enter name of program to boot: ";
char	dflt_sip[B_PATHSIZ] =	"/etc/initprog/sip";
char	dflt_mip[B_PATHSIZ] =	"/etc/initprog/mip";

struct	bootenv	bootenv;
struct	lpcb	lpcb[KERNEL+1];
int		memrng_updated = FALSE;

extern	char	flatdesc;
extern	int	end;
extern  paddr_t	act_part;	/* address of active partition */
extern	paddr_t secboot_mem_loc;
extern	paddr_t entry_ds_si;

extern	short	bps;		/* bytes per sector */
extern	short	spt;		/* disk sectors per track */
extern	short	spc;		/* disk sectors per cylinder */
extern 	int	goprot();
extern 	int	_start();
extern	ushort	bstack;

#ifdef BOOT_DEBUG
uint	dread_cnt=0;
#endif

main()
{
	int	i, t;
	char	bootstring[B_PATHSIZ], dfltbootpath[B_PATHSIZ];
	char	ans[3];
        char    *dest;
        char    *fname;
        char    *bootstr;
	int	status;
	int	cutoff1;
	int	cutoff2;
	struct	bootcntl *btcntlp;
	register struct	lpcb *lpcbp;
	off_t		save_delta;
	bfstyp_t	save_fs_type;

	debug(printf("boot [main()]:\n")); 

	btcntlp = (struct bootcntl *)secboot_mem_loc;
/*	reserved the first argument for kernel pathname			*/
	BTE_INFO.bargc = 1;

	/*
        ** bootflags must be initialized before call to BL_file_init()
        ** because the floppy boot flag will get OR'ed in if needed in
        ** get_fs() which is called by BL_file_init().
        */

        BTE_INFO.bootflags = btcntlp->bc_bootflags;

/* 	initialize the stand alone disk driver 				*/
	BL_file_init();

	switch(boot_fs_type) {

	/* UNKNOWN in case of floppy boot or a hard disk with no BFS */

	case UNKNOWN:
	case s5:

		boot_fs_type = root_fs_type;
		boot_delta = root_delta;

		/* I don't want to break here ! */

	case BFS:

		save_fs_type = boot_fs_type;
		save_delta = boot_delta;
	      	break;

        default:

	        printf("boot.c: Invalid boot file system type");
	      	break;
        }

	/*
	** This restores the use of /stand/boot to the common
	** boot code from ISC.  If /stand/boot (or /etc/initprog/
	** boot) exists, it will be used to override hardcoded
	** values in the bootcntl structure.  If the boot file
	** does NOT exist, the code works the same as always.
	** Restoring this functionality allows developers to alter
	** values such as memory configuration without actually
	** pulling boards from their box.
	*/

        BL_file_open("/etc/initprog/boot", NULL, &status);

	if ( status == E_OK )
		update_bootcntl( btcntlp );

/* 	override the default boot settings from boot control block 	*/
/* 	initialize the boot info block					*/
	bootenv.db_flag = btcntlp->bc_db_flag;

	bootenv.memrngcnt = btcntlp->bc_memrngcnt;
	for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++) 
		bootenv.memrng[i] = btcntlp->bc_memrng[i];

/*	kernel args may have been come from update_bootcntl() above     */
	t = BTE_INFO.bargc;
	BTE_INFO.bargc += btcntlp->bc_argc;

	if ( BTE_INFO.bargc > B_MAXARGS ) {
		i = BTE_INFO.bargc - B_MAXARGS;
		printf( "\nboot: %d too many kernel args in /stand/boot-extras ignored\n", i );
		BTE_INFO.bargc -= i;
		t -= i;
	}

	for (i = 0; i < (int)btcntlp->bc_argc; i++) 
		strcpy(BTE_INFO.bargv[t+i], btcntlp->bc_argv[i]);

	if ((i = strlen(btcntlp->bootmsg)) < B_STRSIZ && i > 0)
		strcpy(bootenv.bootmsg, btcntlp->bootmsg);
	else
		strcpy(bootenv.bootmsg, DFLT_BOOT_MSG);

	if ((i = strlen(btcntlp->bootprompt)) < B_STRSIZ && i > 0)
		strcpy(bootprompt, btcntlp->bootprompt);

	if ((i = strlen(btcntlp->bootstring)) < B_PATHSIZ && i > 0)
		strcpy(dfltbootpath, btcntlp->bootstring);
	else
		strcpy(dfltbootpath, DFLT_BOOT_PATH);

	/*
	** If the boot_fs_type is BFS, then let's cut the "/" off the
	** path so the message "Can't find /unix" says "unix" instead.
	*/

	cutoff1 = boot_fs_type == BFS ? 1 : 0;
	strcpy( bootstring, dfltbootpath + cutoff1 );

	if ((i = strlen(btcntlp->sip)) < B_PATHSIZ && i > 0) {
		strcpy(dflt_sip, btcntlp->sip);
	}

	if ((i = strlen(btcntlp->mip)) < B_PATHSIZ && i > 0) {
		strcpy(dflt_mip, btcntlp->mip);
		strcpy(bootenv.initprog, btcntlp->mip);
	}

	bootenv.timeout  = btcntlp->timeout;
	bootenv.bootsize = (paddr_t) ptround(&end);

/* 	initialize the loadable program control block			*/

	/*
	** for cosmetic reasons, lets chop the "/etc/initprog/" off the
	** path if boot_fs_type == BFS.
	*/

	cutoff2 = boot_fs_type == BFS ? 14 : 0;

	lpcb[SIP].lp_path = dflt_sip + cutoff2;
	lpcb[MIP].lp_path = dflt_mip + cutoff2;
	lpcb[KERNEL].lp_path = bootstring;
	for (i=0; i<=KERNEL; i++)
		lpcb[i].lp_type = i;

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & BOOTDBG) {
		printf("act_part= 0x%x sbml= 0x%x entry_ds_si= 0x%x bps= %d spt= %d spc= %d\n"
		, act_part, secboot_mem_loc, entry_ds_si, bps, spt, spc);
		printf("bootdbflags = %x, bootinfo size = %x\n",
				bootenv.db_flag, sizeof(bootenv));
		printf("bootmsg= %s cntlblk_bootmsg= %s\n", bootenv.bootmsg,
			btcntlp->bootmsg);
		for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++) 
			printf("btcntl_mrng= 0x%x\n", btcntlp->bc_memrng[i].base);
		printf("ADDR:bootenv %x, avail %x, used %x, bend %x[%x]\n",
			&bootenv, &BTE_INFO.memavail[0],
			&BTE_INFO.memused[0], &end, bootenv.bootsize);
		printf("ADDRESS: _start= 0x%x main= 0x%x goprot= 0x%x \n",
			_start, main, goprot);
		printf("PRI_REGS: stack_loc= 0x%x stack_ptr= 0x%x\n", &bstack, &status);
		goany();
	}
#endif

/* 	SIP - loading							*/
	lpcb[SIP].lp_memsrt = bootenv.bootsize;	
	if (bload(&lpcb[SIP]) == FAILURE) 
		bootabort();

/* 	MIP - loading							*/
	lpcb[MIP].lp_memsrt = (paddr_t) ptround(lpcb[SIP].lp_memend);
	if (bload(&lpcb[MIP]) == FAILURE) 
		bootabort();
	bootenv.bootsize = (paddr_t) ptround(lpcb[MIP].lp_memend);

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & BOOTTALK) 
		printf("Init SIP: entry= 0x%x MIP: entry= 0x%x\n", 
			lpcb[SIP].lp_entry, lpcb[MIP].lp_entry);
#endif

/*	SIP - initialization						*/
	( (int (*)()) lpcb[SIP].lp_entry) (&bootenv, &bf, SIP_INIT, &lpcb[SIP]);

#ifdef BOOT_DEBUG
	if (bootenv.db_flag)
		goany();
#endif

/*	MIP - initialization						*/
	( (int (*)()) lpcb[MIP].lp_entry) (&bootenv, &bf, MIP_INIT, &lpcb[MIP]);

	if ( memrng_updated )
	{
		bootenv.memrngcnt = btcntlp->bc_memrngcnt;
	        for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++)
	                bootenv.memrng[i] = btcntlp->bc_memrng[i];
	}

#ifdef BOOT_DEBUG
	if (bootenv.db_flag) 
		goany();
#endif
/*	SIP - prepare kernel loading					*/
	( (int (*)()) lpcb[SIP].lp_entry) (&bootenv, &bf, SIP_KPREP, &lpcb[SIP]);

/* 	The first used memory segment is the boot(us). This will get 
 *	changed later to an extent of RESERVED_SIZE(0x3000) to account
 *	for the real memory page table and the page directory.        
 */
	BTE_INFO.memused[0].base = 0L;
	BTE_INFO.memused[0].extent = bootenv.bootsize;
	BTE_INFO.memused[0].flags = B_MEM_BOOTSTRAP;

/* 	KERNEL loading: present the bootmsg, bootprompt, etc. 		*/
	for (status= FAILURE; status == FAILURE; ) {
		BTE_INFO.memusedcnt = 1;
		kb_flush();
		printf("\n%s \n", bootenv.bootmsg);

		status = bload(&lpcb[KERNEL]);

		if ( status == FAILURE )
		{
			for ( ;; )
			{
				kb_flush();
				printf("\n%s ", bootprompt);

				for (t = bootenv.timeout; !ischar() &&
			     		(bootenv.timeout == 0 || t-- > 0);)
					wait1s();

				if (!ischar() || bgets(bootstring, B_STRSIZ) == 0) 
					strcpy(bootstring, dfltbootpath + cutoff1);

				lpcb[KERNEL].lp_path = bootstring;
				printf("\n");

				if ( bootstring[ 0 ] == '/' &&
						  root_fs_type != s5 )
				{
					printf( "\nThe boot code only supports an s5 root filesystem\n" );
					continue;
				}

				break;
			}

			/*
			** Check if they specified a kernel in the root
			** filesystem, rather than the boot filesystem.
			**
			** If they specify /stand/unix.old, it will FAIL
			** unless the directory /stand has a unix.old.
			** (as opposed to the boot filesystem mounted on
			** /stand.
			*/

			if ( bootstring[ 0 ] == '/' )
			{
				boot_fs_type = root_fs_type;
				boot_delta = root_delta;
			}
			else
			{
				boot_fs_type = save_fs_type;
				boot_delta = save_delta;
			}
		} 
	}

/* 	Copy the booted program name into bargv[0] 			*/

        dest = BTE_INFO.bargv[0];
	fname = bootstring;

        if ( boot_fs_type == BFS )
        {
                /*
                ** Since the bfs file code in the boot code ignores
                ** everything except the final component in the path,
                ** I need to do the same.
                */

                for ( bootstr = bootstring; *bootstr != '\0';)
                {
                        if (*bootstr++ == '/')
                                fname = bootstr;
                }

                strcpy( dest, "/stand/" );
                dest += strlen( "/stand/" );
        }

        strcpy(dest, fname);

#ifdef BOOT_DEBUG
	if (bootenv.db_flag & BOOTTALK) {
		printf("boot: kernel dread_cnt= %d\n", dread_cnt);
		for (i = 0; i < (int)BTE_INFO.bargc; i++) 
			printf("[%d] argv= %s\n", i, BTE_INFO.bargv[i]);
		goany();
	}
#endif

/*	
 *	Under the multiple floppy boot mode, prompt the user to insert 
 *	the hard disk preparation diskette. Do this now since mip
 *	might turn off shadow ram later that MIGHT prevent us to
 *	access the BIOS.
 */
	if ((BTE_INFO.bootflags & (BF_MFLOP_BOOT|BF_FLOPPY)) ==
		(BF_MFLOP_BOOT|BF_FLOPPY)) {

/*		let the floppy DMA channel settle before screen display	*/
/*		this is required for 25M Hz PS2 model 70		*/
		if (bootenv.sysenvmt.machflags & MC_BUS)
			wait1s();

		printf("\nPlease remove the UNIX System Boot Floppy 1 (1 of 3)\n");
		printf("from the floppy drive.\n");
		printf("\nInsert the UNIX System Boot Floppy 2 (2 of 3)\n" );
		printf("into the drive and then strike ENTER/RETURN");
		(void)getchar();
		putchar('\r');
		putchar('\n');
	}

/*	MIP - execute final machine setup procedure			*/
	( (int (*)()) lpcb[MIP].lp_entry) (&bootenv, &bf, MIP_END, &lpcb[MIP]);

/*	invoke the system startup routine				*/
	lpcbp = &lpcb[SIP];
	SIP_fdesc_p = (ulong) &flatdesc;	
	SIP_kentry = (ulong) lpcb[KERNEL].lp_entry;
	SIP_lpcb_p = (ulong) lpcb;
	( (int (*)()) lpcbp->lp_entry) (&bootenv, &bf, SIP_KSTART, lpcbp);
}


/*	flush keyboard						*/
kb_flush()
{
	while ( ischar() )
		(void)getchar();
}

/*	Terminate boot program					*/
bootabort()
{
	kb_flush();
	printf("\007\nUse Ctrl-Alt-Del to reboot\007");

	for ( ;; )
		kb_flush();
}

extern char		*getdef();
extern unsigned long	atol();

#define DEFERROR(x)	printf("\nboot: %s argument missing or incorrect\n", x)

#define SET_BY_BOOT	(B_MEM_BOOTSTRAP|B_MEM_KTEXT|B_MEM_KDATA)

void
update_bootcntl( btcntlp )
struct bootcntl *btcntlp;
{
	off_t	offset = 0;
	int	n;
	int	memrngcnt;
	char	buf[B_STRSIZ];
	struct	bootmem	*mp;
	char	*p, *q;
	unsigned long	t;

	debug(printf("Entering update_bootcntl\n"));

	/* 
	 * search for valid options; this is inefficient, but
	 * at least it's relatively clean
	 */

	for( ; ((n = bfgets(buf, B_STRSIZ, offset)) != 0); offset += n+1 ) {

		/*
		 * Copy buffer to argv before doing anything else,
		 * but only increment argc if it's OK.
		 * Thus, we can stomp on buf during the parsing.
		 */

		strncpy(BTE_INFO.bargv[BTE_INFO.bargc], buf, B_STRSIZ);

		/* if it's a comment, skip it */

		if ( buf[0] == '#' )
			continue;

		if ( p = getdef( buf, "BOOTMSG") ) {
			strncpy(btcntlp->bootmsg, p, B_STRSIZ);

		} else if ( p = getdef( buf, "MEMRANGE") ) {

			memrng_updated = TRUE;
			memrngcnt = 0;
			q = strtok( p, "-");

			do {	
				unsigned long basemem;
				mp = &btcntlp->bc_memrng[memrngcnt];

				/* start of the range */

				if ( q == NULL ) {
					DEFERROR("MEMRANGE");
					break;
				}
				if ( (t = atol(q)) == -1L) {
					DEFERROR("MEMRANGE");
					break;
				}
				mp->base = (paddr_t)ctob( btoct(t) );

				/* end of the range */

				if ( (q = strtok( NULL, ":")) == NULL ) {
					DEFERROR("MEMRANGE");
					break;
				}

				if ( (t = atol(q)) == -1L) {
					DEFERROR("MEMRANGE");
					break;
				}
				debug(printf("mp->base = 0x%x\n", mp->base));
				if ((long) (mp->base) == 0L) {
#ifdef SKIP
				   basemem = ctob((memsz*1024) / NBPP);
				   debug(printf ("0-? range in /etc/default 0x%x\n", t));
				   debug(printf ("0-? range for int 12 0x%x\n", basemem));
				   t = (t < basemem) ? t : basemem;
#endif
				   debug(printf("selected range 0-0x%x\n", t));
				}
				mp->extent = ctob( btoc(t - (long)mp->base) );

				/* flags */

				if ( (q = strtok( NULL, ",\n")) == NULL) {
					DEFERROR("MEMRANGE");
					break;
				}
				if ( (t = atol(q)) == -1L) {
					DEFERROR("MEMRANGE");
					break;
				}
				mp->flags = (ushort)t & ~SET_BY_BOOT;

				memrngcnt++;
				q = strtok( NULL, "-");

			} while ( (q != NULL) && (memrngcnt < B_MAXMEMR) );

			btcntlp->bc_memrngcnt = memrngcnt;

		/*
		 * If the string doesn't seem to be well formed, 
		 * punt silently, so that we don't yell 
		 * when processing /etc/TIMEZONE shell scripts.
		 */

		} else if ( getdef( buf, NULL) == NULL ) {
			debug(printf("bdefault: unknown option\n")); 
			continue;
		} else {

			/* pass to kernel in argv */

			if ( ++BTE_INFO.bargc == B_MAXARGS )
				break;
		} 
	}
}


/* 
 * getdef():	get default entry from string buffer.
 *		returns a pointer to the arguments associated with key,
 *		NULL if key is not present or argument(s) are missing.
 */

char *
getdef(buf, key)
char	*buf;
char	*key;
{
	int 	c = 0;

	if ( (key != NULL) && (strncmp( buf, key, strlen(key)) != 0) )
		return(NULL);

	/* find beginning of arg string, and return a pointer to it */

	for ( ; buf[c]; c++ )
		if ( (buf[c] == '=') )
			return( &buf[c+1] );

	/* if not found, return NULL */

	return( NULL );
}


/* 
 * atol():	sort of, but not exactly, like the libc atol().
 *		We extract a positive integer from the string s,
 *		allowing for the 'k' (*1024) and 'm' (*1024^2)
 *		multiplier abbreviations.
 *		returns the integer if successful, (unsigned long)-1L if not.
 */

unsigned long
atol( p )
register char	*p;
{
	register unsigned long n;

	if ( *p == 0 )
		return(-1L);

	/* gobble white space */

	for ( ;; *p++ ) {
		if ( (*p == ' ') || (*p == '\t') )
			continue;
		break;
	}

	/* grab digits */

	n = 0;
	while ( (*p >= '0') && (*p <= '9') ) 
		n = n * 10 + *p++ - '0';

	/* modifiers */

	switch( *p ) {
	case ('M'):
	case ('m'):
		n *= 1024;
	case ('K'):
	case ('k'):
		n *= 1024;
		p++;
	}

	return( ((*p == '\0') || (*p == '\n')) ? n : -1L );
}
