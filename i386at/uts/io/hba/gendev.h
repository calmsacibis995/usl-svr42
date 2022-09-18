/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/gendev.h	1.11"

/*
 * Definitions for the Generic Device drivers.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1986-1989 INTERACTIVE Systems Corporation
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

#ifdef	_KERNEL_HEADERS

#include <io/target/altsctr.h>
#include <proc/user.h>

#elif defined(_KERNEL)

#include <sys/altsctr.h>
#include <sys/user.h>

#endif	/* _KERNEL_HEADERS */

#define GDEV_MAXIOCTL   8       /* Maximum # of low-level IOCTL's supported */
#define GDEV_MAXDRV     8       /* Maximum # of drives per controller */
#define GDEV_CTL_INT    2       /* Maximum # of interrupt vectors per controller */
#define GDEV_MAX_INT    16      /* maximum # of interrupt levels */
#define GDEV_INTENTS    12      /* Max # of primary & secondary interrupt handlers */
#define GDEV_LOWLEV     10      /* # of doublewords of storage to reserve in */
				/* gdev_ctl_block and gdev_parm_block for low- */
				/* level driver code */
#define GDEV_SHAREMAX   3       /* Max # of drivers that can share a controller */
#define GDEV_MINORMAPS  48      /* Number of minormap entries to allocate */
#define GDEV_MAJORMAPS  48      /* Number of majormap entries to allocate */
#define GDEV_FDISKSLOTS 4       /* Number of 'fdisk' table slots */
#define GDEV_DOSPARTS	11	/* Number of extended dos partitions max */
		/* Number of actual partition entries */
#define GDEV_TOTPARTS   (V_NUMPAR+GDEV_FDISKSLOTS+GDEV_DOSPARTS+1) 

#define SPLGDEV spl5()

/* Change the following 2 defines as necessary to support more stuff */

#define GDEV_MAXCTLS    6       /* Max number of controller boards allowed */
#define GDEV_MAXDRIVS   16      /* Max number of drives allowed */

extern  ushort  gdev_drqblox;
extern  ushort  gdev_maxctls;
extern  ushort  gdev_maxdrivs;
extern  ushort  gdev_max_int;
extern  ushort  gdev_intents;
extern  ushort  gdev_sharemax;

/*
 * The special ioctl definitions block.  This is defined in the config
 * structure and is pointed at by the dev controller block.
 */

struct special_ioctls
	{
	int     spi_count;      /* number of actual entries used */
	struct  {
		int     ioctl_num;      /* ioctl number */
		int     (*ioctl_code)(); /* function to handle that ioctl */
		} spi_ents[GDEV_MAXIOCTL];
	};


/*
 * Structure to hold driver-specific information for a controller.
 * There is one of these structures for each driver using a controller
 * in that controller's dcb.
 */

struct  gdev_driver
	{
	int     (*drv_CMD)();   /* Perform Command function */
	int     (*drv_OPEN)();  /* Device Opening function (or NULL) */
	int     (*drv_CLOSE)(); /* Device Closing function (or NULL) */
	int     (*drv_START)(); /* Start I/O function */
	int     (*drv_drvint)(); /* driver completion/error interrupt routine */
	struct  gdev_parm_block *(*drv_INT[GDEV_CTL_INT])();    /* interrupt funcs */
	struct  special_ioctls *drv_ioctls;     /* special ioctls */
	ushort  drv_drives;     /* Number of drives for this driver */
	ushort  drv_curdriv;    /* Current drive for this driver */
	ushort  drv_firstdriv;  /* idx of first drive for this driver */
	ushort  drv_baseminor;  /* First minormap entry for this driver */
	};


/*
 * Generic Device Controller Block.  This structure contains information about
 * the configuration, capabilities, and status of a given controller board.
 */

struct  gdev_ctl_block {
	char    *dcb_name;      /* Controller Name (text string for err msgs) */
	ulong   dcb_capab;      /* Controller Capability flags (below) */
	paddr_t dcb_memaddr1;   /* Controller Memory Address (prime) */
	paddr_t dcb_memaddr2;   /* Controller Memory Address (secondary) */
	ushort  dcb_ioaddr1;    /* Controller I/O Space Address (prime) */
	ushort  dcb_ioaddr2;    /* Controller I/O Space Address (secondary) */
	ushort  dcb_dmachan1;   /* DMA channel used (prime) */
	ushort  dcb_dmachan2;   /* DMA channel used (secondary) */
	ushort  dcb_maxsec;     /* Max # of blocks in single controller req */
	ushort  dcb_delay;      /* Delay time for switching drives in 10us units */
	ushort  dcb_defsecsiz;  /* Default block size for drives on this ctl */
	ushort  dcb_flags;      /* Controller Activity flags (below) */
	ushort  dcb_drivers;    /* Number of drivers using this controller */
	ushort  dcb_driveridx;  /* current driver index into below struct */
	int     (*dcb_multint)(); /* CCAP_MULTI Master interrupt handler */
	struct  gdev_driver dcb_drv[GDEV_SHAREMAX];     /* driver-specific fields */
	struct  gdev_parm_block          /* Pointers at Gdev Param Blocks */
		*dcb_dpbs[GDEV_MAXDRV];  /* (one for EACH dev on controller) */
	ulong   dcb_lowlev[GDEV_LOWLEV]; /* Scratch space for low-level code */
	int	dcb_hbano;		 /* hba number */
	void	(*dcb_xferok)(); /* pointer to gdev_xferok routine to remove */
				 /* sub-DCD direct dependency on gendev funcs*/
	clock_t	dcb_laststart;	/* lbolt time set at cmd start */
	};

