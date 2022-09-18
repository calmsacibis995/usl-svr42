/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/sd01.c	1.44"
#ident	"$Header: miked 4/6/92, jand 4/1/92$"

#include	<util/types.h>
#include	<io/vtoc.h>
#include	<io/target/fdisk.h>	/* Included for 386 disk layout */
#include	<io/mkdev.h>		/* Included for MAXMAJ		*/

#include	<util/param.h>

#include	<svc/errno.h>
#include	<fs/buf.h>		/* Included for dma routines   */
#include	<svc/bootinfo.h>	/* Included to check boot device */
#include	<io/elog.h>
#include	<io/open.h>
#include	<io/conf.h>
#include	<mem/kmem.h>		/* Included for DDI kmem_alloc routines */

#include	<proc/cred.h>		/* Included for cred structure arg   */
#include	<proc/proc.h>		/* Included for cred structure arg   */
#include	<proc/user.h>
#include	<io/uio.h>		/* Included for uio structure argument*/
#include	<util/cmn_err.h>
#include	<util/debug.h>

#include	<io/target/altsctr.h>
#include	<io/target/alttbl.h>
#include	<io/target/sdi_edt.h>
#include	<io/target/sdi.h>
#include	<io/target/dynstructs.h>
#include	<io/target/scsi.h>
#include	<io/target/sdi_edt.h>
#include	<io/target/sd01.h>
#include	<io/target/sd01_ioctl.h>

#include	<util/mod/moddefs.h>

#include	<io/ddi.h>

#define		DRVNAME	"sd01 - hard disk target driver"

STATIC	int	sd01_load(), sd01_unload();
void	sd01rinit(), sd01init(), sd01start();

MOD_DRV_WRAPPER(sd01, sd01_load, sd01_unload, NULL, DRVNAME);

static	int	mod_dynamic = 0;
static	int	rinit_flag = 0;
struct owner	*ownerlist = NULL;

STATIC int
sd01_load()
{
	mod_dynamic = 1;
	sd01init();
	sd01start();
	return(0);
}

STATIC int
sd01_unload()
{
	return(EBUSY);
}

struct sd01dyn_bbh sd01db = {0, NULL, NULL, NULL, 0, 0, 0, 0};

#ifdef SD01_DEBUG
#define HWREA   0x00001000
#define DXALT   0x00002000
#define DCLR    0x00004000
#define DKADDR	0x00008000
#define DKD	0x00010000
#define DBM	0x00020000
#define DSAR	0x00040000
#define STFIN   0x80000000

uint	sd01alts_debug = 0x28000;
daddr_t	sd01alts_badspot = 0;
#define DK_SAMPLE 	100
#define DK_MAXIO	(45*8)
#define dk_lbolt	sd01db.db_fill[0]
#define dk_bcnt		sd01db.db_fill[1]
#endif

struct 	sb *sd01_fltsbp;		/* SB exclusively for RESUMEs 	*/
struct 	resume sd01_resume;		/* Linked list of disks waiting */

char   	sd01name[NAMESZ];			/* sd01flterr & sd01logerr dev info */
char 	sd01instbl[MAXMAJ];		/* Major number instance table	*/
int 	sd01_diskcnt;			/* Number of disks configured	*/
int 	sd01_tccnt;			/* Number of controllers        */

/* Function table of contents	*/
void		sd01init();		/* Init entry point  - 0dd01000	*/
struct	job	*sd01getjob();		/* Get job structure - 0dd02000	*/
void		sd01freejob();		/* Free job structure- 0dd03000	*/
void  		sd01send(); 		/* Sends jobs from Q - 0dd04000	*/
void  		sd01sendt(); 		/* Send via timeout  - 0dd05000	*/
void		sd01strat1();		/* Level 1 strategy  - 0dd06000	*/
void		sd01strat0();		/* Level 0 strategy  - 0dd06500	*/
void		sd01strategy(); 	/* Strategy entry    - 0dd07000	*/
int		sd01close();		/* Close entry point - 0dd08000	*/
int		sd01read();		/* Read entry point  - 0dd09000	*/
int		sd01write();		/* write entry point - 0dd0a000	*/
int		sd01open1();		/* Core open function- 0dd0b000	*/
int		sd01open(); 		/* Open entry point  - 0dd0c000	*/
void  		sd01done(); 		/* Completion routine- 0dd0d000	*/
void		sd01comp1();		/* Complete disk job - 0dd0e000	*/
void		sd01intf();		/* HA SFB int routine- 0dd0f000	*/
void		sd01logerr();		/* Prints error msg. - 0dd10000	*/
void  		sd01retry(); 		/* Retry failed job  - 0dd11000	*/
void		sd01intn();		/* Normal int routine- 0dd13000	*/
int		sd01phyrw();		/* Do physical I/O   - 0dd14000	*/
int    		sd01cmd();		/* Send normal cmd   - 0dd15000	*/
int		sd01wrtimestamp(); 	/* Write time stamp  - 0dd18000	*/
int		sd01ioctl();		/* Ioctl entry point - 0dd1a000	*/
int		sd01print();		/* Print entry point - 0dd1b000	*/
void    	sd01batch(); 		/* Start new batch   - 0dd1c000	*/
void		sd01insane_dsk();	/* Display warning   - 0dd1f000	*/
int		sd01vtoc_ck();		/* Check VTOC        - 0dd20000	*/
void		sd01szsplit();	 	/* Splits large jobs - 0dd22000	*/
void		sd01flt();		/* HA flt routine    - 0dd23000	*/
void		sd01flts();		/* Start flt recovery- 0dd24000	*/
void		sd01ints();		/* SFB int handler   - 0dd25000	*/
void  		sd01intrq(); 		/* Sense int handler - 0dd26000	*/
void  		sd01intres();		/* Res. int handler  - 0dd27000	*/
void  		sd01resume();		/* Resume disk Q     - 0dd28000	*/
void  		sd01flterr(); 		/* Clean up errors   - 0dd29000	*/
void  		sd01qresume(); 		/* Checks SB is busy - 0dd2a000	*/
void  		sd01fltjob(); 		/* Eval. sense data  - 0dd2b000	*/
int		sd01part_ck();		/* Check partitions  - 0dd2c000	*/
void  		sd01hdelog();		/* Reassign blocks   - 0dd2d000	*/
void		sd01start();		/* Not used          - 0dd2f000	*/
void		sd01lognberr();		/* Not used 	     - 0dd30000	*/
int		sd01config();		/* Get configuration - 0dd31000 */
int		sd01icmd();		/* Send immed. cmd   - 0dd32000	*/
void  		sd01intb(); 		/* HDE Int handler   - 0dd33000	*/
int 		sd01addr();		/* Get device address- 0dd34000	*/
int		sd01slice();		/* Get slice number  - 0dd35000	*/
void		sd01intalts();		/* alts int handler  - 0dd36000 */
int		sd01remap_altsctr();	/* SW remap altsctr  - 0dd37000 */
int		sd01remap_altsec();	/* SW remap altsec   - 0dd38000 */
void 		sd01intfq();		/* SFB int hdl	     - 0dd39000 */
int		sd01dkd();	

extern sdi_icmd();
extern sdi_send();
extern sdi_blkio();			/* Partial block read/write	*/
extern sd01_hiwat;			/* hiwater mark for jobs to HBA */
extern struct head	lg_poolhead;	/* head for dync struct routines */

int sd01devflag = D_NEW | D_NOBRKUP;	/* Indicate new style driver */

/*
 * Messages used by bad block handling.
 */

static char *hde_mesg[] = {
        "!Disk Driver: Soft read error corrected by ECC algorithm: block %d",

	"Disk Driver: A potential bad block has been detected (block %d)\n\
in a critical area of the hard disk. If this block goes bad, the UNIX System\n\
might fail during the next reboot.  Please backup your system now.",

	"!Disk Driver: A potential bad block has been detected (block %d).\n\
The drive is out of spare blocks for surrogates. The block will be \n\
remapped to the alternate sector/track partition.",

	"!Disk Driver: A potential bad block has been detected (block %d).\n\
The controller can not reassign a surrogate block. The block will be \n\
remapped to the alternate sector/track partition.",

	"Disk Driver: An alternate block has been assigned to block %d.",

	"Disk Driver: Block %d is not writable.\nData of this block has been lost.",

	"Disk Driver: A bad block (block %d) has been detected in a critical\n\
area of the hard disk.  The disk drive is not useable in this state.\n\
The drive may need a low level format or repair.",

	"Disk Driver: Data from block %d is not readable.\nData of this block \
has been lost.  An alternate block has\nbeen assigned.",

	"!Disk Driver: Data from block %d is not readable.\nData of this block \
has been lost. Trying to reassign an alternate block.",

	"!Disk Driver: The drive is out of spare blocks for surrogates.\n\
Block %d will be remapped to alternate sector/track partition.",

	"!Disk Driver: The controller can not reassign a surrogate block.\n\
Block %d will be remapped to alternate sector/track partition.",

	"!Disk Driver: Block %d isn't writable. Failed to initialize block.",

	"Disk Driver: Block %d has been successfully remapped to an alternate.",

};

/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver                                  
/* DPR provides levels 0 - 9 of debug information.                  
/*      0: No debugging.
/*      1: Entry points of major routines.
/*      2: Exit points of some major routines. 
/*      3: Variable values within major routines.
/*      4: Variable values within interrupt routine (inte, intn).
/*      5: Variable values within sd01logerr routine.
/*      6: Bad Block Handling
/*      7: Multiple Major numbers
/*      8: - 9: Not used
/*============================================================================*/

#define O_DEBUG         0100

#ifdef DEBUG
#define DPR(l)          if (Sd01_debug[l]) printf
#endif


/* %BEGIN HEADER
 * %NAME: sd01init - Initialize the SCSI disk driver. - 0dd01000
 * %DESCRIPTION:
	This function allocates and initializes the disk driver's data 
	structures.  This function also initializes the disk drivers device 
	instance array. An instance number (starting with zero) will be assigned
	for each set of block and character major numbers. Note: the
	system must allocate the same major number for both the block
	and character devices or disk corruption will occur.
	This function does not access any devices.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The disk queues are set empty and all partitions are marked as
	closed. 
 * %RETURN VALUES: None
 * %ERROR 8dd01001
	The disk driver can not determine equipage.  This is caused by
	insufficient virtual or physical memory to allocate the driver
	Equipped Device Table (EDT) data structure.
 * %END ERROR
 * %ERROR 8dd01002
	The disk driver is not fully configured.  This is caused by insufficient
	virtual or physical memory to allocate internal driver structures.
 * %END ERROR
 * %ERROR 8dd01003
	The disk driver space.c file is not valid.  This is caused by specifying
	an invalid number of subdevices in Sd01_nodes.  The driver does not
	support less then 8 or greater then 16 partitions per drive (OBSOLETE
	ERROR).
 * %END ERROR
 * %END HEADER */
void
sd01init()
{
	struct 	disk 	*sd01_dp2, *diskbase;	/* Temp. disk pointer 	*/
	struct	owner	*op;
	struct drv_majors drv_maj;

	int tc_edtsz,				/* EDT size (in bytes)	*/
	    disk_memsz,				/* Disk size (in bytes) */
	    i,					/* Loop index - level 1	*/
	    j,					/* Loop index - level 2	*/
	    lu,					/* LU num. if configured*/
	    tc;					/* TC num. if configured*/

#ifdef DEBUG
	DPR (1)("\n\t\tSD01 DEBUG DRIVER INSTALLED\n");
#endif
	
/* Call HAD to build SCSI edt */
	sdi_init();

/* Setup the linked list for Resuming LU queues */
 	sd01_resume.res_head = (struct disk *) &sd01_resume;
 	sd01_resume.res_tail = (struct disk *) &sd01_resume;

	sd01_fltsbp = sdi_getblk();

/* Init array to indicate unsupported major numbers   */
        for (i = 0; i < MAXMAJ; i++)
                sd01instbl[i] = DKNOMAJ;

/* Assign instance number for supported major numbers */
        for (i = 0; i < Sd01_cmajors; i++)
                sd01instbl[Sd01_majors[i].c_maj] = i;

	drv_maj.b_maj = drv_maj.c_maj = 0;
	ownerlist = sdi_doconfig(SD01_dev_cfg, SD01_dev_cfg_size,
				"SD01 Disk Driver", &drv_maj, sd01rinit);

	sd01_diskcnt = 0;
	for (op = ownerlist; op; op = (struct owner *)op->res1) {
#ifdef DEBUG
	printf("disks claimed  sd01_discnt = %x\n", sd01_diskcnt);
#endif
		sd01_diskcnt++;
	}

#ifdef DEBUG
	printf("%d disks claimed\n", sd01_diskcnt);
#endif

	if(!sd01_diskcnt) {
		sd01_diskcnt = DKNOTCS;
		return;
	}

	/* Allocate the disk structures */
	disk_memsz = sd01_diskcnt * sizeof(struct disk);
	if((diskbase = (struct disk *) kmem_zalloc(disk_memsz,
	   (mod_dynamic ? KM_SLEEP : KM_NOSLEEP))) == NULL)	{
		cmn_err(CE_WARN, "Disk Driver: Insufficient memory to configure driver.");
		sd01_diskcnt = DKNOTCS;
		return;
	}

/* Initalize the disk structures */
	sd01_dp2 = diskbase;
	for(tc = 0, op = ownerlist; tc < sd01_diskcnt;
			tc++, op=(struct owner *)op->res1, sd01_dp2++) {
					/* Initialize the queue ptrs */
		sd01_dkinit(sd01_dp2, op);
#ifdef DEBUG
	printf("op 0x%x ", op);
	printf("edt 0x%x ", op->edtp);
	printf("hba %d scsi id %d lun %d\n",op->edtp->hba_no,op->edtp->scsi_id,op->edtp->lun);
#endif
		Sd01_dp[tc] = sd01_dp2;
	}
}

/*
** Function name: sd01rinit()
** Description:
**	Called by sdi to perform driver initialization of additional
**	devices found after the dynamic load of HBA drivers. 
**	This function does not access any devices.
*/

