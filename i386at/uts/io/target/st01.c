/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/st01.c	1.25"
#ident	"$Header: miked 1/23/92 $"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*
**	SCSI Tape Target Driver.
*/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <fs/file.h>
#include <io/open.h>
#include <io/ioctl.h>
#include <util/debug.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/target/tape.h>
#include <io/target/st01.h>
#include <io/target/dynstructs.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define	DRVNAME		"st01 - tape target driver"

STATIC	int	st01_load(), st01_unload();
void 	st01rinit(); 
MOD_DRV_WRAPPER(st01, st01_load, st01_unload, NULL, DRVNAME);

/* Allocated in space.c */
extern 	struct tc_data	St01_data[];	/* Array of TC dev info	    */
extern	unsigned 	St01_datasz;	/* Number of supported TCs  */
extern	long	 	St01_cmajor;	/* Character major number   */
extern	int		St01_reserve;	/* Flag for reserving tape on open */
extern  struct  head	lg_poolhead;

static	int 		st01_tapecnt;	/* Num of tapes configured  */
static	struct tc_edt  *st01_edt;	/* Array of EDT structures  */
static	struct tape    *st01_tape;	/* Array of Tape structures */
static	int		st01_errmsgflg;	/* Debug on/off flag	    */
static	int		rinit_flag = 0;	/* flag	to protect rinit func */
static  struct owner   *ownerlist = NULL;	/* List of owner structs from sdi_doconfig */
static	int	mod_dynamic = 0;
static	size_t	mod_memsz = 0;

STATIC	int
st01_load()
{
	mod_dynamic = 1;
	st01init();
	return(0);
}

STATIC	int
st01_unload()
{
	sdi_clrconfig(ownerlist, SDI_DISCLAIM|SDI_REMOVE, st01rinit);

	if(mod_memsz > 0)	{
		ownerlist = NULL;
		kmem_free((caddr_t)st01_tape, mod_memsz);
		st01_tape = NULL;
	}
	return(0);
}

/* Aliases - see scsi.h */
#define ss_code		ss_addr1
#define ss_mode		ss_addr1
#define ss_parm		ss_len
#define ss_len1		ss_addr
#define ss_len2		ss_len
#define ss_cnt1		ss_addr
#define ss_cnt2		ss_len

#define	GROUP0		0
#define	GROUP1		1
#define	GROUP6		6
#define	GROUP7		7

#define	msbyte(x)	(((x) >> 16) & 0xff)
#define	mdbyte(x)	(((x) >>  8) & 0xff)
#define	lsbyte(x)	((x) & 0xff)

#define SPL     	spl5

void	st01intn();
void	st01intf();
void	st01intrq();
void	st01strategy();
int	st01reserve();
int	st01release();

int	st01devflag = D_NEW | D_DMA | D_TAPE; /* SVR4 Driver Flags */

/*
** Function name: st01init()
** Description:
**	Called by kernel to perform driver initialization.
**	This function does not access any devices.
*/

st01init()
{
	register struct tape   *tp;	/* Tape pointer		 */
	struct	owner	*op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  tapesz,			/* Tape size (in bytes)  */
	     tc;			/* TC number		 */

	drv_maj.b_maj = 0;
	drv_maj.c_maj = St01_cmajor;
	ownerlist = sdi_doconfig(ST01_dev_cfg, ST01_dev_cfg_size,
				"ST01 Tape Driver", &drv_maj, st01rinit);
	st01_tapecnt = 0;
	for (op = ownerlist; op; op = (struct owner *)op->res1) {
		st01_tapecnt++;
	}

#ifdef DEBUG
	printf("%d tapes claimed\n", st01_tapecnt);
#endif

	/* Check if there are devices configured */
	if (st01_tapecnt == 0) {
		return;
	}

	/*
	 * Allocate the tape structures
	 */
	tapesz = st01_tapecnt * sizeof(struct tape);
	mod_memsz = tapesz;
        if ((base = kmem_alloc(mod_memsz, (mod_dynamic ? KM_SLEEP : KM_NOSLEEP))) == NULL)
	{
                cmn_err(CE_WARN,
                        "Tape driver: Initialization failure -- insufficient memory for driver");
                cmn_err(CE_CONT,
                        "-- tape driver disabled\n");
		st01_tapecnt = 0;
		mod_memsz = 0;
		return;
	}
	st01_tape = (struct tape *) base;

	/*
	 * Initialize the tape structures
	 */
	tp = st01_tape;
	for(tc = 0, op = ownerlist; tc < st01_tapecnt;
			tc++, op=(struct owner *)op->res1, tp++) {

		/* Initialize state & counters */
		tp->t_state  = 0;
		tp->t_fltcnt = 0;

		/* Allocate the fault SBs */
		tp->t_fltreq = sdi_getblk();  /* Request sense */
		tp->t_fltres = sdi_getblk();  /* Resume */
		tp->t_fltrst = sdi_getblk();  /* Reset */

		/* Initialize the SCSI address */
		tp->t_addr.sa_exlun = 0;
#ifdef DEBUG
	printf("Tape: op 0x%x ", op);
	printf("edt 0x%x ", op->edtp);
	printf("hba %d scsi id %d lun %d\n",op->edtp->hba_no,op->edtp->scsi_id,op->edtp->lun);
#endif
		tp->t_addr.sa_lun    = op->edtp->lun;
	    	tp->t_addr.sa_fill   = (op->edtp->hba_no << 3) |
						(op->edtp->scsi_id);
		tp->t_spec	    = sdi_findspec(op->edtp, st01_dev_spec);
	}
}