typedef struct  gdev_ctl_block  *gdev_dcbp;
extern  struct  gdev_ctl_block  gdev_ctl_blocks[];      /* def'd in space.c */
extern  ushort  gdev_nextctl;           /* next gdev_ctl_block to allocate */

/*
 * The following defines allow access to the driver dcb fields.
 */

#define drvidx(X)       ((X)->dcb_driveridx)
#define dcb_CMD(X)       (X)->dcb_drv[drvidx(X)].drv_CMD
#define dcb_OPEN(X)      (X)->dcb_drv[drvidx(X)].drv_OPEN
#define dcb_CLOSE(X)     (X)->dcb_drv[drvidx(X)].drv_CLOSE
#define dcb_START(X)     (X)->dcb_drv[drvidx(X)].drv_START
#define dcb_drvint(X)    (X)->dcb_drv[drvidx(X)].drv_drvint
#define dcb_INT(X,IDX)   (X)->dcb_drv[drvidx(X)].drv_INT[IDX]
#define dcb_ioctls(X)    (X)->dcb_drv[drvidx(X)].drv_ioctls
#define dcb_drives(X)    (X)->dcb_drv[drvidx(X)].drv_drives
#define dcb_firstdriv(X) (X)->dcb_drv[drvidx(X)].drv_firstdriv
#define dcb_curdriv(X)   (X)->dcb_drv[drvidx(X)].drv_curdriv
#define dcb_baseminor(X) (X)->dcb_drv[drvidx(X)].drv_baseminor


extern  struct  iobuf   gdev_iobufs[];   /* V.3 io buffer headers (1 per drive) */

/* Controller Capability Flags: */
#define CCAP_MULTI      0x01    /* Multiple concurrent I/O requests supported */
#define CCAP_DMA        0x02    /* DMA supported */
#define CCAP_SCATGATH   0x04    /* Scatter-gather I/O supported */
#define CCAP_16BIT      0x08    /* Limit contiguous xfers to 64K (len & bound) */
#define CCAP_CYLLIM     0x10    /* I/O request limited to a single cylinder */
#define CCAP_CHAINSECT  0x20    /* Command chaining at sector boundaries supported */
#define CCAP_NOSEEK     0x40    /* No explicit SEEK commands should be used */
#define CCAP_RETRY      0x80    /* Controller can do automatic retries */
#define CCAP_ERRCOR     0x100   /* Controller can do error correction */
#define CCAP_SHARED     0x200   /* Controller can be shared between different */
				/* drivers (up to GDEV_SHAREMAX of them). */
				/* Used for disk & tape on same controller */
#define CCAP_PIO        0x400   /* Controller use programmed I/O transfers */

/* Controller Activity Flags: */
#define CFLG_BUSY       0x01    /* Controller is busy processing a request. */
				/* For single-thread controllers, this is */
				/* handled entirely by generic code.  For */
				/* multi-thread (CCAP_MULTI) controllers, */
				/* low-level code should clear this and call */
				/* the drv_START routine for the first driver */
				/* when the controller is available */
				/* to process another request */
#define CFLG_EXCLREQ    0x02    /* An exclusive controller request is pending */
				/* (a 'wakeup' on &gdev_ctl_block will be */
				/* done when controller goes idle) */
#define CFLG_EXCL       0x04    /* Someone has exclusive access to controller. */
				/* Do not re-grant exclusivity until released */
#define CFLG_INIT       0x08    /* We are initializing this controller */
#define CFLG_INITDONE   0x10    /* Controller initialization has been done */
				/* (intended for shared controllers) */

/*
 * Generic Device Parameter Block.  This structure contains information about
 * the physical geometry of a device, its activities, and its state.
 */