void
sd01rinit()
{
	register struct disk  *dp, *odp;	/* disk pointer	 */
	struct disk **Sd01_dp_new;
	struct	owner	*ohp, *op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  disksz, diskptrsz,		/* disk size (in bytes) */
	     new_diskcnt,		/* number of additional devs found*/
	     old_diskcnt,		/* previous number of devs found */
	     diskcnt,			/* total number of devs found*/
	     i,j,			/* temp loop variables */
	     found,			/* seach flag variable */
	     prevpl;			/* prec process level - for splx */

	/* set rinit_flag to prevent any access to sd01_disk while were */
	/* updating it for new devices and copying existing devices      */
	rinit_flag = 1;
	diskcnt= 0;

	drv_maj.b_maj = drv_maj.c_maj = 0;
	ohp = sdi_doconfig(SD01_dev_cfg, SD01_dev_cfg_size,
				"SD01 DISK Driver", &drv_maj, NULL);
	for (op = ohp; op; op = (struct owner *)op->res1) {
		diskcnt++;
	}
#ifdef DEBUG
	printf("sd01rinit %d disks claimed, previously %d\n", diskcnt,sd01_diskcnt);
#endif
	/* Check if there are additional devices configured */
	if (sd01_diskcnt == DKNOTCS) {
		old_diskcnt = 0;
		new_diskcnt = diskcnt;
	}
	else {
		old_diskcnt = sd01_diskcnt;
		new_diskcnt = diskcnt - old_diskcnt;
	}
	if (new_diskcnt == 0) {
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Allocate the new disk structures and temporary disk ptr array
	 */
	disksz = new_diskcnt * sizeof(struct disk);
        if ((base = kmem_zalloc(disksz,
	   (mod_dynamic ? KM_SLEEP : KM_NOSLEEP))) == NULL)	{
		cmn_err(CE_WARN,
			"DISK Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",disksz);
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	diskptrsz = diskcnt * sizeof(struct disk *);
        if ((Sd01_dp_new = (struct disk **)kmem_zalloc(diskptrsz,
	   (mod_dynamic ? KM_SLEEP : KM_NOSLEEP))) == NULL)	{
		cmn_err(CE_WARN,
			"DISK Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",diskptrsz);
		kmem_free(base, disksz);
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Initialize the disk structures
	 */
	prevpl = spl5();
	dp = (struct disk *)base;
	for(i=0, op=ohp; i<diskcnt; i++, op=(struct owner *)op->res1) {
		found = 0;
		for (j=0; j < old_diskcnt; j++) {
			odp = Sd01_dp[j];
			if((odp->dk_addr.sa_lun == op->edtp->lun) &&
	    		   (odp->dk_addr.sa_fill == 
			   ((op->edtp->hba_no << 3)|(op->edtp->scsi_id)))){
				found = 1;
				Sd01_dp_new[i] = odp;
				break;
			}
		}
		if (found)
			continue;

		/* Its a new disk device so init disk struct */
		sd01_dkinit(dp, op);
		Sd01_dp_new[i] = dp++;
#ifdef DEBUG
	printf("NEWdisk: op 0x%x edt 0x%x hba %d ",op,op->edtp,op->edtp->hba_no);
	printf("scsi id %d lun %d\n",op->edtp->scsi_id,op->edtp->lun);
#endif
	}

	/* Move new disk pointer array to the original array of disk pointers */
	for(i=0; i<diskcnt; i++) {
		Sd01_dp[i] = Sd01_dp_new[i];
	}
	kmem_free(Sd01_dp_new, diskptrsz);

	sd01_diskcnt = diskcnt;
	splx(prevpl);
	rinit_flag = 0;
	wakeup((caddr_t)&rinit_flag);
}

sd01_dkinit(dp, op)
struct disk	*dp;
struct	owner	*op;
{
	int j;
	dp->dk_forw  = (struct job *) dp;
	dp->dk_back  = (struct job *) dp;
	dp->dk_next  = (struct job *) dp;
	dp->dk_batch = (struct job *) dp;
	dp->dk_outsz_lo   = SD01_OUTSZ_LO;
	dp->dk_outsz_hi   = sd01_hiwat;
	dp->dk_parms.dp_secsiz    = KBLKSZ;
	dp->dk_stat.pttrack      = dp->dk_stat.ptrackq;
	dp->dk_stat.endptrack    = 
		&dp->dk_stat.ptrackq[NTRACK];
	dp->dk_iotype = op->edtp->iotype;
	dp->dk_addr.sa_lun = op->edtp->lun;
	dp->dk_addr.sa_fill = (op->edtp->hba_no << 3) | (op->edtp->scsi_id);
	dp->dk_spec = sdi_findspec(op->edtp, sd01_dev_spec);
}

/* %BEGIN HEADER
 * %NAME: sd01getjob - Allocate a job structure. - 0dd02000
 * %DESCRIPTION:
	This function calls sdi_get for struct to be used as a job
	structure. If dyn alloc routines cannot alloc a struct the
	sdi_get routine will sleep until a struct is available. Upon a
	successful return from sdi_get, sd01getjob will then
	get a \fIscb\fR from the sdi using sdi_getblk.
 * %CALLED BY: sd01strategy and mdstrategy
 * %SIDE EFFECTS: 
	A job structure and SCSI control block is allocated.
 * %RETURN VALUES: 
	A pointer to the allocated job structure.
 * %END HEADER */
struct job *
sd01getjob(dk)
register struct disk *dk;
{
	register struct job *jp;

	jp = (struct job *) sdi_get(&lg_poolhead, KM_SLEEP);
	/* Get an SCB for this job */
	jp->j_cont = sdi_getblk();
	jp->j_cont->sb_type = SCB_TYPE;
	jp->j_cont->SCB.sc_dev = dk->dk_addr;
	return(jp);
}
/* %BEGIN HEADER
 * %NAME: sd01freejob - Free a disk job structure. - 0dd03000
 * %DESCRIPTION: 
	The routine calls sdi_freeblk to return the scb attached to 
	the job structure driver. This function then returns the job 
	structure to the dynamic allocation free list by calling sdi_free. 
 * %CALLED BY: mddone sd01done
 * %SIDE EFFECTS:
	Allocated job and scb structures are returned.
 * %RETURN VALUE None
 * %END HEADER */
void
sd01freejob(jp)
register struct job *jp;
{
	sdi_freeblk(jp->j_cont);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}
/* %BEGIN HEADER
 * %NAME: sd01send - Sends jobs from the work queue to the host adapter. - 0dd04000
 * %DESCRIPTION: 
	This function sends jobs to the host adapter driver.  It will send
	as many jobs as available or the maximum number required to keep the
	logical unit busy.  If the job cannot be accepted by the host
	adapter driver, the function will reschedule itself via the timeout
	mechanizism. This routine must be called at slp6.
 * %CALLED BY: sd01cmd sd01strat1 and sd01intn
 * %SIDE EFFECTS: 
	Jobs are sent to the host adapter driver.
 * %RETURN VALUES: None
 * %ERROR 8dd04001
	The host adapter rejected a request from the SCSI disk driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %END HEADER */
void
sd01send(dk, reqcnt)
register struct disk *dk;
int	 reqcnt;
{
	register int sendret;		/* sd_send return value 	*/ 
	register struct job *jp;	/* Job which caused send error 	*/
	int	 prev_level;
	int 	 i;
	int	 sendcnt; 
	time_t	 lbolt_time; 
	time_t	 since_lasttime; 	/* elapsed time since last perf timestamp */
	char	 ops;			/* operation being sent */

	sendcnt = dk->dk_outsz_hi;
	
#ifdef SD01_DEBUG
	if ((sd01alts_debug&DKADDR) 
		printf("sd01send: ipl= %d \n", prev_level);
#endif
	if (dk->dk_state & DKSEND) {
		dk->dk_state &= ~DKSEND;
		untimeout(dk->dk_sendid);
	}
	while (dk->dk_outcnt<sendcnt && 
		((dk->dk_next!=(struct job *)dk)||dk->dk_syncq_fw)) {
		prev_level = spl5();

		if (dk->dk_syncq_fw) {
			dk->dk_syncq_curl--;
			jp = dk->dk_syncq_fw;
			dk->dk_syncq_fw = jp->j_forw;
			if (jp == dk->dk_syncq_bk)
				dk->dk_syncq_bk = (struct job *)NULL;
		} else {
			jp = dk->dk_next;
			dk->dk_next = jp->j_forw;
	
			if (jp == dk->dk_batch) {/* Start a new batch 	*/
				dk->dk_batch = (struct job *) dk;
				dk->dk_state ^= DKDIR;
			}

/* 			Remove job from the queue			*/
			jp->j_forw->j_back = jp->j_back; 
			jp->j_back->j_forw = jp->j_forw;
		}

		dk->dk_count--;
		dk->dk_outcnt++;

/* 		Update performance stats 				*/
		dk->dk_stat.ios.io_qcnt--;
		ops = (char) *jp->j_cont->SCB.sc_cmdpt;
		if (ops == SM_READ || ops ==  SM_WRITE) {
			drv_getparm(LBOLT, (ulong *)&lbolt_time);
			if (dk->dk_stat.busycnt != 0) {
				since_lasttime = lbolt_time -
							dk->dk_stat.lasttime;
				dk->dk_stat.io_act += since_lasttime;
				dk->dk_stat.io_resp += since_lasttime *
							dk->dk_stat.busycnt;
			}
			dk->dk_stat.busycnt++;
			dk->dk_stat.lasttime = lbolt_time;
		}
		splx(prev_level);

/* 		Swap bytes in the address field 			*/
		if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
			jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
		else {
			jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
			jp->j_cmd.cm.sm_len  = (short)sdi_swap16(
						jp->j_cmd.cm.sm_len);
		}

		if ((sendret = sd01docmd(sdi_send, jp->j_dk, jp->j_cont)) != 
			SDI_RET_OK) {
			if (sendret == SDI_RET_RETRY) {
				dk->dk_outcnt--;
				dk->dk_count++;
				dk->dk_stat.ios.io_qcnt++;
				if (ops == SM_READ || ops ==  SM_WRITE) {
					dk->dk_stat.busycnt--;
				}
/*				Swap bytes back in the address field	*/
				if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
					jp->j_cmd.cs.ss_addr = sdi_swap16(
							jp->j_cmd.cs.ss_addr);
				else {
					jp->j_cmd.cm.sm_addr = sdi_swap32(
							jp->j_cmd.cm.sm_addr);
					jp->j_cmd.cm.sm_len  = (short)sdi_swap16
							(jp->j_cmd.cm.sm_len);
				}
/*				relink back to the queue		*/
				sd01fifo(jp,dk);

/*				do not hold the job queue busy any more */
				dk->dk_state &= ~(DKBUSYQ|DKSYNCDONE);
/* 				Call back later 			*/
				if (!(dk->dk_state & DKSEND)) {
					dk->dk_state |= DKSEND;
					dk->dk_sendid = timeout(sd01sendt,
						(caddr_t) dk, 1);
				}
				return;
			} else {
#ifdef SD01_DEBUG
				cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd04001");
#endif
				sd01comp1(jp);
			}
		}
		dk->dk_state &= ~DKSYNCDONE;
	}
	
/*	do not hold the job queue busy any more				*/
	dk->dk_state &= ~(DKBUSYQ|DKSYNCDONE);
}

/* %BEGIN HEADER
 * %NAME: sd01sendt - Send function timeout. - 0dd05000
 * %DESCRIPTION: 
	This function call \fIsd01send\fR after it turns off the DKSEND
	bit in the disk status work.
 * %CALLED BY: timeout
 * %SIDE EFFECTS: 
	The send function is called and the record of the pending
	timeout is erased.
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01sendt(dk)
register struct disk *dk;
{
	register int	 oldpri;

#ifdef DEBUG
        DPR (1)("sd01sendt: (dk=0x%x) ", dk);
#endif

	dk->dk_state &= ~DKSEND;
/*	exit if job queue is either busy or has been suspended		*/
	if (dk->dk_state & (DKBUSYQ|DKSUSP))
		return;
	if ((sd01db.db_flag & (DBBH_ARM|DBBH_SEND)) == (DBBH_ARM|DBBH_SEND)) {
		if (!(dk->dk_state & DKPENDQ)) {
			dk->dk_state |= DKPENDQ;
			dk->dk_waitp = sd01db.db_waitdk;
			sd01db.db_waitdk = dk;
		}
		if (!(sd01db.db_flag & DBBH_BUSY))
			wakeup((caddr_t)sd01dkd);
	} else {
		dk->dk_state |= DKBUSYQ;
		oldpri = spl0();
		sd01send(dk, -1);
		splx(oldpri);
	}

#ifdef DEBUG
        DPR (2)("sd01sendt: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01strat1 - Level 1 (Core) strategy routine. - 0dd06000
 * %DESCRIPTION: 
	This function takes the information included in the job
	structure and the buffer header, and creates a SCSI bus
	request.  The request is queued according to the elevator
	algorithm.  This routine may be called at interrupt level. 
	The buffer and mode fields are filled in by the calling 
	functions.  If the partition argument is equal to 16 or
	greater the block address is assumed to be physical.
 * %CALLED BY: sd01strategy, sd01phyrw, mdstrategy, and mddone
 * %SIDE EFFECTS: 
	A job is queued for the disk.
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01strat1(jp, dk, bp)
register struct job *jp;
struct disk *dk;
buf_t *bp;
{
	register struct scb *scb;
	register struct scm *cmd;
	struct scsi_iotime *stat;
	int prev_level;
	extern void sd01intn();

#ifdef DEBUG
        DPR (1)("sd01strat1: jp=0x%x ", jp);
#endif
	
	scb = &jp->j_cont->sb_b.b_scb;
	cmd = &jp->j_cmd.cm;
	stat = &dk->dk_stat;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	scb->sc_datapt = jp->j_memaddr;
	scb->sc_datasz = jp->j_seccnt << BLKSHF(dk);
	scb->sc_cmdpt = SCM_AD(cmd);
	sdi_translate(jp->j_cont, bp->b_flags, bp->b_proc);

/* 	Fill in the scb for this job 					*/
	if (bp->b_flags & B_READ) {
		cmd->sm_op = SM_READ;
		scb->sc_mode = SCB_READ;
	} else {
		cmd->sm_op = SM_WRITE;
		scb->sc_mode = SCB_WRITE;
	}
	cmd->sm_lun = dk->dk_addr.sa_lun;
	cmd->sm_res1 = 0;
	cmd->sm_addr = jp->j_daddr;
	cmd->sm_res2 = 0;
	cmd->sm_len = jp->j_seccnt;
	cmd->sm_cont = 0;
	scb->sc_cmdsz = SCM_SZ;
	/* The data elements are filled in by the calling routine */
	scb->sc_link = 0;
	scb->sc_time = JTIME;
	scb->sc_int = sd01intn;
	scb->sc_wd = (long) jp;
	scb->sc_dev = dk->dk_addr;	
	
/* 	Add the job to the queue according to the elevator algorithm 	*/
	dk->dk_state |= DKBUSYQ;
	dk->dk_count++;

/* 	Queue is empty so no elevator 					*/
	if(dk->dk_next == (struct job *) dk) {
		prev_level = spl5();
		jp->j_forw = (struct job *) dk;
		jp->j_back = dk->dk_back;
		dk->dk_forw = jp;
		dk->dk_back = jp;
		dk->dk_next = jp;
		splx(prev_level);
	} else {
#ifndef SORTSYNCQ
/*		use fifo Q if sync write on heavy Q			*/
		if ((dk->dk_count > 30) && !(bp->b_flags & (B_READ|B_ASYNC)))
			sd01fifo(jp,dk);
#else
/*		use sorted sync Q 					*/
		if (!(bp->b_flags & B_ASYNC))
			sd01fifo(jp,dk);
#endif
		else 
			sd01elevator(jp, dk);
		
	}

/* 	Update performance data 					*/
	stat->ios.io_ops++;
	stat->ios.io_qcnt++;
	if (dk->dk_count > stat->maxqlen)
		stat->maxqlen = dk->dk_count;


	sd01send(dk, (int)-1);
	

#ifdef DEBUG
        DPR (2)("sd01strat1: - exit ");
#endif
}

sd01fifo(jp, dk)
register struct job *jp;
register struct disk *dk;
{
	register struct job *curjp;
	register struct job *nxtjp;
	register daddr_t jaddr;
	int prev_level;

	prev_level = spl5();
	dk->dk_syncq_curl++;
	if (dk->dk_syncq_curl > dk->dk_syncq_maxl)
	dk->dk_syncq_maxl = dk->dk_syncq_curl;

#ifndef SORTSYNCQ
/*	append to the sync queue - FIFO			*/
	if (dk->dk_syncq_fw == (struct job *) NULL) {
		dk->dk_syncq_fw = jp;
		jp->j_back = (struct job *) NULL;
	} else {
		dk->dk_syncq_bk->j_forw = jp;
		jp->j_back = dk->dk_syncq_bk;
	}
	jp->j_forw = (struct job *) NULL;
	dk->dk_syncq_bk = jp;
#else
	curjp = dk->dk_syncq_fw;
/*	append to the sync queue - SORTED			*/
	if (curjp == (struct job *) NULL) {
		dk->dk_syncq_fw = jp;
		jp->j_back = (struct job *) NULL;
		jp->j_forw = (struct job *) NULL;
	} else {
		jaddr = jp->j_daddr;
		for ( ;nxtjp=curjp->j_forw; curjp=nxtjp) {
			if ((curjp->j_daddr <= jaddr)&&(jaddr<nxtjp->j_daddr) ||
			    (curjp->j_daddr >= jaddr)&&(jaddr>nxtjp->j_daddr))
				break;
		}
		jp->j_forw = nxtjp;
		jp->j_back = curjp;
		curjp->j_forw = jp;
		if (nxtjp)
			nxtjp->j_back = jp;
	}
#endif
	splx(prev_level);

}


sd01elevator(jp, dk)
struct job *jp;
register struct disk *dk;
{

	register struct job *jp1;
	register daddr_t jaddr;
	int prev_level;

	prev_level = spl5();
	jp1 = dk->dk_next;
	jaddr = jp->j_daddr;

/*	check if elevator algorithm has been disablei - end Q	*/
	if (dk->dk_state & DKEL_OFF) 
		jp1 = (struct job *) dk;
/* 	check if next batch will go up 				*/
	else if (dk->dk_state & DKDIR) {
/*		check job can be grouped into current batch 
 *		which is moving down.
 */
		if (jp1->j_daddr >= jaddr) {
			for (; jp1 != dk->dk_batch; jp1 = jp1->j_forw) {
				if (jp1->j_daddr < jaddr) 
					break;
			}
		} else {
			for(jp1= dk->dk_batch; jp1 != (struct job *) dk;
				jp1 = jp1->j_forw) {
				if (jp1->j_daddr > jaddr)
					break; /* This is our floor */
			}
			if (jp1 == dk->dk_batch)
				dk->dk_batch = jp;
		}
/* 	then next batch will go down 				*/
	} else {
/*		check job can be grouped into current batch 
 *		which is moving up.
 */
		if (jp1->j_daddr <= jaddr) {
			for(; jp1 != dk->dk_batch; jp1 = jp1->j_forw) {
				if (jp1->j_daddr > jaddr) 
					break;
			}
		} else {
			for (jp1=dk->dk_batch; jp1 != (struct job *) dk;
			     jp1 = jp1->j_forw) {
				if (jp1->j_daddr < jaddr)
					break; /* This is our floor */
			}
			if (jp1 == dk->dk_batch)
				dk->dk_batch = jp;
		}
	}

/* 	Add the job into the queue 				*/
	jp->j_forw = (struct job *) jp1;
	jp->j_back = jp1->j_back;
	jp1->j_back->j_forw = jp;
	jp1->j_back = jp;
	if (jp1 == dk->dk_next)
		dk->dk_next = jp;
	splx(prev_level);

}

/* %BEGIN HEADER
 * %NAME: sd01strategy - Strategy routine for SCSI disks. - 0dd07000
 * %DESCRIPTION: 
	\fIsd01strategy\fR  determines the flow of control through
	restricted dma code, by checking the device's I/O capability,
	then sends the request on to \fIsd01strat0\fR.
 * %CALLED BY: kernel
 * %SIDE EFFECTS: 
	DMA devices have data moved to dmaable memory when necessary.
 * %RETURN VALUES: None 
 * %END HEADER */
void
sd01strategy(bp)
register buf_t *bp;
{
	register struct disk *dk;

	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	dk =  DKPTR(bp->b_edev);
	if( dk->dk_iotype & F_DMA ) {
		rdma_filter( sd01strat0, bp );
	} else {
		sd01strat0( bp );
	}
}

/* %BEGIN HEADER
 * %NAME: sd01strat0 - Level 0 Strategy routine for SCSI disks. - 0dd06500
 * %DESCRIPTION: 
	\fIsd01strat0\fR  generates the SCSI command needed to fulfill the
	I/O request.  The buffer pointer is passed from the kernel and
	contains the necessary information to perform the job.  Most of the
	work is done by \fIsd01strat1\fR.
 * %CALLED BY: \fIsd01strategy\fR
 * %SIDE EFFECTS: 
	The I/O statistics are updated for the device and an I/O job is
	added to the work queue. 
 * %RETURN VALUES: None 
 * %END HEADER */
void
sd01strat0(bp)
register buf_t *bp;
{
	register struct disk *dk;
	extern void sd01done();
	register daddr_t start;		/* Starting Logical Sector in part */
	register daddr_t last;		/* Last Logical Sector in partition */
	register daddr_t numblk;		/* # of Logical Sectors requested */
	register int part;
	int	 oldpri;
	int	 ret_val = 0;
	struct scsi_ad	*dev;
	
	dk =  DKPTR(bp->b_edev);
	part = DKSLICE(bp->b_edev);

	/*
	 * Reject the request if the partition has not been opened.
	 */
	if ((dk->dk_part_flag[part] & DKGEN) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}


#ifdef DEBUG
        DPR (1)("sd01strat0: (bp=0x%x) dk=0x%x ", bp, dk);
#endif

	if (dk->dk_vtoc.v_sanity != VTOC_SANE || dk->dk_vtoc.v_version > V_VERSION)
	{
		if (dk->dk_state & DKUP_VTOC && /* Was it just updated */
		(ret_val = sd01open1(getmajor(bp->b_edev), getminor(bp->b_edev), DKGEN)) == 0)
		{			/* Use open1 to read in the VTOC */
			dk->dk_state &= ~DKUP_VTOC;
		}
		else 			/* Fail the request */
		{
			/* Return EBUSY for reservation conflict */
			if (dk->dk_state & DKCONFLICT)
			{
				bp->b_error = EBUSY;
				dk->dk_state &= ~DKCONFLICT;
			}
			else if (ret_val > 0)
				bp->b_error = ret_val;
			else
			{
				if(bootinfo.bootflags != BF_FLOPPY)
					sd01insane_dsk(dk);
				bp->b_error = EIO;
			}
			dk->dk_state &= ~DKUP_VTOC;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
	}

	/*
	 * If this is a partial-sector transfer, then special
	 * handling is required.
	 */
	if (((bp->b_blkno & BLKMSK(dk)) != 0) ||
	    (bp->b_bcount % BLKSZ(dk) != 0)) {
		sdi_blkio(bp, BLKSEC(dk), sd01strategy);
		return;
	}

	/*
	*  The b_resid is initialized before the call to sd01szsplit.
	*  A problem was discovered that if the job exceeds the max size
	*  and must be broken down to smaller jobs by sd01szsplit,
	*  the b_resid field of the original buffer never gets initialized.
	*  Only the temporary buf structures used by sd01szsplit were
	*  initialized properly. It resulted in a successful job appearing
	*  to fail because the b_resid value was non-zero.
	*/
	bp->b_resid = 0;

 	dev = &dk->dk_addr;
 	if ((HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer) && 
	   (bp->b_bcount > HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer))
	{				/* The job is too big to  */
					/* handle all at once. */
		sd01szsplit(bp, dev, sd01strategy);
		return;
	}

	start = bp->b_blkno >> BLKSEC(dk);
	last = dk->dk_vtoc.v_part[part].p_size;
	numblk = (bp->b_bcount + BLKSZ(dk)-1) >> BLKSHF(dk);
	if (bp->b_flags & B_READ)    
	{                          
		if (start + numblk >  last)
		{
			if (start > last)
			{
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
				biodone(bp);
				return;
			}
			
			bp->b_resid = bp->b_bcount - ((last-start)<<BLKSHF(dk));
			if (bp->b_bcount == bp->b_resid)
			{	/* The request is done */
				biodone(bp);
				return;
			}
			else
				bp->b_bcount -= bp->b_resid;
		}
	}
	else	/* This is a write request */
	{
		if ((unsigned)start + numblk >  last) {
			/*
			 * Return an error if entire request is beyond
			 * the End-Of-Media.
			 */
			if (start > last) {
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
				biodone(bp);
				return;
			}
			
			/*
			 * The request begins exactly at the End-Of-Media.
			 * A 0 length request is OK.  Otherwise, its an error.
			 */ 
			if (start == last) {
				if (bp->b_bcount != 0) {
					bp->b_flags |= B_ERROR;
					bp->b_error = ENXIO;
				}
				biodone(bp);
				return;
			}

			/*
			 * Only part of the request is beyond the End-Of-
			 * Media, so adjust the counts accordingly.
			 */ 
			bp->b_resid = bp->b_bcount - ((last-start)<<BLKSHF(dk));
			bp->b_bcount -= bp->b_resid;
		}

		/* Make sure PD and VTOC are safe */
		if (dk->dk_vtoc.v_part[part].p_start <= dk->unixst + VTBLKNO  &&
			start <= dk->unixst + VTBLKNO && 
		        (ret_val = sd01vtoc_ck(dk, bp, part)))
			
		{
			bp->b_flags |= B_ERROR;
			bp->b_error = ret_val;
			biodone(bp);
			return;
		}

		if (dk->dk_vtoc.v_part[part].p_flag & V_RONLY)
		{	/* The parition is read only */
			bp->b_flags |= B_ERROR;
			bp->b_error = EACCES;
			biodone(bp);
			return;
		}
			
		if (dk->dk_vtoc.timestamp[part] != (time_t) 0)
		{
			if (sd01wrtimestamp(dk, part, 0) != 0)
			{				
				bp->b_flags |= B_ERROR;
				bp->b_error = ENODEV;
				biodone(bp);
				return;
			}
		}

	}

	/*
	 * If this is a PAGEIO request, then build the virtual address mapping.
	 */
	if (bp->b_flags & B_PAGEIO)
		bp_mapin(bp);
			
/*	since some of the kernel code will call strategy at spl6
 *	lower the ipl to 0 now! NOTE: should remove after the kernel
 *	code has been changed.
 */
	oldpri = spl0();
	sd01ck_badsec(bp);
	splx(oldpri);

#ifdef DEBUG
        DPR (2)("sd01strat0: - exit ");
#endif
}


/* %BEGIN HEADER
 * %NAME: sd01close - close for the SCSI disk driver. - 0dd08000
 * %DESCRIPTION: 
	Clear the open flags.
 * %CALLED BY: Kernel and mdmirror
 * %SIDE EFFECTS: 
	The device is marked as unopened.
 * %RETURN VALUES: Zero
 * %END HEADER */
int
sd01close(dev, flags, otype, cred_p)
dev_t dev;
int   flags;
int otype;		/* Type of open */
struct cred *cred_p;	/* Pointer to user credential structure */
{
 	register struct disk *dk;
	register int	      part;
	register int sps;
	register struct buf *bp;
	register struct job *jp;
 
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

 	dk = DKPTR(dev);
	part = DKSLICE(dev);

#ifdef DEBUG
        if (flags & O_DEBUG) { /* For DEBUGFLG ioctl */
                return(0);
        }
        DPR (1)("sd01close: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x) ", dev, flags, otype, cred_p);
#endif

        if (dk->dk_spec && dk->dk_spec->last_close) {
                (*dk->dk_spec->last_close)(dk);
        }

	/* Determine the type of close being requested */
	switch(otype)
	{
	  case	OTYP_BLK:			/* Close for Block I/O */
		dk->dk_part_flag[part] &= ~DKBLK;
		break;

	  case	OTYP_MNT:			/* Close for Mounting */
		dk->dk_part_flag[part] &= ~DKMNT;
		break;

	  case	OTYP_CHR:			/* Close for Character I/O */
		dk->dk_part_flag[part] &= ~DKCHR;
		break;

	  case	OTYP_SWP:			/* Close for Swapping Device */
		dk->dk_part_flag[part] &= ~DKSWP;
		break;

	  case	OTYP_LYR:			/* Layered driver close */
		dk->dk_part_flag[part] -= DKLYR;
		break;
	}

	/*
	*  Always clear the DKONLY flag since the partition can no
	*  longer be opened for exclusive open if this function is called.
	*  Check if all flags have been cleared and the layers counter is
	*  zero before clearing the General Open flag.
	*/
 	dk->dk_part_flag[part] &= ~DKONLY;
	if (!(dk->dk_part_flag[part] & ~DKGEN))
 		dk->dk_part_flag[part] &= ~DKGEN;

        /*
        *  If no partitions on this device are open
	*  then forget the cached disk parameters
	*/
        for (part=0; part < V_NUMPAR; part++) {
		if (dk->dk_part_flag[part])
                        return(0);
	}
	/*
	*  If the VTOC has been soft-modified, leave the
	*  v_sanity and dk_state alone.
	*/
	if ( dk->vtoc_state != VTOC_SOFT ) {
        	dk->dk_state &= ~(DKPARMS | DKFDISK);
		dk->dk_vtoc.v_sanity = 0;
	}

	/*
	 * If the media is removable, allow media removal
	 * after last close of the drive.
	 */
	if(dk->dk_iotype & F_RMB)	{
		(void)sd01cmd(dk, SS_LOCK, 0, NULL, 0, 0, SCB_READ);
	}

#ifdef DEBUG
        DPR (2)("sd01close: - exit(0)\n");
#endif

	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01dmabreakup - Raw SCSI disk DMA I/O.
 * %DESCRIPTION:
	This routine checks the capability of the device and
	calls either the routine that breaks the request up into
	contiguous DMA-able pieces or the routine that simply 
	provides a kernel mapping to the largest chunk possible.
	The requests are then passed to sd01strat0()..
 * %CALLED BY: sd01breakup()
 * %SIDE EFFECTS: None.
 * %RETURN VALUES: None.
 * %END HEADER */
void
sd01dmabreakup(bp)
struct buf *bp;
{
	register struct  disk *dk;
	struct scsi_ad *dev;
	ulong  maxtransfer;

	dk   = DKPTR(bp->b_edev);
 	dev = &dk->dk_addr;
	if( dk->dk_iotype & F_SCGTH ) {
 		maxtransfer = HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer ?
 			btodt(HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer) : 
			btodt(0x10000);
		pio_breakup( sd01strat0, bp, maxtransfer);
	} else {
 		dma_pageio(sd01strat0, bp);
	}
}

/* %BEGIN HEADER
 * %NAME: sd01breakup - Raw SCSI disk I/O.
 * %DESCRIPTION:
	This routine determines the DMA capability of the device and
	calls either the restricted DMA routine which copies data
	outside the restricted region into the dmaable region, or
	calls the routine that simply provides a kernel mapping,
	breaking up the pieces only if larger than the DMA allows
	or that this device allows.
	When finished, the request is passed on to sd01dmabreakup or
	to sd01strat0.
 * %CALLED BY: physiock()
 * %SIDE EFFECTS: None.
 * %RETURN VALUES: None.
 * %END HEADER */
void
sd01breakup(bp)
struct buf *bp;
{
	register struct  disk *dk;
	struct scsi_ad *dev;
	ulong maxtransfer;

	dk   = DKPTR(bp->b_edev);
 	dev = &dk->dk_addr;
	if( dk->dk_iotype & F_DMA) {
		rdma_filter( sd01dmabreakup, bp );
	} else {
 		maxtransfer = HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer ?
 			btodt(HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer) : 
			btodt(0x10000);
		pio_breakup( sd01strat0, bp, maxtransfer);
	}
}

/* %BEGIN HEADER
 * %NAME: sd01read - Raw SCSI disk read. - 0dd09000
 * %DESCRIPTION: 
	This is the raw read routine for the SCSI disk driver.  The request
	is validated to see that it is within a legal partition.  A read
	request will start on a disk block boundary reading the requested
	number of bytes.  This routine calls \fIphysiock\fR which locks the
	user buffer into core and checks that the user can access the area.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	Error ENXIO is returned via physiock if the read is not to a legal
	partition.  Indirectly, the user buffer area is locked into core,
	and a SCSI read job is queued for the device.
 * %RETURN VALUES: Error value
 * %END HEADER */
int
sd01read(dev, uio_p, cred_p)
dev_t dev;
struct uio *uio_p;
struct cred *cred_p;
{
	register struct  disk *dk;
	register int part;
	int	 ret_val;

	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	dk   = DKPTR(dev);
	part = DKSLICE(dev);                           

	/*
	 * Reject the request if the partition has not been opened.
	 */
	if ((dk->dk_part_flag[part] & DKGEN) == 0) {
		return(ENXIO);
	}

#ifdef DEBUG
        DPR (1)("sd01read: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x part=0x%x ", dev, uio_p, cred_p, dk, part);
#endif

	ret_val = physiock(sd01breakup, 0, dev, B_READ,
		dk->dk_vtoc.v_part[part].p_size << BLKSEC(dk), uio_p);

#ifdef DEBUG
        DPR (2)("sd01read: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sd01write - Raw SCSI disk write. - 0dd0a000
 * %DESCRIPTION: 
	This function performs a raw write to a SCSI disk.  The request is
	validated to see that it is within a legal partition. The write will
	always start at the beginning of a disk block boundary.
	This function calls \fIphysiock\fR which locks the user buffer into
	core and checks that the user can access the area.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	Error ENXIO is returned via physiock if the write is not to a legal
	partition.  Indirectly, the user buffer area is locked into core,
	and a SCSI read job is queued for the device.
 * %RETURN: Error value
 * %END HEADER */
int
sd01write(dev, uio_p, cred_p)
dev_t dev;
struct uio *uio_p;
struct cred *cred_p;
{
	register struct  disk *dk;
	register int part;
	int	 ret_val;

	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	dk   = DKPTR(dev);
	part = DKSLICE(dev);                           

        /*
	 * Reject the request if the partition has not been opened.
	 */
	if ((dk->dk_part_flag[part] & DKGEN) == 0) {
		return(ENXIO);
	}

#ifdef DEBUG
        DPR (1)("sd01write: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x part=0x%x ", dev, uio_p, cred_p, dk, part);
#endif

	ret_val = physiock(sd01breakup, 0, dev, B_WRITE,
		dk->dk_vtoc.v_part[part].p_size << BLKSEC(dk), uio_p);

#ifdef DEBUG
        DPR (2)("sd01write: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sd01open1 - Core open function. - 0dd0b000
 * %DESCRIPTION: 
	This is the core open routine for the disk and mirror driver.  If
	the VTOC has not been read in yet, \fIsd01open1\fR will attempt to
	read the pdsector and the VTOC.  
	The mode field indicates the type of open exclusive use or
	general use.  Exclusive use is requested by the mirror driver when
	the partition is bound to a mirror partition.  Once the partition is
	opened for exclusive use all other open requests for the partition
	are failed.  The partition must not be open for general use or the
	exclusive use request is failed.  The partition may be opened many
	times for general use. If the open is for exclusive use, the 
	PD sector and VTOC must be valid and the sector must not contain
	the VTOC.
 * %CALLED BY: sd01open, sd01strategy and mdmirror
 * %SIDE EFFECTS: 
	The type of open is noted.
 * %RETURN VALUES: 
	.VL 4 
	.LI "-1. "
	The open succeeded but the VTOC is insane or the partition is
	undefined.
	.LI " 0. "
	The open succeeded.
	.LI ">0. "
	The error value. 
	.LE
 * %ERROR 6dd0b001
	The disk partition table is not sane.  This is caused by a corrupted
	or uninitialized sanity word in the fdisk table (OBSOLETE ERROR).
 * %END ERROR
 * %ERROR 6dd0b002
	The SCSI disk is not configured properly.  This is caused by not
	defining an ACTIVE operating system in the partition table (fdisk).
 * %END ERROR
 * %ERROR 6dd0b003
	The SCSI disk is not configured for the UNIX operating System.  This
	is caused by specifying an ACTIVE partition in the partition table
	that may not be intended to be used with the UNIX Operating System.
 * %END ERROR
 * %ERROR 6dd0b004
	The SCSI disk does not contain a VTOC.  This is caused by specifying a
	different VTOC location in the /etc/partitions file.  The driver expects
	the VTOC to be in the PD sector at location 29.
 * %END ERROR
 * %ERROR 6dd0b005
	The UNIX system partition entry in the VTOC does not match the partition
	defined in the fdisk table.  This is caused by changing the starting
	sector address or size of slice 0 in the VTOC with out performing an
	fdisk(1M).
 * %END ERROR
 * %END HEADER */

int
sd01open1(emaj, emin, mode)
major_t	emaj;
minor_t	emin;
int  mode;
{
	register struct disk *dk;	/* Pointer to disk structure	*/
	register int slice;		/* slice number			*/
	struct   phyio	 phyarg;	/* For reading Pd and VTOC 	*/
	struct	 ipart 	*ipart;		/* The fdisk table entry 	*/
	dev_t 	 dev;			/* External device number	*/

	unsigned long	*proc_p;
	int	 ret_val, i;

	extern void	sd01flt();
	
	phyarg.retval = 0;

#ifdef DEBUG
        DPR (1)("sd01open1: (emaj=0x%x emin=0x%x mode=%d) ", emaj, emin, mode);
#endif
	/* Verify a disk(s) is present so other checks are valid*/
	if (sd01_diskcnt == DKNOTCS) {
		cmn_err(CE_WARN,"Disk Driver: No hard disks found during system startup.\n");
		return(ENODEV);
	}

/* Check if the major number is valid */
	if (sd01instbl[emaj] == DKNOMAJ )
		return(ENODEV);

/* Check if the minor number is valid */
	dev = makedevice(emaj, emin);
	if ((DKINDEX(dev) + 1) > sd01_diskcnt)
		return(ENODEV);
	
	dk = DKPTR(dev);
	slice = DKSLICE(emin);
	
/* Check if the slice is busy already */
recheck:
	if(dk->dk_part_flag[slice] & DKONLY)
	{
		return(EBUSY);
	}

	if(mode & DKONLY && dk->dk_part_flag[slice] & DKGEN)
	{
		return(EBUSY);
	}
	
#ifdef DEBUG
        DPR (3)("sa_fill=0x%x ", dk->dk_addr.sa_fill);
#endif

/* Is the VTOC sane? If not try to read it in. */
	if(dk->dk_vtoc.v_sanity != VTOC_SANE)
	{

#ifdef DEBUG
        DPR (3)("VTOC insane ");
#endif
		if ((dk->dk_state & DKINIT) == 0)
		{
			/* Initialize disk structure */
			drv_getparm(UPROCP, (ulong*)&proc_p);
			dk->dk_fltsus  = sdi_getblk();	/* Suspend       */
			dk->dk_fltreq  = sdi_getblk();	/* Request Sense */
			dk->dk_fltres  = sdi_getblk();	/* Reserve       */
			dk->dk_fltrblk = sdi_getblk();	/* Read Block 	 */
			dk->dk_fltwblk = sdi_getblk();	/* Write Block	 */
			dk->dk_fltmblk = sdi_getblk();	/* Reassign Block*/
			dk->dk_fltfqblk = sdi_getblk();	/* Flush Queue   */

			dk->dk_bbh_rjob = sd01getjob(dk);/* Read Block job */
			dk->dk_bbh_wjob = sd01getjob(dk);/* Write Block job*/
			dk->dk_bbh_mjob = sd01getjob(dk);/* Reassgn Blk job*/

			dk->dk_bbh_rbuf = getrbuf(KM_SLEEP);/* Read Block buf*/
			dk->dk_bbh_wbuf = getrbuf(KM_SLEEP);/* Write Blk buf*/
			dk->dk_bbh_mbuf = getrbuf(KM_SLEEP);/* Reasgn Blk buf*/

			dk->dk_fltsus->sb_type  = SFB_TYPE;
			dk->dk_fltreq->sb_type  = ISCB_TYPE;
			dk->dk_fltres->sb_type  = ISCB_TYPE;
			dk->dk_fltrblk->sb_type = ISCB_TYPE;
			dk->dk_fltwblk->sb_type = ISCB_TYPE;
			dk->dk_fltmblk->sb_type = ISCB_TYPE;
			dk->dk_fltfqblk->sb_type = SFB_TYPE;

			dk->dk_stat.maj = emaj;
			dk->dk_stat.min = emin;
			dk->dk_addr.sa_major = emaj;
			dk->dk_addr.sa_minor = emin;

			dk->dk_fltsus->SFB.sf_dev    = dk->dk_addr;

			dk->dk_fltreq->SCB.sc_datapt = SENSE_AD(&dk->dk_sense);
			dk->dk_fltreq->SCB.sc_datasz = SENSE_SZ;
			dk->dk_fltreq->SCB.sc_mode   = SCB_READ;
			dk->dk_fltreq->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
			dk->dk_fltreq->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltreq, B_READ, (caddr_t)proc_p);

			dk->dk_fltres->SCB.sc_datapt = NULL;
			dk->dk_fltres->SCB.sc_datasz = 0;
			dk->dk_fltres->SCB.sc_mode   = SCB_WRITE;
			dk->dk_fltres->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
			dk->dk_fltres->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltres, B_WRITE,(caddr_t)proc_p);

			dk->dk_fltrblk->SCB.sc_datapt = dk->blkbuf;
			dk->dk_fltrblk->SCB.sc_datasz = BLKSZ(dk);
			dk->dk_fltrblk->SCB.sc_mode   = SCB_READ;
			dk->dk_fltrblk->SCB.sc_cmdpt  = SCM_AD(&dk->dk_blkcmd);
			dk->dk_fltrblk->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltrblk, B_READ, (caddr_t)proc_p);

			dk->dk_fltwblk->SCB.sc_datapt = dk->blkbuf;
			dk->dk_fltwblk->SCB.sc_datasz = BLKSZ(dk);
			dk->dk_fltwblk->SCB.sc_mode   = SCB_WRITE;
			dk->dk_fltwblk->SCB.sc_cmdpt  = SCM_AD(&dk->dk_blkcmd);
			dk->dk_fltwblk->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltwblk, B_WRITE, (caddr_t)proc_p);

			dk->dk_fltmblk->SCB.sc_datapt = dk->dk_dl_data;
			dk->dk_fltmblk->SCB.sc_datasz = RABLKSSZ;
			dk->dk_fltmblk->SCB.sc_mode   = SCB_WRITE;
			dk->dk_fltmblk->SCB.sc_cmdpt  = SCS_AD(&dk->dk_blkcmd);
			dk->dk_fltmblk->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltmblk, B_WRITE, (caddr_t)proc_p);

			/* Set by sd01intn, cleared by sd01intres */
			dk->dk_fltres->SCB.sc_wd = NULL;
			dk->dk_state |= DKINIT;
		}

                if (dk->dk_spec && dk->dk_spec->first_open) {
                        (*dk->dk_spec->first_open)(dk);
                }
		
		/* Is VTOC being read */
		if ((dk->dk_state & DKVTOC) == 0) 
		{
			struct 	pdinfo 	*pdptr;
			struct 	vtoc 	*vtocptr;
			uint 	voffset;
			daddr_t	pdsect, vtocsect;
			char 	*p;
			caddr_t secbuf;
			int	i, z;

			dk->dk_state |=  DKVTOC;
			dk->dk_state &= ~DKCONFLICT;
			dk->dk_state &= ~DKFDISK;
                	dk->unixst   = 0;

			/* Check if disk parameters need to be set */
			if ((dk->dk_state & DKPARMS) == 0) {
				ret_val = sd01config(dk);
				if (ret_val > 0) {
					dk->dk_state &= ~DKVTOC;
					wakeup((caddr_t)dk);
					return(ret_val);
				}
				/*
				 * Perhaps sd01open1() should not continue
				 * if sd01config() gets a non-fatal error.
				 */
			}

			/* Allocate temporary memory for sector 0 */
			if((secbuf = kmem_alloc(BLKSZ(dk), KM_SLEEP)) == NULL)
			{ 
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
				wakeup((caddr_t)dk);
				return(-1);
			}

			/* Read in the FDISK sector */
			phyarg.sectst  = FDBLKNO; /* sector zero */
			phyarg.memaddr = (long) secbuf;
  			phyarg.datasz  = BLKSZ(dk);
			sd01phyrw(dk, V_RDABS, &phyarg, SD01_KERNEL);
			if (phyarg.retval != 0 ||
			   ((struct mboot *)secbuf)->signature != MBB_MAGIC)
			{
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
				wakeup((caddr_t)dk);
				kmem_free(secbuf, BLKSZ(dk));
				secbuf = NULL;
				if (slice > 0)
					return(ENODEV);
				return(-1);
			}

			ipart = (struct ipart *)((struct mboot *)secbuf)->parts;

                	/*
                 	 * Find active Operating System partition.
                 	 */

                	for (z = FD_NUMPART; ipart->bootid != ACTIVE; ipart++) {
                              if (--z == 0) {
			          dk->dk_state &= ~DKVTOC;
			          dk->dk_part_flag[slice] |= DKGEN & mode;
			          wakeup((caddr_t)dk);
					kmem_free(secbuf, BLKSZ(dk));
					secbuf = NULL;
				  if (slice > 0) {
                                  	cmn_err(CE_NOTE, "!Disk Driver: No ACTIVE partition. Err: 6dd0b002");
					return(ENODEV);
				  }
			          return(-1);
                              }
                	}

			/* Indicate Fdisk is valid */
			dk->dk_state |=  DKFDISK;

			/* Check if UNIX is the active partition */
                	if (ipart->systid != UNIXOS) {
			          dk->dk_state &= ~DKVTOC;
			          dk->dk_part_flag[slice] |= DKGEN & mode;
			          wakeup((caddr_t)dk);
				  kmem_free(secbuf, BLKSZ(dk));
				  secbuf = NULL;
				  if (slice > 0) {
                        		cmn_err(CE_NOTE, "!Disk Driver: ACTIVE partition is not a UNIX System partition. Err: 6dd0b003");
					return(ENODEV);
				  }
			          return(-1);
                	}

			/* Save starting sector of UNIX partition */
                	dk->unixst = ipart->relsect;
			
			/* Make slice 0 the whole UNIX partition */

			dk->dk_vtoc.v_part[0].p_start = dk->unixst;
			dk->dk_vtoc.v_part[0].p_size  = ipart->numsect;
                	dk->dk_parms.dp_pstartsec     = dk->unixst; 

			/* Read in the PD and VTOC sector */
			phyarg.sectst  = dk->unixst + HDPDLOC; 
			phyarg.memaddr = (long) secbuf;
  			phyarg.datasz  = BLKSZ(dk);
			sd01phyrw(dk, V_RDABS, &phyarg, SD01_KERNEL);

			pdptr = (struct pdinfo *) secbuf;
			if (pdptr->sanity != VALID_PD ||
				phyarg.retval != 0)
			{
				/* Mark the VTOC as insane */
				dk->dk_vtoc.v_sanity = 0;
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
#ifdef DEBUG
        DPR (2)("sd01open1: PD not valid - return(-1) ");
#endif
				wakeup((caddr_t)dk);
				kmem_free(secbuf, BLKSZ(dk));
				secbuf = NULL;
				if (slice > 0)
					return(ENODEV);
				return(-1);
			}

			/* Compare pdinfo parameters with drivers */
                	if ((pdptr->cyls != dk->dk_parms.dp_cyls) ||
                    	(pdptr->tracks != dk->dk_parms.dp_heads) ||
                    	(pdptr->sectors != dk->dk_parms.dp_sectors) ||
                    	(pdptr->bytes != BLKSZ(dk))) {

#ifdef DEBUG
        DPR (3)("SD01: Pdinfo doesn't match disk parameters\n");
        DPR (3)("cyls=0x%x / 0x%x tracks=0x%x / 0x%x sec=0x%x / 0x%x bytes=0x%x / 0x%x ", 
	pdptr->cyls, dk->dk_parms.dp_cyls, pdptr->tracks, dk->dk_parms.dp_heads,
	pdptr->sectors, dk->dk_parms.dp_sectors, pdptr->bytes, BLKSZ(dk));
#endif

				dk->dk_parms.dp_heads   = pdptr->tracks;
				dk->dk_parms.dp_cyls    = pdptr->cyls;
				dk->dk_parms.dp_sectors = pdptr->sectors;
				dk->dk_state |= DKPARMS; 
			}

			/* Update the in-core PD info structure */
			p = (caddr_t) &dk->dk_pdsec;
			for (z = 0; z < sizeof(dk->dk_pdsec); z++,p++)
				*p = secbuf[z];

                	/* Determine VTOC location on the disk */
                	vtocsect = dk->unixst + (dk->dk_pdsec.vtoc_ptr >> BLKSHF(dk));

			/* Check if VTOC and PD info are in the same sector */
                	voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
                	if (vtocsect != (dk->unixst + HDPDLOC)) {
                        	/* VTOC is in a different sector. */
				/* Mark the VTOC as insane */
				dk->dk_vtoc.v_sanity = 0;
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
				wakeup((caddr_t)dk);
				kmem_free(secbuf, BLKSZ(dk));
				secbuf = NULL;
				return(-1);
                	}

                        vtocptr = (struct vtoc *) (secbuf + voffset);
	
                	if (vtocptr->v_sanity != VTOC_SANE) {
#ifdef DEBUG
        DPR (3)("sd01open1: VTOC not valid - return(-1) ");
#endif
				/* Mark the VTOC as insane */
				dk->dk_vtoc.v_sanity = 0;
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
				wakeup((caddr_t)dk);
				kmem_free(secbuf, BLKSZ(dk));
				secbuf = NULL;
				if (slice > 0)
					return(ENODEV);
				return(-1);
                	}

                	/* Make sure slice 0 is correct.  */
                	if (vtocptr->v_part[0].p_start != dk->dk_vtoc.v_part[0].p_start
                               	|| vtocptr->v_part[0].p_size != dk->dk_vtoc.v_part[0].p_size) {
#ifdef SD01_DEBUG
				  if (slice > 0)
                        		cmn_err(CE_NOTE, "!Disk Driver: Invalid slice 0");
#endif
				/* Mark the VTOC as insane */
				dk->dk_vtoc.v_sanity = 0;
				dk->dk_state &= ~DKVTOC;
				dk->dk_part_flag[slice] |= DKGEN & mode;
				wakeup((caddr_t)dk);
				kmem_free(secbuf, BLKSZ(dk));
				secbuf = NULL;
				return(-1);
                	}

			/* Update the in-core VTOC structure */
			p = (caddr_t) &dk->dk_vtoc;
			for (z=voffset, i=0; i < sizeof(dk->dk_vtoc); i++,p++) {
				*p = secbuf[z++];
			}
			dk->vtoc_state = VTOC_HARD;

			/* Get alternate sector table		*/
			sd01getalts(dk);

			dk->dk_state &= ~DKVTOC;
			wakeup((caddr_t)dk);
			kmem_free(secbuf, BLKSZ(dk));
			secbuf = NULL;
		}
		else 
		{			/* Someone else is reading VTOC */
			while(dk->dk_state & DKVTOC)
				sleep((caddr_t)dk, PRIBIO);
			goto recheck;
		}
	}

	/* If slice has size 0, its not a valid slice and open should fail */
	if ((slice > 0) && (dk->dk_vtoc.v_part[slice].p_size <= 0))
		return(ENODEV);

	/* Check if VTOC is valid */
	if (dk->dk_vtoc.v_sanity != VTOC_SANE || 
	    dk->dk_vtoc.v_version > V_VERSION ||
	    dk->dk_vtoc.v_part[slice].p_tag == V_OTHER) {
#ifdef DEBUG
        DPR (3)("sanity=0x%x version=0x%x size=0x%x ",dk->dk_vtoc.v_sanity, dk->dk_vtoc.v_version, dk->dk_vtoc.v_part[slice].p_size);
        DPR (2)("sd01open1: VTOC still insane - return(-1) ");
#endif
		dk->dk_part_flag[slice] |= mode & DKGEN;
		return(-1);
	}

	dk->dk_part_flag[slice] |= mode;

#ifdef DEBUG
        DPR (2)("sd01open1: - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01open - Open routine for SCSI disks. - 0dd0c000
 * %DESCRIPTION: 
	If this is the first open to the device, \fIsd01open\fR
	will read in the VTOC.  If the time stamp is set for this slice,
	\fIsd01strategy\fR will write the time stamp back
	with zero when the first write occurs to this slice.
	\fIsd01open\fR will fail if the slice is part of a mirrored pair
	or if the pdsector and VTOC cannont be read in.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The PD and VTOC onformation is read into the disk data structure.
 * %RETURN VALUES: Zero or error value 
 * %END HEADER */
int
sd01open(dev_p, flags, otype, cred_p)
dev_t *dev_p;
int flags;
int otype;		/* Type of open */
struct cred *cred_p;	/* Pointer to user credential structure */
{
	register struct disk *dk;
	register int	      part;
	major_t	 emaj;
	minor_t	 emin;
	long	 save_flags;
	int	 ret_val;
	int	 m_lock, i;

#ifdef DEBUG
        DPR (1)("\nsd01open: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x) ", *dev_p, flags, otype, cred_p);

        if (flags & O_DEBUG)	 /* For DEBUGFLG ioctl */
                return(0);
#endif

	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	emaj = getmajor(*dev_p);
	emin = getminor(*dev_p);

	if ((DKINDEX(*dev_p) + 1) > sd01_diskcnt)
		return(ENODEV);

	dk   = DKPTR(*dev_p);
	part = DKSLICE(*dev_p);

	/*
	 * If the media is removable, prevent media removal
	 * after first open of the drive.
	 */
	if(dk->dk_iotype & F_RMB)	{
		m_lock = 1;
		for(i=0; i < V_NUMPAR; i++) {
			if(dk->dk_part_flag[i])	{
				m_lock = 0;
				break;
			}
		}
	}
	else	{
		m_lock = 0;
	}

	/* Don't set open flag unless open succeeds */
	if ((ret_val = sd01open1(emaj, emin, DKGEN)) > 0)
		return(ret_val);

	save_flags = dk->dk_part_flag[part];

	/* Determine the type of open being requested */
	switch(otype)
	{
	  case	OTYP_BLK:		/* Open for Block I/O */
		dk->dk_part_flag[part] |= DKBLK;
		break;

	  case	OTYP_MNT:		/* Open for Mounting */
		if(ret_val == -1)
		{ /* bad vtoc or PD sector */
			/* If no flags were set before the open, clear DKGEN */
			if (!(save_flags & ~DKGEN))
				dk->dk_part_flag[part] &= ~DKGEN;
			return(ENXIO);
		}
		if (dk->dk_vtoc.v_sanity == VTOC_SANE &&
		    dk->dk_vtoc.v_part[part].p_flag == V_UNMNT)
		{
			/* If no flags were set before the open, clear DKGEN */
			if (!(save_flags & ~DKGEN))
				dk->dk_part_flag[part] &= ~DKGEN;
			return(EACCES);
		}
		else
			dk->dk_part_flag[part] |= DKMNT;
		break;

	  case	OTYP_CHR:		/* Open for Character I/O */
		dk->dk_part_flag[part] |= DKCHR;
		break;

	  case	OTYP_SWP:		/* Open for Swapping Device */
		dk->dk_part_flag[part] |= DKSWP;
		break;

	  case	OTYP_LYR:		/* Layered driver counter */
		dk->dk_part_flag[part] += DKLYR;
		break;
	}

	if(m_lock)	{
		(void)sd01cmd(dk, SS_LOCK, 0, NULL, 0, 1, SCB_READ);
	}

#ifdef DEBUG
        DPR (2)("\nsd01open: - exit ");
#endif

	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01done - Disk driver I/O completion routine. - 0dd0d000
 * %DESCRIPTION: 
	This function completes the I/O request after a job is returned by
	the host adapter. It will return the job structure and call
	\fIbiodone\fR.
 * %CALLED BY: sd01comp1
 * %SIDE EFFECTS: 
	The kernel is notified that one of its requests completed.
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01done(jp)
register struct job *jp;
{
	register buf_t *bp = jp->j_bp;
	register struct disk *dk = jp->j_dk;
	time_t 		lbolt_time; 
	time_t	 since_lasttime; 	/* elapsed time since last perf timestamp */
	char 	 ops;

#ifdef DEBUG
        DPR (1)("sd01done: (jp=0x%x) ", jp);
#endif

	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
/*
 *  		Check to see if the failed job was an update of
 *  		the time stamps for a mirrored partition.
 *  		Don't set the error since the mirrored disk may complete.
 */
		if (jp->j_dk->dk_state & DKTSMD) 
			jp->j_dk->dk_state &= ~DKTSMD;

		else {	/* Record the error for a normal job */
			bp->b_flags |= B_ERROR;
			if (jp->j_cont->SCB.sc_comp_code == SDI_NOTEQ)
				bp->b_error = ENODEV;
			else if (jp->j_cont->SCB.sc_comp_code == SDI_QFLUSH)
				bp->b_error = EFAULT;
			else if (jp->j_cont->SCB.sc_comp_code == SDI_OOS)
				bp->b_error = EIO;
			else if (jp->j_cont->SCB.sc_comp_code == SDI_CKSTAT && 
				 jp->j_cont->SCB.sc_status == S_RESER) {
				bp->b_error = EBUSY;/* Reservation Conflict */
				jp->j_dk->dk_state |= DKCONFLICT;
			} else
				bp->b_error = EIO;
		}
	}

/*
 *	hang job encountered bad sector to the recovery queue and 
 *	wakup the dynamic bad block deamon
 */
	if (sd01db.db_flag & DBBH_ARM) {
		if (bp->b_flags & B_BAD) {
			bp->b_flags &= ~B_BAD;
			jp->j_forw = sd01db.db_badjob;
			sd01db.db_badjob = jp;
			if (!(sd01db.db_flag & DBBH_BUSY))
				wakeup((caddr_t)sd01dkd);
			return;
		} else if (bp->b_error==EFAULT) {
/* 		job returned from queue flush				*/
			if (!jp->j_fwmate && !jp->j_bkmate) {
				jp->j_forw = sd01db.db_fq;
				sd01db.db_fq = jp;
			} else {
				if (jp->j_fwmate) 
					jp->j_fwmate->j_bkmate = jp->j_bkmate;
				if (jp->j_bkmate) 
					jp->j_bkmate->j_fwmate = jp->j_fwmate;
				sd01freejob(jp);
			}
			return;
		}
	}
	
/* 	Update performance stat 					*/
/* 	If this is a read or write, also update the I/O queue 		*/
	ops = (char) *jp->j_cont->SCB.sc_cmdpt;
	drv_getparm(LBOLT, (ulong*)&lbolt_time);
	if (ops == SM_READ || ops ==  SM_WRITE) {
		if (ops == SM_READ)
			dk->dk_stat.tnrreq++;
		else
			dk->dk_stat.tnwreq++;
		if(dk->dk_stat.pttrack >= dk->dk_stat.endptrack)
                        dk->dk_stat.pttrack = &dk->dk_stat.ptrackq[0];
                dk->dk_stat.pttrack->b_blkno = jp->j_daddr;
                dk->dk_stat.pttrack->bp = jp->j_bp;
                dk->dk_stat.pttrack++;
		since_lasttime = lbolt_time - dk->dk_stat.lasttime;
		dk->dk_stat.io_act += since_lasttime;
		dk->dk_stat.io_resp += since_lasttime * dk->dk_stat.busycnt;
		dk->dk_stat.io_bcnt += jp->j_seccnt;
		dk->dk_stat.busycnt--;
		dk->dk_stat.lasttime = lbolt_time;
	}

	if (!jp->j_fwmate && !jp->j_bkmate) {
		biodone(bp);
	} else {
		if (jp->j_fwmate) 
			jp->j_fwmate->j_bkmate = jp->j_bkmate;
		if (jp->j_bkmate) 
			jp->j_bkmate->j_fwmate = jp->j_fwmate;
	}


	/*
	 * Undo the virtual address remapping if previously done.
	 * Mapout call is probably not needed, comment it out
	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	 */

	sd01freejob(jp);

#ifdef DEBUG
        DPR (2)("sd01done: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01comp1 - Complete a disk job. - 0dd0e000
 * %DESCRIPTION: 
	This function removes the job from the queue.  Updates the disk
	counts.  Restarts the logical unit queue if necessary, and prints an
	error for failing jobs.
 * %CALLED BY: sd01intn 
 * %SIDE EFFECTS: 
	Removes the job from the disk queue.
 * %RETURN VALUES: None
 * %ERROR 6dd0e001
	An I/O request failed due to an error returned by the host adpater.
	All recovery action failed and the I/O request was returned to the 
	requestor.  The secondary error code is equal to the SDI return
	code.  See the SDI error codes for more information.
 * %END ERROR
 * %ERROR 6dd0e002
	A bad block was detected.  The driver can not read this block. A 
	Error Correction Code (ECC) was unsuccessful.  The data in this block
	has been lost.  The data can only be retrived from a previous backup.
 * %END ERROR
 * %END HEADER */
void
sd01comp1(jp)
register struct job *jp;
{
	register struct disk *dk;
	int		sps;
	
	dk = jp->j_dk;
	dk->dk_outcnt--;

#ifdef SD01_DEBUG
if (sd01alts_debug & DKD) {
	if (sd01alts_badspot && (sd01alts_badspot >= jp->j_daddr) &&
	    (sd01alts_badspot<(jp->j_daddr+jp->j_seccnt))) {
		dk->dk_state |= DKSUSP;
		dk->hde_state |= HDEECCERR;
		dk->dk_sense.sd_ba = sd01alts_badspot;
		sd01alts_badspot = 0;
	}
}
#endif

/* 	Log error if necessary 						*/
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
		if(dk->hde_state & HDEECCERR) {
			sd01logerr(jp->j_cont, jp, 0x6dd0e002);
		} else
			sd01logerr(jp->j_cont, jp, 0x6dd0e001);
	}

/* 
 *	bad sector detected - start hardware reassignment
 *	skip if hardware reassignment is in progress
 *	lu Q has been suspended
 */
	if ((dk->hde_state & HDEBADBLK) && !(dk->hde_state & HDEHWREA)) {
		minor_t	 min;			/* Device minor number	*/
		min = dk->dk_fltreq->SCB.sc_dev.sa_minor; 

/* 		Check if dynamic BBH has been disable or
 *		bad block is in non-UNIX area of the disk.		
 */
		if ((Sd01bbh_flg & BBH_DYNOFF) ||
		    (dk->dk_vtoc.v_sanity == VTOC_SANE && 
		    dk->dk_vtoc.v_part[DKSLICE(min)].p_tag == V_OTHER)) {
/* 			Resume the Q if suspended			*/
			if (dk->dk_state & DKSUSP)	
				sd01qresume(dk);

/* 			Clear bad block state 				*/
			dk->hde_state = 0;
		} else {
/* 			enter hardware bad sector reassignment state 	*/
			dk->dk_hdesec 	= dk->dk_sense.sd_ba;
			dk->dk_state 	|= DKMAPBAD;	
			dk->hde_state	|= HDEHWREA;
			jp->j_bp->b_flags |= (B_ERROR|B_BAD);
			sd01hdelog(dk);
		}
	} else if (!(dk->hde_state & HDEBADBLK) && (dk->dk_state & DKSUSP))
		sd01qresume(dk); 	/* Resume Que 			*/
	
	dk->dk_jberr = 0;		/* Reset retry counter 		*/

/*	start next job if necessary					*/
	if (dk->dk_outcnt<=dk->dk_outsz_hi && !(dk->hde_state & HDEBADBLK) &&
		!(dk->dk_state & (DKBUSYQ|DKSUSP|DKSEND)) &&
		(dk->dk_next!=(struct job *)dk || dk->dk_syncq_fw)) {
/*		use disk deamon if armed & configured to do so		*/
		if ((sd01db.db_flag & (DBBH_ARM|DBBH_SEND)) == 
			(DBBH_ARM|DBBH_SEND)) {
			if (!(dk->dk_state & DKPENDQ)) {
				dk->dk_state |= DKPENDQ;
				dk->dk_waitp = sd01db.db_waitdk;
				sd01db.db_waitdk = dk;
			}
			if (!(sd01db.db_flag & DBBH_BUSY))
				wakeup((caddr_t)sd01dkd);
/*		use timeout to restart the queue			*/
		} else {
			dk->dk_state |= DKSEND;
			dk->dk_sendid = timeout(sd01sendt,(caddr_t) dk, 1);
		}
	} else if (dk->dk_state & DKBUSYQ) {
#ifdef SD01_DEBUG
		if(sd01alts_debug & DBM) 
			if (!dk->dk_outcnt) 
				printf("sd01comp1: outcnt==0 and BUSY Q\n");
#endif
		if (dk->dk_state & DKSEND) {
			untimeout(dk->dk_sendid);
			dk->dk_state &= ~DKSEND;
		}
		if (!(jp->j_bp->b_flags&B_ASYNC)||(jp->j_bp->b_flags&B_WANTED))
			dk->dk_state |= DKSYNCDONE;
	}
	jp->j_done(jp);			/* Call the done routine 	*/
}

/* %BEGIN HEADER
 * %NAME: sd01intf - Function block completion handler. - 0dd0f000
 * %DESCRIPTION: 
	This function is called by the host adapter driver when a host
	adapter function request has completed.  If the request completed
	without error, then the block is marked as free.  If there was error
	the request is retried.  Used for resume function completions.
 * %CALLED BY: Host Adapter driver.
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %ERROR 4dd0f001
	A SCSI disk driver function request was retried.  The retry 
	performed because the host adapter driver detected an error.  
	The SDI return code is the second error code word.  See
	the SDI return codes for more information.
 * %END ERROR
 * %ERROR 8dd0f002
	The host adapter rejected a request from the SCSI disk driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %ERROR 6dd0f003
	A SCSI disk driver function request failed because  the host
	adapter driver detected a fatal error or the retry count was
	exceeded.  This failure will cause the effected unit to
	hang.  The system must be rebooted. The SDI return code is
	the second error code word.  See the SDI return codes for
	more information.
 * %END ERROR
 * %END HEADER */
void
sd01intf(sbp)
register struct sb *sbp;
{
	register struct disk *dk;
	dev_t dev;			/* External device number	*/

	extern void  sd01resume();
	
	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = DKPTR(dev);

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01intf: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SFB.sf_comp_code & SDI_RETRY &&
		dk->dk_spcount < SD01_RETRYCNT)
	{				/* Retry the function request */

		dk->dk_spcount++;
		dk->dk_error++;
		sd01logerr(sbp, (struct job *) NULL, 0x4dd0f001);
		if (SD01ICMD(dk,sbp) == SDI_RET_OK) 
			return;
	}
	
	if (sbp->SFB.sf_comp_code != SDI_ASW)
	{
		sd01logerr(sbp, (struct job *) NULL, 0x6dd0f003);
	}

	dk->dk_spcount = 0;		/* Zero retry count */
 
	/*
	*  Currently, only RESUME SFB uses this interrupt handler
	*  so the following block of code is OK as is.
	*/

 	/* This disk LU has just been resumed */
 	sd01_resume.res_head = dk->dk_fltnext;
 
	/* Are there any more disk queues needing resuming */
 	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Queue is empty */

		/*
		*  There is a pending resume for this device so 
		*  since the Q is empty, just put the device back
		*  at the head of the Q.
		*/
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;
			sd01_resume.res_head = dk;
			sd01resume(sd01_resume.res_head);
		}
		/*
		*  The Resume has finished for this device so clear
		*  the bit indicating that this device was on the Q.
		*/
		else
		{
			dk->dk_state &= ~DKONRESQ;
 			sd01_resume.res_tail = (struct disk *) &sd01_resume;
		}
	}
 	else
	{	/* Queue not empty */

		/*  Is there another RESUME pending for this disk */
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;

			/* Move Next Resume for this disk to end of Queue */
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
		else	/* Resume next disk */
			dk->dk_state &= ~DKONRESQ;

 		sd01resume(sd01_resume.res_head);
	}

#ifdef DEBUG
        DPR (2)("sd01intf: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01logerr - Logs and prints error messages. - 0dd10000
 * %DESCRIPTION: 
	This function will log and print the error messages for errors
	detected by the host adapter driver.  No message will be printed
	for thoses errors that the host adapter driver has already 
	detected.  Other errors such as write protection are
	not reported.  The job argument maybe NULL.
 * %CALLED BY: sd01comp1, sd01intf, sd01intn, sd01ints, sd01intrq, sd01intres
 * %SIDE EFFECTS: An error report is generated.
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01logerr(sbp, jp, err_code)
register struct sb *sbp;
register struct job *jp;
register int err_code;
{
	register struct disk *dk;
	buf_t *bp;
	dev_t dev;			/* External device number 	*/


#ifdef DEBUG
        DPR (1)("sd01logerr: (sbp=0x%x jp=0x%x err_code=0x%x) ", sbp, jp, err_code);
#endif
	
	/* If OOS, then don't complain */
	if (sbp->sb_type == SFB_TYPE && sbp->SFB.sf_comp_code != SDI_OOS)
	{
		dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
		dk  = DKPTR(dev);
#ifdef DEBUG
		DPR (4)("SD01: SFB failed. SB addr = %x\n", sbp);
		DPR (4)("Completion code = %x\n", sbp->SFB.sf_comp_code);
		DPR (4)("Interrupt function address = %x\n", sbp->SFB.sf_int);
		DPR (4)("Major number = %d\n", sbp->SFB.sf_dev.sa_major);
		DPR (4)("Minor number = %d\n", sbp->SFB.sf_dev.sa_minor);
		DPR (4)("Logical unit = %d\n", sbp->SFB.sf_dev.sa_lun);
		DPR (4)("Function code = %x\n", sbp->SFB.sf_func);
		DPR (4)("Word = %x\n", sbp->SFB.sf_wd);
#endif
		sdi_errmsg("Disk",&sbp->SFB.sf_dev,sbp,&dk->dk_sense,SDI_SFB_ERR,err_code);
		return;
	}

	
#ifdef DEBUG
	DPR(5)("Disk Driver: SCB failed. SB addr = %x, Job addr = %x\n", sbp, jp);
	DPR(5)("Completion code = %x\n", sbp->SCB.sc_comp_code);
	DPR(5)("Device address = %d, %d, %d\n", sbp->SCB.sc_dev.sa_major,
		sbp->SCB.sc_dev.sa_minor, sbp->SCB.sc_dev.sa_lun);
	DPR(5)("Command Addr = %x, Size = %x\n",
		sbp->SCB.sc_cmdpt, sbp->SCB.sc_cmdsz);
	DPR(5)("Data = %x, Size = %x\n",
		sbp->SCB.sc_datapt, sbp->SCB.sc_datasz);
	DPR(5)("Word = %x\n", sbp->SCB.sc_wd);

	DPR(5)("\nStatus = %x\n", sbp->SCB.sc_status);
	DPR(5)("Interrupt function address = %x\n", sbp->SCB.sc_int);
	DPR(5)("Residue = %d\n", sbp->SCB.sc_resid);
	DPR(5)("Time = %d\n", sbp->SCB.sc_time);
	DPR(5)("Mode = %x\n", sbp->SCB.sc_mode);
	DPR(5)("Link = %x\n", sbp->SCB.sc_link);
#endif

/* Ignore the error if it is unequipped or out of service. */
	if (sbp->SCB.sc_comp_code == SDI_OOS ||
		sbp->SCB.sc_comp_code == SDI_NOTEQ)
	{
		return;
	}

	/* Ignore the error if we are doing a test unit ready	*/
	/* test unix ready fails if the LU is not equipped	*/
	if ((char) *sbp->SCB.sc_cmdpt == SS_TEST)
	{
		return;
	}

	/* Don't report RESERVATION Conflicts   */
	/* User will know them from errno value */
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT &&
	    sbp->SCB.sc_status == S_RESER)
	{
		return;
	}
	
/* Now check for a Check Status error */

	if (jp == NULL)
		return;
	if ((dk = jp->j_dk) == NULL)
		return;
		
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		(char) *sbp->SCB.sc_cmdpt != SS_REQSEN)
	{

		sdi_errmsg("Disk",&sbp->SCB.sc_dev,sbp,&dk->dk_sense,SDI_CKCON_ERR,err_code);

		dk->dk_sense.sd_key = 0;
		dk->dk_sense.sd_sencode = 0;
		return;
	}
	
	if (sbp->SCB.sc_comp_code  == SDI_CKSTAT)
	{
		sdi_errmsg("Disk",&sbp->SCB.sc_dev,sbp,&dk->dk_sense,SDI_CKSTAT_ERR,err_code);
		return;
	}
	
	sdi_errmsg("Disk",&sbp->SCB.sc_dev,sbp,&dk->dk_sense,SDI_DEFAULT_ERR,err_code);

#ifdef DEBUG
        DPR (2)("sd01logerr: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01retry - retry a failed job. - 0dd11000
 * %DESCRIPTION: 
	This function retries a failed job. 
 * %CALLED BY: sd01intf, and sd01intn
 * %SIDE EFFECTS: If necessary the que suspended bit is set.
 * %RETURN VALUES: None
 * %ERROR 8dd11001
	The host adapter rejected a request from the SCSI disk driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %END HEADER */
void
sd01retry(jp)
register struct job *jp;
{
	register struct sb *sbp;
	struct disk *dk;

	sbp = jp->j_cont;
	dk = jp->j_dk;

#ifdef DEBUG
        DPR (1)("sd01retry: (jp=0x%x) dk=0x%x", jp, dk);
#endif
	
	dk->dk_jberr++;			/* Update the error counts */
	dk->dk_error++;
	
	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;
	}
		
	sbp->SCB.sc_time = JTIME;	/* Reset the job time */
	sbp->sb_type = ISCB_TYPE;
	
	if (SD01ICMD(dk,sbp) != SDI_RET_OK)
	{
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd11001");
#endif
					/* Fail the job */
		sd01comp1((struct job *)sbp->SCB.sc_wd);
	}

#ifdef DEBUG
        DPR (2)("sd01retry: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01inte - Request sense job completion handler. - 0dd12000
 * %DESCRIPTION: 
	This function and its error numbers are obsolete.
 * %END HEADER */

/* %BEGIN HEADER
 * %NAME: sd01intn - Normal interrupt routine for SCSI job completion. - 0dd13000
 * %DESCRIPTION: 
	This function is called by the host adapter driver when a
	SCSI job completes.  If the job completed normally the job
	is removed from the disk job queue, and the requester's
	completion function is called to complete the request.  If
	the job completed with an error the job will be retried when
	appropriate.  Requests which still fail or are not retried
	are failed and the error number is set to EIO.    Device and
	controller errors are logged and printed to the user
	console.
 * %CALLED BY: Host adapter driver
 * %SIDE EFFECTS: 
	Requests are marked as complete.  The residual count and error
	number are set if required.
 * %RETURN VALUES: None 
 * %ERROR 8dd13001
	The host adapter rejected a request sense job from the SCSI 
	disk driver.  The originally failing job will also be failed.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %ERROR 4dd13002
	The SCSI disk driver is retrying an I/O request because of a fault
	which was detected by the host adapter driver.  The second error
	code indicates the SDI return code.  See the SDI return code for 
	more information as to the detected fault.
 * %END ERROR
 * %ERROR 4dd13003
	The addressed SCSI disk returned an unusual status.  The job
	will be  retried later.  The second error code is the status
	which was  returned.  This condition is usually caused by a
	problem in  the target controller.
 * %END ERROR
 * %END HEADER */
void
sd01intn(sbp)
register struct sb *sbp;
{
	register struct disk *dk;
	extern void sd01flts();
	extern sd01sendcmd();
	
	if ((sbp->SCB.sc_comp_code == SDI_ASW)
	   || (sbp->SCB.sc_comp_code == SDI_QFLUSH))
	{
		sd01comp1((struct job *)sbp->SCB.sc_wd);
		return;
	}
	
	dk = ((struct job *) sbp->SCB.sc_wd)->j_dk;

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01intn: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;	/* Note the queue is suspended */
	}

	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		dk->dk_jberr < SD01_RETRYCNT)
	{				/* We need to do a request sense */
 		/*
		*  Enter the gauntlet!
		*  The job pointer is set here and cleared in the
		*  gauntlet when the job is eventually retried
		*  or when the gauntlet exits due to an error.
 		*/
		dk->dk_fltres->SCB.sc_wd = sbp->SCB.sc_wd;
 		sd01flts(dk);
#ifdef DEBUG
        DPR (2)("sd01intn: - return ");
#endif
 		return;
	}
	
 	if (sbp->SCB.sc_comp_code == SDI_CRESET || sbp->SCB.sc_comp_code == SDI_RESET)
 	{
 		/*
		*  Enter the gauntlet!
		*  The job pointer will be cleared when the job
		*  eventually is retried or when the gauntlet
		*  decides to exit due to an error.
		*/
		dk->dk_fltres->SCB.sc_wd = sbp->SCB.sc_wd;
 		sd01flts(dk);
 		return;
 	}
	
	/*
	*  To get here, the failure of the job was not due to a bus reset
	*  nor to a Check Condition state.
	*
	*  Now check for the following conditions:
	*     -  The RETRY bit was not set on SDI completion code.
	*     -  Exceeded the retry count for this job.
	*     -  Job status indicates RESERVATION Conflict.
	*
	*  If one of the conditions is TRUE, then return the failed job
	*  to the user.
	*/
	if ((sbp->SCB.sc_comp_code & SDI_RETRY) == 0 ||
		dk->dk_jberr >= SD01_RETRYCNT ||
		(sbp->SCB.sc_comp_code == SDI_CKSTAT && sbp->SCB.sc_status == S_RESER))
	{
		sd01comp1((struct job *)sbp->SCB.sc_wd);
		return;
	}
	
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT)
	{				/* Retry the job later */
#ifdef DEBUG
		DPR(4)("SD01: Controller %d, %d, returned an abnormal status: %x.\n",
			dk->dk_addr.sa_major, dk->dk_addr.sa_minor, 
			sbp->SCB.sc_status);
#endif
		sd01logerr(sbp, (struct job *) 0, 0x4dd13003);
		timeout(sd01retry, (caddr_t)sbp->SCB.sc_wd, MSEC2HZ(LATER));
		return;
	}

/* Retry the job */
	sd01logerr(sbp, (struct job *) sbp->SCB.sc_wd, 0x4dd13002);
	sd01retry((struct job *) sbp->SCB.sc_wd);

#ifdef DEBUG
        DPR (2)("sd01intn: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01phyrw - Perform a physical read or write. - 0dd14000
 * %DESCRIPTION: 
	This function performs a physical read or write without
	regrad to the VTOC.   It is called by the \fIioctl\fR.  The
	argument for the \fIioctl\fR is  a pointer to a structure
	which indicates the physical block address and the address
	of the data buffer in which the data is to be transferred. 
	The mode indicates whether the buffer is located in user or
	kernel space.
 * %CALLED BY: sd01wrtimestamp, sd01ioctl, sd01open1
 * %SIDE EFFECTS: 
	 The requested physical sector is read or written. If the PD sector
	 or VTOC have been updated they will be read in-core on the next access 
	 to the disk (DKUP_VTOC set in sd01vtoc_ck). However, if the FDISK 
	 sector is updated it will be read in-core on the next open.
 * %RETURN VALUES: 
	Status is returned in the \fIretval\fR byte of the structure
	for driver routines only.
 * %END HEADER */
int
sd01phyrw(dk, dir, phyarg, mode)
register struct disk *dk;
long dir;
struct phyio *phyarg;
int mode;
{
	register struct job *jp;
	register buf_t *bp;
	char		*tmp_buf;
	int 	 size;			/* Size of the I/O request 	*/
	register char *ptr1, *ptr2;	/* Used for data copies 	*/
	int 	 i,			/* Index 			*/
		 ret_val = 0;		/* Return value			*/
	unsigned long	*proc_p;
	
#ifdef DEBUG
        DPR (1)("sd01phyrw: (dk=0x%x dir=0x%x mode=0x%x) ", dk, dir, mode);
        DPR (1)("phyarg: sectst=0x%x memaddr=0x%x datasz=0x%x ", phyarg->sectst, phyarg->memaddr, phyarg->datasz);
#endif

/* If the request is for the PD sector override the supplied sector parameter */
	if (dk->dk_pdsec.sanity == VALID_PD &&
		phyarg->sectst == dk->unixst + HDPDLOC && mode == SD01_USER) {
		if (phyarg->datasz > BLKSZ(dk))
			phyarg->datasz = BLKSZ(dk);
	}
	
	bp = getrbuf(KM_SLEEP);
	bp->b_flags |= B_BUSY;
	bp->b_error = 0;
 	bp->b_iodone = NULL;
 	bp->b_vp = NULL;
	bp->b_bcount = BLKSZ(dk);

	if (mode != SD01_KERNEL) {
		/* Note: OK to sleep since its not a Kernel I/O request */
		tmp_buf = (char *) kmem_alloc(BLKSZ(dk), KM_SLEEP); 
		if (tmp_buf == NULL) {
			cmn_err(CE_NOTE, 
				"!Disk Driver: Unable to allocate temporary I/O buffer\n");
			if (dir == V_RDABS) {
			    phyarg->retval = V_BADREAD;
			} else {
			    phyarg->retval = V_BADWRITE;
			}
			freerbuf(bp);
			return(ENOMEM);
		}
	}
	
	phyarg->retval = 0;
	while(phyarg->datasz > 0) {
		size = phyarg->datasz > BLKSZ(dk) ? BLKSZ(dk) : phyarg->datasz;
		if ( mode == SD01_KERNEL)
			bp->b_un.b_addr = (char *) phyarg->memaddr;
		else
			bp->b_un.b_addr = tmp_buf;
		bp->b_bcount = size;
		drv_getparm(UPROCP, (ulong*)&proc_p);
		bp->b_proc = (struct proc *)proc_p;
		bp->b_flags &= ~B_DONE;
		bp->b_error  = 0;
		bp->b_blkno = phyarg->sectst << BLKSEC(dk);

		if (dir == V_RDABS) 
			bp->b_flags |= B_READ;
		else {
			bp->b_flags |= B_WRITE;
			if (mode != SD01_KERNEL) {	/* Copy user's data */
				if(copyin((caddr_t)phyarg->memaddr,
					(caddr_t)paddr(bp), size)) {
					phyarg->retval = V_BADWRITE;
					ret_val=EFAULT;
					freerbuf(bp);
					break;
				}
/*
 * 			If over-writing the PD sector and VTOC, make sure the PD
 * 			sector and VTOC data is sane before updating the disk.
 * 			Also, make sure the user is not destroying a mounted or 
 * 			mirrored partition.
 */
				if ((phyarg->sectst == dk->unixst + HDPDLOC) &&
				    (ret_val=sd01vtoc_ck(dk, bp, SD01_PBLK))) {
					phyarg->retval = V_BADWRITE;
					break;
				}
			}
		}

		jp = sd01getjob(dk); /* Job is returned by sd01done */
		set_sjq_daddr(jp,phyarg->sectst);
		set_sjq_memaddr(jp,paddr(bp));
		jp->j_seccnt = size >> BLKSHF(dk);

		sd01batch(dk);
		sd01strat1(jp, dk, bp);
		sd01batch(dk);
		biowait(bp);
		if(bp->b_flags & B_ERROR) {			
			if (bp->b_error == EFAULT) { /* retry after software remap */
				continue;
			} else {		/* Fail the request */
				phyarg->retval = dir == V_RDABS ? V_BADREAD:V_BADWRITE;
				ret_val = bp->b_error;
				break;
			}
		}
		
		/* Check if we are updating the fdisk table */
		if (dir == V_WRABS && phyarg->sectst == FDBLKNO 
			&& mode == SD01_USER)
		{	
			if (((struct mboot *)bp->b_un.b_addr)->signature 
			   != MBB_MAGIC) {
				dk->dk_state |= DKUP_VTOC;
				dk->dk_vtoc.v_sanity = 0;
			}
		}

		if(dir == V_RDABS && mode != SD01_KERNEL)	
		{			/* Update the requestor's buffer */
			if (copyout((caddr_t)bp->b_un.b_addr, (caddr_t)phyarg->memaddr, size))
			{
				phyarg->retval = V_BADREAD;
				ret_val=EFAULT;
				break;
			}
		}
		
		phyarg->memaddr += size;
		phyarg->datasz -= size;
		phyarg->sectst++;
	}
	
	if (mode != SD01_KERNEL) {
		kmem_free(tmp_buf, BLKSZ(dk));
		tmp_buf = NULL;
	}

	freerbuf(bp);

#ifdef DEBUG
        DPR (2)("sd01phyrw: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sd01cmd - Perform a SCSI command. - 0dd15000
 * %DESCRIPTION: 
	This funtion performs a SCSI command such as Mode Sense on
	the addressed disk. The op code indicates the type of job
	but is not decoded by this function. The data area is
	supplied by the caller and assumed to be in kernel space. 
	This function will sleep.
 * %CALLED BY: sd01wrtimestamp, sd01flt, sd01ioctl
 * %SIDE EFFECTS: 
	A Mode Sense command is added to the job queue and sent to
	the host adapter.
 * %RETURN VALUES:
	Zero is returned if the command succeeds. 
 * %END HEADER */
int
sd01cmd(dk, op_code, addr, buffer, size, length, mode)
register struct disk *dk;
char op_code;
unsigned int addr;			/* Address field of command 	*/
char *buffer;				/* Buffer for Mode Sense data 	*/
unsigned int size;			/* Size of the data buffer 	*/
unsigned int length;			/* Block length specified in CDB*/
unsigned short mode;			/* Direction of the transfer 	*/
{
	extern void sd01done();
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	register struct scsi_iotime *stat;
	int error, spls;
	unsigned long *proc_p;

#ifdef DEBUG
        DPR (1)("sd01cmd: (dk=0x%x) ", dk);
#endif
	
	bp = getrbuf(KM_SLEEP);
	bp->b_flags |= B_BUSY;
	bp->b_error = 0;
 	bp->b_iodone = NULL;
 	bp->b_vp = NULL;
	
	for (;;) 
	{
	stat = &dk->dk_stat;
	jp   = sd01getjob(dk);
	scb  = &jp->j_cont->SCB;
	
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sd01intn */
	bp->b_bcount = size;
	bp->b_error = 0;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	
	if (op_code & 0x20) { /* Group 1 commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = dk->dk_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else {	/* Group 0 commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = dk->dk_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	drv_getparm(UPROCP, (ulong*)&proc_p);

	scb->sc_int = sd01intn;
	scb->sc_dev = dk->dk_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;
	sdi_translate(jp->j_cont, bp->b_flags,(caddr_t)proc_p);
	dk->dk_count++;
	stat->ios.io_ops++;
	stat->ios.io_misc++;
	stat->ios.io_qcnt++;
	
/* 	Add job to the end of the queue and batch the queue 		*/
	dk->dk_state |= DKBUSYQ;

	jp->j_forw = (struct job *) dk;
	jp->j_back = dk->dk_back;
	dk->dk_back->j_forw = jp;
	dk->dk_back = jp;
	if (dk->dk_next == (struct job *) dk)
		dk->dk_next = jp;
	dk->dk_batch = (struct job *) dk;
	
	sd01send(dk, (int)-1);
	biowait(bp);

/*
 *	exit if success or if error not due to software bad sector remap
 */
	if (bp->b_flags & B_ERROR) {
		if (bp->b_error != EFAULT)
			break;
	} else
		break;

	}

#ifdef DEBUG
	if (bp->b_flags & B_ERROR)
        	DPR (2)("sd01cmd: - exit(%d) ", bp->b_error);
	else
        	DPR (2)("sd01cmd: - exit(0) ");
#endif
	if (bp->b_flags & B_ERROR)
		error = bp->b_error;
	else
		error = 0;
	freerbuf(bp);
	return (error);
}

/* %BEGIN HEADER
 * %NAME: sd01wrtimestamp - Write the disk time stamp. - 0dd18000
 * %DESCRIPTION: 
	This function is used to update the time stamps in the VTOC on disk.
	The time stamp will not be effected if the disk or the partition is
	write protected.  The function will fail if the write fail or the
	VTOC is undefined. 
 * %CALLED BY: sd01strategy, mdstrategy, mdmirror, mdclose
 * %SIDE EFFECTS: 
	The time stamp is set to \fItime\fR and the VTOC and PD info sector 
	is written to disk.
 * %RETURN VALUES: 
	.VL 4
	.LI "-1. "
	The VTOC could not be written.
	.LI " 0. "
	The time stamp was updated normally.
	.LE
 * %END HEADER */
int
sd01wrtimestamp(dk, part, time)
register struct disk *dk;
register int part;
time_t time;
{
	struct 	 phyio arg;
	register char *ptr1, *ptr2;	/* Used for data copies 	*/
	unsigned voffset;
	caddr_t  secbuf;
	int 	 i;

	if (dk->dk_vtoc.v_sanity != VTOC_SANE)
		return(-1);
		
	if (dk->dk_vtoc.v_part[part].p_flag & V_RONLY)
		return(0);

	/*
	*  Disktd has been asked to update the timestamp, make sure that
	*  if the partition is mirrored, set the flag so that a resulting
	*  error does not kill the job!
	*  Once the flag is set, it must be cleared before returning.
	*/
	if (dk->dk_part_flag[part] & DKONLY)
	{
		while(dk->dk_state & DKTSMD)
			sleep((caddr_t)&dk->dk_state, PRIBIO);
		dk->dk_state |= DKTSMD;
	}
	
	/* Allocate temporary memory for the PD/VTOC sector */
	if((secbuf = kmem_alloc(BLKSZ(dk), KM_SLEEP)) == NULL)
		return(-1);

	/* Update the sector buffer PD info */
	ptr1 = (char *) &dk->dk_pdsec;
	ptr2 = secbuf;
	for(i = 0; i < sizeof(dk->dk_pdsec); i++)
		*ptr2++ = *ptr1++;

	/* Update the time stamp */
	dk->dk_vtoc.timestamp[part] = time;
	
	/* Update the sector buffer VTOC */
        voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
	ptr1 = (char *) &dk->dk_vtoc;
	ptr2 = secbuf + voffset;
	for(i = 0; i < sizeof(dk->dk_vtoc); i++)
		*ptr2++ = *ptr1++;

	/* Write the time stamp */
	arg.sectst  = dk->unixst + HDPDLOC;
	arg.memaddr = (long) secbuf;
	arg.datasz  = BLKSZ(dk);
	sd01phyrw(dk, V_WRABS, &arg, SD01_KERNEL);

	/* Clear the update timestamp for a mirrored partition flag */
	dk->dk_state &= ~DKTSMD;
	wakeup((caddr_t)&dk->dk_state);
	kmem_free(secbuf, BLKSZ(dk));
	secbuf = NULL;
	
	if (arg.retval)			/* Check if the write was done */
		return(-1);
	else
		return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01rdtimestamp - Read the partition's time stamp. - 0dd18000
 * %DESCRIPTION: 
	This function and its error numbers are obsolete.
 * %END HEADER */

/* %BEGIN HEADER
 * %NAME: sd01ioctl - ioctl function for SCSI disk driver. - 0dd1a000
 * %DESCRIPTION: 
	This function provides a number of different functions for use by
	utilities. They are: physical read or write, and read or write
	physical descriptor sector.  
	.VL 10
	.LI "READ ABSOLUTE"
	The Absolute Read command is used to read a physical sector on the
	disk regardless of the VTOC.  The data is transferred into buffer
	specified by the argument structure.
	.LI "WRITE ABSOLUTE"
	The Absolute Write command is used to write a physical sector on the
	disk regardless of the VTOC.  The data is transferred from a buffer
	specified by the argument structure.
	.LI "PHYSICAL READ"
	The Physical Read command is used to read any size data block on the
	disk regardless of the VTOC or sector size.  The data is transferred 
	into buffer specified by the argument structure.
	.LI "PHYSICAL WRITE"
	The Physical Write command is used to write any size data block on the
	disk regardless of the VTOC or sector size.  The data is transferred 
	from a buffer specified by the argument structure.
	.LI "READ PD SECTOR"
	This function reads the physical descriptor sector on the disk.
	.LI "WRITE PD SECTOR"
	This function writes the physical descriptor sector on the disk.
	.LI "CONFIGURATION"
	The Configuration command is used by mkpart to reconfigure a drive.
	The driver will update the in-core disk configuration parameters.
	.LI "GET PARAMETERS"
	The Get Parameters command is used by mkpart and fdisk to get 
	information about a drive.  The disk parameters are transferred
	into the disk_parms structure specified by the argument.
	.LI "RE-MOUNT"
	The Remount command is used by mkpart to inform the driver that the 
	contents of the VTOC has been updated.  The driver will update the
	in-core copy of the VTOC on the next open of the device.
	.LI "PD SECTOR NUMBER"
	The PD sector number command is used by 386 utilities that need to
	access the PD and VTOC information. The PD and VTOC information
	will always be located in the 29th block of the UNIX partition.
	.LI "PD SECTOR LOCATION"
	The PD sector location command is used by SCSI utilities that need to
	access the PD and VTOC information. The absolute address of this
	sector is transferred into an integer specified by the argument.
	.LI "ELEVATOR"
	This Elevator command allows the user to enable or disable the
	use of the elevator algorithm.
	.LI "RESERVE"
	This Reserve command will reserve the addressed device so that no other
	initiator can use it.
	.LI "RELEASE"
	This Release command releases a device so that other initiators can
	use the device.
	.LI "RESERVATION STATUS"
	This Reservation Status command informs the host if a device is 
	currently reserved, reserved by another host or not reserved.
	.LE
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The requested action is performed.
 * %RETURN VALUES: Error value 
 * %END HEADER */
int
sd01ioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t dev;
register int cmd;
register int arg;
int mode;
struct cred *cred_p;
int *rval_p;			/* Success return value; not used here */
{
	register struct disk *dk;
	int	state;			/* Device's RESERVE Status 	*/
	struct 	phyio 	  phyarg;	/* Physical block(s) I/O	*/
	union 	io_arg 	  ioarg;
	struct 	absio 	  absarg;	/* Absolute sector I/O only	*/
	struct 	disk_parms  darg;
	struct 	vtoc 	 vtocarg;	/* soft-VTOC only */
	dev_t 	pt_dev;			/* Pass through device number 	*/
	int 	ret_val = 0;
	int	sd01_nodes = 16;	/* used by B_GET_SUBDEVS ioclt */
	int	part;

	extern 	void	sd01flt();
	
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

 	dk = DKPTR(dev);
	part = DKSLICE(dev);

	/*
	 * Reject the request if the partition has not been opened.
	 */
	if ((dk->dk_part_flag[part] & DKGEN) == 0) {
		return(ENXIO);
	}

#ifdef DEBUG
        DPR (1)("sd01ioctl: (dev=0x%x cmd=0x%x, arg=0x%x) dk=0x%x ", dev, cmd, arg, dk);
#endif

        if (cmd == SD01_DEBUGFLG) {
#ifdef DEBUG
                register short  dbg;

		if (copyin ((char *)arg, Sd01_debug, 10) != 0)
			return(EFAULT);

                printf ("\nNew debug values :");

                for (dbg = 0; dbg < 10; dbg++) {
                        printf (" %d", Sd01_debug[dbg]);
                }
                printf ("\n");
#endif
                return(0);
        }

	switch(cmd)
	{
		case V_WRABS: 	/* Write absolute sector number */
		case V_PWRITE:	/* Write physical data block(s)	*/
		case V_PDWRITE:	/* Write PD sector only		*/

                	/* Make sure user has permission */
			if (ret_val = drv_priv(cred_p))
			{
				return(ret_val);
			}

#ifdef NOUPDVTOC
                	/* Make sure vtoc version is supported */
			if (dk->dk_vtoc.v_sanity == VTOC_SANE && 
				dk->dk_vtoc.v_version > V_VERSION)
			{
				sd01insane_dsk(dk);
				return(EIO);
			}
#endif

                	/* Make sure partition is not mirrored */
			if (dk->dk_part_flag[DKSLICE(dev)] & DKONLY)
                        	return(EBUSY);


		case V_RDABS: 	/* Read absolute sector number 	*/
		case V_PREAD:	/* Read physical data block(s)	*/
		case V_PDREAD:	/* Read PD sector only		*/

			if (cmd == V_WRABS || cmd == V_RDABS) {
				if (copyin((caddr_t)arg, (caddr_t)&absarg, sizeof(absarg)))
					return(EFAULT);

				phyarg.sectst  = (unsigned long) absarg.abs_sec;
				phyarg.memaddr = (unsigned long) absarg.abs_buf;
				phyarg.datasz  = BLKSZ(dk);

				ret_val = sd01phyrw(dk, cmd, &phyarg, SD01_USER);
			}
			else {
				if (copyin((caddr_t)arg, (caddr_t)&phyarg, sizeof(phyarg)))
					return(EFAULT);

				/* Assign PD sector address */
				if (cmd == V_PDREAD || cmd == V_PDWRITE)
				{
					phyarg.sectst = dk->unixst + HDPDLOC;
					cmd = cmd == V_PDREAD ? V_RDABS : V_WRABS;
				}
				else
					cmd = cmd == V_PREAD ? V_RDABS : V_WRABS;

				ret_val = sd01phyrw(dk, cmd, &phyarg, SD01_USER);
				/* Copy out return value to user */
                		if (copyout((caddr_t)&phyarg, (caddr_t)arg, sizeof(phyarg)))
                        		return(EFAULT);
			}

			return(ret_val);

		/* Change drive configuration parameters. */
		case V_CONFIG: {
			int part;

			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

			/* Make sure no other partitions are open or mirrored */
			/* (This also insures DKSLICE(dev) == 0) */
			for (part = 1; part < V_NUMPAR; part++) {
				if (dk->dk_part_flag[part] != DKFREE)
								return(EBUSY);
			}
            /* Make sure partition 0 is not mirrored */
			if (dk->dk_part_flag[0] & DKONLY)
                        	return(EBUSY);

			if (copyin((caddr_t)arg, (caddr_t)&ioarg, sizeof(ioarg)))
				return(EFAULT);

			/* Don't allow user to change sector size. */
			if (ioarg.ia_cd.secsiz != BLKSZ(dk))
				return(EINVAL);

			/* Don't allow user to set dp_cyls to zero. */
			if (ioarg.ia_cd.ncyl == 0)
				return(EINVAL);

			dk->dk_parms.dp_heads   = ioarg.ia_cd.nhead;
			dk->dk_parms.dp_cyls    = ioarg.ia_cd.ncyl;
			dk->dk_parms.dp_sectors = ioarg.ia_cd.nsec;

			/* Indicate drive parms have been set */
			dk->dk_state |= DKPARMS; 
			break;
		}

		/* Get info about the current drive configuration */
		case V_GETPARMS: {
			struct disk_parms	dk_parms;
			int part = DKSLICE(dev);

			if((dk->dk_state & DKPARMS) == 0)
				return(ENXIO);

			/*
			** This used to be hard coded to DPT_SCSI_HD since
			** sd01 used to be strictly a scsi driver, but with
			** the new Generic Disk Interface from ISC we're
			** incorporating into our source, this driver also
			** is the high level interface to dcd drives also.
			**
			** The string "DCD" is defined in sdi/space.c
			*/

			if (strncmp(IDP(HBA_tbl[SDI_HAN(&dk->dk_addr)].idata)->name, "DCD", 3) == 0)
                                dk_parms.dp_type     = DPT_WINI;
			else
                                dk_parms.dp_type     = DPT_SCSI_HD;

			dk_parms.dp_heads    = dk->dk_parms.dp_heads;
			dk_parms.dp_cyls     = dk->dk_parms.dp_cyls;
			dk_parms.dp_sectors  = dk->dk_parms.dp_sectors;
			dk_parms.dp_secsiz   = dk->dk_parms.dp_secsiz;
			dk_parms.dp_ptag     = dk->dk_vtoc.v_part[part].p_tag;
			dk_parms.dp_pflag    = dk->dk_vtoc.v_part[part].p_flag;
			dk_parms.dp_pstartsec= dk->dk_vtoc.v_part[part].p_start;
			dk_parms.dp_pnumsec  = dk->dk_vtoc.v_part[part].p_size;

                	if (copyout((caddr_t)&dk_parms, (caddr_t)arg, sizeof(dk_parms))) 
				return(EFAULT);
                	break;
		}

		/* Force read of vtoc on next open.*/
		case V_REMOUNT:	 {
           	register int    part;

           	/* Make sure user is root */
			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

           	/* Make sure no partitions other than 0 are open. */
			for (part=0; part < V_NUMPAR; part++)
			{
				if ((dk->dk_part_flag[part] != DKFREE) &&
				     part != 0)
					break;
			}

                	if (part != V_NUMPAR)
                        	return(EBUSY);

			/* We have made no other checks here. */
			dk->dk_state |= DKUP_VTOC;
			dk->dk_vtoc.v_sanity = 0;
                	break;

		}

		/* Tell user the block number of the pdinfo structure */
        	case V_PDLOC: {	
                	unsigned long   pdloc;

			/* Check if fdisk is sane */
			if((dk->dk_state & DKFDISK)) 
                		pdloc = HDPDLOC;
			else 
                        	return(ENXIO);

                	if (copyout((caddr_t)&pdloc, (caddr_t)arg, sizeof(pdloc)))
                        	return(EFAULT);
                	break;
		}

/*		update the internal alternate bad sector/track table	*/
		case V_ADDBAD: {
			ret_val=sd01addbad(dev,arg);
			break;
		}

		/* Update soft VTOC in DK structure. */
		case V_SET_SOFT_VTOC: {
			if ( ! Sd01_boot_time )
				return(EINVAL);

			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

            /* Make sure partition 0 is not mirrored */
			if (dk->dk_part_flag[0] & DKONLY)
                        	return(EBUSY);

			if (copyin((caddr_t)arg, (caddr_t)&vtocarg, sizeof(vtocarg)))
				return(EFAULT);

			bcopy((caddr_t)&vtocarg, (caddr_t)&(dk->dk_vtoc), sizeof(vtocarg));

			/* Indicate soft-VTOC has been set */
			dk->vtoc_state = VTOC_SOFT; 
			break;
		}

		/* Get the soft VTOC from the DK structure. */
		case V_GET_SOFT_VTOC: {
			if ((dk->dk_vtoc.v_sanity != VTOC_SANE))
				return(ENXIO);

        		if (copyout((caddr_t)&(dk->dk_vtoc), (caddr_t)arg, sizeof(vtocarg))) 
				return(EFAULT);
                	break;
		}

		/* Tell user where pdinfo structure is on the disk */
        	case SD_PDLOC: {	
                	unsigned long   pdloc;

			/* Check if fdisk is sane */
			if((dk->dk_state & DKFDISK))
                		pdloc = dk->unixst + HDPDLOC;
			else 
                        	return(ENXIO);

                	if (copyout((caddr_t)&pdloc, (caddr_t)arg, sizeof(pdloc)))
                        	return(EFAULT);
                	break;
		}

		case SD_ELEV:
			if ((long) arg)
				dk->dk_state |= DKEL_OFF;
			else
				dk->dk_state &= ~DKEL_OFF;
			break;

		case SDI_RESERVE:
			if (ret_val = sd01cmd(dk, SS_RESERV, 0, NULL, 0, 0, SCB_WRITE) == 0)
			{
				dk->dk_state |= DKRESERVE;
				dk->dk_state |= DKRESDEVICE;
				sdi_fltinit(&dk->dk_addr, sd01flt, dk);
			}
			break;

		case SDI_RELEASE:
			if (ret_val = sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ) == 0) {
				if (ret_val = sd01cmd(dk, SS_RELES, 0, NULL, 0, 0, SCB_WRITE) == 0)
				{
					dk->dk_state &= ~DKRESERVE;
					dk->dk_state &= ~DKRESDEVICE;
					dk->dk_state |= DKUP_VTOC;
					dk->dk_vtoc.v_sanity = 0;
					sdi_fltinit(&dk->dk_addr, NULL, 0);
				}
			}
			break;

		case SDI_RESTAT:
			if (dk->dk_state & DKRESERVE)
				state = 1;	/* Currently Reserved */
			else {
				if(sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ) == EBUSY)
					state = 2;	/* Reserved by another host*/
				else
					state = 0;	/* Not Reserved */
			}

			if (copyout((caddr_t)&state, (caddr_t)arg, 4))
				return(EFAULT);
			break;

		case B_GETTYPE:
			if (copyout((caddr_t)"scsi", 
				((struct bus_type *) arg)->bus_name, 5))
			{
				return(EFAULT);
			}
			if (copyout((caddr_t)"sd01", 
				((struct bus_type *) arg)->drv_name, 5))
			{
				return(EFAULT);
			}
			break;

		case B_GETDEV:
			sdi_getdev(&dk->dk_addr, &pt_dev);
			if (copyout((caddr_t)&pt_dev, (caddr_t)arg, sizeof(pt_dev)))
				return(EFAULT);
			break;	
		case B_GET_SUBDEVS:
			if (copyout((caddr_t)&sd01_nodes, (caddr_t)arg, sizeof(sd01_nodes)))
				return(EFAULT);
			break;	
		default:
			return(EINVAL);
	}

#ifdef DEBUG
        DPR (2)("sd01ioctl: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sd01print - Print routine for the kernel. - 0dd1b000
 * %DESCRIPTION: 
	The function prints the name of the addressed disk unit along 
	with an error message provided by the kernel.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %END HEADER */
int
sd01print(dev, str)
dev_t dev;
register char *str;
{
	register struct disk *dk;
	char name[NAMESZ];
	struct scsi_ad addr;

#ifdef DEBUG
        DPR (1)("sd01print: (dev=0x%x) ", dev);
#endif
	
	addr.sa_major = getmajor(dev);
	addr.sa_minor = getminor(dev);
	dk = DKPTR(dev);
	addr.sa_lun   = dk->dk_addr.sa_lun;
	addr.sa_exlun = dk->dk_addr.sa_exlun;
	addr.sa_fill  = dk->dk_addr.sa_fill;
	sdi_name(&addr, name);
	cmn_err(CE_WARN, "Disk Driver (%s), Unit %d, Slice %d:  %s", name, 
		addr.sa_lun, DKSLICE(dev), str);

#ifdef DEBUG
        DPR (2)("sd01print: - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01batch - Start a new batch in the disk queue. - 0dd1c000
 * %DESCRIPTION: 
	This function set a new batch in the disk queue.  It is to 
	ensure that the following jobs which are sent will not be
	reordered with any currently pending jobs.  
 * %CALLED BY: mdrestore
 * %SIDE EFFECTS: A new batch group is setup.
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01batch(dk)
register struct disk *dk;
{
	dk->dk_batch = (struct job *) dk;
	dk->dk_state ^= DKDIR;
}

/* %BEGIN HEADER
 * %NAME: sd01insane_dsk - Log an error for an insane PD sector or VTOC. - 0dd1f000
 * %DESCRIPTION: 
	This function prints and logs an error when some attempts to 
	access a disk which does not have a valid PD sector or VTOC.
 * %CALLED BY: sd01strategy, sd01open1, sd01ioctl
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %ERROR 6dd1f001
	The physical descriptor sector is bad on the addressed disk.
	The disk must be formated before it can be accessed for 
	normal use. See format(1M).
 * %END ERROR
 * %ERROR 6dd1f002
	The Volume Table of Contents is bad on the addressed disk. The 
	UNIX system must be partitioned before it can be accessed for normal 
	use. See mkpart(1M) and edvtoc(1M).
 * %END ERROR
 * %ERROR 6dd1f003
	The Volume Table of Contents version on the addressed disk is not 
	supported. The disk must be re-formated and re-partitioned before 
	it can be accessed for normal use. See scsiformat(1M), mkpart(1M)
	and edvtoc(1M).
 * %END ERROR
 * %ERROR 6dd1f004
	The Partition Table is bad on the addressed disk. The disk must 
	be partitioned before it can be accessed for normal use. See fdisk(1M).
 * %END ERROR
 * %END HEADER */
void
sd01insane_dsk(dk)
register struct disk *dk;
{
	char name[46];

#ifdef DEBUG
        DPR (1)("sd01insane_dsk: (dk=0x%x) ",dk);
#endif
	
	sdi_name(&dk->dk_addr, name);
	
	/* Check if fdisk has been initialized */
	if(!(dk->dk_state & DKFDISK)) {
		cmn_err(CE_WARN, "Disk Driver: %s Unit %d, Invalid disk partition table\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* Check if PD sector is sane */
	else if (dk->dk_pdsec.sanity != VALID_PD)
	{
		cmn_err(CE_WARN, "Disk Driver: %s Unit %d, Invalid physical descriptor sector\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* Check if VTOC version is supported */
	else if (dk->dk_vtoc.v_sanity == VTOC_SANE && dk->dk_vtoc.v_version > V_VERSION)
	{
		cmn_err(CE_WARN, "Disk Driver: %s Unit %d, Invalid disk VTOC version\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* VTOC must be insane */
	else {

		cmn_err(CE_WARN, "Disk Driver: %s Unit %d, Invalid disk VTOC\n",
		 	name, dk->dk_addr.sa_lun);
	}

#ifdef DEBUG
        DPR (2)("sd01insane_dsk: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01vtoc_ck - Check that the VTOC will not be destroyed. - 0dd20000
 * %DESCRIPTION: 
	This function checks writes which might destroy the VTOC
	sector.  This is to prevent the user from accidently over
	writing the VTOC or PD sector with invalid data.  It will also
	invalidate the current copy of the VTOC so that the next
	access will read in both the VTOC and PD info. The user data must 
	be directly accessable when this function is called.
	If the partition argument is equal to 16 or greater the block address 
	is assumed to be physical (same as sd01strat1).
 * %CALLED BY: sd01strategy, sd01phyrw, mdstrategy, and mddone
 * %SIDE EFFECTS: The VTOC entry is updated.
 * %RETURN VALUES: 0 if the write is ok, else an error number.
 * %END HEADER */
int
sd01vtoc_ck(dk, bp, part)
register struct disk *dk;
register buf_t *bp;
int part;
{
	char  	*pt1, *pt2;
	struct	vtoc  *vtocptr;
	long    i, start, slice, voffset, vtocblk, blksz, offset, retval;

        voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
	slice   = DKSLICE(bp->b_edev);
	vtocblk = dk->unixst + VTBLKNO;
	blksz   = sizeof(dk->dk_pdsec) + sizeof(dk->dk_vtoc);
	retval  = 0;

	if (part < SD01_PBLK)			/* Logical address  */
		start = (bp->b_blkno >> BLKSEC(dk)) +
			dk->dk_vtoc.v_part[slice].p_start;
	else					/* Physical address */
		start = bp->b_blkno >> BLKSEC(dk);

#ifdef DEBUG
        DPR (1)("sd01vtoc_ck: (dk=0x%x bp=0x%x) start=0x%x bcount=0x%x blkno=0x%x ", dk, bp, start, bp->b_bcount, bp->b_blkno);
#endif

	if (start > vtocblk || start < vtocblk 
	   && bp->b_bcount <= BLKSZ(dk) * (vtocblk - start)) {
		return(0);	/* We are not hitting the PD/VTOC sector */
	}
		
	if ((start < vtocblk && bp->b_bcount > BLKSZ(dk) * (vtocblk-start)) &&
		(bp->b_bcount < BLKSZ(dk) * (vtocblk-start) + blksz) ||
		(start == vtocblk && bp->b_bcount < blksz)) {
		return(EACCES);	/* Must write the entire PD/VTOC sector */
	}
		
	/* Check in-core VTOC sanity */
	if (dk->dk_vtoc.v_sanity != VTOC_SANE)
	{
		/* Update new VTOC and PD sector on next access. */
		dk->dk_state |= DKUP_VTOC;
		dk->dk_vtoc.v_sanity = 0;
		return(0);	/* Let user overwrite bad in-core copy. */
	}

	/* Check PD info sanity */
	offset = (char *) &dk->dk_pdsec.sanity - (char *) &dk->dk_pdsec;
	pt1 = (char *) &dk->dk_pdsec.sanity;
	pt2 = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start)) + offset;

	for (i = 0; i < sizeof(dk->dk_pdsec.sanity); i++)
	{
		if (*pt1++ != *pt2++) {
			return(EACCES);	/* The new one is not sane. */
		}
	}

	/* Check new VTOC sanity */
	offset = (char *) &dk->dk_vtoc.v_sanity - (char *) &dk->dk_vtoc;
	pt1 = (char *) &dk->dk_vtoc.v_sanity;
	pt2 = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start));
	voffset = ((struct pdinfo *)pt2)->vtoc_ptr & (BLKSZ(dk)-1);
	pt2 += voffset + offset;

	for (i = 0; i < sizeof(dk->dk_vtoc.v_sanity); i++)
	{
		if (*pt1++ != *pt2++) {
			return(EACCES);	/* The new one is not sane */
		}
	}

	/* Check if the number of partitions is supported */
	pt2 = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start)) + voffset;

	if(((struct vtoc *)pt2)->v_nparts > V_NUMPAR)
		return(EACCES);

	/* Check for mount/mirror violations */
	if (retval = sd01part_ck(dk, bp, start))
	{
		return(retval);
	}

	/* We have passed all of the checks. */
	dk->dk_state |= DKUP_VTOC;
	dk->dk_vtoc.v_sanity = 0;

#ifdef DEBUG
        DPR (2)("sd01vtoc_ck: - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01szsplit - Splits large transfers. - 0dd22000
 * %DESCRIPTION: 
	This function splits up large transfers so they do not
	use lots of resources.  Each piece is done separately.
 * %CALLED BY: sd01strategy, mdstrategy
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %ERROR 0dd22001
	The disk driver can not allocate a raw buffer header.  This is 
	caused by insufficient virtual or physical memory.
 * %END ERROR
 * %END HEADER */
void
sd01szsplit(obp, dev, strategy)
register buf_t *obp;
struct scsi_ad *dev;
void (*strategy)();
{
	register buf_t *bp;
	register int count;
	
	/*
	 * PAGEIO requests cannot be split up.
	 */

 	if ((bp = (buf_t *) getrbuf(KM_SLEEP)) == 0)	{
 		obp->b_flags |= B_ERROR;
 		obp->b_error = EIO;
 		cmn_err(CE_NOTE, "!Disk Driver: could not allocate buffer header.");
 		biodone(obp);
 		return;
 	}
	*bp = *obp;
	count = obp->b_bcount;
	while(count > 0)
	{
		bp->b_flags |= B_BUSY;
		bp->b_flags &= ~(B_PAGEIO|B_REMAPPED|B_DONE|B_ASYNC);
		bp->b_error = 0;
		bp->b_iodone = NULL;
		bp->b_vp = NULL;
 		bp->b_bcount = count > HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer ?
 			HIP(HBA_tbl[SDI_HAN(dev)].info)->max_xfer : count;
		
		strategy(bp);
		biowait(bp);
		if (bp->b_flags & B_ERROR)
		{
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
			break;
		}
		
		bp->b_un.b_addr += bp->b_bcount;
		bp->b_blkno += bp->b_bcount >> KBLKSHF;
		count -= bp->b_bcount;
	}
	
	freerbuf(bp);
	biodone(obp);
}

/* %BEGIN HEADER
 * %NAME: sd01flt - Fault Handling routine called by HAD. - 0dd23000
 * %DESCRIPTION:
	This function is called by the host adapter driver when either
	a Bus Reset has occurred or a device has been closed after
	using Pass-Thru. This function begins a series of steps that
	ensure that the device is RESERVED before trying further jobs.
	If this function is called due to Pass-Thru, then the PD Sector
	and VTOC should be updated. This function cannot set the
	job pointer since you may be overwriting a valid job pointer.
 * %CALLED BY:  Host Adapter Driver
 * %SIDE EFFECTS:
	If due to Pass-Thru, PD sector and VTOC will be updated.
 * %RETURN VALUES: None.
 * %ERROR 8dd23001
	The HAD called this function due to a fault condition on the
	SCSI bus but passed an unknown parameter to this function.
 * %END ERROR
 * %END HEADER */
void
sd01flt(dk, flag)
struct	disk	*dk;	/* Points to Disk Structure */
long		flag;	/* Type of fault */
{

	extern	void	sd01flts();

#ifdef DEBUG
        DPR (1)("sd01flt: (dk=0x%x flag=%d) ", dk, flag);
#endif

	switch (flag)
	{
	  case	SDI_FLT_RESET:		/* LU was reset */
		break;

	  case	SDI_FLT_PTHRU:		/* Pass-Thru was used */
		dk->dk_state |= DKUP_VTOC;
		dk->dk_vtoc.v_sanity = 0;

		/*
		*  If the device had previously been RESERVED,
		*  then try to re-RESERVE it.
		*/
		if (dk->dk_state & DKRESDEVICE)
		{
			/*
			*  If the RESERVE fails, then the gauntlet will
			*  have been entered and the appropriate message
			*  will have been printed.
			*/
			if (sd01cmd(dk, SS_RESERV, 0, NULL, 0, 0, SCB_WRITE) == 0)
			{
				dk->dk_state |= DKRESERVE;
				sdi_fltinit(&dk->dk_addr, sd01flt, dk);
			}
		}
		return;

	  default:			/* Unknown type */
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type from HA Err: 8dd23001");
#endif
		return;
	}

	/* Go on to next step */
	sd01flts(dk);

#ifdef DEBUG
        DPR (2)("sd01flt: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01flts - Common start point for handling faults. - 0dd24000
 * %DESCRIPTION:
	This is the beginning of the 'gauntlet'. All faults must
	come thru this function in case something caused the device
	to become released. Before any corrective action can be taken,
	the device must be reserved again. A suspend SFB is sent to HAD
	to stop any further jobs to the device. The interrupt routine
	will handle the next step of the gauntlet.
 * %CALLED BY: sd01flt, sd01intn
 * %SIDE EFFECTS:
	A suspend SFB is issued for this device.
 * %RETURN VALUES: None.
 * %ERROR 8dd24001
	The HAD rejected a Function request to Suspend the LU queue
	for the current device. The disk driver cannot proceed with the
	handling of the fault so the original I/O request will be failed.
 * %END ERROR
 * %END HEADER */
void
sd01flts(dk)
struct	disk *dk;
{

#ifdef DEBUG
        DPR (1)("sd01flts: (dk=0x%x) ", dk);
#endif

	/* Initialize the RESERVE Reset counter for later in the gauntlet */
	dk->dk_rescnt = 0;

	/* If disk is already in the gauntlet, do nothing */
	if (dk->dk_state & DKFLT)
		return;
	
	dk->dk_state |= DKFLT;

	dk->dk_spcount = 0;
	dk->dk_fltsus->sb_type = SFB_TYPE;
	dk->dk_fltsus->SFB.sf_int = sd01ints;
	dk->dk_fltsus->SFB.sf_func = SFB_SUSPEND;
	dk->dk_fltsus->SFB.sf_dev = dk->dk_addr;

	if (SD01ICMD(dk,dk->dk_fltsus) != SDI_RET_OK)
	{
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd24001");
#endif
		/* Clean up after the job */
		sd01flterr(dk, 0);
	}

#ifdef DEBUG
        DPR (2)("sd01flts: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01ints - Interrupt Handler for SUSPEND SFB jobs. -  0dd25000
 * %DESCRIPTION:
	This function is called by the Host Adapter Driver when the
	SUSPEND function has completed. If it failed, retry the job until
	the retry limit is exceeded. Once the disk queue is suspended,
	send a Request Sense job to find out what happened to the device.
 * %CALLED BY:  Host Adapter Driver
 * %SIDE EFFECTS:  Disk queue SUSPEND flag will be set. A Request Sense
	job will have been started.
 * %RETURN VALUES:  None.
 * %ERROR 4dd25001
	The SCSI disk driver retried a Function request. The retry
	was performed because the HAD detected an error.
 * %END ERROR
 * %ERROR 6dd25002
	The HAD detected an error with the SUSPEND function request
	and the retry count has already been exceeded.
 * %END ERROR
 * %ERROR 8dd25003
	The HAD rejected a Request Sense job from the SCSI disk
	driver. The original job will also be failed.
 * %END ERROR
 * %END HEADER */
void
sd01ints(sbp)
struct	sb *sbp;	/* SFB */
{
	register struct disk *dk;
	dev_t	dev;			/* External device number 	*/

	extern void	sd01intrq();
	extern void	sd01flterr();


	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = DKPTR(dev);

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01ints: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SFB.sf_comp_code & SDI_RETRY && dk->dk_spcount < SD01_RETRYCNT)
	{
		/* Retry the Suspend SFB */
		dk->dk_spcount++;
		dk->dk_error++;
		sd01logerr(sbp, (struct job *) NULL, 0x4dd25001);
		if (SD01ICMD(dk,sbp) != SDI_RET_OK)
		{
#ifdef DEBUG
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd25003");
#endif
			sd01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SFB.sf_comp_code != SDI_ASW)
	{
		sd01logerr(sbp, (struct job *) NULL, 0x6dd25002);
		sd01flterr(dk, 0);
		return;
	}

	/*
	*  The device is now SUSPENDED. Start the Request Sense Job.
	*/
	dk->dk_state |= DKSUSP;

	dk->dk_spcount = 0;
	dk->dk_fltreq->sb_type = ISCB_TYPE;
	dk->dk_fltreq->SCB.sc_int = sd01intrq;
	dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltreq->SCB.sc_link = 0;
	dk->dk_fltreq->SCB.sc_resid = 0;
	dk->dk_fltreq->SCB.sc_time = JTIME;
	dk->dk_fltreq->SCB.sc_mode = SCB_READ;
	dk->dk_fltreq->SCB.sc_dev = sbp->SFB.sf_dev;
	dk->dk_fltcmd.ss_op = SS_REQSEN;
	dk->dk_fltcmd.ss_lun = sbp->SFB.sf_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = SENSE_SZ;
	dk->dk_fltcmd.ss_cont = 0;
	dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */

	if (SD01ICMD(dk,dk->dk_fltreq) != SDI_RET_OK)
	{

#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd25003");
#endif
		sd01flterr(dk, 0);
		return;
	}

#ifdef DEBUG
        DPR (2)("sd01ints: - exit ");
#endif
}

/* BEGIN HEADER
 * %NAME: sd01intrq - Request Sense Interrupt handler. - 0dd26000
 * %DESCRIPTION:
	This function is called by the host adapter driver when a
	Request Sense job completes. This function will not examine the
	Sense data because there is still one more step before a normal
	I/O job can be restarted. Send the RESERVE command to the device
	to prevent some other host from using the device.
 * %CALLED BY: Host Adapter Driver
 * %SIDE EFFECTS:
	Either the Request Sense will be retried or the RESERVE command
	is sent to the device.
 * %RETURN VALUES: None
 * %ERROR 4dd26001
	The SCSI disk driver retried a Request Sense job that the
	HAD failed.
 * %END ERROR
 * %ERROR 8dd26002
	The HAD rejected a Request Sense job issued by the SCSI disk driver.
	The original job will also be failed.
 * %END ERROR
 * %ERROR 6dd26003
	The HAD detected an error in the last Request Sense job issued by the
	SCSI disk driver. The retry count has been exceeded so the original
	I/O request will also be failed.
 * %END ERROR
 * %ERROR 8dd26004
	The HAD rejected a Reserve job requested by the SCSI disk
	driver. The original job will also be failed.
 * %END ERROR
 * %END HEADER */
void
sd01intrq(sbp)
struct sb *sbp;		/* SCB */
{
	register struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = DKPTR(dev);

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01intrq: (sbp=0x%x) ", sbp);
        DPR (6)("sd01intrq: (sbp=0x%x) ", sbp);
#endif

	if (sbp->SCB.sc_comp_code != SDI_CKSTAT &&
	    sbp->SCB.sc_comp_code & SDI_RETRY &&
	    dk->dk_spcount <= SD01_RETRYCNT)
	{
		dk->dk_spcount++;
		dk->dk_error++;
		sbp->SCB.sc_time = JTIME;
		sd01logerr(sbp, (struct job *) NULL, 0x4dd26001);

		if (SD01ICMD(dk,sbp) != SDI_RET_OK)
		{
#ifdef DEBUG
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd26002");
#endif
			sd01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW)
	{
		dk->dk_error++;
		sd01logerr(sbp, (struct job *) NULL, 0x6dd26003);
		sd01flterr(dk, 0);
		return;
	}


	/*
	*  The sc_wd field must have been filled in when the
	*  fault was first detected by either sd01flt or sd01intn.
	*  It indicates if there is a real job associated with this fault!
	*/
	dk->dk_spcount = 0;
	dk->dk_fltres->sb_type = ISCB_TYPE;
	dk->dk_fltres->SCB.sc_int = sd01intres;
	dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltres->SCB.sc_datapt = NULL;
	dk->dk_fltres->SCB.sc_datasz = 0;
	dk->dk_fltres->SCB.sc_link = 0;
	dk->dk_fltres->SCB.sc_resid = 0;
	dk->dk_fltres->SCB.sc_time = JTIME;
	dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
	dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
	dk->dk_fltcmd.ss_op = SS_RESERV;
	dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = 0;
	dk->dk_fltcmd.ss_cont = 0;

	/*
	*  If the device is not suppose to be reserved,
	*  then go directly to the function to restart the original job.
	*  Put the check here so that the SB is initialized.
	*  Some of its data will be used even if no RESERVE is issued.
	*/
	if ((dk->dk_state & DKRESDEVICE) == 0)
	{
		sd01fltjob(dk);
		return;
	}

	if (SD01ICMD(dk,dk->dk_fltres) != SDI_RET_OK)
	{

#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd26004");
#endif
		sd01flterr(dk, 0);
	}

#ifdef DEBUG
        DPR (2)("sd01intrq: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01intres - Interrupt Handler for RESERVE device jobs -  0dd27000
 * %DESCRIPTION:
	This function is called by the host adapter driver when a RESERVE
	job completes. The job will be retried if it failed.
	If a RESET or CRESET prevented the RESERVE from completing, then
	the gauntlet must be started again. The SUSPEND job need not be redone
	since the queue is already suspended so go back to the Request Sense.
	If the SUSPEND succeeded, then if there was a real I/O job in progress
	the Request Sense data read previously will be examined to determine
	what to do.  Clear the disk fault flag that indicates this disk is in
	the gauntlet.
 * %CALLED BY: Host Adapter Driver
 * %SIDE EFFECTS: Either the RESERVE will be retried, the gauntlet will
 	be restarted, or an I/O job will be restarted.
 * %RETURN VALUES: None.
 * %ERROR 8dd27001
	The HAD rejected a Request Sense job issued by the SCSI disk
	driver. The original job will also be failed.
 * %END ERROR
 * %ERROR 8dd27002
	The HAD rejected a Reserve job issued by the SCSI disk driver.
	The original job will also be failed.
 * %END ERROR
 * %ERROR 6dd27003
	The HAD detected a failure in the last Reserve job issued
	by the SCSI disk driver. The retry count has been exceeded
	so the original job has been failed.
 * %END ERROR
 * %ERROR 4dd27004
	The SCSI disk driver is retrying an I/O request because of an error
	detected by the target controller. The cause of the error is
	indicated by the second and third error codes. These error
	codes are the sense key and extended sense code respectively.
	See the disk target controller code for more information.
	(OBSOLETE ERROR)
 * %END ERROR
 * %ERROR 4dd27005
	The disk controller performed retry or ECC which was
	successful. The cause of the error is indicated by the second
	and third error codes. There error codes are the sense key and
	extended sense code respectively. See the disk target controller
	codes for more information.  (OBSOLETE ERROR)
 * %END ERROR
 * %ERROR 6dd27006
	The RESERVE command caused the bus to reset and has exceeded
	its maximum retry count. The original job will be failed and the
	error handling code will be exited.
 * %END ERROR
 * %END HEADER */
void
sd01intres(sbp)
struct	sb *sbp;	/* SCB */
{
	struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = DKPTR(dev);

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01intres: (sbp=0x%x) ", sbp);
        DPR (6)("sd01intres: (sbp=0x%x) ", sbp);
#endif

	if (sbp->SCB.sc_comp_code & SDI_RETRY && dk->dk_spcount <= SD01_RETRYCNT)
	{
		if (sbp->SCB.sc_comp_code == SDI_RESET ||
		    sbp->SCB.sc_comp_code == SDI_CRESET ||
		    (sbp->SCB.sc_comp_code == SDI_CKSTAT &&
		     sbp->SCB.sc_status == S_CKCON))
		{
			/*
			*  Must restart the gauntlet!
			*  The queue has already been SUSPENDED, so go
			*  back and do the Request Sense job.
			*/
			if (sbp->SCB.sc_comp_code == SDI_CRESET && dk->dk_rescnt > SD01_RST_ERR)
			{
				/* This job has caused to many resets */
				sd01logerr(sbp, (struct job *) NULL, 0x6dd27006);
				sd01flterr(dk, 0);
				return;
			}

			dk->dk_rescnt++;
			dk->dk_spcount = 0;
			dk->dk_fltreq->sb_type = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int = sd01intrq;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time = JTIME;
			dk->dk_fltreq->SCB.sc_mode = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_REQSEN;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = SENSE_SZ;
			dk->dk_fltcmd.ss_cont = 0;
			dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */

			if (SD01ICMD(dk,dk->dk_fltreq) != SDI_RET_OK)
			{

#ifdef DEBUG
				cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd27001");
#endif
				sd01flterr(dk, 0);
			}
			return;
		}
		else 	/* Not RESET or CRESET */
		{
			dk->dk_fltres->sb_type = ISCB_TYPE;
			dk->dk_fltres->SCB.sc_int = sd01intres;
			dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltres->SCB.sc_link = 0;
			dk->dk_fltres->SCB.sc_resid = 0;
			dk->dk_fltres->SCB.sc_time = JTIME;
			dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
			dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_RESERV;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = 0;
			dk->dk_fltcmd.ss_cont = 0;
			dk->dk_sense.sd_key = 0;

			dk->dk_spcount++;
			dk->dk_error++;
		
			if (SD01ICMD(dk,dk->dk_fltres) != SDI_RET_OK)
			{
#ifdef DEBUG
				cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd27002");
#endif
				sd01flterr(dk, 0);
			}
			return;
		}
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW)
	{
		dk->dk_error++;
		sd01logerr(sbp, (struct job *) NULL, 0x6dd27003);
		sd01flterr(dk, 0);
		return;
	}

	/* RESERVE Job completed ASW */
	dk->dk_state |= DKRESERVE;
	sd01fltjob(dk);

#ifdef DEBUG
        DPR (2)("sd01intres: - exit ");
#endif
}

/* BEGIN HEADER
 * %NAME: sd01resume - Resume a suspended disk queue - 0dd28000
 * %DESCRIPTION:
	This function is called only if a queue has been suspended and must
	now be resumed. It is called by sd01comp1 when a job has been
	failed and a disk queue must be resumed or by sdflterr when there
	is no job to fail but the queue needs to be resumed anyway.
 * %CALLED BY: sd01comp1, sd01flterr,
 * %SIDE EFFECTS: THe LU queue will have been resumed.
 * %RETURN VALUES: None.
 * %ERROR 8dd28001
	The HAD rejected a Resume function request by the SCSI disk driver.
	This is caused by a parameter mismatch within the driver.
	The system should be rebooted.
 * %END ERROR
 * %END HEADER */
void
sd01resume(dk)
struct disk *dk;
{
	extern void	sd01intf();

#ifdef DEBUG
        DPR (1)("sd01resume: (dk=0x%x) ", dk);
#endif

	dk->dk_spcount = 0;	/* Reset special count */
	sd01_fltsbp->sb_type = SFB_TYPE;
	sd01_fltsbp->SFB.sf_int = sd01intf;
	sd01_fltsbp->SFB.sf_dev = dk->dk_addr;
	sd01_fltsbp->SFB.sf_func = SFB_RESUME;

	SD01ICMD(dk,sd01_fltsbp);
	dk->dk_state &= ~DKSUSP;

#ifdef DEBUG
        DPR (2)("sd01resume: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01flterr - Clean up after an unrecoverable error - 0dd29000
 * %DESCRIPTION:
	This function is called in the gauntlet when an error
	occurs. If the Gauntlet was unable to re-RESERVE the device,
	then let the user know with the logged error.
	If there is a job waiting to be retried, call
	sd01comp1 to fail the job and resume the queue.
	Otherwise, just resume the queue. A separate function was
	made since this test will need to be made all thru the gauntlet.
	It is assumed that the error has already been logged before
	this function is called.
 * %CALLED BY: sd01intn, sd01flt, sd01flts, sd01ints, sd01intrq, sd01intres
 * %SIDE EFFECTS: None.
 * %RETURN VALUES: None.
 * %ERROR 6dd29001
	The SCSI Disk Driver was unable to re-RESERVE a device.
	Some hardware problem or the device was reserved by some
	other host is probably causing the driver to fail in its attempt
	to RESERVE the device.
 * %END ERROR
 * %END HEADER */
void
sd01flterr(dk, res_flag)
struct disk *dk;
int	res_flag;	/* Indicates if device is still RESERVED */
{
	register struct job *jp;

#ifdef DEBUG
        DPR (1)("sd01flterr: (dk=0x%x res_flag=%d) ", dk, res_flag);
#endif

	/*
	*  If the device was RESERVED but the gauntlet was unable to
	*  re-RESERVE the device, clear the RESERVE flag, let user
	*  know there was a problem and log the error.
	*  Also set the flag indicating that the device should be RESERVED
	*  the next chance it gets.
	*/
	if (dk->dk_state & DKRESERVE && res_flag == 0)
	{
		dk->dk_state &= ~DKRESERVE;

		sdi_name(&dk->dk_addr, sd01name);
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Dev not RESERVED Err: 6dd29001");
		cmn_err(CE_WARN, "%s, Unit = %d", sd01name, dk->dk_addr.sa_lun);
#endif
	}

	/*
	*  The gauntlet is finished!
	*  Clear the state and reset the job pointer so that
	*  the next time the gauntlet is entered, it has been initialized
	*  to the proper state.
	*/
	jp = (struct job *)dk->dk_fltres->SCB.sc_wd;
#ifdef DEBUG
        DPR (2)("jp in comp 0x%x ",jp);
#endif
	dk->dk_fltres->SCB.sc_wd = NULL;
	dk->dk_state &= ~DKFLT;

	/* Is there a job to restart */
	if (jp == NULL)
		sd01qresume(dk);	/* No job */
	else
		sd01comp1(jp);		/* Return the job */

#ifdef DEBUG
        DPR (2)("sd01flterr: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01qresume - Checks if the SB for Resuming s LU queue is busy - 0dd2a000
 * %DESCRIPTION:
	This function will check if the SB used for resuming a LU queue
	is currently busy. If it is busy, the current disk is added to the
	end of the list of disks waiting for a resume to be issued.
	If the SB is not busy, this disk is put at the front of the
	list and the resume for this disk is started immediately.
 * %CALLED BY: sd01comp1, sd01flterr
 * %SIDE EFFECTS: A disk structure is added to the Resume queue.
 * %RETURN VALUES: None.
 * %END HEADER */
void
sd01qresume(dk)
struct disk *dk;
{

#ifdef DEBUG
        DPR (1)("sd01qresume: (dk=0x%x) ", dk);
#endif
	
	/* Check if the Resume SB is currently being used */
	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Resume Q not busy */

		dk->dk_state |= DKONRESQ;
		sd01_resume.res_head = dk;
		sd01_resume.res_tail = dk;
		dk->dk_fltnext = (struct disk *) &sd01_resume;
		sd01resume(dk);
	}
	else
	{	/* Resume Q is Busy */

		/*
		*  This disk may already be on the Resume Queue.
		*  If it is, then set the flag to indicate that
		*  another Resume is pending for this disk.
		*/
		if (dk->dk_state & DKONRESQ)
		{
			dk->dk_state |= DKPENDRES;
		}
		else
		{	/* Not on Q, put it there */
			dk->dk_state |= DKONRESQ;
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
	}
#ifdef DEBUG
        DPR (2)("sd01qresume: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sd01fltjob - Determine what to do with the original job - 0dd2b000
 * %DESCRIPTION:
	This function uses the Request Sense information to determine
	what to do with the original job. Of course, there may not be an
	original job if the gauntlet had been entered via the HAD bus
	reset entry point.
 * %CALLED BY: sd01intrq, sd01intres
 * %SIDE EFFECTS: May restart the original job.
 * %RETURN VALUES: None.
 * %ERROR 4dd2b001
	The SCSI disk driver is retrying an I/O request because of an error
	detected by the target controller. The cause of the error is
	indicated by the second and third error codes. These error codes
	are the sense key and extended sense code respectively. See the
	disk target controller code for more information.
 * %END ERROR
 * %ERROR 4dd2b002
	The disk controller performed retry or ECC which was successful.
	The cause of the error is indicated by the second and third
	error codes. These error codes are the sense key and extended
	sense codes respectively. See the disk target controller codes
	for more information.
 * %END ERROR
 * %END HEADER */
void
sd01fltjob(dk)
struct disk *dk;
{
	struct job *jp;		/* Job structure to be restarted */
	struct sb  *osbp;	/* Original job SB pointer */

#ifdef DEBUG
        DPR (1)("sd01fltjob: (dk=0x%x) ", dk);
        DPR (6)("sd01fltjob: (dk=0x%x) ", dk);
#endif

	if ((jp = (struct job *) dk->dk_fltres->SCB.sc_wd) != NULL)
		osbp = jp->j_cont;	/* SB of a real job */
	else
		osbp = dk->dk_fltres;	/* No Job but still need an SB */

	dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

	/* Request Sense information */
	switch(dk->dk_sense.sd_key){
		case SD_NOSENSE:
		case SD_ABORT:
		case SD_VENUNI:
			sd01logerr(osbp, jp, 0x4dd2b001);

		case SD_UNATTEN: /* Don't log unit attention */

			/* Is there a real job to retry */
			if (jp !=  (struct job *) NULL)
			{
				
				/*
				*  If the job retry count or the reset count
				*  has exceeded it's limit, then fail the job.
				*  Otherwise try it again.
				*/
				if ((osbp->SCB.sc_comp_code == SDI_CRESET &&
				    dk->dk_jberr >= SD01_RST_ERR) ||
				    dk->dk_jberr >= SD01_RETRYCNT)
					sd01flterr(dk, DKRESERVE);
				else
				{
					/*
					*  Exit the gauntlet before
					*  retrying the original job.
					*/
					dk->dk_fltres->SCB.sc_wd = NULL;
					dk->dk_state &= ~DKFLT;
					sd01retry(jp);
				}
			}
			else
				/* No job to retry! Clean up as usual */
				sd01flterr(dk, DKRESERVE);
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;

		case SD_RECOVER:
			dk->dk_error++;
			if (Sd01log_marg) 
			{
				switch(dk->dk_sense.sd_sencode)
				{
			  	case SC_DATASYNCMK:
			  	case SC_RECOVRRD:
			  	case SC_RECOVRRDECC:
			  	case SC_RECOVRIDECC:
					/* Indicate marginal bad block found */
					dk->hde_state |= HDERECERR; 
					break;
			  	default:
					sd01logerr(osbp, jp, 0x4dd2b002);
				}
			}
			osbp->SCB.sc_comp_code = SDI_ASW;
			sd01flterr(dk, DKRESERVE);
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;

		case SD_MEDIUM:
			switch(dk->dk_sense.sd_sencode)
			{
			  case SC_IDERR:
			  case SC_UNRECOVRRD:
			  case SC_NOADDRID:
			  case SC_NOADDRDATA:
			  case SC_NORECORD:
			  case SC_DATASYNCMK:
			  case SC_RECOVRIDECC:
				/* Indicate actual bad block found */
				dk->hde_state |= HDEECCERR; 
			}
		default:
			dk->dk_error++;
			sd01flterr(dk, DKRESERVE);
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;
	}
}

/* %BEGIN HEADER
 * %NAME: sd01part_ck - Check that mounted/mirrored partitions do not change - 0dd2c000
 * %DESCRIPTION:
	Now that the Disk Driver uses the 'open type' field in sd01open()
	The driver can make further checks on the state of a partition before
	allowing a user to change the VTOC out from under: 1) a mounted
	partition, 2) a mirrored partition, by changing the starting address
	of a partition or by changing the size of a partition, or by changing
	the flag of a mounted partition to unmountable.
	This function should only be called when it is determined
	that a new VTOC is to be written.
 * %CALLED BY: sd01strategy, sd01phyrw, sd01vtoc_ck
 * %SIDE EFFECTS: none
 * %RETURN VALUES:
	Returns 0 if the requested change does not conflict with the
	current partitions and returns EBUSY if something was found.
	The errno will be set to busy for the error case to indicate to the
	user that the partition is already in use.
 * %END HEADER */
int
sd01part_ck(dk, bp, start)
register struct disk *dk;
register buf_t *bp;
register long	start;
{
	register int part;
	int	offset,
		voffset;
	int	i;
	long	vtocblk;
	char	*pt1, *pt2;
	ushort	flag = V_UNMNT;
	union {
		char	cstr[4];	/* Value is built a char at a time */
		long	val;		/* Used to examine the value */
	} psize;			/* Size of a partition */

#ifdef DEBUG
        DPR (1)("sd01part_ck: (dk=0x%x bp=0x%x) ", dk, bp);
#endif

        voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
	vtocblk = dk->unixst + VTBLKNO;

	/* Look for any partition that is mounted or mirrored */
	for (part=0; part < V_NUMPAR; part++)
	{
		if (dk->dk_part_flag[part] & DKMNT ||
		    dk->dk_part_flag[part] & DKONLY)
		{

			/* CHECK: p_start must be same! */
			offset = (char *) &dk->dk_vtoc.v_part[part].p_start - (char *) &dk->dk_vtoc;
			pt1 = (char *) &dk->dk_vtoc.v_part[part].p_start;
			pt2 = (char *) bp->b_un.b_addr + 
				(BLKSZ(dk)*(vtocblk-start)) + voffset + offset;
			for (i=0; i < sizeof(dk->dk_vtoc.v_part[part].p_start); i++)
			{
				if (*pt1++ != *pt2++)
					return(EBUSY);
			}

			/* CHECK: p_size cannot change (except for root) */
			offset = (char *) &dk->dk_vtoc.v_part[part].p_size - (char *) &dk->dk_vtoc;
			pt2 = (char *) bp->b_un.b_addr + 
				(BLKSZ(dk)*(vtocblk-start)) + voffset + offset;

			/* copy the data into a union */
			for (i=0; i < sizeof(dk->dk_vtoc.v_part[part].p_size); i++)
				psize.cstr[i] = *pt2++;

			/* Is the user trying to change the size */
			if (psize.val != dk->dk_vtoc.v_part[part].p_size)
			{	
				/*
				*  I could not determine a 'clean' scheme to prevent
				*  a user from changing the size of mounted partitions
				*  but still allow the Root partition to be changed.
				*  A few ideas were considered:
				*   1) Only let the boot device change a Root partition.
				*      (What does a boot device mean on an Adjunct)
				*   2) Allow a Root partition to be changed if it is
				*      the only mounted partition.
				*      (What about any device other than the boot device)
				*   3) A combination of 1 & 2 still has the same
				*      problem as idea 1.
				*
				*   So my current solution is to keep it simple
				*   and allow any mounted Root partition to be changed.
				*/
				if (!(dk->dk_part_flag[part] & DKMNT && dk->dk_vtoc.v_part[part].p_tag == V_ROOT))
					return(EBUSY);
			}

			/* CHECK: That a Mounted partition is not UNMOUNTABLE */
			offset = (char *) &dk->dk_vtoc.v_part[part].p_flag - (char *) &dk->dk_vtoc;
			pt1 = (char *) &flag;
			pt2 = (char *) bp->b_un.b_addr + 
				(BLKSZ(dk)*(vtocblk-start)) + voffset + offset;
			for (i=0; i < sizeof(dk->dk_vtoc.v_part[part].p_flag); i++)
			{
				if (*pt1++ != *pt2++)
					break;	/* Partition is mountable */
			}
			/*
			*  If i == sizeof(), then Partition is not mountable
			*  but if it is also currently mounted, fail!
			*/
			if (i == sizeof(dk->dk_vtoc.v_part[part].p_flag) &&
			    dk->dk_part_flag[part] & DKMNT)
			{
				return(EBUSY);
			}
		}
	}

#ifdef DEBUG
        DPR (2)("sd01part_ck: - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01hdelog - Reassign hard disk errors. - 0dd2d000
 * %DESCRIPTION:
	This function is called for mapping bad sector errors on disk drives.
 * %CALLED BY: sd01comp, sd01intb
 * %SIDE EFFECTS: 
	Data in an actual bad block is lost. No information is logged.
 * %RETURN VALUES: None
 * %ERROR 0dd2d001
	A marginal bad block was detected.  The driver is having difficulty
	reading this block. A Error Correction Code (ECC) had to be used.
 * %END ERROR
 * %END HEADER */
void
sd01hdelog (dk)
register struct disk *dk;		/* Pointer to disk structure	*/
{
	daddr_t	blkaddr;		/* Block address of error 	*/
	register int part;		/* Partition number		*/
	major_t	 maj;			/* Device major number		*/
	minor_t	 min;			/* Device minor number		*/
	char 	 name[NAMESZ];		/* Device information		*/
	int	 critical,		/* critical flag		*/
		 ptag,			/* Partition tag		*/
		 resflag,		/* Reassign bad block flag	*/
		 i,			/* Loop index			*/
		 rc;			/* Return error code		*/

	maj = dk->dk_addr.sa_major;
	min = dk->dk_addr.sa_minor; 

	part    = DKSLICE(min); 
	ptag    = dk->dk_vtoc.v_part[part].p_tag;
	blkaddr = dk->dk_hdesec;

#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01hdelog: blkaddr= 0x%x\n", blkaddr);
}
#endif

#ifdef DEBUG
        DPR (1)("sd01hdelog: (blkaddr=0x%x) ", blkaddr);
        DPR (6)("sd01hdelog: (blkaddr=0x%x) state=0x%x ",blkaddr,dk->hde_state);
        DPR (6)("part=0x%x p_tag=0x%x ",part, ptag);
#endif

	sdi_name(&dk->dk_addr, name);

	/* Clear reassign and critical flags */
	resflag = 0;
	critical  = 0;

	/* Check if bad block is in critical area of UNIX System partition. */
	if(blkaddr >= dk->unixst && blkaddr <= (dk->unixst + HDPDLOC))
		critical = 1;

	/* If Sd01 log flag is invalid, set it to default mode. */
	if(Sd01log_marg > 2)
		Sd01log_marg = 0;

	/* Set up defect list header */
	dk->dk_dl_data[0] = 0;
	dk->dk_dl_data[1] = 0;
	dk->dk_dl_data[2] = 0;
	dk->dk_dl_data[3] = 4;

	/* Swap defect address */
	dk->dk_dl_data[4] = ((blkaddr & (unsigned) 0xFF000000) >> 24);
	dk->dk_dl_data[5] = ((blkaddr & 0x00FF0000) >> 16);
	dk->dk_dl_data[6] = ((blkaddr & 0x0000FF00) >> 8);
	dk->dk_dl_data[7] = ( blkaddr & 0x000000FF);

	/* Determine error type */
	switch(dk->hde_state & HDEMASK) {
	case HDERECERR:

		switch(dk->hde_state & HDESMASK) {
		case HDESINIT:	/* Start by sending a Read command */

#ifdef DEBUG
        DPR (6)("marginal bad block 0x%x ", blkaddr);
#endif

			if(critical)
				cmn_err(CE_WARN, hde_mesg[HDESACRED], blkaddr);

			else if(Sd01log_marg == 1)
				cmn_err(CE_NOTE, hde_mesg[HDEECCMSG], blkaddr);

			else if(Sd01log_marg == 2) 
			{
				/* Send READ command to obtain data. */
				sd01icmd(dk,SM_READ,blkaddr,dk->blkbuf,
					BLKSZ(dk),1,SCB_READ,sd01intb);
				return;
			}
			break;

		case HDESI:	 /* Read failed	*/
			cmn_err(CE_WARN, hde_mesg[HDEBADRED], blkaddr);
			/* Pass the job and continue to next case HDESII*/
			dk->hde_state += 1;

		case HDESII:	/* Read passed	*/
			/* Send REASSIGN BLOCKS to map out the bad sector */
			sd01icmd(dk,SS_REASGN,0,dk->dk_dl_data,RABLKSSZ,0,SCB_WRITE,sd01intb);
			return;

		case HDESIII:	/* Reassign failed */
			if(dk->hde_state & HDEENOSPR)
				cmn_err(CE_WARN, hde_mesg[HDENOSPAR], blkaddr);
			else
				cmn_err(CE_WARN, hde_mesg[HDEBADMAP], blkaddr);
			dk->hde_state |= HDEREAERR;
			break;

		case HDESIV:	/* Reassign passed */
			resflag = 1;

			cmn_err(CE_NOTE, hde_mesg[HDEMAPBLK], blkaddr);

			/* Send WRITE command to initialize sector. */
			sd01icmd(dk,SM_WRITE,blkaddr,dk->blkbuf,
				BLKSZ(dk),1,SCB_WRITE,sd01intb);
			return;

		case HDESV:	/* Write failed */
			cmn_err(CE_WARN, hde_mesg[HDEBADWRT], blkaddr);
			break;

		default:
			/* Write passed - fall out of case statements */
			break;

		}
		break;

	case HDEECCERR:

		switch(dk->hde_state & HDESMASK) {
		case HDESINIT:	/* Start by sending a Reassign command */
#ifdef DEBUG
        DPR (6)("actual bad block 0x%x ", blkaddr);
#endif
			if(critical)
				cmn_err(CE_NOTE, hde_mesg[HDEBSACRD], blkaddr);

			/* Set state to skip read stage */
			dk->hde_state += 2;
			/* Send REASSIGN BLOCKS to map out the bad sector */
			sd01icmd(dk,SS_REASGN,0,dk->dk_dl_data,RABLKSSZ,0,SCB_WRITE,sd01intb);
			return;

		case HDESIII:	/* Reassign failed */
			if(dk->hde_state & HDEENOSPR)
				cmn_err(CE_WARN, hde_mesg[HDEBNOSPR], blkaddr);
			dk->hde_state |= HDEREAERR;
			break;

		case HDESIV:	/* Reassign passed */
			resflag = 1;
			cmn_err(CE_WARN, hde_mesg[HDEREASGN], blkaddr);

			/* Clear buffer */
			for(i=0; i < BLKSZ(dk); i++)
				dk->blkbuf[i] = 0;

			/* Send WRITE command to initialize sector. */
			sd01icmd(dk,SM_WRITE,blkaddr,dk->blkbuf,
				BLKSZ(dk),1,SCB_WRITE,sd01intb);
			return;

		case HDESV:	/* Write failed */
			cmn_err(CE_WARN, hde_mesg[HDENOINIT], blkaddr);
			break;

		default:
			/* Write passed - fall out of case statements */
			break;
		}
		break;
	}

	/* Check if a block was reassigned */
	if(resflag && dk->dk_vtoc.v_sanity == VTOC_SANE)
	{
		/* Check if block was in a valid file system. */
		if (dk->dk_vtoc.v_part[part].p_flag & V_VALID && ptag == V_ROOT || ptag == V_USR)
        	{
			/* Mark the file system dirty */
			fshadbad(makedevice(maj,min), blkaddr - dk->dk_vtoc.v_part[part].p_start);
		}
	}

	/* Clear hardware bad block reassign state 			*/
	dk->hde_state &= HDEHWMASK;
	dk->hde_state &= ~HDEHWREA;
	wakeup((caddr_t)&dk->hde_state);

#ifdef DEBUG
        DPR (2)("sd01hdelog: - exit ");
#endif

}

/* %BEGIN HEADER
 * %NAME: sd01start - Start access to a device. - 0dd2f000
 * %DESCRIPTION:
	This function is not used but is required by UNIX.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %END HEADER */
void
sd01start()
{
}

/* %BEGIN HEADER
 * %NAME: sd01config - Determine drive configuration. - 0dd31000
 * %DESCRIPTION:
	This function initializes the driver's disk parameter structure.
	If the READ CAPACITY or MODE SENSE commands fail, a non-fatal
	error status is returned so that sd01open1() routine does not
	fail.  In this case, the V_CONFIG ioctl can still be used to set
	the drive parameters.
 * %CALLED BY: sd01open1
 * %SIDE EFFECTS: The disk state flag will indicate if the drive parameters 
	 	  are valid.
 * %RETURN VALUES:
	-1: Non-fatal error detected
 	 0: Successfull completion
	>0: Fatal error detected - Error code is returned
 * %ERROR 6dd31001
	The sectors per cylinder parameter calculates to less than or equal
	to zero.  This is caused by incorrect data returned by the MODE
	SENSE command or the drivers master file. This may not be an AT&T 
	supported device.
 * %END ERROR
 * %ERROR 6dd31002
	The number of cylinder calculates to less than or equal to zero.  
	This is caused by incorrect data returned by the READ CAPACITY
	command. This may not be an AT&T supported device.
 * %END ERROR
 * %ERROR 6dd31003
	The READ CAPACITY command failed.
 * %END ERROR
 * %ERROR 6dd31004
	The Logical block size returned by the READ CAPACITY command is
	invalid i.e. it is not a power-of-two multiple of the kernel
	block size (KBLKSZ).
 * %END ERROR
 * %ERROR 6dd31005
	Unable able to allocate memory for the Margianl Block data area
	for this device.
 * %END ERROR
 * %ERROR 6dd31006
	The MODE SENSE command for Page 3 failed.
 * %END ERROR
 * %ERROR 6dd31007
	The MODE SENSE command for Page 4 failed.
 * %END ERROR
 * %END HEADER */
int
sd01config(dk)
register struct disk *dk;
{
	DADF_T	   *dadf = (DADF_T *) NULL;
	RDDG_T	   *rddg = (RDDG_T *) NULL;
	CAPACITY_T *cap  = (CAPACITY_T *) NULL;

	uint	pg_asec_z;
	long	cyls;
	long	sec_cyl;
	dev_t	dev;
	int	i;
	
#ifdef DEBUG
        DPR (1)("sd01config: (dk=0x%x) ", dk);
#endif
	dev = makedevice (dk->dk_addr.sa_major, dk->dk_addr.sa_minor);

	/* Send READ CAPACITY to obtain last sector address */
	if (sd01cmd(dk,SM_RDCAP,0,dk->dk_rc_data,RDCAPSZ,0,SCB_READ)) {
		return(EIO);
	}

	cap = (CAPACITY_T *) (dk->dk_rc_data);

	cap->cd_addr = sdi_swap32(cap->cd_addr);
	cap->cd_len  = sdi_swap32(cap->cd_len);

	/*
	 * Init the Block<->Logical Sector convertion values.
	 */
	for (i=0; i < (32-KBLKSHF); i++) {
		if ((KBLKSZ << i) == cap->cd_len) {
			break;
		}
	}
	if ((KBLKSZ << i) != cap->cd_len) {
#ifdef DEBUG
		cmn_err (CE_WARN, "Disk Driver: Sect size (%x) Not power-of-two of %x", cap->cd_len, KBLKSZ);
#endif
		return(-1);
	}

	if ((dk->blkbuf == NULL) ||
		(cap->cd_len != dk->dk_parms.dp_secsiz)) {
		if (dk->blkbuf != NULL) {
			kmem_free(dk->blkbuf, dk->dk_parms.dp_secsiz);
			dk->blkbuf = NULL;
		}
		dk->blkbuf = (char *) kmem_alloc(cap->cd_len, KM_SLEEP); 
		if (dk->blkbuf == NULL) {
			cmn_err(CE_NOTE, "!Disk Driver: Unable to allocate Marginal Block data area - Err 6dd31005");
			return(ENOMEM);
		}
	}

	dk->dk_parms.dp_secsiz = cap->cd_len;
	dk->dk_sect.sect_blk_shft = i;
	dk->dk_sect.sect_byt_shft = KBLKSHF + i;
	dk->dk_sect.sect_blk_mask = ((1 << i) - 1);

	/*
	 * If this is a SCSI based drive, then use the vitual
	 * geometry if specified.  Otherwise, use the physical
	 * geometry as returned by the MODE SENSE Page 3 and 4.
	 */
	if ((strncmp(IDP(HBA_tbl[SDI_HAN(&dk->dk_addr)].idata)->name, "DCD", 3) != 0) &&
	    (Sd01diskinfo[i] != 0)) {
#ifdef DEBUG
        DPR (3)("Using a Virtual geometry");
#endif
		/*
		 * Use the virtual geometry specified in the space.c file
		 */
		dk->dk_parms.dp_sectors  = (Sd01diskinfo[i] >> 8) & 0x00FF;
		dk->dk_parms.dp_heads    = Sd01diskinfo[i] & 0x00FF;

		sec_cyl = dk->dk_parms.dp_sectors * dk->dk_parms.dp_heads;
	} else {
#ifdef DEBUG
        DPR (3)("Using the disks Physical geometry");
#endif
		/*
		 * Get the sectors/track value from MODE SENSE Page-3.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0300,dk->dk_ms_data,FPGSZ,FPGSZ,SCB_READ)) {
			return(-1);
		}

		dadf = (DADF_T *) (dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *) dk->dk_ms_data)->plh_bdl);

		/* Swap Page-3 data */
		dadf->pg_sec_t   = sdi_swap16(dadf->pg_sec_t);
		dadf->pg_asec_z  = sdi_swap16(dadf->pg_asec_z);
		dadf->pg_bytes_s = sdi_swap16(dadf->pg_bytes_s);

		dk->dk_parms.dp_sectors =
			(dadf->pg_bytes_s * dadf->pg_sec_t) / cap->cd_len;  

		pg_asec_z = dadf->pg_asec_z;

		/*
		 * Get # of heads from MODE SENSE Page-4.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0400,dk->dk_ms_data,RPGSZ,RPGSZ,SCB_READ)) {
			return(-1);
		}

		rddg = (RDDG_T *) (dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *) dk->dk_ms_data)->plh_bdl);

		sec_cyl = rddg->pg_head * (dk->dk_parms.dp_sectors - pg_asec_z);

		dk->dk_parms.dp_heads = rddg->pg_head;
	}

	/* Check sec_cly calculation */
	if (sec_cyl <= 0)
	{
       		cmn_err(CE_NOTE, "!Disk Driver: Sectors per cylinder error cyls=0x%x\n",cyls);
		return(-1);
	}

	cyls = (cap->cd_addr + 1) / sec_cyl;

	/* Check cyls calculation */
	if (cyls <= 0)
	{
       		cmn_err(CE_NOTE, "!Disk Driver: Number of cylinders error. Err: 6dd31002");
		return(-1);
	}

	/* Make room for diagnostic scratch area */
	if ((cap->cd_addr + 1) == (cyls * sec_cyl))
		cyls--;

	/* Assign cylinder parameter for V_GETPARMS */
	dk->dk_parms.dp_cyls = cyls;

	/* Indicate parameters are set and valid */
	dk->dk_state |= DKPARMS; 

#ifdef SD01_DEBUG
if(sd01alts_debug & DKADDR) {
        printf("sd01config: sec/trk=0x%x secsiz=0x%x heads=0x%x cyls=0x%x ", 
		dk->dk_parms.dp_sectors, dk->dk_parms.dp_secsiz, 
		dk->dk_parms.dp_heads, dk->dk_parms.dp_cyls);
}
#endif

#ifdef DEBUG
        DPR (3)("sec=0x%x siz=0x%x heads=0x%x cyls=0x%x ", dk->dk_parms.dp_sectors, dk->dk_parms.dp_secsiz, dk->dk_parms.dp_heads, dk->dk_parms.dp_cyls);
#endif
	
#ifdef DEBUG
        DPR (2)("sd01config: parms set - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01icmd - Perform an immediate SCSI command. - 0dd32000
 * %DESCRIPTION: 
	This funtion performs a SCSI command such as Reassign Blocks for
	the drivers bad block handling routine. The op code determines 
	the SCB for the job. The data area is supplied by the caller and 
	assumed to be in kernel space. 
 * %CALLED BY: sd01hdelog
 * %SIDE EFFECTS: 
	A immediate command is sent to the host adapter. It is NOT
	sent via the drivers job queue.
 * %RETURN VALUES:
	None;
 * %ERROR 8dd03201
	The host adapter rejected a request from the SCSI disk driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %END HEADER */
sd01icmd(dk, op_code, addr, buffer, size, length, mode, intfunc)
register struct disk *dk;
char op_code;				/* Command Opcode		*/
unsigned int addr;			/* Address field of command 	*/
char *buffer;				/* Buffer for CDB data 		*/
unsigned int size;			/* Size of the buffer 		*/
unsigned int length;			/* Block length in the CDB	*/
unsigned short mode;			/* Direction of the transfer 	*/
void	(*intfunc)();			/* interrupt handler		*/
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t  *bp;
	int	 mystatus = 1;

#ifdef DEBUG
        DPR (1)("sd01icmd: (dk=0x%x) ", dk);
        DPR (6)("sd01icmd: (dk=0x%x) ", dk);
#endif

	/* Set up SB, jp, and bp pointers */
	if (op_code == SM_READ) {
		jp = dk->dk_bbh_rjob;
		bp = dk->dk_bbh_rbuf;
		jp->j_cont = dk->dk_fltrblk;
	}
	else if (op_code == SM_WRITE) {
		jp = dk->dk_bbh_wjob;
		bp = dk->dk_bbh_wbuf;
		jp->j_cont = dk->dk_fltwblk;
	}
	else  {
		jp = dk->dk_bbh_mjob;
		bp = dk->dk_bbh_mbuf;
		jp->j_cont = dk->dk_fltmblk;
	}
	bioreset(bp);
	bp->b_iodone = NULL;
	bp->b_vp = NULL;

	jp->j_cont->sb_type = ISCB_TYPE;
	
	/* Set up buffer header */
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sd01intb */
	bp->b_bcount = size;
	bp->b_error  = 0;
	
	/* Set up job structure */
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = 0;				/* not used in sd01intb */
	
	/* Set up SCB pointer */
	scb = &jp->j_cont->SCB;

	if (op_code & 0x20) { /* Group 1 commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = dk->dk_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else { /* Group 0 commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = dk->dk_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	/* Swap bytes in the address and length fields */
	if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
		jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
	else {
		jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
		jp->j_cmd.cm.sm_len  = (short) sdi_swap16(jp->j_cmd.cm.sm_len);
	}
	
	/* Initialize SCB */
	scb->sc_int    = intfunc;
	scb->sc_dev    = dk->dk_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode   = mode;
	scb->sc_resid  = 0;
	scb->sc_time   = JTIME;
	scb->sc_wd     = (long) jp;
	sdi_translate(jp->j_cont, bp->b_flags, (caddr_t)0);

	/* Send the job */
	if (SD01ICMD(dk,jp->j_cont) != SDI_RET_OK) {
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd32001");
#endif
		if ((*intfunc) == sd01intb) {
			/* Fail the job */
			dk->hde_state += 1;
			sd01hdelog(dk);
		}
		return (mystatus);
	}

#ifdef DEBUG
        DPR (2)("sd01icmd: - exit(%d) ", jp->j_bp->b_error);
        DPR (6)("sd01icmd: - exit(%d) ", jp->j_bp->b_error);
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sd01intb - Interrupt routine for Bad Block Handling. - 0dd33000
 * %DESCRIPTION: 
	This function is called by the host adapter driver when a
	SCSI Bad Block job or Request Sense job completes.  If the job fails, 
	the job is retried.  If the retry fails, the error is marked in the 
	buffer and the function returns.  However, if the job that failed 
	was a Reassign Blocks command, this function will send a Request 
	Sense to determine if the drive has run out of spare sectors.
 * %CALLED BY: Host adapter driver
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None 
 * %ERROR 8dd03301
	The host adapter rejected a retry job request from the SCSI disk driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END ERROR
 * %ERROR 8dd03302
	The host adapter rejected a Request Sense request from the SCSI disk 
	driver.  This is caused by a parameter mismatch within the driver. The 
	system should be rebooted.
 * %END ERROR
 * %END HEADER */
void
sd01intb(sbp)
register struct sb *sbp;
{
	register struct job *jp;
	register struct disk *dk;
	register struct scb *scb;
	register struct scs *cmd;

	jp  = (struct job *) sbp->SCB.sc_wd;
	scb = &jp->j_cont->SCB;
	cmd = &jp->j_cmd.cs;
	dk  = jp->j_dk;

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

#ifdef DEBUG
        DPR (1)("sd01intb: (sbp=0x%x) jp=0x%x ", sbp, jp);
        DPR (6)("sd01intb: (sbp=0x%x) jp=0x%x ", sbp, jp);
#endif

	/* Check if interrupt was due to a Request Sense job */
	if (sbp == dk->dk_fltreq) {

		dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01intb: Request Sense sd_ba= 0x%x sd_valid= 0x%x\n", 
		dk->dk_sense.sd_ba, dk->dk_sense.sd_valid);
}
#endif

		/* Fail the job */
		dk->hde_state += 1;

		/* Check if the Request Sense was ok */
		if (sbp->SCB.sc_comp_code == SDI_ASW) 
		{
			switch(dk->dk_sense.sd_key)
			{
				case SD_RECOVER:
					switch(dk->dk_sense.sd_sencode)
					{
			  		case SC_DATASYNCMK:
			  		case SC_RECOVRRD:
			  		case SC_RECOVRRDECC:
			  		case SC_RECOVRIDECC:
						/* Pass the job */
						dk->hde_state += 1;
					}
					break;
				case SD_MEDIUM:
					/* Check for Reassign command */
					if (cmd->ss_op == SS_REASGN)
						/* Indicate no spare sectors */
						dk->hde_state |= HDEENOSPR;
			}
		}
		/* Resume execution */
		sd01hdelog(dk);
		return;
	}
	
	/* Check if Bad Block job completed successfully. */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW)
	{
		/* Retry the job */
		if (jp->j_bp->b_error < SD01_RETRYCNT)
		{
#ifdef DEBUG
        DPR (6)("retry ");
#endif
			jp->j_bp->b_error++;	  	 /* Update error count*/
			jp->j_cont->SCB.sc_time = JTIME; /* Reset the job time*/

			if (SD01ICMD(jp->j_dk,jp->j_cont) != SDI_RET_OK)
			{
#ifdef DEBUG
				cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd33001");
#endif
				/* Fail the job */
				dk->hde_state += 1;
			}
			else
				return;
		}
		else {
#ifdef DEBUG
        DPR (6)("send REQSEN ");
#endif
			/* Send a Request Sense job */
			dk->dk_fltreq->sb_type 	    = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int   = sd01intb;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link  = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time  = JTIME;
			dk->dk_fltreq->SCB.sc_mode  = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev   = scb->sc_dev;
			dk->dk_fltreq->SCB.sc_wd    = (long) jp;
			dk->dk_fltcmd.ss_op         = SS_REQSEN;
			dk->dk_fltcmd.ss_lun        = cmd->ss_lun;
			dk->dk_fltcmd.ss_addr1      = 0;
			dk->dk_fltcmd.ss_addr       = 0;
			dk->dk_fltcmd.ss_len        = SENSE_SZ;
			dk->dk_fltcmd.ss_cont       = 0;
			dk->dk_sense.sd_key         = SD_NOSENSE; 

			if (SD01ICMD(dk,dk->dk_fltreq) == SDI_RET_OK)
				return;

			/* Fail the job */
			dk->hde_state += 1;
		}
	}
	else
		dk->hde_state += 2;

#ifdef DEBUG
        DPR (2)("sd01intb: - exit ");
        DPR (6)("sd01intb: - exit ");
#endif

	/* Resume execution */
	sd01hdelog(dk);
}

/* %BEGIN HEADER
 * %NAME: sd01addr - Get device structure address. - 0dd34000
 * %DESCRIPTION: 
	This function returns the address of a disk structure
	for the specified device.
 * %CALLED BY: Mirror driver
 * %SIDE EFFECTS: None
 * %RETURN VALUES:  Address if device number is valid, NULL if not.
 * %END HEADER */
int
sd01addr(dev)
dev_t dev;
{
	/* Check if the device number is valid */
	if (sd01instbl[getemajor(dev)] == DKNOMAJ ||
	   (DKINDEX(dev) + 1) > sd01_diskcnt)
		return(NULL);
	else
		return((int)(DKPTR(dev)));
}

/* %BEGIN HEADER
 * %NAME: sd01slice - Get device slice number. - 0dd35000
 * %DESCRIPTION: 
	This function returns the slice number for the specified device.
 * %CALLED BY: Mirror driver
 * %SIDE EFFECTS: None
 * %RETURN VALUES: 0 through V_NUMPAR
 * %END HEADER */
int
sd01slice(dev)
dev_t dev;
{
	return(DKSLICE(dev));                           
}
/* %BEGIN HEADER
 * %NAME: sd01size - Get size of a logical device (partition).
 * %DESCRIPTION:
	This function returns the number of 512-byte units on a partition.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: None
 * %RETURN VALUES: Number of 512-byte units, or -1 for failure.
 * %END HEADER */
int
sd01size(dev)
dev_t dev;
{
	register struct disk *dk;
	register int part;

	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}
	if(sd01_diskcnt == DKNOTCS) {
		return(-1);
	}

	if((dk = DKPTR(dev)) == NULL) {
		return(-1);
	}

	part = DKSLICE(dev);

	if (dk->dk_vtoc.v_sanity != VTOC_SANE)
		sd01open1(getmajor(dev), getminor(dev), DKFREE);

	if (dk->dk_vtoc.v_sanity == VTOC_SANE &&
		dk->dk_vtoc.v_part[part].p_flag & V_VALID)
		return(dk->dk_vtoc.v_part[part].p_size << BLKSEC(dk));
	else
		return(-1);
}

SD01ICMD(diskp, sbp)
struct disk *diskp;
struct sb *sbp;
{
        return sd01docmd(sdi_icmd, diskp, sbp);
}

int
sd01docmd(fcn, diskp, sbp)
int (*fcn)();
struct disk *diskp;
struct sb *sbp;
{
	struct dev_spec *dsp = diskp->dk_spec;
	int cmd;

	if (dsp && sbp->sb_type != SFB_TYPE) {
		cmd = ((struct scs *)sbp->SCB.sc_cmdpt)->ss_op;
		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(diskp, sbp);
		}
	}

	return (*fcn)(sbp);
}


/*
 *	get bad sector/track alternate tables
 */
sd01getalts(dk)
struct	disk	*dk;
{
	struct	partition *pp = &(dk->dk_vtoc.v_part[0]);
	int	i;
	int	status;

#ifdef SD01_DEBUG
if(sd01alts_debug & DKADDR) {
	printf("sd01getalts: ADDR &dk_altcount= 0x%x &dk_firstalt= 0x%x &dk_alts_parttbl= 0x%x &dk_amp= 0x%x &dk_wkamp_p= 0x%x &dk_ast_p= 0x%x\n",
		&(dk->dk_altcount[0]), &(dk->dk_firstalt[0]), &(dk->dk_alts_parttbl), &DK_AMP, &DK_WKAMP_P, &DK_AST_P);
	printf("&dk= 0x%x &dk_fltreq= 0x%x\n", dk, &(dk->dk_fltreq));
	printf("&dk_syncq_fw = 0x%x\n", &(dk->dk_syncq_fw));
	printf("&dk_stat.maxqlen = 0x%x\n", &(dk->dk_stat.maxqlen));
}
#endif

/*	search for partition ALTSCTR				*/
	for (i=V_NUMPAR-1; i>=0; i--) {
		if ((pp[i].p_tag == V_ALTSCTR) && (pp[i].p_flag & V_VALID))
			break;
	}

	if (i >= 0)  {
		dk->dk_remapalts = sd01remap_altsctr;
		status = sd01get_altsctr(dk, &pp[i]);
	} else {
		dk->dk_remapalts = sd01remap_altsec;
		status = sd01get_altsec(dk);
	}
	return(status);
}


/*
 *  translate AT&T alternates table into common entry table
 */
sd01xalt(dk,alttbl,enttbl)
struct	disk *dk;
struct 	alt_info *alttbl;		
struct 	alts_ent *enttbl;
{
	unsigned long spt = dk->dk_parms.dp_sectors;
	daddr_t good;
	int 	trk; 
	int	idx;
	int 	j; 
	int	entused;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01xalt\n");
}
#endif

	entused = alttbl->alt_trk.alt_used + alttbl->alt_sec.alt_used;

/* 	get base of AT&T good tracks for bad track mapping		*/
/* 	all good sectors are contiguous from here  			*/
	good = alttbl->alt_trk.alt_base;

/*  	Now process each AT&T bad track  				*/
	for (idx=0; idx <(int)alttbl->alt_trk.alt_used; idx++) {  
		enttbl[idx].bad_start  = alttbl->alt_trk.alt_bad[idx]*spt;
		enttbl[idx].bad_end    = enttbl[idx].bad_start + spt - 1;
		enttbl[idx].good_start = good; 
		good = good + spt;
	}

/* 	get base of AT&T good sectors for bad sector mapping		*/
/* 	Now translate the sectors  					*/
	good = alttbl->alt_sec.alt_base;

	for (j=0; j<(int)alttbl->alt_sec.alt_used; idx++, good++, j++) {
		enttbl[idx].bad_start  = alttbl->alt_sec.alt_bad[j];
		enttbl[idx].bad_end    = enttbl[idx].bad_start;
		enttbl[idx].good_start = good; 
	}

/*	sort the alternate entry table in ascending bad sector order	*/
	sd01sort_altsctr(enttbl,entused);

}


/*
 *	read from disk the AT&T alternate table information
 */
sd01get_altsec(dk)
struct	disk	*dk;
{
	struct  phyio alt_phyarg;	
	struct 	alt_info *alttblp;	/* AT&T disk alt structure 	*/
	int	status;
	int	mystatus = 1;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01get_altsec\n");
}
#endif
	if (sd01ast_init(dk))
		return(mystatus);

	alt_phyarg.sectst = dk->unixst + (dk->dk_pdsec.alt_ptr >> BLKSHF(dk));
	alt_phyarg.memaddr = (long) DK_AST_P->ast_alttblp;
	alt_phyarg.datasz = DK_AST_P->ast_altsiz;
	sd01phyrw(dk, V_RDABS, &alt_phyarg, SD01_KERNEL);

	if (alt_phyarg.retval) {       /* Error reading alts table */
		cmn_err(CE_NOTE, "!Disk Driver: error %d reading ALT_TBL on dskp 0x%x", alt_phyarg.retval, dk);
		sd01ast_free(dk);
		return(mystatus);
	}

	alttblp = DK_AST_P->ast_alttblp;
/*	sanity check							*/
	if (alttblp->alt_sanity != ALT_SANITY) {
		sd01ast_free(dk);
		return(mystatus);
	}

	DK_AST_P->ast_entused = alttblp->alt_trk.alt_used + 
				alttblp->alt_sec.alt_used;

	if (!DK_AST_P->ast_entused) {
		DK_AST_P->ast_entp = (struct alts_ent *)NULL;
	} else {
/* 		allocate storage for common incore alternate entry tbl	*/
		if ((DK_AST_P->ast_entp = (struct alts_ent *)kmem_alloc(
			byte_to_blksz((ALTS_ENT_SIZE * DK_AST_P->ast_entused),
			BLKSZ(dk)), KM_SLEEP)) == NULL) {
			sd01ast_free(dk);
			return(mystatus);
		}
/*		translate AT&T altsec format to common entry table	*/
		sd01xalt(dk,DK_AST_P->ast_alttblp, DK_AST_P->ast_entp);
	}

	sd01upd_altsec(dk);

/*	set index table for each partition to the alts entry table	*/
	sd01setalts_idx(dk);

	return (0);
}

/*
 *	Update with new alts info 
 */
sd01upd_altsec(dk)
struct	disk	*dk;
{

/*	release the original alts entry table				*/
	sd01rel_altsctr(dk);

/*	initialize the alternate partition table			*/
	bzero((caddr_t)&(dk->dk_alts_parttbl), ALTS_PARTTBL_SIZE);
	dk->dk_alts_parttbl.alts_sanity   = DK_AST_P->ast_alttblp->alt_sanity;
	dk->dk_alts_parttbl.alts_version  = DK_AST_P->ast_alttblp->alt_version;
	dk->dk_alts_parttbl.alts_ent_used = DK_AST_P->ast_entused;

/*	record new alts info  and alts entry tables			*/
	dk->dk_amp.ap_entp = DK_AST_P->ast_entp;
	dk->dk_amp.ap_ent_secsiz = byte_to_blksz((ALTS_ENT_SIZE * 
				DK_AST_P->ast_entused),BLKSZ(dk));
	dk->dk_alttbl = DK_AST_P->ast_alttblp;
	bzero((caddr_t)DK_AST_P, ALTSECTBL_SIZE);
}


/*
 *	Handler for growing bad sector
 */
sd01addbad(dev, arg)
dev_t 		dev;
caddr_t		arg;
{
	register struct disk *dk;	/* Pointer to disk structure	*/
	struct 	alts_mempart amp;
	int	gbadsz;
	struct	alts_ent *gbp;
	int	i;
	int 	status;
	int 	oldpri;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01addbad\n");
}
#endif
	dk = DKPTR(dev);

/* 	Wait if vtoc is currently being read 				*/
	while (dk->dk_state & DKVTOC)
		sleep ((caddr_t)dk, PRIBIO);

/*
 *	if the drive has invalid vtoc or has not been initialized,
 *	return error.
 */
	if ((dk->dk_vtoc.v_sanity != VTOC_SANE) ||
	   ((dk->dk_state & DKINIT) == 0)) { 
		return(ENXIO);
	}

/*	get growing bad sectors from user				*/
	if (copyin(arg, (caddr_t)&amp, ALTS_MEMPART_SIZE)) 
		return(EFAULT);

	gbadsz = amp.ap_gbadcnt * ALTS_ENT_SIZE;
	if ((gbp = (struct alts_ent *)kmem_alloc(gbadsz,KM_SLEEP)) == NULL)
		return(EFAULT);

	if (copyin((caddr_t)amp.ap_gbadp, (caddr_t)gbp, gbadsz))
		return(EFAULT);
/*
 *	set spl before altering the incore alternate entry table.
 */
	oldpri = spl5();
	while(dk->dk_state & DKMAPBAD)
		sleep((caddr_t)&(dk->dk_state), PRIBIO);
	dk->dk_state |= DKMAPBAD;

	dk->dk_gbadsec_cnt = amp.ap_gbadcnt; 
	dk->dk_gbadsec_p   = gbp;

/*	assume add bad sector succeed					*/
	status = 0;
/*	suspend the hba queue						*/
	sd01isfb(dk, SFB_SUSPEND);

/*	check hba queue being suspended					*/
	if (dk->dk_state & DKSUSP) {
/*		attempt hardware reassign				*/
		for (gbp=dk->dk_gbadsec_p, i=0; i<dk->dk_gbadsec_cnt; i++) {
			if (gbp[i].bad_start == ALTS_ENT_EMPTY)
				continue;
			dk->dk_hdesec = gbp[i].bad_start;
			dk->hde_state = (HDEHWREA | HDEECCERR);
#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01addbad: HW REASGN sector= %d hde_state= 0x%x\n", 
		gbp[i].bad_start, dk->hde_state);
}
#endif
			sd01hdelog(dk);

			while (dk->hde_state & HDEHWREA)
				sleep((caddr_t)&(dk->hde_state), PRIBIO);

/*			check for HW reassign error			*/
			if (dk->hde_state & HDEREAERR) {
				dk->hde_state &= ~HDEREAERR;
#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01addbad: HW REASGN FAIL sector= %d hde_state= 0x%x\n", 
		gbp[i].bad_start, dk->hde_state);
}
#endif
				break;
			} else 
				gbp[i].bad_start = ALTS_ENT_EMPTY;
		}

		if (i<dk->dk_gbadsec_cnt) {	
/*			software remap					*/
			status = (*dk->dk_remapalts)(dk);

/*			flush job queue					*/
			sd01flush_job(dk);

/*			flush hba queue					*/
			sd01isfb(dk, SFB_FLUSHR);
		}
/*		resume the queue					*/
		sd01qresume(dk);
	} else
		status = 1;

	kmem_free(dk->dk_gbadsec_p, gbadsz);
	dk->dk_gbadsec_cnt = 0;
	dk->dk_gbadsec_p = NULL;

	dk->dk_state &= ~DKMAPBAD;
	wakeup((caddr_t)&(dk->dk_state));
	splx(oldpri);

#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01addbad: return with status= %d\n", status);
}
#endif
	if (status) 
		return(EIO);

	return(0);
}

/*
 *	read the alternate sector/track partition information from disk
 */
sd01get_altsctr(dk, partp)
struct	disk *dk;
struct 	partition *partp;
{
	char 	*dsk_tblp;
	char 	*mem_tblp;
	int	dsk_tbl_secsiz;
	int	mem_tbl_secsiz;
	struct  alts_parttbl *ap;
	struct  phyio alt_phyarg;	
	int	mystatus = 1;

/*	allocate incore alternate partition table			*/
	dsk_tbl_secsiz = byte_to_blksz(ALTS_PARTTBL_SIZE, BLKSZ(dk));
	if ((dsk_tblp = (char *)kmem_alloc(dsk_tbl_secsiz,KM_SLEEP)) == NULL)
		return (mystatus);

	alt_phyarg.sectst  = partp->p_start;
	alt_phyarg.memaddr = (long) dsk_tblp;
	alt_phyarg.datasz  = dsk_tbl_secsiz;

	sd01phyrw(dk, V_RDABS, &alt_phyarg, SD01_KERNEL);
	if (alt_phyarg.retval) {       	/* Error reading alts table 	*/
		cmn_err(CE_NOTE, "!Disk Driver: error %d reading alts_parttbl for dk 0x%x", alt_phyarg.retval, dk);
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		return(mystatus);
	}

/*	sanity checking							*/
	ap = (struct alts_parttbl *) dsk_tblp;
	if (ap->alts_sanity != ALTS_SANITY) {
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		return(mystatus);
	}

	if (ap->alts_ent_used == 0) {
		mem_tblp = (char *) NULL;
		mem_tbl_secsiz = 0;
	} else {
/*		allocate incore alternate entry table			*/
		mem_tbl_secsiz=(ap->alts_ent_end-ap->alts_ent_base+1)*BLKSZ(dk);
		if ((mem_tblp = (char *)kmem_alloc(mem_tbl_secsiz,KM_SLEEP)) 
			== NULL) {
			kmem_free(dsk_tblp, dsk_tbl_secsiz);
			dsk_tblp = NULL;
			return (mystatus);
		}

		alt_phyarg.sectst  = partp->p_start +
			((struct alts_parttbl *) dsk_tblp)->alts_ent_base;
		alt_phyarg.memaddr = (long) mem_tblp;
		alt_phyarg.datasz  = mem_tbl_secsiz;

		sd01phyrw(dk, V_RDABS, &alt_phyarg, SD01_KERNEL);

/* 		if error reading entry table 				*/
		if (alt_phyarg.retval) {
			cmn_err(CE_NOTE, "!Disk Driver: error %d reading alts_enttbl for dk 0x%x", alt_phyarg.retval, dk);
			kmem_free(dsk_tblp, dsk_tbl_secsiz);
			dsk_tblp = NULL;
			kmem_free(mem_tblp, mem_tbl_secsiz);
			mem_tblp = NULL;
			return(mystatus);
		}
	}

	if (sd01alloc_wkamp(dk, partp, ap)) {
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		if (mem_tblp) {
			kmem_free(mem_tblp, mem_tbl_secsiz);
			mem_tblp = NULL;
		}
		return(mystatus);
	} else {
		kmem_free(DK_WKAMP_P->ap_tblp, DK_WKAMP_P->ap_tbl_secsiz);
		DK_WKAMP_P->ap_tblp = NULL;
	}

	DK_WKAMP_P->ap_tblp = (struct alts_parttbl *)dsk_tblp;
	DK_WKAMP_P->ap_entp = (struct alts_ent *)mem_tblp;
	DK_WKAMP_P->ap_tbl_secsiz = dsk_tbl_secsiz;
	DK_WKAMP_P->ap_ent_secsiz = mem_tbl_secsiz;

/*	get the alts map						*/
	alt_phyarg.sectst  = partp->p_start + ap->alts_map_base;
	alt_phyarg.memaddr = (long) DK_WKAMP_P->ap_mapp;
	alt_phyarg.datasz  = DK_WKAMP_P->ap_map_secsiz;

	sd01phyrw(dk, V_RDABS, &alt_phyarg, SD01_KERNEL);

/* 	if error reading alts map 					*/
	if (alt_phyarg.retval) {
		cmn_err(CE_NOTE, "!Disk Driver: error %d reading alts_map for dk 0x%x", alt_phyarg.retval, dk);
		sd01free_wkamp(dk);
		return(mystatus);
	}

/*	transform the disk image bit-map to incore char map		*/
	sd01expand_map(DK_WKAMP_P);

/*	assign new alts partition and entry tables			*/
	sd01upd_altstbl(dk);

/*	set index table for each partition to the alts entry table	*/
	sd01setalts_idx(dk);

	return (mystatus = 0);
}



/*
 * For each partition, except partition 0 which is the whole disk, find the
 * first alternate entry for that partition and count how
 * many apply to it.
 */
sd01setalts_idx(dk)
struct	disk *dk;
{
	struct 	partition *pp = &(dk->dk_vtoc.v_part[0]);
	struct 	alts_ent *ap = dk->dk_amp.ap_entp;
	daddr_t lastsec;
	int 	i, j;
	int	oldpri;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01setalts_idx\n");
}
#endif
	oldpri = spl5();

/* 	go through all partitions 				*/
	for (i=0; i<V_NUMPAR; i++) {	
/*		initialize the index table for each partition	*/
		dk->dk_firstalt[i] = NULL;
		dk->dk_altcount[i] = 0;
/*		if no bad sector, then skip			*/
		if ((dk->dk_alts_parttbl.alts_ent_used == 0) || !ap)
			continue;
/*		if partition is empty, then skip		*/
		if (pp[i].p_size == 0)
			continue;
		lastsec = pp[i].p_start + pp[i].p_size - 1;
		for (j=0; j < dk->dk_alts_parttbl.alts_ent_used; j++) {
/*
 *			if bad sector cluster is less than partition range
 *			then skip
 */
			if ((ap[j].bad_start < pp[i].p_start) &&
			    (ap[j].bad_end   < pp[i].p_start))
				continue;
/*
 *			if bad sector cluster passed the end of the partition
 *			then stop
 */
			if (ap[j].bad_start > lastsec)
				break;
			if (dk->dk_firstalt[i] == NULL) 
				dk->dk_firstalt[i] =(struct alt_ent *)&ap[j];
			dk->dk_altcount[i]++;
		}
#ifdef SD01_DEBUG
if(sd01alts_debug & DXALT) {
	printf("sd01setalts_idx: firstalt= 0x%x cnt= %d\n", dk->dk_firstalt[i], dk->dk_altcount[i]);
}
#endif
	}

	splx(oldpri);

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Leaving sd01setalts_idx\n");
}
#endif
}


/*
 *	binary search for key entry in the buf array. Return -1; if
 *	key is not found.
 *
 *	When exp_ret is set to DK_FIND_BIG, then 
 *	binary search for the largest bad sector index in the alternate
 *	entry table which overlaps or BIGGER than the given key entry
 *	Return -1; if given key is bigger than all bad sectors.
 */
sd01bsearch(buf, cnt, key, exp_ret)
struct	alts_ent buf[];
int	cnt;
daddr_t	key;
int	exp_ret;
{
	int	i;
	int	ind;
	int	interval;
	int	mystatus = -1;

	if (!cnt) return (mystatus);

	for (i=1; i<=cnt; i<<=1)
	    ind=i;

	for (interval=ind; interval; ) {
	    if ((key >= buf[ind-1].bad_start) && 
		(key <= buf[ind-1].bad_end)) {
		return(mystatus=ind-1);
	    } else {
	    	interval >>= 1;
		if (key < buf[ind-1].bad_start) {
/*		    record the largest bad sector index			*/
		    if (exp_ret & DK_FIND_BIG)
		    	mystatus = ind-1;
		    if (!interval) break;
		    ind = ind - interval;
		} else {
 /* 		    if key is larger than the last element then break	*/
		    if ((ind == cnt) || !interval) break;
		    if ((ind+interval) <= cnt)
		    	ind += interval;
		}
	    }
	}
	return(mystatus);
}


/*
 * 	bubble sort the entry table into ascending order
 */
sd01sort_altsctr(buf, cnt)
struct	alts_ent buf[];
int	cnt;
{
struct	alts_ent temp;
int	flag;
int	i,j;

	for (i=0; i<cnt-1; i++) {
	    temp = buf[cnt-1];
	    flag = 1;
	    
	    for (j=cnt-1; j>i; j--) {
		if (buf[j-1].bad_start < temp.bad_start) {
		    buf[j] = temp;
		    temp = buf[j-1];
		} else {
		    buf[j] = buf[j-1];
		    flag = 0;
		}
	    }
	    buf[i] = temp;
	    if (flag) break;
	}

}

/*
 *	compress all the contiguous bad sectors into a single entry 
 *	in the entry table. The entry table must be sorted into ascending
 *	before the compression.
 */
sd01compress_ent(buf, cnt)
struct	alts_ent buf[];
int	cnt;
{
int	keyp;
int	movp;
int	i;

	for (i=0; i<cnt; i++) {
	    if (buf[i].bad_start == ALTS_ENT_EMPTY)
		continue;
	    for (keyp=i, movp=i+1; movp<cnt; movp++) {
	    	if (buf[movp].bad_start == ALTS_ENT_EMPTY)
			continue;
/*		check for duplicate bad sectors			*/
		if (buf[keyp].bad_end == buf[movp].bad_start) {
			buf[movp].bad_start = ALTS_ENT_EMPTY;
			continue;
		}
/*		check for contiguous bad sector			*/
		if (buf[keyp].bad_end+1 != buf[movp].bad_start)
		    break;
		buf[keyp].bad_end++;
		buf[movp].bad_start = ALTS_ENT_EMPTY;
	    }
	    if (movp == cnt) break;
	}
}

/*
 *	release the existing bad sector/track alternate entry table
 */

sd01rel_altsctr(dk)
struct	disk *dk;
{
	int	oldpri;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01rel_altsctr\n");
}
#endif
	oldpri = spl5();

	if (DK_AMP.ap_entp) {
		kmem_free(DK_AMP.ap_entp, DK_AMP.ap_ent_secsiz);
		DK_AMP.ap_entp = NULL;
	}

	if (DK_AMP.ap_mapp) {
		kmem_free(DK_AMP.ap_mapp, DK_AMP.ap_map_secsiz);
		DK_AMP.ap_mapp = NULL;
	}

	if (DK_AMP.ap_memmapp) {
		kmem_free(DK_AMP.ap_memmapp, DK_AMP.part.p_size);
		DK_AMP.ap_memmapp = NULL;
	}

	if (dk->dk_alttbl) {
		kmem_free(dk->dk_alttbl,byte_to_blksz(dk->dk_pdsec.alt_len,BLKSZ(dk)));
		dk->dk_alttbl = NULL;
	}

	splx(oldpri);
#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Leaving sd01rel_altsctr\n");
}
#endif
}

/*
 *	transform the disk image alts bit map to incore char map
 */
sd01expand_map(amp_p)
struct	alts_mempart *amp_p; 
{
	int 	i;

	for (i=0; i<amp_p->part.p_size; i++) {
	    (amp_p->ap_memmapp)[i] = sd01altsmap_vfy(amp_p, i);
	}
}


/*
 *	given a bad sector number, search in the alts bit map
 *	and identify the sector as good or bad
 */
sd01altsmap_vfy(amp_p, badsec)
struct	alts_mempart *amp_p; 
daddr_t	badsec;
{
	int	slot = badsec / 8;
	int	field = badsec % 8;	
	unchar	mask;

	mask = ALTS_BAD<<7; 
	mask >>= field;
	if ((amp_p->ap_mapp)[slot] & mask)
	     return(ALTS_BAD);
	return(ALTS_GOOD);
}

sd01remap_altsctr(dk)
struct	disk *dk;
{
	int	status;
	int	mystatus = 1;

	if (status = sd01gen_altsctr(dk)) 
		return (mystatus);
	sd01compress_map(DK_WKAMP_P);
	if (status = sd01wr_altsctr(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	}
	sd01upd_altstbl(dk);
	sd01setalts_idx(dk);
	return (0);
}



/*
 *	generate alternate entry table by merging the existing and
 *	the growing entry list.
 */
sd01gen_altsctr(dk)
struct	disk *dk;
{
	int	ent_used;
	int	ent_secsiz;
	struct	alts_ent *entp;
	int	status;
	int	mystatus = 1;

	if (status= sd01alloc_wkamp(dk,&(DK_AMP.part),&(dk->dk_alts_parttbl))) {
 		cmn_err(CE_NOTE, "!Disk Driver: unable to allocate working alts partition info for disk 0x%x.", dk);
		return (mystatus);
	}

	bcopy((caddr_t)&(dk->dk_alts_parttbl), (caddr_t)DK_WKAMP_P->ap_tblp,
			ALTS_PARTTBL_SIZE);
	bcopy((caddr_t)DK_AMP.ap_mapp, (caddr_t)DK_WKAMP_P->ap_mapp, 
		DK_WKAMP_P->ap_map_secsiz);
	bcopy((caddr_t)DK_AMP.ap_memmapp,(caddr_t)DK_WKAMP_P->ap_memmapp,
		DK_WKAMP_P->part.p_size);

	if (dk->dk_alts_parttbl.alts_ent_used) {
		if ((DK_WKAMP_P->ap_entp = (struct alts_ent *)kmem_alloc(
			DK_AMP.ap_ent_secsiz, KM_SLEEP)) == NULL ) {
			sd01free_wkamp(dk);
			return (mystatus);
		}
		bcopy((caddr_t)DK_AMP.ap_entp,(caddr_t)DK_WKAMP_P->ap_entp,
			DK_AMP.ap_ent_secsiz);
		DK_WKAMP_P->ap_ent_secsiz = DK_AMP.ap_ent_secsiz;
	}

	DK_WKAMP_P->ap_gbadcnt   = dk->dk_gbadsec_cnt;
	DK_WKAMP_P->ap_gbadp	 = dk->dk_gbadsec_p;
	if (status = sd01ck_gbad(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	}

	ent_used = DK_WKAMP_P->ap_tblp->alts_ent_used + 
			DK_WKAMP_P->ap_gbadcnt;
	ent_secsiz = byte_to_blksz(ent_used*ALTS_ENT_SIZE,BLKSZ(dk));
	entp=(struct alts_ent *)kmem_alloc(ent_secsiz, KM_SLEEP);
	if (entp == NULL) {
		sd01free_wkamp(dk);
		return (mystatus);
	} 

	sd01sort_altsctr(DK_WKAMP_P->ap_gbadp, DK_WKAMP_P->ap_gbadcnt);
	sd01compress_ent(DK_WKAMP_P->ap_gbadp, DK_WKAMP_P->ap_gbadcnt);

	ent_used = sd01ent_merge(entp, DK_WKAMP_P->ap_entp,
			DK_WKAMP_P->ap_tblp->alts_ent_used,
			DK_WKAMP_P->ap_gbadp, DK_WKAMP_P->ap_gbadcnt);

	kmem_free(DK_WKAMP_P->ap_entp, DK_WKAMP_P->ap_ent_secsiz); 
	DK_WKAMP_P->ap_entp = entp;
	DK_WKAMP_P->ap_tblp->alts_ent_used = ent_used;
	DK_WKAMP_P->ap_ent_secsiz = ent_secsiz;
	DK_WKAMP_P->ap_gbadp = NULL;
	DK_WKAMP_P->ap_gbadcnt = 0;

	if (!ent_used) {
		if (entp) {
			kmem_free(entp, ent_secsiz); 
			entp = NULL;
		}
		DK_WKAMP_P->ap_entp = NULL;
		DK_WKAMP_P->ap_ent_secsiz = 0;
		return (0);
	}

/*	assign alternate sectors to the bad sectors			*/
	if (status = sd01asgn_altsctr(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	} 

/*	allocate the alts_entry on disk skipping possible bad sectors	*/
	DK_WKAMP_P->ap_tblp->alts_ent_base = 
		sd01alts_assign(DK_WKAMP_P->ap_memmapp, 
			DK_WKAMP_P->ap_tblp->alts_map_base + 
			DK_WKAMP_P->ap_map_sectot, DK_WKAMP_P->part.p_size, 
			DK_WKAMP_P->ap_ent_secsiz/(uint)BLKSZ(dk), ALTS_MAP_UP);
	if (DK_WKAMP_P->ap_tblp->alts_ent_base == NULL) {
 		cmn_err(CE_NOTE, "!Disk Driver: can't alloc space in alt partition for entry table.");
		sd01free_wkamp(dk);
		return (mystatus);
	}

	DK_WKAMP_P->ap_tblp->alts_ent_end= DK_WKAMP_P->ap_tblp->alts_ent_base + 
			(DK_WKAMP_P->ap_ent_secsiz / (uint)BLKSZ(dk)) - 1; 

	return (0);
}

/*
 *	Allocate a working copy of alternate partition info table
 */
sd01alloc_wkamp(dk, partp, ap)
struct	disk *dk;
struct	partition *partp;
struct	alts_parttbl *ap;
{
	int	mystatus = 1;

/*	allocate the working incore alts partition info			*/
	if ((DK_WKAMP_P=(struct alts_mempart *)kmem_alloc(
			ALTS_MEMPART_SIZE, KM_SLEEP)) == NULL) {
		return (mystatus);
	}
	bzero((caddr_t)DK_WKAMP_P, ALTS_MEMPART_SIZE);

	DK_WKAMP_P->ap_tbl_secsiz = byte_to_blksz(ALTS_PARTTBL_SIZE, BLKSZ(dk));
	if ((DK_WKAMP_P->ap_tblp=(struct alts_parttbl *)kmem_alloc(
			DK_WKAMP_P->ap_tbl_secsiz, KM_SLEEP)) == NULL ) {
		kmem_free(DK_WKAMP_P, ALTS_MEMPART_SIZE);
		DK_WKAMP_P = NULL;
		return (mystatus);
	}

/*	allocate buffer for the alts partition map (sector size)	
 *	( the disk image bit map )
 */
	DK_WKAMP_P->ap_map_secsiz = byte_to_blksz(ap->alts_map_len,BLKSZ(dk));
	DK_WKAMP_P->ap_map_sectot = DK_WKAMP_P->ap_map_secsiz / BLKSZ(dk);
	if ((DK_WKAMP_P->ap_mapp=(unchar *)kmem_alloc(DK_WKAMP_P->ap_map_secsiz,
		KM_SLEEP)) == NULL) {
		kmem_free(DK_WKAMP_P->ap_tblp, DK_WKAMP_P->ap_tbl_secsiz);
		kmem_free(DK_WKAMP_P, ALTS_MEMPART_SIZE);
		DK_WKAMP_P = NULL;
		return(mystatus);    
	}

/*	clear the buffers to zero					*/
	bzero((caddr_t)DK_WKAMP_P->ap_mapp,DK_WKAMP_P->ap_map_secsiz);
	
/*	allocate buffer for the incore transformed char map		*/
	if ((DK_WKAMP_P->ap_memmapp = (unchar *)kmem_alloc(partp->p_size,
		KM_SLEEP)) == NULL) {
		kmem_free(DK_WKAMP_P->ap_mapp, DK_WKAMP_P->ap_map_secsiz);
		kmem_free(DK_WKAMP_P->ap_tblp, DK_WKAMP_P->ap_tbl_secsiz);
		kmem_free(DK_WKAMP_P, ALTS_MEMPART_SIZE);
		DK_WKAMP_P = NULL;
		return(mystatus);    
	}
	bzero((caddr_t)DK_WKAMP_P->ap_memmapp,partp->p_size);

	DK_WKAMP_P->part = *partp;	/* struct copy			*/
	return(0);
}


/*
 *	Deallocate the working copy of alternate partition info table
 */
sd01free_wkamp(dk)
struct	disk *dk;
{
	struct	alts_mempart *ap;

	if ((ap = DK_WKAMP_P) == NULL) 
		return;

	if (ap->ap_memmapp) {
		kmem_free(ap->ap_memmapp, ap->part.p_size);
		ap->ap_memmapp = NULL;
	}
	if (ap->ap_mapp) {
		kmem_free(ap->ap_mapp, ap->ap_map_secsiz);
		ap->ap_mapp = NULL;
	}
	if (ap->ap_tblp) {
		kmem_free(ap->ap_tblp, ap->ap_tbl_secsiz);
		ap->ap_tblp = NULL;
	}
	if (ap->ap_entp) {
		kmem_free(ap->ap_entp, ap->ap_ent_secsiz);
		ap->ap_entp = NULL;
	}
	kmem_free(ap, ALTS_MEMPART_SIZE);

	DK_WKAMP_P = NULL;
}

/*
 *	allocate a range of sectors from the alternate partition
 */
sd01alts_assign(memmap, srt_ind, end_ind, cnt, dir)
unchar	memmap[];
daddr_t	srt_ind;
daddr_t	end_ind;
int	cnt;
int	dir;
{
	int	i;
	int	total;
	int	first_ind;

	for (i=srt_ind, first_ind=srt_ind, total=0; i!=end_ind; i+=dir) {
	    	if (memmap[i] == ALTS_BAD) {
			total = 0;
			first_ind = i + dir;
			continue;
	    	}
	    	total++;
	    	if (total == cnt)
			return(first_ind);

	}
	return(NULL);
}



/*
 *	merging two entry tables into a single table. In addition,
 *	all empty slots in the entry table will be removed.
 */
sd01ent_merge(buf, list1, lcnt1, list2, lcnt2)
struct	alts_ent buf[];
struct	alts_ent list1[];
int	lcnt1;
struct	alts_ent list2[];
int	lcnt2;
{
	int	i;
	int	j1,j2;

	for (i=0, j1=0, j2=0; j1<lcnt1 && j2<lcnt2;) {
		if (list1[j1].bad_start == ALTS_ENT_EMPTY) {
			j1++;
			continue;	
	    	}
	    	if (list2[j2].bad_start == ALTS_ENT_EMPTY) {
			j2++;
			continue;
	    	}
	    	if (list1[j1].bad_start < list2[j2].bad_start)
			buf[i++] = list1[j1++];
	    	else 
			buf[i++] = list2[j2++];
		}
	for (; j1<lcnt1; j1++) {
    		if (list1[j1].bad_start == ALTS_ENT_EMPTY) 
			continue;	
    		buf[i++] = list1[j1];
	}
	for (; j2<lcnt2; j2++) {
	    	if (list2[j2].bad_start == ALTS_ENT_EMPTY) 
			continue;	
	    	buf[i++] = list2[j2];
	}
	return (i);
}


/*
 *	assign alternate sectors for bad sector mapping
 */
sd01asgn_altsctr(dk)
struct	disk *dk;
{
	int	i;
	int	j;
	daddr_t	alts_ind;
	int	cluster;
	int	mystatus = 0;

	for (i=0; i<DK_WKAMP_P->ap_tblp->alts_ent_used; i++) {
		if ((DK_WKAMP_P->ap_entp)[i].bad_start == ALTS_ENT_EMPTY)
			continue;
		if ((DK_WKAMP_P->ap_entp)[i].good_start != 0)
			continue;
		cluster = (DK_WKAMP_P->ap_entp)[i].bad_end-
			  (DK_WKAMP_P->ap_entp)[i].bad_start +1; 
		alts_ind = sd01alts_assign(DK_WKAMP_P->ap_memmapp,
				DK_WKAMP_P->part.p_size-1, 
				DK_WKAMP_P->ap_tblp->alts_map_base + 
				DK_WKAMP_P->ap_map_sectot - 1, 
				cluster, ALTS_MAP_DOWN);
		if (alts_ind == NULL) {
			cmn_err(CE_NOTE,"!Disk Driver: can't alloc alts for bad sector %d.\n", 
			(DK_WKAMP_P->ap_entp)[i].bad_start);
			return (mystatus);
		}
		alts_ind = alts_ind - cluster + 1;
		(DK_WKAMP_P->ap_entp)[i].good_start =
				alts_ind +DK_WKAMP_P->part.p_start; 
/*		flag out assigned alternate sectors			*/
		for (j=0; j<cluster; j++) {
			(DK_WKAMP_P->ap_memmapp)[alts_ind+j] = ALTS_BAD;
		}
	}
	return (0);
}

/*
 *	transform the incore alts char map to the disk image bit map
 */
sd01compress_map(ap)
struct	alts_mempart *ap;
{

	int 	i;
	int	bytesz;
	char	mask = 0;
	int	maplen=0;

	for (i=0, bytesz=7; i<ap->part.p_size; i++) {
	    	mask |= ((ap->ap_memmapp)[i] << bytesz--);
	    	if (bytesz < 0) {
			(ap->ap_mapp)[maplen++] = mask;
			bytesz = 7;
			mask = 0;
	    	}
	}
/*
 *	if partition size != multiple number of bytes	
 *	then record the last partial byte			
 */
	if (bytesz != 7)
	    	(ap->ap_mapp)[maplen] = mask;
	    
}


/*
 *	update the new alternate partition tables on disk
 */
sd01wr_altsctr(dk)
struct	disk *dk;
{
	int	mystatus = 1;
	int	status;
	daddr_t	srtsec;

	if (!DK_WKAMP_P || !DK_WKAMP_P->ap_tblp) 
		return(mystatus);

	srtsec = DK_WKAMP_P->part.p_start;

	status = sd01cmdalts(dk,SM_WRITE,srtsec,
			DK_WKAMP_P->ap_tblp,DK_WKAMP_P->ap_tbl_secsiz,
			DK_WKAMP_P->ap_tbl_secsiz/BLKSZ(dk),SCB_WRITE);
	if (status) {
	    	cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate sector partition.");
		return(mystatus);
	}

	status = sd01cmdalts(dk,SM_WRITE,
			srtsec+DK_WKAMP_P->ap_tblp->alts_map_base,
			DK_WKAMP_P->ap_mapp,DK_WKAMP_P->ap_map_secsiz,
			DK_WKAMP_P->ap_map_secsiz/BLKSZ(dk),SCB_WRITE);
	if (status) {
	    	cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate partition map.");
		return(mystatus);
	}

	if (DK_WKAMP_P->ap_tblp->alts_ent_used != 0) {
		status = sd01cmdalts(dk,SM_WRITE,
			srtsec+DK_WKAMP_P->ap_tblp->alts_ent_base,
			DK_WKAMP_P->ap_entp,DK_WKAMP_P->ap_ent_secsiz,
			DK_WKAMP_P->ap_ent_secsiz/BLKSZ(dk),SCB_WRITE);
		if (status) {
	    		cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate sector entry table.");
			return(mystatus);
		}
	}
	return(0);
}

/*
 *	Update the incore alternate partition info tables
 */
sd01upd_altstbl(dk)
struct	disk *dk;
{
	int	oldpri;

	oldpri = spl5();

	sd01rel_altsctr(dk);

	bcopy((caddr_t)DK_WKAMP_P->ap_tblp, (caddr_t)&(dk->dk_alts_parttbl),
		ALTS_PARTTBL_SIZE);
	kmem_free(DK_WKAMP_P->ap_tblp, DK_WKAMP_P->ap_tbl_secsiz);
	DK_WKAMP_P->ap_tblp = NULL;

	bcopy((caddr_t)DK_WKAMP_P, (caddr_t)&(dk->dk_amp), ALTS_MEMPART_SIZE);
	kmem_free(DK_WKAMP_P, ALTS_MEMPART_SIZE);
	DK_WKAMP_P = NULL;
	splx(oldpri);
}

/* 
 *	interrupt handler for bad sector software remap
 */
void
sd01intalts(sbp)
register struct sb *sbp;
{
	register struct job *jp;
	register struct disk *dk;
	register struct scb *scb;
	register struct scs *cmd;
	int	 status = 1;

	jp  = (struct job *) sbp->SCB.sc_wd;
	scb = &jp->j_cont->SCB;
	cmd = &jp->j_cmd.cs;
	dk  = jp->j_dk;

        if (dk->dk_spec && dk->dk_spec->intr) {
                (*dk->dk_spec->intr)(dk, sbp);
        }

/* 	Check if interrupt was due to a Request Sense job 		*/
	if (sbp == dk->dk_fltreq) {

		dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

/* 		Check if the Request Sense was ok 			*/
		if (sbp->SCB.sc_comp_code == SDI_ASW) {
			switch(dk->dk_sense.sd_key) {
				case SD_RECOVER:
					switch(dk->dk_sense.sd_sencode) {
				  		case SC_DATASYNCMK:
				  		case SC_RECOVRRD:
				  		case SC_RECOVRRDECC:
				  		case SC_RECOVRIDECC:
							status = 0;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		} 
		if (status) {
			jp->j_bp->b_flags |= B_ERROR;
			jp->j_bp->b_error =  EIO;
		}
		if (dk->hde_state & HDESWMAP) {
			dk->hde_state &= ~HDESWMAP;
			wakeup((caddr_t)&(dk->hde_state));
		}
#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
        	printf("sd01intalts: return from request sense b_flags= 0x%x\n",
			jp->j_bp->b_flags);
	}
#endif
		return;
	}
	
/* 	Check if Bad Block job completed successfully. 			*/
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
        	printf("sd01intalts: job completed with error. cnt= 0x%x\n", 
			jp->j_bp->b_error);
	}
#endif
/* 		Retry the job 						*/
		if (jp->j_bp->b_error < SD01_RETRYCNT) {
			jp->j_bp->b_error++;	  	 /* Update error count*/
			jp->j_cont->SCB.sc_time = JTIME; /* Reset the job time*/

			if (SD01ICMD(jp->j_dk,jp->j_cont) == SDI_RET_OK)
				return;
#ifdef DEBUG
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd33001");
#endif
		} else {
/* 			Send a Request Sense job 			*/
			dk->dk_fltreq->sb_type 	    = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int   = sd01intalts;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link  = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time  = JTIME;
			dk->dk_fltreq->SCB.sc_mode  = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev   = scb->sc_dev;
			dk->dk_fltreq->SCB.sc_wd    = (long) jp;
			dk->dk_fltcmd.ss_op         = SS_REQSEN;
			dk->dk_fltcmd.ss_lun        = cmd->ss_lun;
			dk->dk_fltcmd.ss_addr1      = 0;
			dk->dk_fltcmd.ss_addr       = 0;
			dk->dk_fltcmd.ss_len        = SENSE_SZ;
			dk->dk_fltcmd.ss_cont       = 0;
			dk->dk_sense.sd_key         = SD_NOSENSE; 
			sdi_translate(dk->dk_fltreq, B_READ, (caddr_t)0);

			if (SD01ICMD(dk,dk->dk_fltreq) == SDI_RET_OK)
				return;
#ifdef DEBUG
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd33002");
#endif
		}
	} else
		status = 0;

	if (status) {
		jp->j_bp->b_flags |= B_ERROR;
		jp->j_bp->b_error =  EIO;
	} else
		jp->j_bp->b_flags |= B_DONE;

	if (dk->hde_state & HDESWMAP) {
		dk->hde_state &= ~HDESWMAP;
		wakeup((caddr_t)&(dk->hde_state));
	}

#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
        	printf("sd01intalts: return b_flags=0x%x\n", jp->j_bp->b_flags);
	}
#endif
	return;
}

sd01cmdalts(dk, op_code, addr, buffer, size, length, mode)
register struct disk *dk;
char op_code;				/* Command Opcode		*/
unsigned int addr;			/* Address field of command 	*/
char *buffer;				/* Buffer for CDB data 		*/
unsigned int size;			/* Size of the buffer 		*/
unsigned int length;			/* Block length in the CDB	*/
unsigned short mode;			/* Direction of the transfer 	*/
{
	int	status;
	int	mystatus = 1;
	int	oldpri;
	buf_t	*bp;

	oldpri = spl5();
	dk->hde_state |= HDESWMAP;

	status = sd01icmd(dk, op_code, addr, buffer, size, length, mode, sd01intalts);
	if (status) {
		dk->hde_state &= ~HDESWMAP;
		(void) splx(oldpri);
		return (mystatus);
	}
	while (dk->hde_state & HDESWMAP)
		sleep((caddr_t)&(dk->hde_state), PRIBIO);

	(void) splx(oldpri);
	if(op_code == SM_READ)
		bp = dk->dk_bbh_rbuf;
	else if (op_code == SM_WRITE)
		bp = dk->dk_bbh_wbuf;
	else
		bp = dk->dk_bbh_mbuf;

	if (bp->b_flags & B_ERROR)
		return (mystatus);	

	return (0);
}



/*
 *	checking growing bad sectors in ALTSCTR partition
 *	and sectors that may have already been remapped
 */
sd01ck_gbad(dk)
struct	disk *dk;
{
	daddr_t	badsec;
	daddr_t	altsp_srtsec = DK_WKAMP_P->part.p_start;
	daddr_t	altsp_endsec = DK_WKAMP_P->part.p_start + 
				DK_WKAMP_P->part.p_size - 1;
	int	cnt;
	int	status;
	int	mystatus = 1;

	for (cnt=0; cnt < DK_WKAMP_P->ap_gbadcnt; cnt++) {
	    badsec = (DK_WKAMP_P->ap_gbadp)[cnt].bad_start;

/*	    check if bad sector is within the ATLSCTR partition		*/
	    if ((badsec >= altsp_srtsec) && (badsec <= altsp_endsec)) {

/*		check for assigned alternate sector			*/
		if ((DK_WKAMP_P->ap_memmapp)[badsec - altsp_srtsec]==ALTS_BAD) {
		    	status = sd01ck_badalts(dk,cnt);
			if (status)
				return (mystatus);
		} else {	/* non-assigned alternate sector	*/
		    if ((badsec >= altsp_srtsec) && (badsec <= (altsp_srtsec +
			DK_WKAMP_P->ap_tbl_secsiz / (uint)BLKSZ(dk) - 1))) {
		    	cmn_err(CE_NOTE,"!Disk Driver: Alt partition table is bad.");
		    	return (mystatus);
	    	    }
		    if ((badsec>=altsp_srtsec+DK_WKAMP_P->ap_tblp->alts_map_base) && 
			(badsec <= (altsp_srtsec + DK_WKAMP_P->ap_tblp->alts_map_base +
			DK_WKAMP_P->ap_map_sectot - 1))) {
		    	cmn_err(CE_NOTE, "!Disk Driver: Alt part map is bad.");
		    	return (mystatus);
	    	    }
		    if ((badsec >= altsp_srtsec+DK_WKAMP_P->ap_tblp->alts_ent_base) && 
			(badsec <= (altsp_srtsec + DK_WKAMP_P->ap_tblp->alts_ent_base +
			DK_WKAMP_P->ap_ent_secsiz / (uint)BLKSZ(dk) - 1))) {
		    	cmn_err(CE_NOTE, "!Disk Driver: Alt partition entry is bad.");
		    	return(mystatus);
	    	    }
		    (DK_WKAMP_P->ap_memmapp)[badsec - altsp_srtsec] = ALTS_BAD;
	            (DK_WKAMP_P->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		}
	    } else {
/*
 *		binary search for bad sector in the alts entry table
 */
		status = sd01bsearch(DK_WKAMP_P->ap_entp, 
				DK_WKAMP_P->ap_tblp->alts_ent_used,
				(DK_WKAMP_P->ap_gbadp)[cnt].bad_start, 0);
/*
 *		if the bad sector had already been remapped(found in alts_entry)
 *		then ignore the bad sector
 */
		if (status != -1) {
	            (DK_WKAMP_P->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		}
	    }
	}
	return (0);
}



/*
 *	check for bad sector in assigned alternate sectors
 */
sd01ck_badalts(dk, gidx)
struct	disk *dk;
int	gidx;
{
	daddr_t	badsec;
	int	i;
	int	j;
	daddr_t	numsec;
	int	cnt = DK_WKAMP_P->ap_tblp->alts_ent_used;
	daddr_t intv[3];
	struct	alts_ent ae[3];
	struct	alts_ent *atmp_entp;
	int	atmp_ent_secsiz;
	int	mystatus = 1;

	badsec = (DK_WKAMP_P->ap_gbadp)[gidx].bad_start;

	for (i=0; i<cnt; i++) {
	    numsec = (DK_WKAMP_P->ap_entp)[i].bad_end - 
			(DK_WKAMP_P->ap_entp)[i].bad_start;
	    if ((badsec >= (DK_WKAMP_P->ap_entp)[i].good_start) &&
		(badsec <= ((DK_WKAMP_P->ap_entp)[i].good_start + numsec))) {
		cmn_err(CE_NOTE, "!Disk Driver: Bad sector %ld is an assigned alt", badsec);
/*		if the assigned alternate sector is mapped to 
 *		a single bad sector , then reassign the mapped bad sector
 */ 
		if (!numsec) {
        	    (DK_WKAMP_P->ap_gbadp)[gidx].bad_start = ALTS_ENT_EMPTY;
		    (DK_WKAMP_P->ap_entp)[i].good_start = 0;
		    return(0);
		}

		intv[0] = badsec - (DK_WKAMP_P->ap_entp)[i].good_start;
		intv[1] = 1;
		intv[2] = (DK_WKAMP_P->ap_entp)[i].good_start + numsec - badsec;

		if (intv[0]) {
			ae[0].bad_start = (DK_WKAMP_P->ap_entp)[i].bad_start;
			ae[0].bad_end = ae[0].bad_start + intv[0] -1;
			ae[0].good_start = (DK_WKAMP_P->ap_entp)[i].good_start;
		}

		ae[1].bad_start = (DK_WKAMP_P->ap_entp)[i].bad_start + intv[0];
		ae[1].bad_end = ae[1].bad_start;
		ae[1].good_start = 0;

		if (intv[2]) {
			ae[2].bad_start = ae[1].bad_end + 1;
			ae[2].bad_end = ae[2].bad_start + intv[2] -1;
			ae[2].good_start = (DK_WKAMP_P->ap_entp)[i].good_start+
						intv[0] + intv[1];
		}

/*		swap in the original bad sector				*/
        	(DK_WKAMP_P->ap_gbadp)[gidx] = ae[1];

/*		check for bad sec at the beginning or the end of the cluster */
		if (!intv[0] || !intv[2]) {
			if (!intv[0])
		    		(DK_WKAMP_P->ap_entp)[i] = ae[2];
			else
		    		(DK_WKAMP_P->ap_entp)[i] = ae[0];
			return(0);
		}

/*	        allocate an expanded alternate entry table		*/
	        atmp_ent_secsiz = byte_to_blksz(((cnt+1)*ALTS_ENT_SIZE),
				BLKSZ(dk));
	        if ((atmp_entp = (struct alts_ent *)kmem_alloc(atmp_ent_secsiz,
		     KM_SLEEP)) == NULL) {
			cmn_err(CE_NOTE, "!Disk Driver: Unable to alloc alternate entry table.");
			return (mystatus);
	    	}

		for (j=0; j<i; j++)
			atmp_entp[j] = (DK_WKAMP_P->ap_entp)[j];
		atmp_entp[i] = ae[0];
		atmp_entp[i+1] = ae[2];
		for (j=i+1; j<cnt; j++)
			atmp_entp[j+1] = (DK_WKAMP_P->ap_entp)[j];

		kmem_free(DK_WKAMP_P->ap_entp, DK_WKAMP_P->ap_ent_secsiz);
		DK_WKAMP_P->ap_tblp->alts_ent_used++;
		DK_WKAMP_P->ap_ent_secsiz = atmp_ent_secsiz; 
		DK_WKAMP_P->ap_entp = atmp_entp;
		return (0);
	    } 
	}

/*	the bad sector has already been identified as bad		*/
        (DK_WKAMP_P->ap_gbadp)[gidx].bad_start = ALTS_ENT_EMPTY;
	cmn_err(CE_NOTE, "!Disk Driver: hitting a bad sector that has been identified as bad.");
	return(0);

}


sd01ast_init(dk)
struct	disk *dk;
{
	int	mystatus = 1;

	if (!DK_AST_P) {
		if ((DK_AST_P = (struct altsectbl *)kmem_alloc(ALTSECTBL_SIZE,
			KM_SLEEP)) == NULL)
			return (mystatus);
	}
	bzero((caddr_t)DK_AST_P, ALTSECTBL_SIZE);

	DK_AST_P->ast_altsiz = byte_to_blksz(dk->dk_pdsec.alt_len,BLKSZ(dk));
/* 	allocate storage for disk image alternates info table		*/
	if ((DK_AST_P->ast_alttblp = (struct alt_info *)kmem_alloc(
		DK_AST_P->ast_altsiz,KM_SLEEP)) == NULL)
		return(mystatus);

	return (0);
}

sd01ast_free(dk)
struct	disk *dk;
{
	if (!DK_AST_P)
		return; 

	if (DK_AST_P->ast_alttblp)  {
		kmem_free(DK_AST_P->ast_alttblp, DK_AST_P->ast_altsiz);
		DK_AST_P->ast_alttblp = NULL;
	}

	if (DK_AST_P->ast_entp)  {
		kmem_free(DK_AST_P->ast_entp, byte_to_blksz(
			DK_AST_P->ast_entused*ALTS_ENT_SIZE,BLKSZ(dk)));
		DK_AST_P->ast_entp = NULL;
	}

	bzero((caddr_t)DK_AST_P, ALTSECTBL_SIZE);
}


sd01remap_altsec(dk)
struct	disk *dk;
{
	int	status;
	int	mystatus = 1;
	daddr_t	srtsec;

/*	check for reserved alternate space				*/
	if (dk->dk_alttbl->alt_sec.alt_reserved == 0) {
		cmn_err(CE_NOTE, "!Disk Driver: No alternate table in VTOC");
		return(mystatus);
	}

	if (sd01ast_init(dk))
		return(mystatus);

	bcopy((caddr_t)dk->dk_alttbl,(caddr_t)DK_AST_P->ast_alttblp, 
		dk->dk_pdsec.alt_len);

	if (((DK_AST_P->ast_gbadp = dk->dk_gbadsec_p)==NULL) ||
	    ((DK_AST_P->ast_gbadcnt = dk->dk_gbadsec_cnt)==0)) {
		sd01ast_free(dk);
		return(mystatus);
	}

	if (status = sd01asgn_altsec(dk)) {
		sd01ast_free(dk);
		return(mystatus);
	} else
		DK_AST_P->ast_entused = DK_AST_P->ast_alttblp->alt_trk.alt_used 
				+ DK_AST_P->ast_alttblp->alt_sec.alt_used;

	if (!DK_AST_P->ast_entused) {
		DK_AST_P->ast_entp = (struct alts_ent *)NULL;
	} else {
/* 		allocate storage for common incore alternate entry tbl	*/
		if ((DK_AST_P->ast_entp = (struct alts_ent *)kmem_alloc(
			byte_to_blksz((ALTS_ENT_SIZE * DK_AST_P->ast_entused),
			BLKSZ(dk)), KM_SLEEP)) == NULL) {
			sd01ast_free(dk);
			return(mystatus);
		}
/*		translate AT&T altsec format to common entry table	*/
		sd01xalt(dk,DK_AST_P->ast_alttblp, DK_AST_P->ast_entp);
	}

/*	write new altsec info back to disk				*/
	srtsec = dk->unixst + (dk->dk_pdsec.alt_ptr >> BLKSHF(dk));
	status = sd01cmdalts(dk,SM_WRITE,srtsec,
			DK_AST_P->ast_alttblp,DK_AST_P->ast_altsiz,
			DK_AST_P->ast_altsiz/BLKSZ(dk),SCB_WRITE);
	if (status) {
	    	cmn_err(CE_NOTE,"!Disk Driver: can't write alternate table");
		sd01ast_free(dk);
		return(mystatus);
	}

/*	update incore altsec info					*/
	sd01upd_altsec(dk);

/*	set index table for each partition to the alts entry table	*/
	sd01setalts_idx(dk);

	return (0);
}

/*
 *	assign bad sectors based on the AT&T mapping scheme
 */
sd01asgn_altsec(dk)
struct	disk *dk;
{
	struct	alts_ent *growbadp;
	daddr_t curbad;
	int	cnt;
	int	i;
	struct	alt_info *alttblp = DK_AST_P->ast_alttblp;
	int	chgflg = 0;
	int	mystatus = 1;

	growbadp=DK_AST_P->ast_gbadp;
	for (cnt=0; cnt<DK_AST_P->ast_gbadcnt; cnt++) {
		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

/*		search through the bad sector entry table		*/
		for (i=0; i < (int)alttblp->alt_sec.alt_used;i++) {
/* 			check for any assigned alternate is bad		*/
			if (alttblp->alt_sec.alt_base + i == curbad) {
/*				skip if it is a known bad sector	*/
				if (alttblp->alt_sec.alt_bad[i] == -1) {
					growbadp[cnt].bad_start=ALTS_ENT_EMPTY;
					break;
				}

				cmn_err(CE_NOTE,"!Disk Driver: Bad sector %ld is an assigned alternate for sector %ld\n",
				    curbad, alttblp->alt_sec.alt_bad[i]);
/*				swap growing bad sector to become
 *				original mapped bad sector
 */
				growbadp[cnt].bad_start = alttblp->alt_sec.alt_bad[i];
				alttblp->alt_sec.alt_bad[i] = -1;
				chgflg++;
				break;
			}

/* 			check if bad block already mapped (already in list) */
			if (alttblp->alt_sec.alt_bad[i] == curbad) {
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				break;
			}
		}

		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

/* 		if unused alternate is bad, then excise it from the list. */
		for (i = alttblp->alt_sec.alt_used; i < (int)alttblp->alt_sec.alt_reserved; i++) {
			if (alttblp->alt_sec.alt_base + i == curbad) {
				alttblp->alt_sec.alt_bad[i] = -1;
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				chgflg++;
				break;
			}
		}

		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

/*		make sure alt_used is an index to an available 
 *		alternate entry for remapped
 */
		while (alttblp->alt_sec.alt_used<alttblp->alt_sec.alt_reserved){
			if (alttblp->alt_sec.alt_bad[alttblp->alt_sec.alt_used]
			    != -1)
				break;
			alttblp->alt_sec.alt_used++;
		}

/*		check for reserved table overflow			*/
		if (alttblp->alt_sec.alt_used >= alttblp->alt_sec.alt_reserved){
			cmn_err(CE_NOTE, "!Disk Driver: Insufficient alts for sect %ld!\n", curbad);
			if (chgflg)
				return(0);
			else
				return(mystatus);
		}

		alttblp->alt_sec.alt_bad[alttblp->alt_sec.alt_used] = curbad;
		alttblp->alt_sec.alt_used++;
		chgflg++;
	}

	return (0);
}



sd01isfb(dk, func)
struct 	disk *dk;
ulong	func;
{

	switch (func) {
		case SFB_FLUSHR:
			if (dk->dk_state & DKFLUSHQ)
				return;
			dk->dk_state |= DKFLUSHQ;
			break;
		case SFB_SUSPEND:
			if (dk->dk_state & DKSUSP)
				return;
			dk->dk_state |= DKSUSP_WAIT;
			break;
		default:
			return;
	}

	dk->dk_spcount = 0;	/* Reset special count 			*/
	dk->dk_fltfqblk->SFB.sf_dev = dk->dk_addr;
	dk->dk_fltfqblk->SFB.sf_int = sd01intfq;
	dk->dk_fltfqblk->SFB.sf_func = func;

	if (SD01ICMD(dk,dk->dk_fltfqblk) != SDI_RET_OK) {
#ifdef DEBUG
		cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd28001");
#endif
		switch (func) {
			case SFB_FLUSHR:
				dk->dk_state &= ~DKFLUSHQ;
				break;
			case SFB_SUSPEND:
				dk->dk_state &= ~DKSUSP_WAIT;
				break;
		}
		return;
	}

#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01isfb: WAIT: dk_state= 0x%x\n", dk->dk_state);
}
#endif
	switch (func) {
		case SFB_FLUSHR:
			while (dk->dk_state & DKFLUSHQ)
				sleep((caddr_t)&dk->dk_state, PRIBIO);
			break;
		case SFB_SUSPEND:
			while (dk->dk_state & DKSUSP_WAIT)
				sleep((caddr_t)&dk->dk_state, PRIBIO);
			break;
	}
}



void
sd01intfq(sbp)
register struct sb *sbp;
{
	register struct disk *dk;
	dev_t dev;			/* External device number	*/

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = DKPTR(dev);

        if (dk->dk_spec && dk->dk_spec->intr) 
                (*dk->dk_spec->intr)(dk, sbp);

	if (sbp->SFB.sf_comp_code & SDI_RETRY && dk->dk_spcount<SD01_RETRYCNT) {
/* 		Retry the function request 				*/
		dk->dk_spcount++;
		dk->dk_error++;
		sd01logerr(sbp, (struct job *) NULL, 0x4dd0f001);
		if (SD01ICMD(dk,sbp) == SDI_RET_OK) 
			return;
#ifdef DEBUG
		else {	
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd0f002");
		}
#endif
	} else {
		if (sbp->SFB.sf_comp_code != SDI_ASW) 
			sd01logerr(sbp, (struct job *) NULL, 0x6dd0f003);

		dk->dk_spcount = 0;	/* Zero retry count 		*/
	}
	switch (sbp->SFB.sf_func) {
		case SFB_FLUSHR:
			if (dk->dk_state & DKFLUSHQ) {
				dk->dk_state &= ~DKFLUSHQ;
				wakeup((caddr_t)&dk->dk_state);
			}
			break;
		case SFB_SUSPEND:
			if (sbp->SFB.sf_comp_code == SDI_ASW) 
				dk->dk_state |= DKSUSP;
			if (dk->dk_state & DKSUSP_WAIT) {
				dk->dk_state &= ~DKSUSP_WAIT;
				wakeup((caddr_t)&dk->dk_state);
			}
			break;
		default:
			return;
	}
#ifdef SD01_DEBUG
if(sd01alts_debug & HWREA) {
	printf("sd01intfq: dk_state= 0x%x\n", dk->dk_state);
}
#endif
}


/*
 *	Flush all jobs in the disk queue
 */
sd01flush_job(dk)
struct	disk *dk;
{
	struct	job *jp;
	struct	job *jp_next;

	jp = dk->dk_next;
	dk->dk_next = (struct job *)dk;
	dk->dk_batch = (struct job *)dk;
	dk->dk_forw = (struct job *)dk;
	dk->dk_back = (struct job *)dk;

	for (; jp!= (struct job *)dk; jp=jp_next) {
		jp_next = jp->j_forw;

		jp->j_forw = (struct job *)NULL;
		jp->j_back = (struct job *)NULL;
		jp->j_cont->SCB.sc_comp_code = SDI_QFLUSH;

		sd01done(jp);
	}

	for (jp=dk->dk_syncq_fw; jp; jp=jp_next) {
		jp_next = jp->j_forw;

		jp->j_forw = (struct job *)NULL;
		jp->j_back = (struct job *)NULL;
		jp->j_cont->SCB.sc_comp_code = SDI_QFLUSH;
		sd01done(jp);
	}

}


/*
 *	check for any bad sector for the disk request
 */
sd01ck_badsec(bp)
buf_t	*bp;
{
	struct	disk *dk;
	int	part;
	int	oldpri;
	int	error;
	extern	proc_t *proc_init;

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Entering sd01ck_badsec\n");
}
#endif
	dk   = DKPTR(bp->b_edev);
	part = DKSLICE(bp->b_edev);

/* 	Arm dynamic bad block handler deamon 				*/
 	if (!(sd01db.db_flag&(DBBH_ARM|DBBH_ARMING)) && proc_init) {
		sd01db.db_flag |= DBBH_ARMING;
		wakeup((caddr_t)sd01dkd);
	}
	if (bp->b_bcount % dk->dk_parms.dp_secsiz) {
 		bp->b_flags |= B_ERROR;
 		bp->b_error = EINVAL;

#ifdef DEBUG
 		cmn_err(CE_WARN, "Disk Driver: Request was not a multiple of block size.");
#endif
 		biodone(bp);
 		return;
 	}

/*	wait until dynamic bad sector mapping has been completed	*/
	oldpri = spl7();
	while(dk->dk_state & DKMAPBAD)
		sleep((caddr_t)&(dk->dk_state), PRIBIO);
	splx(oldpri);

/*	keep start time							*/
	drv_getparm(LBOLT, (ulong*)&(bp->b_start));

	sd01gen_sjq(dk,part,bp);

#ifdef SD01_DEBUG
if(sd01alts_debug & STFIN) {
	printf("Leaving sd01ck_badsec\n");
}
#endif
}

/*
 *	Generate the scsi job queue
 */
sd01gen_sjq(dk, part, bp)
struct	disk *dk;
int	part;
buf_t	*bp;
{
	register struct	job *diskhead = NULL;
	register struct	job *jp;

/*	generate a scsi disk job					*/
	diskhead = (struct job *)sd01getjob(dk);
	diskhead->j_seccnt = bp->b_bcount >> BLKSHF(dk);
	set_sjq_memaddr(diskhead,paddr(bp));
	set_sjq_daddr(diskhead,sd01_blk2sec(bp->b_blkno,dk,part));

#ifdef SD01_DEBUG
if(sd01alts_debug & DXALT) {
	printf("sd01gen_sjq: diskhead: part= %d seccnt= 0x%x memaddr= 0x%x daddr= 0x%x\n",
		part,diskhead->j_seccnt,sjq_memaddr(diskhead), 
		sjq_daddr(diskhead));
	printf("buf: bcount= 0x%x paddr(bp)= 0x%x blkno= 0x%x\n",
		bp->b_bcount, paddr(bp), bp->b_blkno);
}
#endif

/*	check for bad sector in this disk request			*/
	if (dk->dk_altcount[part]) 
		sd01alt_badsec(dk, diskhead, part);

	for (jp=diskhead; jp; jp=jp->j_fwmate) {
		sd01strat1(jp,dk,bp);
	}
}

sd01alt_badsec(dk, diskhead, partition)
struct	disk *dk;
struct  job *diskhead;     
ushort  partition;
{
	register struct alts_ent *altp;
	register struct job *curdisk = diskhead;
	register struct job *jp;   	
	int  	alts_used = dk->dk_altcount[partition];
	ushort  secsiz = BLKSZ(dk); /* local copy (saves indirect ref) 	*/
	daddr_t lastsec;        	   /* last sec of section 	*/
	int  	i;

#ifdef SD01_DEBUG
	unsigned int  flag = 0;
if(sd01alts_debug & STFIN) {
	printf("Entering sd01alt_badsec, partition= %d, dk= 0x%x curdisk= 0x%x\n", partition, dk, curdisk);
}
if(sd01alts_debug & DCLR) {
		printf("sd01alt_badsec: sector= %d count= %d mem= 0x%x\n",
		sjq_daddr(curdisk), curdisk->j_seccnt, sjq_memaddr(curdisk));
}
#endif
	altp = (struct alts_ent *)dk->dk_firstalt[partition];
	lastsec = sjq_daddr(curdisk) + ((daddr_t)(curdisk->j_seccnt) - 1);
/*
 *	binary search for the largest bad sector index in the alternate
 *	entry table which overlaps or larger than the starting 
 *	sjq_daddr(curdisk)
 */
	i = sd01bsearch(altp, alts_used, sjq_daddr(curdisk), DK_FIND_BIG);
/*	if starting sector is > the largest bad sector, return 		*/
	if (i == -1)
		return;
/*	i is the starting index, and set altp to the starting entry addr*/
	altp += i;

	for (; i<alts_used; ) {
/*	CASE 1:								*/
		while (lastsec < altp->bad_start) { 
			if (curdisk = curdisk->j_fwmate)
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
			else break;
		}
		if (!curdisk) break;    

/*	CASE 3:								*/
		if (sjq_daddr(curdisk) > altp->bad_end) {
			i++;
			altp++;
			continue;      
		}

#ifdef SD01_DEBUG
if(sd01alts_debug & DXALT) {
		flag = 1;
		printf("sd01alt_badsec: sector= %d count= %d mem= 0x%x\n",
		sjq_daddr(curdisk), curdisk->j_seccnt, sjq_memaddr(curdisk));
}
#endif
/*	CASE 2 and 7:							*/
		if ((sjq_daddr(curdisk) >= altp->bad_start) &&
		    (lastsec <= altp->bad_end)) {       
#ifdef SD01_DEBUG
if(sd01alts_debug & DCLR) {
	printf("sd01alt_badsec: CASE 2 & 7 \n");
}
#endif
			set_sjq_daddr (curdisk, (altp->good_start + 
				sjq_daddr(curdisk) - altp->bad_start));
			if (curdisk=curdisk->j_fwmate) {
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
				continue;       /* do remaining sects 	*/
			}
			else    break;  /* that's all for this guy. 	*/
		}

/* at least one bad sector in our section.  break it. 			*/
		jp = sd01getjob(dk);
		jp->j_fwmate = curdisk->j_fwmate;
		jp->j_bkmate = curdisk;
		if (curdisk->j_fwmate)
			curdisk->j_fwmate->j_bkmate = jp;
		curdisk->j_fwmate = jp;
/*	CASE 6:								*/
		if ((sjq_daddr(curdisk) <= altp->bad_end) &&
		    (sjq_daddr(curdisk) >= altp->bad_start)) {       
#ifdef SD01_DEBUG
if(sd01alts_debug & DCLR) {
	printf("sd01alt_badsec: CASE 6 \n");
}
#endif
			jp->j_seccnt = curdisk->j_seccnt 
				- (altp->bad_end - sjq_daddr(curdisk) + 1); 
			curdisk->j_seccnt -= jp->j_seccnt;
			set_sjq_daddr (jp, altp->bad_end + 1);
			set_sjq_memaddr(jp, ((int)sjq_memaddr(curdisk) +
					curdisk->j_seccnt * secsiz));
			set_sjq_daddr (curdisk, (altp->good_start +
				sjq_daddr(curdisk) - altp->bad_start));
			curdisk = jp;
			continue;       /* check rest of section 	*/
		}

/*	CASE 5:								*/
		if ((lastsec >= altp->bad_start) && (lastsec <=altp->bad_end)) {
#ifdef SD01_DEBUG
if(sd01alts_debug & DCLR) {
	printf("sd01alt_badsec: CASE 5 \n");
}
#endif
			jp->j_seccnt = lastsec - altp->bad_start + 1;
			curdisk->j_seccnt -= jp->j_seccnt;
			set_sjq_daddr (jp, altp->good_start);
			set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
					 curdisk->j_seccnt*secsiz));
			if (curdisk=jp->j_fwmate) {
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
				continue;       /* do remaining sects 	*/
			} else    break;  /* that's all for this guy. 	*/
		}
/*	CASE 4:								*/
#ifdef SD01_DEBUG
if(sd01alts_debug & DCLR) {
	printf("sd01alt_badsec: CASE 4\n");
}
#endif
		jp->j_seccnt = altp->bad_end - altp->bad_start + 1;
		curdisk->j_seccnt = altp->bad_start - sjq_daddr(curdisk);
		set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
				 curdisk->j_seccnt * secsiz));
		set_sjq_daddr (jp, altp->good_start);
		curdisk = jp;
		jp = sd01getjob(dk);
		jp->j_fwmate = curdisk->j_fwmate;
		jp->j_bkmate = curdisk;
		if (curdisk->j_fwmate)
			curdisk->j_fwmate->j_bkmate = jp;
		curdisk->j_fwmate = jp;
		jp->j_seccnt = lastsec - altp->bad_end;
		set_sjq_daddr (jp, altp->bad_end + 1);
		set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
				curdisk->j_seccnt * secsiz));
		curdisk = jp; /* continue with rest of section 		*/
	}

#ifdef SD01_DEBUG
if(sd01alts_debug & DCLR) {
	for (i=0,jp=diskhead; flag&&jp; i++,jp=jp->j_fwmate) {
		printf("sd01alt_badsec: [%d]", i);
		printf(" sector= %d count= %d mem= %x\n", sjq_daddr(jp), 
			jp->j_seccnt, sjq_memaddr(jp));
	}
}
if(sd01alts_debug & STFIN) {
	printf("Leaving sd01alt_badsec\n");
}
#endif
}

sd01dkd()
{
	struct 	job *jp;
	struct 	disk *dk;
	buf_t	*bp;
	int	part;
	int	status;
	int	oldpri;

	oldpri = splhi();
	sd01db.db_flag |= DBBH_ARM;
	sd01db.db_flag &= ~DBBH_ARMING;

dkd_loop:
	sd01db.db_flag |= DBBH_BUSY;

	while (sd01db.db_waitdk || sd01db.db_badjob) 
	{
	dk = sd01db.db_waitdk;
	/* if the disk doesn't have a sane vtoc it can't have bad sects  */
	/* remapped since it most likely doesn't have a valid alts tables */
	/* so fail the jobs and clear the states and wakeup/restore state */
	if (((dk != NULL) && (dk->dk_vtoc.v_sanity != VTOC_SANE)) ||
	   (((dk = sd01db.db_badjob->j_dk) != NULL) && 
	   (dk->dk_vtoc.v_sanity != VTOC_SANE))) {
		dk->dk_state &= ~DKMAPBAD;
		dk->dk_state &= ~DKPENDQ;
		dk->dk_waitp = NULL; 
		sd01db.db_waitdk = NULL;
		while (sd01db.db_badjob != NULL) {
			jp = sd01db.db_badjob;
			bp = jp->j_bp;
			bp->b_error = EIO;
			biodone(bp);
			sd01db.db_badjob = jp->j_forw;
			sd01freejob(jp);
		if (dk->dk_state & DKSUSP)
			sd01qresume(dk);
		wakeup((caddr_t)&(dk->dk_state));
		}
		goto sd01dkd_exit;
	}
	while (dk) {
		dk->dk_state |= DKBUSYQ;
		dk->dk_state &= ~DKPENDQ;
		sd01db.db_waitdk = dk->dk_waitp; 
		dk->dk_waitp = NULL; 
		spl0();
		sd01send(dk, -1);
		oldpri = splhi();
		dk = sd01db.db_waitdk;
	}

	jp = sd01db.db_badjob;
	while (jp) {
		sd01db.db_badjob = jp->j_forw;
		dk = jp->j_dk;
		bp = jp->j_bp;

/*		wait for hardware reassign to complete			*/
		while (dk->hde_state & HDEHWREA)
			sleep((caddr_t)&(dk->hde_state),PRIBIO);
		spl0();

/*		check for hardware reassign error			*/
		if (dk->hde_state & HDEREAERR) {
			dk->hde_state &= ~HDEREAERR;

/*			set growing bad sector info			*/
			dk->dk_gbadsec.bad_start = dk->dk_hdesec;
			dk->dk_gbadsec.bad_end   = dk->dk_hdesec;
			dk->dk_gbadsec.good_start= 0;
			dk->dk_gbadsec_cnt = 1;
			dk->dk_gbadsec_p = &(dk->dk_gbadsec);

/*			software remap					*/
			if (dk->dk_remapalts == NULL)
				status = 1; /* Fake failed call */
			else
				status = (*dk->dk_remapalts)(dk);

/*			check for software remap completion status	*/
			if (!status) {
				cmn_err(CE_NOTE, hde_mesg[HDESWALTS], dk->dk_hdesec);
			}
		} 

/*		if software remap fails or return error on read,
 *		then set error code to EIO and prepare to quit
 */
		if (status || ((Sd01bbh_flg & BBH_RDERR) &&
	      		(bp->b_flags & B_READ))) {
			bp->b_error = EIO;
			sd01freejob(jp);
			biodone(bp);
		} else {
/*			retry job requests				*/
			while (jp) {
				dk = jp->j_dk;
				bp = jp->j_bp;
				bp->b_error = 0;
				bp->b_flags &= ~(B_BAD|B_ERROR);
				part = DKSLICE(bp->b_edev);
				sd01freejob(jp);
				sd01gen_sjq(dk,part,bp);
				oldpri = splhi();
				jp = sd01db.db_fq;
				if (jp)
					sd01db.db_fq = jp->j_forw;
				splx(oldpri);
			}
		}

/*		queue resume						*/
		if (dk->dk_state & DKSUSP)
			sd01qresume(dk);

		dk->dk_state &= ~DKMAPBAD;
		wakeup((caddr_t)&(dk->dk_state));
		oldpri = splhi();
		jp = sd01db.db_badjob;
	}
	}


#ifdef SD01_DEBUG
	if(sd01alts_debug & DKD) {
		printf("sd01dkd: exit\n");
	}
#endif

sd01dkd_exit:
	sd01db.db_flag &= ~DBBH_BUSY;
	sleep((caddr_t)sd01dkd, PRIBIO);
	goto dkd_loop;
}

#ifdef SD01_DEBUGPR
sd01prt_jq(dk)
struct	disk *dk;
{
	struct	job *jp;
	struct 	scm *cmd;
	int	i;
	int	oldpri;
	short	inchr;
	extern	short dbgetchar();

	if (dk == (struct disk *)NULL)
		dk = Sd01_dp[0];

	printf("sd01print_job dk=0x%x\n", dk);
	printf("SYNC QUEUE\n");
	jp = dk->dk_syncq_fw;
	for (i=0; jp!= (struct job *)NULL; jp=jp->j_forw, i++) {

		cmd = &jp->j_cmd.cm;

		if (jp->j_cont->sb_type == SCB_TYPE) {
			printf("JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr",
				cmd->sm_addr, cmd->sm_len,
				jp->j_cont->SCB.sc_datapt);
		}
		if ((i!=0) && !(i%16)) {
			inchr = dbgetchar();
			if (inchr != '\n')
				break;
		}
	}
	printf("ASYNC QUEUE\n");
	jp = dk->dk_next;
	for (i=0; jp!= (struct job *)dk; jp=jp->j_forw, i++) {

		cmd = &jp->j_cmd.cm;
		if (jp == dk->dk_batch) 
			printf("NEW BATCH\n");

		if (jp->j_cont->sb_type == SCB_TYPE) {	
			printf("JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr", 
				cmd->sm_addr, cmd->sm_len, 
				jp->j_cont->SCB.sc_datapt);
		}
		if ((i!=0) && !(i%16)) {
			inchr = dbgetchar();
			if (inchr != '\n')
				break;
		}
	}
}

sd01prt_job(jp)
struct job *jp;
{
	printf("JOB jp= 0x%x j_cont= 0x%x j_done= 0x%x bp=0x%x dk= 0x%x\n",
		jp, jp->j_cont, jp->j_done, jp->j_bp, jp->j_dk);
	printf("fwmate= 0x%x bkmate= 0x%x daddr=0x%x memaddr= 0x%x seccnt= 0x%x\n",
		jp->j_fwmate, jp->j_bkmate, jp->j_daddr,
		jp->j_memaddr, jp->j_seccnt);
}

sd01prt_dsk(idx)
int	idx;
{
	struct	disk	*dp;

	dp = Sd01_dp[idx];

	printf("DISK dp= 0x%x dk_state= 0x%x dk_addr.sa_fill= 0x%x dk_addr.sa_lun= 0x%x\n", dp, dp->dk_state, dp->dk_addr.sa_fill, dp->dk_addr.sa_lun);
}
#endif