/*
** Function name: st01rinit()
** Description:
**	Called by sdi to perform driver initialization of additional
**	devices found after the dynamic load of HBA drivers. This 
**	routine is called only when st01 is a static driver.
**	This function does not access any devices.
*/
void
st01rinit()
{
	register struct tape  *tp, *otp;	/* tape pointer	 */
	struct	owner	*nohp, *op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  tapesz,			/* tape size (in bytes) */
	     new_tapecnt,		/* number of additional devs found*/
	     otapecnt,			/* number of devs previously found*/
	     tmpcnt,			/* temp count of devs */
	     found,			/* search flag */
	     prevpl,			/* prev process level for splx */
	     tapecnt;			/* tape instance	*/

	/* set rinit_flag to prevent routines from accessing st01_tape */
	/* while its being copied to bigger array.		       */
	rinit_flag = 1;
	new_tapecnt= 0;
	drv_maj.b_maj = 0;
	drv_maj.c_maj = St01_cmajor;
	/* call sdi_doconfig with NULL func so we don't get called again */
	nohp = sdi_doconfig(ST01_dev_cfg, ST01_dev_cfg_size,
				"ST01 CDROM Driver", &drv_maj, NULL);

	for (op = nohp; op; op = (struct owner *)op->res1) {
		new_tapecnt++;
	}
#ifdef DEBUG
	printf("st01rinit %d tapes claimed\n", new_tapecnt);
#endif
	/* Check if there are additional devices configured */
	if (new_tapecnt == st01_tapecnt) {
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Allocate the tape structures
	 */
	tapesz = new_tapecnt * sizeof(struct tape);
        if ((base = kmem_alloc(tapesz, KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN,
			"CD-ROM Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",tapesz);
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Initialize the tape structures
	 */
	otapecnt = st01_tapecnt;
	found = 0;
	prevpl = spl5();
	for(tp = (struct tape *)base, tapecnt = 0, op = nohp; 
	    tapecnt < new_tapecnt;
	    tapecnt++, op=(struct owner *)op->res1, tp++) {

		/* Initialize new tape structs by copying existing tape */
		/* structs into new struct and initializing new instances */
		if (otapecnt) {
			for (otp = st01_tape, tmpcnt = 0; 
			     tmpcnt < st01_tapecnt; otp++,tmpcnt++) {
				if ((otp->t_addr.sa_lun == op->edtp->lun) &&
	    			   (otp->t_addr.sa_fill == 
				   ((op->edtp->hba_no << 3)|(op->edtp->scsi_id)))){
					found = 1;
					break;
				}
			}
			if (found) { /* copy otp to tp */
				tp->t_addr.sa_exlun = otp->t_addr.sa_exlun;
				tp->t_addr.sa_lun = otp->t_addr.sa_lun;
	    			tp->t_addr.sa_fill = otp->t_addr.sa_fill;
				tp->t_state = otp->t_state;
				tp->t_bsize = otp->t_bsize;
				tp->t_fltcnt = otp->t_fltcnt;
				tp->t_fltjob = otp->t_fltjob;
				tp->t_fltreq = otp->t_fltreq;
				tp->t_fltres = otp->t_fltres;
				tp->t_fltrst = otp->t_fltrst;
				tp->t_sense = otp->t_sense;
				tp->t_mode = otp->t_mode;
				tp->t_blklen = otp->t_blklen;
				tp->t_ident = otp->t_ident;
				tp->t_spec = otp->t_spec;
				found = 0;
				otapecnt--;
				continue;
			}
		}
		/* Its a new tape device so init tape struct */
		tp->t_state  = 0;
		tp->t_fltcnt = 0;
		/* Allocate the fault SBs */
		tp->t_fltreq = sdi_getblk();  /* Request sense */
		tp->t_fltres = sdi_getblk();  /* Resume */
		tp->t_fltrst = sdi_getblk();  /* Reset */
#ifdef DEBUG
	printf("tape: op 0x%x edt 0x%x hba %d ",op,op->edtp,op->edtp->hba_no);
	printf("scsi id %d lun %d\n",op->edtp->scsi_id,op->edtp->lun);
#endif
		tp->t_addr.sa_exlun = 0;
		tp->t_addr.sa_lun = op->edtp->lun;
	    	tp->t_addr.sa_fill = (op->edtp->hba_no << 3) | (op->edtp->scsi_id);
		tp->t_spec = sdi_findspec(op->edtp, st01_dev_spec);
	}
	/* Free up previously allocated space, if any allocated */
	if (st01_tapecnt > 0)
		kmem_free(st01_tape,mod_memsz);
	st01_tapecnt = new_tapecnt;
	st01_tape = (struct tape *)base;
	mod_memsz = tapesz;
	ownerlist = nohp;
	splx(prevpl);
	rinit_flag = 0;
	wakeup((caddr_t)&rinit_flag);
}

/*
** Function name: st01getjob()
** Description:
**	This function will allocate a tape job structure from the free
**	list.  The function will sleep if there are no jobs available.
**	It will then get a SCSI block from SDI.
*/

struct job *
st01getjob()
{
	register struct job *jp;

	jp = (struct job *)sdi_get(&lg_poolhead, 0);

	/* Get an SB for this job */
	jp->j_sb = sdi_getblk();
	return(jp);
}


/*
** Function name: st01freejob()
** Description:
**	This function returns a job structure to the free list. The
**	SCSI block associated with the job is returned to SDI.
*/

st01freejob(jp)
register struct job *jp;
{
	sdi_freeblk(jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}

#define ST01IOCTL(cmd, arg) st01ioctl(dev, cmd, arg, oflag, cred_p, (int*)0)

/*
** Function name: abort_open
** Description:
**	Clean up flags, so a failed open call can be aborted.
**	Clears T_OPENING flag, and wakes up anyone sleeping
**	for the current unit.
*/
void
abort_open(tp)
struct tape *tp;
{
    (void) st01release(tp);
    tp->t_state &= ~T_OPENING;
    (void) wakeup((caddr_t) &tp->t_state);
}

/*
** Function name: st01open()
** Description:
** 	Driver open() entry point.  Opens the device and reserves
**	it for use by that process only.
*/

st01open(devp, oflag, otyp, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int oflag, otyp;
{
	dev_t		dev = *devp;
	register struct tape *tp;
	unsigned	unit;
	int		oldpri; /* save processor priority level*/
	int		error; /* save rval of subfunctions */

	if (oflag & FAPPEND)
		return(ENXIO);

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	/* Only one open allowed at a time */
	oldpri = SPL();
	while (tp->t_state & T_OPENING)
		sleep((caddr_t) &tp->t_state, PRIBIO);
	tp->t_state |= T_OPENING;
	splx(oldpri);

	if (tp->t_state & T_OPENED) {
		tp->t_state &= ~T_OPENING;
		return(EBUSY);
	}

	tp->t_state &= (T_PARMS | T_OPENING);
		/* clear all bits except T_PARMS and T_OPENING */

	tp->t_addr.sa_major = getmajor(dev);
	tp->t_addr.sa_minor = getminor(dev);

	/* Initialize the fault SBs */
	tp->t_fltcnt = 0;
	tp->t_fltres->SFB.sf_dev    = tp->t_addr;
	tp->t_fltrst->SFB.sf_dev    = tp->t_addr;

	tp->t_fltreq->sb_type = ISCB_TYPE;
	tp->t_fltreq->SCB.sc_datapt = SENSE_AD(&tp->t_sense);
	tp->t_fltreq->SCB.sc_datasz = SENSE_SZ;
	tp->t_fltreq->SCB.sc_mode   = SCB_READ;
	tp->t_fltreq->SCB.sc_cmdpt  = SCS_AD(&tp->t_fltcmd);
	tp->t_fltreq->SCB.sc_dev    = tp->t_addr;
	sdi_translate(tp->t_fltreq, B_READ, 0);

	if (St01_reserve) {
		if (error=st01reserve(tp)) {
			abort_open(tp);
			return(error);
		}
	}

	if (((tp->t_state & T_PARMS) == 0) && (error=st01config(tp))) {
		abort_open(tp);
		return(error);
	}

	/***
	** Return an access error if tape is
	** write protected, and user wants to write.
	***/
	if (tp->t_mode.md_wp && (oflag & FWRITE)) {
		tp->t_state &= ~T_PARMS;
		abort_open(tp);
		return(EACCES);
	}

	if (RETENSION_ON_OPEN(dev))
		ST01IOCTL(T_RETENSION, 0);
	tp->t_state |= T_OPENED;
	tp->t_state &= ~T_OPENING;
	(void) wakeup((caddr_t) &tp->t_state);
	return(0);
}

/*
** Function name: st01close()
** Description:
** 	Driver close() entry point.  If the device has been opened
**	for writing, a file mark will be written.  If the device
**	has been opened to rewind on close, a rewind will be
**	performed; otherwise the tape head will be positioned after
**	the first filemark.
*/

st01close(dev, oflag, otyp, cred_p)
cred_t	*cred_p;
register dev_t dev;
int oflag, otyp;
{
	register struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	if (NOREWIND(dev)) {
		/* move past next file mark */
		if ((tp->t_state & T_READ)
		&& !(tp->t_state & (T_FILEMARK | T_TAPEEND)))
			ST01IOCTL(T_SFF, 1);
		/* write filemark */
		if (tp->t_state & T_WRITTEN)
			ST01IOCTL(T_WRFILEM, 1);
	} else {
		/* rewind the tape */
		ST01IOCTL(T_RWD, 0);
		ST01IOCTL(T_UNLOAD, 0);
		tp->t_state &= ~T_PARMS;
	}

	tp->t_state &= ~T_OPENED;
	if (St01_reserve) {
		if (st01release(tp)) {
			return(ENXIO);
		}
	}

	return(0);
}


/*
** Function name: st01strategy()
** Description:
** 	Driver strategy() entry point.  Initiate I/O to the device.
**	This function only checks the validity of the request.  Most
**	of the work is done by st01io().
*/
void
st01strategy(bp)
register struct buf *bp;
{
	register struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(bp->b_dev);
	tp = &st01_tape[unit];

	/*
	 * If the tape drive is configured for fixed size blocks,
	 * check that request is for an integral number of blocks
	 */
	if (tp->t_bsize && (bp->b_bcount % tp->t_bsize > 0)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		biodone(bp);
		return;
	}

	bp->b_resid = bp->b_bcount;

	if (tp->t_state & T_TAPEEND) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENOSPC;
		biodone(bp);
		return;
	}

	if ((tp->t_state & T_FILEMARK) && (bp->b_flags & B_READ)) {
		biodone(bp);
		return;
	}

	if (bp->b_bcount > ST01_MAXSIZE) { 	/* The job is too big to*/
						/* handle all at once	*/
		st01szsplit(bp, st01strategy);
		return;
	}

	st01io(tp, bp);
}


/*
** Function name: st01read()
** Description:
** 	Driver read() entry point.  Performs a raw read from the
**	device.  The function calls physio() which locks the user
**	buffer into core.
*/

st01read(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	return(uiophysio(st01strategy, NULL, dev, B_READ, uio_p));
}


/*
** Function name: st01write()
** Description:
** 	Driver write() entry point.  Performs a raw write to the
**	device.  The function calls physio() which locks the user
**	buffer into core.
*/

st01write(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	return(uiophysio(st01strategy, NULL, dev, B_WRITE, uio_p));
}



/*
** Function name: st01ioctl()
** Description:
**	Driver ioctl() entry point.  Used to implement the following
**	special functions:
**
**    B_GETTYPE		-  Get bus type and driver name
**    B_GETDEV		-  Get pass-through major/minor numbers
**
**    T_RWD		-  Rewind tape to BOT
**    T_WRFILEM		-  Write file mark
**    T_SFF/SFB		-  Space filemarks forward/backwards
**    T_SBF/SBB		-  Space blocks forward/backwards
**    T_LOAD/UNLOAD	-  Medium load/unload
**    T_ERASE		-  Erase tape
**    T_RETENSION	-  Retension tape
**    T_RST		-  Reset tape
**
**    T_PREVMV		-  Prevent medium removal
**    T_ALLOMV		-  Allow medium removal
**    T_RDBLKLEN	-  Read block length limits
**    T_WRBLKLEN	-  Write block length to be used
**    T_STD		-  Set tape density
**
*/

st01ioctl(dev, cmd, arg, oflag, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t dev;
int cmd, oflag;
caddr_t arg;
{
	register struct job *jp;
	struct tape *tp;
	unsigned unit;
	int ret_code = 0;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	switch(cmd)
	{
	case T_RST:
		/*
		 * Reset tape
		 */
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}
		tp->t_fltrst->sb_type =     SFB_TYPE;
		tp->t_fltrst->SFB.sf_int =  st01intf;
		tp->t_fltrst->SFB.sf_dev =  tp->t_addr;
		tp->t_fltrst->SFB.sf_wd =   (long) tp;
		tp->t_fltrst->SFB.sf_func = SFB_RESETM;
		ret_code = ST01ICMD(tp, tp->t_fltrst);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_RWD:
		/*
		 * Rewind to beginning of tape
		 */
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}
		ret_code = st01cmd(tp, SS_REWIND, 0, NULL, 0, 0, SCB_READ, 0, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_WRFILEM:
		/*
		 * Write filemarks
		 */
		{
		register int cnt = (int) arg;

		if (cnt < 0)
			return(EINVAL);
		ret_code = st01cmd(tp, SS_FLMRK, cnt>>8, NULL, 0, cnt, SCB_WRITE, 0, 0);
		tp->t_state |= T_FILEMARK;
		tp->t_state &= ~T_WRITTEN;
		break;
		}

	case T_SFF:	/* Space filemarks forward   */
	case T_SFB:	/* Space filemarks backwards */
	case T_SBF:	/* Space  blocks  forward    */
	case T_SBB:	/* Space  blocks  backwards  */
		{
		register int cnt = (int) arg;
		register int code;

		if (tp->t_state & T_WRITTEN)
			return(EINVAL);

		if (cnt < 0)
			return(EINVAL);

		if (cmd == T_SFF || cmd == T_SFB)
			code = FILEMARKS;
		else
			code = BLOCKS;

		if (cmd == T_SFB || cmd == T_SBB)
			cnt = (-1) * cnt;

		ret_code = st01cmd(tp, SS_SPACE, cnt>>8, NULL, 0, cnt, SCB_READ, code, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;
		}

	case T_LOAD:
		/*
		 * Medium load/unload
		 */
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}
		ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD, SCB_READ, 0, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_UNLOAD:
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}
		ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, UNLOAD, SCB_READ, 0, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		tp->t_state &= ~T_PARMS;
		break;

	case T_ERASE:
		/*
		 * Erase tape
		 */
		ST01IOCTL(T_RWD, 1);	/* Must rewind first */
		ret_code = st01cmd(tp, SS_ERASE, 0, NULL, 0, 0, SCB_WRITE, LONG, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_RETENSION:
		/*
		 * Retension tape
		 */
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}
		ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, RETENSION, SCB_READ, 0, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case B_GETTYPE:
		/*
		 * Tell user bus and driver name
		 */
		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5))
			return(EFAULT);
		if (copyout("st01", ((struct bus_type *)arg)->drv_name, 5))
			return(EFAULT);
		break;

	case B_GETDEV:
		/*
		 * Return pass-thru device major/minor
		 * to user in arg.
		 */
		{
		dev_t	pdev;

		sdi_getdev(&tp->t_addr, &pdev);
		if (copyout((caddr_t)&pdev, arg, sizeof(pdev)))
			return(EFAULT);
		break;
		}

	case T_ERRMSGON:
		/*
		 * System Error message ON
		 */
		st01_errmsgflg = 0;
		break;

	case T_ERRMSGOFF:
		/*
		 * System Error message OFF
		 */
		st01_errmsgflg = 1;
		break;

	/*
	 * The following ioctls are group 0 commands
	 */
#ifdef T_TESTUNIT
	case T_TESTUNIT:
		/*
		 * Test Unit Ready
		 */
		ret_code = st01cmd(tp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0);
		break;
#endif

	case T_PREVMV:
		/*
		 * Prevent media removal
		 */
		ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 1, SCB_READ, 0, 0);
		break;

	case T_ALLOMV:
		/*
		 * Allow media removal
		 */
		ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 0, SCB_READ, 0, 0);
		break;

#ifdef T_INQUIR
	case T_INQUIR:
		/*
		 * Inquiry
		 */
		{
		register struct ident *idp;
		idp = (struct ident *)&tp->t_ident;

		if (ret_code = st01cmd(tp, SS_INQUIR, 0, idp, sizeof(struct ident),
				sizeof(struct ident), SCB_READ, 0, 0)) {
			break;
		}
		if (copyout((char *)idp, arg, sizeof(struct ident)))
			return(EFAULT);
		}
		break;
#endif

	case T_RDBLKLEN:
		/*
		 * Read block length limits
		 */
		{
		register struct blklen *cp;

		cp = &tp->t_blklen;
		if (ret_code = st01cmd(tp, SS_RDBLKLEN, 0, cp, RDBLKLEN_SZ, 0,
			SCB_READ, 0, 0))
			break;
		cp->max_blen = sdi_swap24(cp->max_blen);
		cp->min_blen = sdi_swap16(cp->min_blen);
		if (copyout((char *)cp, arg, sizeof(struct blklen)))
			return(EFAULT);
		}
		break;

	case T_WRBLKLEN:
		/*
		 * Write block length to be used
		 */
		{
		register struct blklen *cp;
		register struct mode *mp = &tp->t_mode;

		cp = &tp->t_blklen;
		if (copyin(arg, (char *)cp, sizeof(struct blklen)) < 0)
			return(EFAULT);

		if (cp->max_blen != cp->min_blen)
			return(EINVAL);

		if (ret_code = st01cmd(tp, SS_MSENSE, 0, mp, sizeof(struct mode),
			sizeof(struct mode), SCB_READ, 0, 0))
			break;

		mp->md_len = 0;			/* Reserved field 	*/
		mp->md_media = 0;		/* Reserved field 	*/
		mp->md_wp = 0;			/* Reserved field 	*/
		mp->md_nblks = 0;		/* Reserved field 	*/
		mp->md_bsize = sdi_swap24(cp->max_blen); /* Fix block size */
		if (ret_code = st01cmd(tp, SS_MSELECT, 0, mp, sizeof(struct mode),
			sizeof(struct mode), SCB_WRITE, 0, 0))
			break;

		tp->t_bsize = mp->md_bsize;	/* Note the new block size */
		}
		break;

	case T_STD:
		/*
		 * Set Tape Density
		 */
		{
		register int dc = (int) arg;	/* density code */
		register struct mode *mp = &tp->t_mode;

		if (dc < 0 || dc > 0xff)
			return(EINVAL);

		if (ret_code = st01cmd(tp, SS_MSENSE, 0, mp, sizeof(struct mode),
			sizeof(struct mode), SCB_READ, 0, 0))
			break;

		mp->md_len = 0;			/* Reserved field 	*/
		mp->md_media = 0;		/* Reserved field 	*/
		mp->md_wp = 0;			/* Reserved field 	*/
		mp->md_nblks = 0;		/* Reserved field 	*/
		mp->md_dens = dc;		/* User's density code */
		if (ret_code = st01cmd(tp, SS_MSELECT, 0, mp, sizeof(struct mode),
			sizeof(struct mode), SCB_WRITE, 0, 0))
			break;
		}
		break;

	case T_EOD:
		/*
		 * Position to end of recorded data
		 */
		{
		register int code = EORD;

		if (tp->t_state & T_WRITTEN)
			return(EINVAL);

		ret_code = st01cmd(tp, SS_SPACE, 0, NULL, 0, 0, SCB_READ, code, 0);
		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		/* These flags are always cleared when the tape is
		 * moved by an operation other than read or write.
		 */
		}
		break;

#ifdef T_MSENSE
	case T_MSENSE:
		/*
		 * Mode sense
		 * (Application has to swap all multi-byte fields)
		 */
		{
		register struct mode *cp;
		cp = &tp->t_mode;

		if (copyin(arg, (char *)cp, sizeof(struct mode)) < 0)
			return(EFAULT);

		if (ret_code = st01cmd(tp, SS_MSENSE, 0, cp, sizeof(struct mode),
			sizeof(struct mode), SCB_READ, 0, 0))
			break;

		if (copyout((char *)cp, arg, sizeof(struct mode)))
			return(EFAULT);
		}
		break;
#endif
#ifdef T_MSELECT
	case T_MSELECT:
		/*
		 * Mode select
		 * (Application has to swap all multi-byte fields)
		 */
		{
		register struct mode *cp;
		cp = &tp->t_mode;

		if (copyin(arg, (char *)cp, sizeof(struct mode)) < 0)
			return(EFAULT);

		ret_code = st01cmd(tp, SS_MSELECT, 0, cp, sizeof(struct mode),
			sizeof(struct mode), SCB_WRITE, 0, 0);
		}
		break;
#endif

	default:
		ret_code = EINVAL;
	}

	return(ret_code);
}