struct  gdev_parm_block {
	ulong   dpb_flags;      /* Drive flags (below) */
	ulong   dpb_cyls;       /* Number of available cylinders on drive */
	ulong   dpb_rescyls;    /* Number of reserved cylinders on drive */
	ulong   dpb_wpcyl;      /* Write-precomp cyl (if necessary) */
	ulong   dpb_parkcyl;    /* Head-parking cyl (if necessary) */
	ulong   dpb_pdsect;     /* Default 'pdinfo' sector number */
	ushort  dpb_heads;      /* Number of heads per cylinder */
	ushort  dpb_sectors;    /* Number of available sectors per track */
	ushort  dpb_ressecs;    /* Number of reserved sectors per track */
	ushort  dpb_secsiz;     /* Sector size (in bytes) */
	ushort  dpb_secovhd;    /* Sector overhead (in bytes) */
	ushort  dpb_state;      /* Drive state (below) */
	ushort  dpb_intret;     /* What happened on last interrupt (below) */
	unchar  dpb_devtype;    /* Device type indicator (below) */
	unchar  dpb_subtype;    /* Device sub-type (below) */
	ushort  dpb_command;    /* Current command being executed on drive */
	ushort  dpb_savecmd;    /* Saved command for retry-restore */
	ushort  dpb_drvcmd;     /* Actual drive-level command being executed */
	ushort  dpb_drvflags;   /* Drive-specific flags */
	ulong	dpb_pcyls;	/* physical cylinders; 0 if unknown */
	ulong	dpb_pheads;	/* physical heads */
	ulong	dpb_psectors;	/* physical sectors/track */
	ulong	dpb_pbytes;	/* physical bytes/sector */
	unchar  dpb_interleave; /* Hardware Interleave Factor used on drive */
	unchar  dpb_skew;       /* Skew factor (if known & intlv == 1, 0 otherwise */
	ushort  dpb_drverror;   /* Last error code from drive */
	ushort  dpb_numparts;   /* Number of partitions on drive */
	ushort  dpb_altsiz;     /* Size of alternates map for drive (bytes) */
	struct  malt_table *dpb_alts;   /* Pointer at alternates map for drive */
	struct  drq_entry *dpb_req;     /* pointer at current request entry */
	dcdblk_t *dpb_que;       /* pointer at request queue head */

/*
 * The following 5 fields are set up by Generic code for new transfers
 * and for retries.  They are to be updated by low-level code (using
 * 'gdev_xferok' in genblklib.c) ONLY after SUCCESSFUL transfer of
 * some number of sectors to/from the drive has been assured.
 */
	paddr_t dpb_curaddr;    /* Current physical memory transfer address */
	paddr_t dpb_virtaddr;   /* Current virtual memory transfer address */
	daddr_t dpb_cursect;    /* Current sector number on drive */
	proc_t  *dpb_procp;
	ulong   dpb_totcount;   /* Total bytes xfer'd on this request so far */
	ulong   dpb_sectcount;  /* Sectors remaining in current disk section */
	struct  drq_entry *dpb_memsect; /* Current memory section (if used) */

/*
 * The following 3 fields are used for maintaining running values during
 * the course of a transfer until the success or failure of the transfer
 * can be determined.  They are set from the corresponding fields above
 * for each new disk section or retry operation.
 */
	paddr_t dpb_newaddr;    /* Active physical memory address for xfer */
	paddr_t dpb_newvaddr;   /* Active virtual memory address for xfer */
	ulong   dpb_newcount;   /* Bytes remaining in active memory section */
				/* This gets set from dpb_memsect's */
				/* drq_count */
	struct drq_entry *dpb_newdrq;   /* active memory section */

	ushort  dpb_curcyl;     /* Current DISK cylinder. MUST BE MAINTAINED */
				/* unless CCAP_NOSEEK is specified */
	ushort  dpb_retrycnt;   /* Number of retries performed on command */
	short   dpb_blkshft;    /* shift for blk->sector conversion (<0 == <<) */
	ushort  dpb_secbufsiz;  /* size in bytes of sector buffers */
	char   *dpb_secbuf[2];  /* -> MEMXFER buffer (2 diff ones if chaining) */
	ulong 	dpb_unix_begin;           /* Beginning of Unix Part on Drive */
	struct  sense dpb_reqdata;		/* request sense data */
	struct	ident dpb_inqdata;		/* inquiry data */
	char	*dpb_cbque;       /* pointer at call back queue head */
	dcdblk_t *dpb_nextque;      /* pointer to next set of jobs queued */
	ulong   dpb_lowlev[GDEV_LOWLEV]; /* Scratch space for low-level code */
	};


typedef struct  gdev_parm_block *gdev_dpbp;
extern  struct  gdev_parm_block gdev_parm_blocks[];     /* def'd in space.c */
extern  ushort  gdev_nextdriv;  /* next gdev_parm_block to allocate */

/* Drive flags: */
#define DFLG_BUSY       0x01    /* Drive is currently busy */
#define DFLG_RETRY      0x02    /* Retries should be attempted on commands */
#define DFLG_ERRCORR    0x04    /* Error correction should be attempted */
#define DFLG_OPENING    0x08    /* Drive is currently being opened */
#define DFLG_VTOCOK     0x10    /* Drive has a valid pdinfo/vtoc/etc */
#define DFLG_CLOSING    0x20    /* Last partition on drive is being closed */
#define DFLG_VTOCSUP    0x40    /* VTOC/PDINFO/ALTS/etc supported on drive */
#define DFLG_REMOVE     0x80    /* Drive supports removable media */
#define DFLG_SPECIAL    0x100   /* A special (no queue entry) request in progress */
				/* (a 'wakeup' on gdev_param_block will be done at */
				/* completion of the request).  */
#define DFLG_READING    0x200   /* This drive is currently involved in a READ */
				/* request.  Used in deciding whether READ or */
				/* WRITE commands should be issued for I/O */
#define DFLG_OPEN       0x400   /* This drive is currently opened */
#define DFLG_OFFLINE    0x800   /* Drive is offline for some reason */
#define DFLG_FIXREC     0x1000  /* Drive supports fixed-length records only */
				/* Above is intended for tape (cart vs mag) */
#define DFLG_EXCLREQ    0x2000  /* Someone wants access to specific device   */
				/* We will grant access when device goes idle*/
#define DFLG_SUSPEND	0x4000	/* The queue has been suspended */


/* Drive states: */
#define DSTA_IDLE       0       /* Drive is idle */
#define DSTA_SEEKING    1       /* Drive is seeking */
#define DSTA_NORMIO     2       /* Drive is performing normal I/O (read/write) */
#define DSTA_RECAL      3       /* Drive is doing diagnostic recalibrate */
#define DSTA_GETERR     4       /* Drive is getting extended error code */
#define DSTA_RDVER	5	/* Drive is performing read/verify cmd */
#define DSTA_FORMAT	6	/* Drive is being formatted */

/*
 * Values for dpb_intret.  This value is set by the controller-level
 * interrupt code to let the driver know what occurred during processing
 * of the last interrupt.  This drives a switch after the controller-level
 * interrupt handler to determine what to do next.
 */

#define DINT_CONTINUE   1       /* Current controller command is continuing */
				/* No intervention by generic code is necessary. */
#define DINT_COMPLETE   2       /* Current request has completed normally */
#define DINT_GENERROR   3       /* Some general error occurred */
				/* (dpb_drverror is not accurate yet -- */
				/* generic code will request error code) */
#define DINT_ERRABORT   4       /* Current command terminated abnormally */
				/* (generic error code is in dpb_drverror) */
#define DINT_NEEDCMD    5       /* A command needs to be issued */
#define DINT_NEWSECT    6       /* Processing needs to be initiated on a new */
				/* section request. */

/* Device type codes: */
#define DTYP_UNKNOWN    0       /* Unknown device type */
#define DTYP_DISK       1       /* Disk type device */
#define DTYP_TAPE       2       /* Tape type device */


/* Device sub-type codes: */
/* NONE OF THESE ARE CURRENTLY DEFINED. */


/*
 * Generic Device Error Numbers.  Each controller-level handler must convert
 * its own controller's error codes into these.  A standard array of these
 * numbers in a well-known order should be part of the driver's space.c
 * file to facilitate this translation.  It may not be possible to get all
 * error codes from all controller/drive combinations.  This list attempts
 * to be exhaustive.
 */

#define DERR_NOERR      0       /* No error found */
#define DERR_DAMNF      1       /* Data address mark not found */
#define DERR_TRK00      2       /* Unable to recalibrate to track 0 */
#define DERR_WFAULT     3       /* Write Fault on drive */
#define DERR_DNOTRDY    4       /* Drive is not ready */
#define DERR_CNOTRDY    5       /* Controller will not come ready */
#define DERR_NOSEEKC    6       /* Seek will not complete */
#define DERR_SEEKERR    7       /* Seek error (wrong cylinder found) */
#define DERR_NOIDX      8       /* No Index signal found */
#define DERR_WRTPROT    9       /* Medium is write-protected */
#define DERR_NODISK     10      /* Medium is not present in drive */
#define DERR_BADSECID   11      /* Error found in sector ID field */
#define DERR_SECNOTFND  12      /* Sector not found */
#define DERR_DATABAD    13      /* (uncorrectable) Error found in sector data */
#define DERR_BADMARK    14      /* Sector or track was marked bad */
#define DERR_FMTERR     15      /* Error during Format operation */
#define DERR_BADCMD     16      /* Illegal/erroneous command */
#define DERR_CTLERR     17      /* Controller error or failure */
#define DERR_ABORT      18      /* Command aborted with no apparent cause */
#define DERR_SEEKING    19      /* Drive is still seeking (try again later) */
#define DERR_MEDCHANGE  20      /* Medium has been changed in drive */
#define DERR_PASTEND    21      /* I/O past end of drive */
#define DERR_OVERRUN    22      /* Data overrun */
#define DERR_TIMEOUT    23      /* Command timeout */
#define DERR_DRVCONFIG  24      /* Unable to get valid drive configuration */
#define DERR_UNKNOWN    25      /* Undetermined error */
#define DERR_EOF        26      /* Found EOF on Read or EOM on Write */
#define DERR_ERRCOR     27      /* Correctable data error occurred */

/*
 * The following table is used to process dev errors.
 * It is indexed by the Generic error code in dpb_drverror.
 */