/*
** Function name: st01print()
** Description:
**	This function prints the error message provided by the kernel.
*/

int
st01print(dev, str)
dev_t dev;
register char *str;
{
	char name[NAMESZ];
	register struct tape *tp;

	tp = &st01_tape[UNIT(dev)];
	sdi_name(&tp->t_addr, name);
        cmn_err(CE_WARN, "Tape driver: %s, %s\n", name, str);
	return(0);
}

/*
** Function name: st01io()
** Description:
**	This function creates and sends a SCSI I/O request.
*/

st01io(tp, bp)
register struct tape *tp;
register buf_t *bp;
{
	register struct job *jp;
	register struct scb *scb;
	register struct scs *cmd;
	register struct mode *mp;
	register int nblk;
	int s;

	jp = st01getjob();
	jp->j_tp = tp;
	jp->j_bp = bp;
	mp = &tp->t_mode;

	jp->j_sb->sb_type = SCB_TYPE;
	jp->j_time = JTIME;

	/*
	 * Fill in the scb for this job.
	 */

	scb = &jp->j_sb->SCB;
	scb->sc_cmdpt = SCS_AD(&jp->j_cmd);
	scb->sc_cmdsz = SCS_SZ;
	scb->sc_datapt = bp->b_un.b_addr;
	scb->sc_datasz = bp->b_bcount;
	scb->sc_link = NULL;
	scb->sc_mode = (bp->b_flags & B_READ) ? SCB_READ : SCB_WRITE;
	scb->sc_dev = tp->t_addr;

	sdi_translate(jp->j_sb, bp->b_flags, bp->b_proc);

	scb->sc_int = st01intn;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/*
	 * Fill in the command for this job.
	 */

	cmd = (struct scs *)&jp->j_cmd;
	cmd->ss_op = (bp->b_flags & B_READ) ? SS_READ : SS_WRITE;
	cmd->ss_lun  = tp->t_addr.sa_lun;

	if (tp->t_bsize) {	 /* Fixed block transfer mode	*/
		cmd->ss_mode = FIXED;
		nblk = bp->b_bcount / tp->t_bsize;
		cmd->ss_len1 = mdbyte(nblk) << 8 | msbyte(nblk);
		cmd->ss_len2 = lsbyte(nblk);
	} else {		/* Variable block transfer mode */
		cmd->ss_mode = VARIABLE;
		cmd->ss_len1 = mdbyte(bp->b_bcount) << 8 | msbyte(bp->b_bcount);
		cmd->ss_len2 = lsbyte(bp->b_bcount);
	}
	cmd->ss_cont = 0;

	ST01SEND(jp);	/* send it */
}