struct  gdev_err_msg {
	ushort  err_unixcod;    /* Unix error code to return */
	ushort  err_flags;      /* Error flags (see below) */
	char    *err_msgptr;    /* Error message text */
	};

/* Error message flag values: */

#define ERF_PANIC       0x01    /* This error type should panic system */
#define ERF_QUIET       0x02    /* Handle this one without saying anything */
#define ERF_NORETRY     0x04    /* Don't bother retrying this one... */

extern struct gdev_err_msg gdev_err_msgs[];     /* defined in space.c */


/* Drive Commands: */
#define DCMD_READ       1       /* Read Sectors/Blocks */
#define DCMD_WRITE      2       /* Write Sectors/Blocks */
#define DCMD_FORMAT     3       /* Format Tracks */
#define DCMD_SETPARMS   4       /* Set Drive Parameters */
#define DCMD_RECAL      5       /* Recalibrate */
#define DCMD_GETERR     6       /* Get Generic Error Code */
#define DCMD_SEEK       7       /* Seek to Cylinder */
#define DCMD_RETRY      9       /* Restart current command for retry */
#define DCMD_FMTDRV     10      /* Format entire drive */
#define DCMD_REWIND     11      /* Rewind */
#define DCMD_SEOF       12      /* Skip to File Mark */
#define DCMD_UNLOAD     13      /* Unload removable medium */
#define DCMD_SERVWRT    14      /* Write Servo Data */
#define DCMD_ERASE      15      /* Erase Tape */
#define DCMD_RETENSION  16      /* Retension Tape */
#define DCMD_WFM        17      /* Write File Mark */
#define DCMD_GETPARMS   18      /* Get Parameters from device */
#define DCMD_RDVER	19	/* Read Verify sectors on disk */
#define DCMD_FMTBAD	20	/* Format Bad Track */
#define DCMD_LOAD	21	/* Load removable medium */
#define DCMD_LOCK	22	/* Lock removable medium */
#define DCMD_UNLOCK	23	/* Unlock removable medium */
#define DCMD_RESET	24	/* Reset tape device */
#define DCMD_MSENSE	25	/* Mode Sense command */
#define DCMD_REQSEN	26	/* Request Sense command */
#define DCMD_RELES	27	/* release command */

/*
 * Device Request Queue.  This structure is used to contain all information
 * necessary to completely process a single strategy-level driver request.
 * It is pointed at by the first buffer header in a list for a request.
 * The entries in the chain are singly linked through drq_link.  See the
 * description of the 'strategy' routine, above, for how this all works.
 *
 * Driver 'init' code creates a pool of these elements.  'strategy' calls
 * 'getdrq' to allocate them (which can cause a sleep if there are none).
 * Any controller-level code which advances the dpb_req pointer should call
 * 'reldrq' on the old one to give it back to the pool, or we run out
 * REAL quick.
 */

struct  drq_entry       {
	union   {
		struct  {
			uint    drq_xcnt  : 24; /* Byte or sector count */
			uint    drq_xtyp  :  7; /* Entry type (see below) */
			uint    drq_xflgs :  1; /* special flag (during strategy ONLY) */
			} drq_flags;
		uint    drq_flgint;     /* for zeroing all of the first word */
		} drq_word1;
	ulong   drq_addr1;              /* Memory address 1 */
	char 	*drq_priv;	/* private ptr for dynamic alloc routines */
				/* DO NOT USE or modify	*/
	ulong   drq_addr2;              /* Disk or memory address 2 */
	ulong   drq_vaddr;              /* virtual address of memory */
	struct drq_entry *drq_link;     /* Pointer at next entry in chain */
	};

#define drq_count       drq_word1.drq_flags.drq_xcnt
#define drq_type        drq_word1.drq_flags.drq_xtyp
#define drq_phys        drq_word1.drq_flags.drq_xflgs     /* START or CONT has physical memaddr */
#define drq_virt        drq_word1.drq_flags.drq_xflgs     /* MEMBREAK has virt addr of brk in bufp */
#define drq_daddr(X)    (daddr_t)((X)->drq_addr2)
#define set_drq_daddr(X,VAL) (X)->drq_addr2 = (ulong)(VAL)
#define drq_bufp(X)     (dcdblk_t *)((X)->drq_addr2)
#define set_drq_bufp(X,VAL) (X)->drq_addr2 = (ulong)(VAL)
#define drq_memaddr(X)  (paddr_t)((X)->drq_addr1)
#define set_drq_memaddr(X,VAL) (X)->drq_addr1 = (ulong)(VAL)
#define drq_vaddr(X)  (paddr_t)((X)->drq_vaddr)
#define set_drq_vaddr(X,VAL) (X)->drq_vaddr = (ulong)(VAL)
#define drq_drqp(X)     (struct drq_entry *)((X)->drq_addr1)
#define set_drq_ptr(X,VAL) (X)->drq_addr1 = (ulong)(VAL)
#define drq_srcaddr(X)  (paddr_t)((X)->drq_addr1)
#define set_drq_srcaddr(X,VAL) (X)->drq_addr1 = (ulong)(VAL)
#define drq_dstaddr(X)  (paddr_t)((X)->drq_addr2)
#define set_drq_dstaddr(X,VAL) (X)->drq_addr2 = (ulong)(VAL)


extern  struct  drq_entry       drq_entries[];          /* def'd in space.c */
extern  struct  drq_entry       *drq_freelist;          /* def'd in space.c */

extern  struct  drq_entry       *getdrq();
extern  struct  drq_entry       *reldrq();

/*
 * drq_type values:
 */
#define DRQT_START      1       /* Start new transfer.  drq_count is sector */
				/* count, drq_daddr is start sector, drq_memaddr */
				/* is beginning physical memory address.  Need */
				/* to seek to start, good time to allow overlap. */

#define DRQT_CONT       2       /* Like DRQT_START, but just a continuation of */
				/* transfer in progress.  Shouldn't need to seek */
				/* or allow other I/O to intervene */

#define DRQT_MEMBREAK   3       /* Memory Address Discontinuity.  drq_count is */
				/* number of bytes before break point, drq_memaddr */
				/* is new transfer address, drq_bufp (if non-NULL) */
				/* points at buffer header to call 'iodone' on after */
				/* break point is passed */

#define DRQT_MEMXFER    4       /* Memory-Memory Transfer.  Used to simulate */
				/* scatter/gather for non-buffered controllers that */
				/* don't support it.  drq_count is byte count, */
				/* drq_srcaddr and drq_dstaddr are source & dest */
				/* memory addresses, respectively.  NOTE: most */
				/* controller-level code should NEVER see these */
				/* types of entry -- they're handled by generic code */
				/* unless CCAP_CHAINSECT is set. */

/*
 * The following 2 drq_types are for TAPE ONLY.  To allow a cartridge tape
 * unit (which supports only fixed blocks) to support the appearance of being
 * a variable-record-size device, we make use of a sector (block) buffer
 * allocated during tape drive initialization.  If someone wants to write a
 * block that is not a multiple of <dpb_secsiz>, we write all but the last
 * block as normal, then copy the remnant into the block buffer with a
 * DRQT_MEMXFER.  We then use DRQT_MEMCLEAR to zero the remainder of the
 * block.  This block is then written to the tape.  If someone wants to
 * read a non-multiple, we read all but the last block as normal, read the
 * last block into the block buffer, use DRQT_MEMXFER to transfer the fragment
 * the user wants, and use DRQT_IGNORE to discard the remaining bytes (this
 * causes alignment between memory and device sections).
 */

#define DRQT_MEMCLEAR   5       /* drq_srcaddr is the beginning of the area */
				/* to zap, drq_count is the number of bytes */
#define DRQT_IGNORE     6       /* drq_count is the number of bytes to ingore */
#define DRQT_TPCMD	7


/*
 * Device Configuration Tables.  These tables contain information about expected
 * configurations of hardware present in a system.  They map major and minor
 * device numbers to physical hardware devices, provide for default I/O and
 * memory address space values, controller capabilities expected, etc.  The
 * information in these tables is advisory only!  It is the job of the Board
 * Initialization routines to determine which (if any) of these capabilities
 * and parameters are actually valid.  These are then used to fill in the
 * appropriate Device Controller Block and Device Parameter Block(s) for a
 * given controller and drive(s).
 */

struct  cfg_int_entry
	{
	ushort  cfg_intlev;                     /* interrupt level */
	struct  gdev_parm_block *(*cfg_INT)();  /* interrupt func */
	};

struct  gdev_cfg_entry  {       /* subset of the gdev_ctl_blk... */
	char    *cfg_name;      /* Controller Name (text string for err msgs) */
	ulong   cfg_capab;      /* Controller Capability flags (below) */
	paddr_t cfg_memaddr1;   /* Controller Memory Address (prime) */
	paddr_t cfg_memaddr2;   /* Controller Memory Address (secondary) */
	ushort  cfg_ioaddr1;    /* Controller I/O Space Address (prime) */
	ushort  cfg_ioaddr2;    /* Controller I/O Space Address (secondary) */
	ushort  cfg_dmachan1;   /* DMA channel used (prime) */
	ushort  cfg_dmachan2;   /* DMA channel used (secondary) */
	ushort  cfg_maxsec;     /* Max # of sectors in single controller req */
	ushort  cfg_drives;     /* Max number of drives on controller */
	ushort  cfg_delay;      /* Delay time for switching drives in 10us units */
	ushort  cfg_baseminor;  /* First entry in 'minormap' to be used */
	ushort  cfg_defsecsiz;  /* Default sector size for drives on this ctl */
	int     (*cfg_BINIT)(); /* Initialize Board function */
	int     (*cfg_DINIT)(); /* Initialize Drive function */
	int     (*cfg_CMD)();   /* Perform Command function */
	int     (*cfg_OPEN)();  /* Device Opening function (or NULL) */
	int     (*cfg_CLOSE)(); /* Device Closing function (or NULL) */
	int     (*cfg_multint)(); /* CCAP_MULTI Master interrupt handler */
	struct  cfg_int_entry   cfg_ints[GDEV_CTL_INT]; /* hardware interrupts */
	struct  special_ioctls  cfg_ioctls;     /* special ioctl routines */
	};