/*
** Function name: st01comp()
** Description:
**	Called on completion of a job, both successful and failed.
**	The function de-allocates the job structure used and calls
**	biodone().  Restarts the logical unit queue if necessary.
*/

st01comp(jp)
register struct job *jp;
{
        register struct tape *tp;
	register struct buf *bp;

        tp = jp->j_tp;
	bp = jp->j_bp;

	/* Check if job completed successfully */
	if (jp->j_sb->SCB.sc_comp_code == SDI_ASW) {
		bp->b_resid  = 0;
		tp->t_lastop = jp->j_cmd.ss.ss_op;
		if (tp->t_lastop == SS_READ)
			tp->t_state |= T_READ;
		if (tp->t_lastop == SS_WRITE) {
			tp->t_state &= ~T_FILEMARK;
			tp->t_state |= T_WRITTEN;
		}
	} else {
		bp->b_flags |= B_ERROR;
		if (jp->j_sb->SCB.sc_comp_code == SDI_NOSELE)
			bp->b_error = ENODEV;
		else
			bp->b_error = EIO;

		if (tp->t_state & T_TAPEEND)	{
			if(bp->b_bcount && (bp->b_resid == bp->b_bcount))    {
				bp->b_error = ENOSPC;
			}
			else	{
				bp->b_flags &= ~B_ERROR;
			}
		}
		else if(tp->t_state & T_FILEMARK)	{
			bp->b_flags &= ~B_ERROR;
		}
	}

	biodone(bp);
	st01freejob(jp);

	/* Resume queue if suspended */
	if (tp->t_state & T_SUSPEND)
	{
		tp->t_fltres->sb_type = SFB_TYPE;
		tp->t_fltres->SFB.sf_int  = st01intf;
		tp->t_fltres->SFB.sf_dev  = tp->t_addr;
		tp->t_fltres->SFB.sf_wd = (long) tp;
		tp->t_fltres->SFB.sf_func = SFB_RESUME;
		ST01ICMD(tp, tp->t_fltres);

		tp->t_state &= ~T_SUSPEND;
		tp->t_fltcnt = 0;
	}

	return;
}