typedef struct gdev_cfg_entry *gdev_cfgp;

extern  int (*gdev_init_routines[])();    /* driver init funcs from space.c */

/*
 * Device Mapping Tables:  Each class of devices (disks, tapes, etc)
 * supported by the Generic Driver code has its own major device number(s).
 * Usually there is only one major number assigned per class, but large
 * classes may have more than one (if more than 16 disk devices are needed,
 * for example).  As each of the included Generic initialization routines
 * is run, it must announce its use of major numbers by filling in the
 * 'majormap' array entry/entries for itself.  Each majormap entry indicates
 * the beginning of a contiguous group of 'minormap' entries (below) that
 * will be used for the class.  All the devices in the class have the same
 * major number (or set of major numbers, if more than 1).
 * This will allow for a single 'config' file for each class
 * Given support for 16 partitions (or floppy disk formats) per
 * drive (4 bits), this leaves us 3 bits to indicate which device we want
 * and 1 bit to tell us whether we're dealing with Unix partitions or
 * physical device partitions.  We actually use the 3 bit device code
 * to index into the 'minormap' table, below.  This will
 * allow us to partition up the space as controllers require.
 * Our default partitioning (so we can always make the devs the same) is
 * to allow for 4 drives on controller 0 (devs 0-15, 16-31, 32-47, and
 * 48-63), three drives on controller 1 (devs 64-79, 80-95, and 96-111),
 * and the RAMdisk drive uses devs 240-255 (actually only 240, but they
 * come in clumps).  Joe User is free to re-arrange this as needed, provided
 * he makes the devs appropriately.  Note that when we start supporting
 * floppies on this scheme, the disk driver will require more than 1 major
 * number, as we've kinda exhausted the minor number space.
 * We don't care much about partition #, except for devices with the
 * high-order (Physical) bit set.  In this case, the partition number indiates
 * which 'fdisk' partition we want (for VTOC-ish disks) or 0 means the
 * entire physical drive.  Other than that, we pretty much ignore partition #,
 * although a floppy's device-open routine will need it to set up the drive
 * parms appropriately.  This should make ANY user happy!
 * So, the minor device number looks like:
 *               7 654 3210
 *              +-+---+----+
 *              |P|MIX|PART|
 *              +-+---+----+
 * where P indicates the physical drive (if PART == 0) or an FDISK partition
 * (1 <= PART < 4 indicating which partition), MIX is the index into
 * 'minormap' (which gives us the controller  and drive on that controller)
 * and PART is the partition number (or floppy/tape format code).
 *
 * The minormap entries are filled in at init time by the generic code.
 * Each controller's cfg_entry says where it should start in the minormap
 * and how many entries it needs.  If a controller is found, those are
 * filled in.
 * The driver init routine should call 'majorset' to allocate up to 8
 * minormap entries for each major device number it will use.  This routine
 * returns the index of the first minormap entry available for that major
 * number, so they can be filled in appropriately.
 *
 */

#define basedev(X)      ((getminor(X) & 0x70) | 0x80)  /* dev for fdisk partition 0 */
#define unixdev(X)      (getminor(X) & 0x70)       /* dev for unix partition 0 */
#define baseidx(X)      ((getminor(X) >> 4) & 7)   /* minormap idx for basedev */
#define PARTITION(X)    ((getminor(X) & 0x0F) + ((X & 0x80) ? V_NUMPAR : 0))
						/* partition # for dev */

struct majormap {
	unchar  cnf_valid;      /* If non-zero, this entry has been set */
	unchar  cnf_minorcnt;   /* # of minormap entries for major # */
	struct  minormap *cnf_minorp;   /* points at first minormap entry */
	};

struct  minormap {
	unchar  cnf_valid;      /* If non-zero, this entry has been set */
	unchar  cnf_bd;         /* Board index */
	unchar  cnf_drv;        /* Drive index on controller board */
	unchar  cnf_drvidx;     /* driver's index into dcb_drv array */
	struct  gdev_ctl_block *cnf_dcb;        /* points at dcb for board */
	} ;

extern  struct majormap majormap[];
extern  struct minormap minormap[];     /* defined in space.c */
extern  ushort  gdev_nextminor;         /* next minormap entry to allocate */
extern  ushort  gdev_minormaps;         /* # of minormap entries available */
extern  ushort  gdev_majormaps;         /* # of majormap entries available */

#define board(X)        (majormap[getmajor(X)].cnf_minorp[baseidx(X)].cnf_bd)
#define unit(X)         (majormap[getmajor(X)].cnf_minorp[baseidx(X)].cnf_drv)
#define valid_dev(X)    ((majormap[getmajor(X)].cnf_minorp!=NULL) && \
			 (majormap[getmajor(X)].cnf_minorp[baseidx(X)].cnf_valid))
#define driver_idx(X)   (majormap[getmajor(X)].cnf_minorp[baseidx(X)].cnf_drvidx)
#define dcbptr(X)       (majormap[getmajor(X)].cnf_minorp[baseidx(X)].cnf_dcb)
#define dpbptr(C,X)     ((C)->dcb_dpbs[unit(X)])

/*
 * Here's how interrupts work...
 * Each driver, as it is processing its configuration table, builds up
 * dcb's -- one per controller, possibly shared between multiple drivers.
 * It is assumed that a controller can generate up to GDEV_CTL_INT unique
 * interrupts (each on a different vector, but with the SAME intpri).
 * ALL these interrupt vectors are pointed at the gendev_intr routine.
 * This routine looks at the gdev_int_routines table using the interrupt
 * level as an index, and finds a list of controllers using that interrupt,
 * as well as an index into that controller's interrupt table for the level
 * (in case a controller generates more than one interrupt).  For non-multi-
 * thread controllers, the drv_int[int_idx] routine for each driver using
 * the controller is called in turn.  These low-level hardware interrupt
 * routines get the dcb and the int_idx as arguments.  If any of them return
 * non-NULL values, then the drv_drvint routine routine for that driver is
 * called with the dcb and the returned dpb as arguments.  For multi-thread
 * controllers, the dcb_mastint routine is called.  It must do whatever
 * initialization is required and call the other low-level driver interrupt
 * handlers as needed.  For example, on a mailbox interface, the master
 * routine loops through the mailboxes, and calls each handler on a per-
 * mailbox basis.  It is the responsibility of each of these low-level
 * driver interrupt routines to invoke the drv_drvint routine with the
 * passed dcb and the dpb for the device whenever a completion or error
 * condition is noted.
 */

struct  gdev_int_entry  {
	struct gdev_ctl_block   *int_ctl;  /* -> dcb for this interrupt */
	ushort int_idx; /* index into drv_int tables for this interrupt */
	struct gdev_int_entry   *int_link; /* link to next interrupt entry */
	};

extern  struct  gdev_int_entry  *gdev_int_routines[];   /* declared in space.c */
extern  ushort  gdev_next_int;  /* next int entry to allocate */
extern  struct  gdev_int_entry  gdev_int_entries[];     /* Pool decl'd in space.c */


/*
 * some useful macros...
 */

#define blk_to_sect(BUFP, DPBP)  (daddr_t) \
				 (((DPBP)->dpb_blkshft >= 0) ? \
				  ((BUFP)->b_blkno >> (DPBP)->dpb_blkshft) : \
				  ((BUFP)->b_blkno << (0 - (DPBP)->dpb_blkshft)))
#define sect_to_blk(SECT, DPBP) (daddr_t) \
				 (((DPBP)->dpb_blkshft >= 0) ? \
				  ((SECT) << (DPBP)->dpb_blkshft) : \
				  ((SECT) >> (0 - (DPBP)->dpb_blkshft)))

#define DSK_EEOF        127     /* for signalling EOF in b_error */

/*
 * The following are used in genblklib.c and space.c for allocating
 * memory areas...
 */

#define GDEV_MEM_GRAN   256     /* Memory allocation granularity (bytes) */
#define GDEV_MEM_BITS   32      /* Number of bits in a section bitmap */
#define GDEV_MEM_MAXBLK 8       /* Maximum # of sections to allocate */

struct  gdev_mem_block  {
	paddr_t dmb_baseaddr;   /* Base address of memory section */
	ushort  dmb_maxhole;    /* Size of biggest hole (in GDEV_MEM_GRAN units */
	ulong   dmb_bitmap;     /* The allocation bitmap for the section */
	};

extern  ushort  gdev_nextmem;   /* Number of currently allocated blocks */
extern  struct  gdev_mem_block  gdev_mem_blocks[];      /* The actual pool. */
extern  ushort  gdev_mem_lock;  /* Lock for memory map */
extern  ushort  gdev_mem_gran;  /* set in space.c */
extern  ushort  gdev_mem_maxblk; /* set in space.c */

/*
 * definitions of some useful routines in gendev.c...
 */

extern paddr_t gdev_getmem();   /* Get physically contiguous kernel memory */
extern void gdev_relmem();      /* Release previously allocated memory */
extern void gdev_getexcl();     /* Get exclusive access to controller */
extern void gdev_relexcl();     /* Release exclusive controller access */
extern struct drq_entry *getdrq();      /* get an empty drq_entry */
extern struct drq_entry *reldrq();      /* release a drq_entry to pool */
extern void reldrq_list();      /* release a list of drq_entries */
extern void gdev_xferok();      /* indicate [partial] transfer complete */
extern gdev_dcbp gdev_shrdcb(); /* get a [possibly shared] gdev_ctl_block */
extern void gdev_filldcb();     /* add values from cfg_entry to a dcb */
extern int gdev_cplerr();       /* handle device completion or error */