/*
** Function name: st01intn()
** Description:
**	This function is called by the host adapter driver when an
**	SCB job completes.  If the job completed with an error, the
**	appropriate error handling is performed.
*/

void
st01intn(sp)
register struct sb *sp;
{
	register struct tape *tp;
	register struct job *jp;

	jp = (struct job *)sp->SCB.sc_wd;
	tp = jp->j_tp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		st01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code & SDI_SUSPEND)
		tp->t_state |= T_SUSPEND;

	if (sp->SCB.sc_comp_code == SDI_CKSTAT &&
	    sp->SCB.sc_status == S_CKCON)
	{
		tp->t_fltjob = jp;

		tp->t_fltreq->sb_type = ISCB_TYPE;
		tp->t_fltreq->SCB.sc_int = st01intrq;
		tp->t_fltreq->SCB.sc_cmdsz = SCS_SZ;
		tp->t_fltreq->SCB.sc_time = JTIME;
		tp->t_fltreq->SCB.sc_mode = SCB_READ;
		tp->t_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		tp->t_fltreq->SCB.sc_wd = (long) tp;
		tp->t_fltcmd.ss_op = SS_REQSEN;
		tp->t_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		tp->t_fltcmd.ss_addr1 = 0;
		tp->t_fltcmd.ss_addr = 0;
		tp->t_fltcmd.ss_len = SENSE_SZ;
		tp->t_fltcmd.ss_cont = 0;

		/* clear old sense key */
		tp->t_sense.sd_key = SD_NOSENSE;

		ST01ICMD(tp, tp->t_fltreq);
	}
	else
	{
		/* Fail the job */
		st01logerr(tp, sp);
		st01comp(jp);
	}

	return;
}


/*
** Function name: st01intrq()
** Description:
**	This function is called by the host adapter driver when a
**	request sense job completes.  The job will be retried if it
**	failed.  Calls st01sense() on successful completions to
**	examine the request sense data.
*/

void
st01intrq(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  &&
	    sp->SCB.sc_comp_code &  SDI_RETRY   &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		sp->SCB.sc_time = JTIME;
		ST01ICMD(tp, sp);
		return;
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	}

	st01sense(tp);
}


/*
** Function name: st01intf()
** Description:
**	This function is called by the host adapter driver when a host
**	adapter function request has completed.  If there was an error
**	the request is retried.  Used for resume function completions.
*/

void
st01intf(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY  &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		ST01ICMD(tp, sp);
		return;
	}

	if (sp->SFB.sf_comp_code != SDI_ASW)
		st01logerr(tp, sp);
}


/*
** Function name: st01sense()
** Description:
**	Performs error handling based on SCSI sense key values.
*/

st01sense(tp)
register struct tape *tp;
{
	register struct job *jp;
	register struct sb *sp;
	register struct mode *mp;

	jp = tp->t_fltjob;
	sp = jp->j_sb;
	mp = &tp->t_mode;

        switch(tp->t_sense.sd_key) {
	case SD_VOLOVR:
		tp->t_state |= T_TAPEEND;
		/* FALLTHROUGH */

	case SD_NOSENSE:
		if (jp->j_cmd.ss.ss_op == SS_READ  ||
		    jp->j_cmd.ss.ss_op == SS_WRITE)
		{
			register struct buf *bp = jp->j_bp;
			register nblks;

			if (tp->t_sense.sd_valid) {
				if (tp->t_bsize) {	/* Fixed Block Len */
					nblks = sdi_swap32(tp->t_sense.sd_ba);
					bp->b_resid = tp->t_bsize * nblks;
				} else			/* Variable	   */
					bp->b_resid = sdi_swap32(tp->t_sense.sd_ba);
			}
			else if(tp->t_sense.sd_key == SD_VOLOVR)	{
				bp->b_resid = 0;
			}
			if (tp->t_sense.sd_ili) {
				st01logerr(tp, sp);
                                cmn_err(CE_WARN,
                                "Tape driver: Block length mismatch.");
                                cmn_err(CE_CONT,
                                "Driver requested %d, Tape block length = %d\n",
                                bp->b_bcount, bp->b_bcount - (int) bp->b_resid);
			/*
			 * The t_sense.sd_ba information field now contains
			 *  (signed) ((bp->b_bcount) - (actual recordsize))
			 *
			 * If the actual record size was larger than the
			 * requested amount, requested data has been read,
			 * but we have skipped over the rest of the record.
			 */
				if ((int) bp->b_resid < 0)
					bp->b_resid = 0;
				bp->b_flags |= B_ERROR;
			}
		}

		if (tp->t_sense.sd_fm)
			tp->t_state |= T_FILEMARK;
		if (tp->t_sense.sd_eom)
			tp->t_state |= T_TAPEEND;

		st01comp(jp);
		break;

	case SD_UNATTEN:
		if (++tp->t_fltcnt > MAX_RETRY) {
			st01logerr(tp, sp);
			st01comp(jp);
		} else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = JTIME;
			ST01ICMD(tp, sp);
		}
		break;

	case SD_RECOVER:
		st01logerr(tp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		st01comp(jp);
		break;

	/* Some drives give a blank check during positioning */
	case SD_BLANKCK:
		/*
		if ((jp->j_cmd.ss.ss_op == SS_READ)
		||  (jp->j_cmd.ss.ss_op == SS_WRITE))
			st01logerr(tp, sp);
		else
			sp->SCB.sc_comp_code = SDI_ASW;
		*/
		st01logerr(tp, sp);
		st01comp(jp);
		break;

	default:
		st01logerr(tp, sp);
		st01comp(jp);
	}
}

/*
** Function name: st01logerr()
** Description:
**	This function will print the error messages for errors detected
**	by the host adapter driver.  No message will be printed for
**	those errors that the host adapter driver has already reported.
*/

st01logerr(tp, sp)
register struct tape *tp;
register struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_SFB_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT  && sp->SCB.sc_status == S_CKCON) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_CKCON_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_CKSTAT_ERR,0);
		}
	}
}

/*	 st01szsplit - Splits large transfers.	*/

st01szsplit(obp, strategy)
register buf_t *obp;
void (*strategy)();
{
	register buf_t *bp;
	register int count;
	register int actual;

	bp = (buf_t *) getrbuf(KM_SLEEP);
	*bp = *obp;
	bp->b_iodone = (void (*)())NULL;
	count = obp->b_bcount;
	while(count > 0)
	{
		bp->b_bcount = count > ST01_MAXSIZE ? ST01_MAXSIZE : count;
		bp->b_flags &= ~B_DONE;
		strategy(bp);
		biowait(bp);
		actual = bp->b_bcount - bp->b_resid;

		/****
		** If ENOSPC has been set for bp (one portion of the
		** original i/o request), don't report an error, but
		** report a residual count.  ENOSPC will be reported
		** for the next user i/o request without doing any i/o.
		** This is necessary for multi-volume archives.
		****/
		if (bp->b_flags & B_ERROR && bp->b_error != ENOSPC)
		{
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
			break;
		}
		if (actual == 0)
			break;
		bp->b_un.b_addr += actual;
		count -= actual;
	}
	obp->b_resid = count;
	freerbuf(bp);
	biodone(obp);
}

/*
** Function name: st01config()
** Description:
**	Initializes the tape driver's tape parameter structure. To
**	support Autoconfig, the command sequence should be:
**
**		INQUIRY*
**		LOAD
**		TEST UNIT READY
**		MODE SENSE
**		MODE SELECT*
**
**	Autoconfig is not implemented at this point. In this case, both
**	INQUIRY and MODE SELECT are not called.	 The driver will use
**	whatever settings returned by MODE SENSE as default.
*/

st01config(tp)
register struct tape *tp;
{
	register struct blklen *cp;
	register struct mode *mp;

	cp = &tp->t_blklen;

	if (st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD, SCB_READ, 0, 0)) {
                cmn_err(CE_WARN, "!Tape driver: Logical Unit %d - tape failed to load",
                        tp->t_addr.sa_lun);
		return(EIO);
	}
	if (st01cmd(tp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
                cmn_err(CE_WARN, "!Tape driver: Logical Unit %d - Device Not Ready",
                        tp->t_addr.sa_lun);
		return(EIO);
	}
	/* Send READ BLOCK LIMIT to obtain max/min block length limit */
	if (st01cmd(tp, SS_RDBLKLEN, 0, (char *)cp, RDBLKLEN_SZ, 0, SCB_READ, 0, 0))
	{
                cmn_err(CE_WARN, "!Tape driver: Logical Unit %d - Unable to determine tape block length parameters",
                        tp->t_addr.sa_lun);
		return(EIO);
	}
	cp->max_blen = sdi_swap24(cp->max_blen);
	cp->min_blen = sdi_swap16(cp->min_blen);

	/*
	 * Check if the parameters are valid
	 */
	if (cp->max_blen < 0)
	{
                cmn_err(CE_WARN,
                        "!Tape driver: Logical Unit %d - Illegal Max Block Length - 0x%x",
                        tp->t_addr.sa_lun, cp->max_blen);
		return(ENXIO);
	}
	if (cp->min_blen < 0)
	{
                cmn_err(CE_WARN, "!Tape driver: Logical Unit %d - Illegal Min Block Length - 0x%x",
                        tp->t_addr.sa_lun, cp->min_blen);
		return(ENXIO);
	}

	/* Send Mode Sense	*/
	mp = &tp->t_mode;
	if (st01cmd(tp, SS_MSENSE, 0, mp, sizeof(struct mode),
		sizeof(struct mode), SCB_READ, 0, 0)) {
                cmn_err(CE_WARN, "!Tape driver: Logical Unit %d - Mode sense command failed",
                tp->t_addr.sa_lun);
		return(EIO);
	}
	mp->md_bsize = sdi_swap24(mp->md_bsize);
	if ((mp->md_bsize < cp->min_blen) && (mp->md_bsize != 0)) {
                cmn_err(CE_WARN,
         "!Tape driver: Logical Unit %d - block size, %d, less than minimum block length, %d",
                        tp->t_addr.sa_lun, mp->md_bsize, cp->min_blen);
		return(ENXIO);
	}
	if ((mp->md_bsize > cp->max_blen) && (mp->md_bsize != 0)
			&& (cp->max_blen !=0)) {
                cmn_err(CE_WARN,
         "!Tape driver: Logical Unit %d - block size %d, greater than maximum block length, %d",
                        tp->t_addr.sa_lun, mp->md_bsize, cp->max_blen);
		return(ENXIO);
	}
	tp->t_bsize = mp->md_bsize;

	/*
	 * Indicate parameters are set and valid
	 */
	tp->t_state |= T_PARMS;
	return(0);
}

/*
** Function name: st01cmd()
** Description:
**	This function performs a SCSI command such as Mode Sense on
**	the addressed tape.  The op code indicates the type of job
**	but is not decoded by this function.  The data area is
**	supplied by the caller and assumed to be in kernel space.
**	This function will sleep.
*/

st01cmd(tp, op_code, addr, buffer, size, length, mode, param, control)
register struct tape	*tp;
unsigned char	op_code;		/* Command opcode		*/
unsigned int	addr;			/* Address field of command 	*/
char		*buffer;		/* Buffer for command data 	*/
unsigned int	size;			/* Size of the data buffer 	*/
unsigned int	length;			/* Block length in the CDB	*/
unsigned short	mode;			/* Direction of the transfer 	*/
unsigned int	param;
unsigned int	control;
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	int error;

	bp = getrbuf(KM_SLEEP);

	jp = st01getjob();
	scb = &jp->j_sb->SCB;

	bp->b_iodone = NULL;
	bp->b_blkno = addr;
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;

	jp->j_bp = bp;
	jp->j_tp = tp;
	jp->j_sb->sb_type = SCB_TYPE;

	switch(op_code >> 5){
	case	GROUP7:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = tp->t_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_res1 = param;
		cmd->sm_cont = control;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP6:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = tp->t_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = length;
		cmd->ss_cont  = control;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
	case	GROUP1:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = tp->t_addr.sa_lun;
		cmd->sm_res1 = param;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP0:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = tp->t_addr.sa_lun;
		cmd->ss_addr1 = param;
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
#ifdef DEBUG
        default:
                cmn_err(CE_WARN,"Tape driver: Unknown op_code = %x\n",op_code);
#endif
	}

	/* Fill in the SCB */
	scb->sc_int = st01intn;
	scb->sc_dev = tp->t_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = 180 * ONE_MIN;
	scb->sc_wd = (long) jp;
	sdi_translate(jp->j_sb, bp->b_flags, (caddr_t)0);

	ST01SEND(jp);

	biowait(bp);

	if (bp->b_flags & B_ERROR)
		error = bp->b_error;
	else
		error = 0;
	freerbuf(bp);
	return(error);
}
extern sdi_send(), sdi_icmd();

ST01SEND(jobp)
struct job *jobp;
{
	return st01docmd(sdi_send, jobp->j_tp, jobp->j_sb);
}

ST01ICMD(tapep, sbp)
struct tape *tapep;
struct sb *sbp;
{
	return st01docmd(sdi_icmd, tapep, sbp);
}

int
st01docmd(fcn, tapep, sbp)
int (*fcn)();
struct tape *tapep;
struct sb *sbp;
{
	struct dev_spec *dsp = tapep->t_spec;
	int cmd;

	if (dsp && sbp->sb_type != SFB_TYPE) {
		cmd = ((struct scs *)sbp->SCB.sc_cmdpt)->ss_op;
		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(tapep, sbp);
		}
	}

	return (*fcn)(sbp);
}


/*
** Function name: st01reserve()
** Description:
**	Reserve a device.
*/
int
st01reserve(tp)
register struct tape *tp;
{
	int ret_val;

	if (tp->t_state & T_RESERVED)		/* if already reserved */
		return(0);
	if (ret_val = st01cmd(tp, SS_RESERV, 0, NULL, 0, 0, SCB_WRITE, 0, 0)) {
#ifdef DEBUG
                cmn_err(CE_WARN, "Tape driver: LU %d - reserve command failed",
                        tp->t_addr.sa_lun);
#endif
		return(ret_val);
	}
	tp->t_state |= T_RESERVED;
	return(0);
}

/*
** Function name: st01release()
** Description:
**	Release a device.
*/
int
st01release(tp)
register struct tape *tp;
{
	int ret_val;

	if (! (tp->t_state & T_RESERVED))	/* if already released */
		return(0);
	if (ret_val = st01cmd(tp, SS_RELES, 0, NULL, 0, 0, SCB_WRITE, 0, 0)) {
		/* release cmd returns an error only when device was reserved
		 * and release failed to release the device.  In this case,
		 * the device is still reserved, so don't clear the
		 * T_RESERVED flag.
		 */
#ifdef DEBUG
                cmn_err(CE_WARN, "Tape driver: LU %d - release command failed",
                        tp->t_addr.sa_lun);
#endif
		return(ret_val);
	}
	tp->t_state &= ~T_RESERVED;
	return(0);
}
