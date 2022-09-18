/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_SDI_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/target/sdi.h	1.15"
#ident	"$Header: $"

/*=================================================================*/
/* sdi.h
/*=================================================================*/

#define	SDI_EXLUN	0x80		/* Indicates extended logical unit # */

#define	SCB_TYPE	1
#define	ISCB_TYPE	2

		/* mode field */
#define	SCB_WRITE	0x00		/* Non-read job                       */
#define	SCB_READ	0x01		/* Read data job                      */
#define	SCB_LINK	0x02		/* SCSI command linking is used       */
#define	SCB_HAAD	0x04		/* Address supplied by HA             */
#define SCB_PARTBLK	0x08		/* Partial block transfer	      */

		/* completion code field */
#define	SDI_NOALLOC	0x00000000	/* This block is not allocated      */
#define	SDI_ASW		0x00000001	/* Job completed normally           */
#define	SDI_LINKF0	0x00000002	/* Linked command done without flag */
#define	SDI_LINKF1	0x00000003	/* Linked command done with flag    */
#define	SDI_QFLUSH	0xE0000004	/* Job was flushed                  */
#define	SDI_ABORT	0xF0000005	/* Command was aborted              */
#define	SDI_RESET	0xF0000006	/* Reset was detected on the bus    */
#define	SDI_CRESET	0xD0000007	/* Reset was caused by this unit    */
#define	SDI_V2PERR	0xA0000008	/* vtop failed                      */
#define	SDI_TIME	0xD0000009	/* Job timed out                    */
#define	SDI_NOTEQ	0x8000000A	/* Addressed device not present     */
#define	SDI_HAERR	0xE000000B	/* Host adapter error               */
#define	SDI_MEMERR	0xA000000C	/* Memory fault                     */
#define	SDI_SBUSER	0xA000000D	/* SCSI bus error                   */
#define	SDI_CKSTAT	0xD000000E	/* Check the status byte  	    */
#define	SDI_SCBERR	0x8000000F	/* SCB error                        */
#define	SDI_OOS		0xA0000010	/* Device is out of service         */
#define	SDI_NOSELE	0x90000011	/* The SCSI bus select failed       */
#define	SDI_MISMAT	0x90000012	/* parameter mismatch               */
#define	SDI_PROGRES	0x00000013	/* Job in progress                  */
#define	SDI_UNUSED	0x00000014	/* Job not in use                   */
#define	SDI_ONEIC	0x80000017	/* More than one immediate request  */
#define SDI_SFBERR	0x80000019	/* SFB error			    */
#define SDI_TCERR	0x9000001A	/* Target protocol error detected   */

#define	SDI_ERROR	0x80000000	/* An error was detected         */
#define	SDI_RETRY	0x40000000	/* Retry the job                 */
#define	SDI_MESS	0x20000000	/* A message has been sent       */
#define	SDI_SUSPEND	0x10000000	/* Processing has been suspended */

#define	SFB_TYPE	3

	/* Defines for the command field */
#define	SFB_NOPF	0x00		/* No op function                  */
#define	SFB_RESETM	0x01		/* Send a bus device reset message */
#define	SFB_ABORTM	0x02		/* Send an abort message           */
#define	SFB_FLUSHR	0x03		/* Flush queue request             */
#define SFB_RESUME	0x04		/* Resume the normal job queue	   */
#define SFB_SUSPEND	0x05		/* Suspend the normal job queue    */

#define	SDI_3B20	0x01
#define	SDI_3B15	0x02
#define	SDI_3B5		0x03
#define	SDI_3B2		0x04
#define	SDI_APACHE_FS	0x05
#define	SDI_386_AT	0x06
#define	SDI_386_MCA	0x07

#define SDI_BASIC1              0x0001
#define SDI_FLT_HANDLING        0x0002

	/* Return values for sdi_send and sdi_icmd */
#define SDI_RET_OK 	0
#define SDI_RET_ERR 	-1
#define SDI_RET_RETRY	1

	/* Ioctl command codes */
#define SDI_SEND	0X0081		/* Send a SCSI command		*/
#define SDI_TRESET	0X0082		/* Reset a target controller	*/
#define SDI_BRESET	0X0084		/* Reset the SCSI bus		*/
#define HA_VER		0X0083		/* Get the host adapter version */
#define SDI_RESERVE     0X0085          /* Reserve the device           */
#define SDI_RELEASE     0X0086          /* Release the device           */
#define SDI_RESTAT      0X0087          /* Device Reservation Status    */
#define HA_GETPARMS	0x008a		/* Get HBA disk geometry        */
#define IHA_GETPARMS	0x008b		/* Get HBA disk geometry        */
#define HA_SETPARMS	0x008c		/* Set HBA disk geometry	*/
#define IHA_SETPARMS	0x008d		/* Set HBA disk geometry	*/
#define HA_GETPPARMS	0x008e		/* Get real disk geometry       */

        /* Fault handler flags */
#define SDI_FLT_RESET   0x00000001      /* logical unit was reset       */
#define SDI_FLT_PTHRU   0x00000002      /* pass through was used        */

struct scsi_ad{
	long		sa_major;	/* Major number                 */
	long		sa_minor;	/* Minor number                 */
	unsigned char	sa_lun;		/* logical unit number          */
	unsigned char	sa_exlun;	/* extended logical unit number */
	short		sa_fill;	/* Fill word                    */
};

#define ad2dev_t(ad)	(makedevice((ad).sa_major,(ad).sa_minor))

struct scb{
	unsigned long	sc_comp_code;	/* Current job status              */
	char		*sc_priv;	/* private ptr for Dyn alloc routines*/
					/* DO NOT USE or MODIFY		   */
	void		(*sc_int)();	/* Target Driver interrupt handler */
	caddr_t		sc_cmdpt;	/* Target command                  */
	caddr_t		sc_datapt;	/* Data area			   */
	long		sc_wd;		/* Target driver word              */
	time_t		sc_time;	/* Time limit for job              */
	struct scsi_ad	sc_dev;		/* SCSI device address             */
	unsigned short	sc_mode;	/* Mode flags for current job      */
	unsigned char	sc_status;	/* Target status byte              */
	char		sc_fill;	/* Fill byte                       */
	struct sb	*sc_link;	/* Link to next scb command        */
	long		sc_cmdsz;	/* Size of command                 */
	long		sc_datasz;	/* Size of data			   */
	long		sc_resid;	/* Bytes to xfer after data 	   */
	clock_t		sc_start;	/* Start time (job to controller)  */
};

struct sfb{
	unsigned long	sf_comp_code;	/* Current job status              */
	char		*sf_priv;	/* private ptr for Dyn alloc routines*/
					/* DO NOT USE or MODIFY		   */
	void		(*sf_int)();	/* Target Driver interrupt handler */
	struct scsi_ad	sf_dev;		/* SCSI device address             */
	unsigned long	sf_func;	/* Function to be performed        */
	int		sf_wd;		/* Target driver word		   */    
};

struct ver_no {
	unsigned char	sv_release;	/* The release number */
	unsigned char	sv_machine;	/* The running machine */
	short		sv_modes;	/* Supported modes     */
};

struct sb {
	unsigned long	sb_type;
	union{
		struct scb	b_scb;
		struct sfb	b_sfb;
	}sb_b;
};


#define SCB sb_b.b_scb
#define SFB sb_b.b_sfb

extern long		sdi_started;
extern long	init_time;	/* set when interrupts off, I/O needs polling */

extern struct ver_no	sdi_ver;

extern struct sb	*sdi_getblk ();
extern struct sdi_edt	*sdi_redt();

struct owner {
	struct owner *next;		/* link to next owner */
	struct sdi_edt *edtp;		/* back ptr to edt entry */
	struct drv_majors maj;		/* owner's major numbers */
	void	(*fault)();		/* routine to call after a bus reset */
	long	flt_parm;		/* parameter to (*fault)() */
	ulong	res1;			/* reserved for periph driver */
	ulong	res2;			/* reserved for sdi */
	char	*name;			/* owner's name */
};

#define SDI_CLAIM	0x01		/* claim a device for driver access */
#define SDI_ADD		0x02		/* add an owner block to list */
#define SDI_DISCLAIM	0x04		/* release claim to device */
#define SDI_REMOVE	0x08		/* remove driver from owner list */

struct sdi_edt {
	struct sdi_edt *hash_p;	/* next sdi_edt in hash list */
	short	hba_no;			/* HBA/path id */
	unsigned char	scsi_id;	/* SCSI id */
	unsigned char	lun;		/* logical unit number */
	struct owner *curdrv;		/* pointer to current owner */
	struct owner *owner_list;	/* Chain of owner attributes */
	unsigned long	res1;		/* reserved */
	int	pdtype;			/* SCSI physical device type */
	unsigned char	iotype;		/* I/O capability such as DMA, PIO */
	char	inquiry[INQ_LEN];	/* INQUIRY string returned */
};

/* The following defines are for the sdi_edt iotype field */
#define F_DMA	 	0x001		/* Device is a DMA device */
#define F_PIO	 	0x002		/* Device is a progammed I/O device */
#define F_SCGTH	 	0x004		/* Device supports scatter-gather DMA */
#define F_RMB		0x008		/* Device is removable media */

struct xsb {
	struct sb sb;			/* actual sb */
	struct hbadata *hbadata_p;	/* ptr to HBA driver's private data */
	struct owner *owner_p;		/* target driver's owner block */
};


#define NSDIDEV		32		/* max number of SDI devices */
#define EDT_HASH_LEN	7		/* edt hash table size;  use a prime */
#define NOWNER		28		/* only old target drivers need these */
#define NSDISB		(EDT_HASH_LEN+NSDIDEV)*3  /* max number of sb structs */

extern struct sdi_edt edtpool[NSDIDEV];	/* pool of edt entries */
extern struct sdi_edt edt_hash[EDT_HASH_LEN];
extern struct xsb sbpool[];	/* pool of sb structs */
extern struct owner owner_pool[NOWNER];

#define EDT_HASH(hba, scsi_id, lun)		\
			edt_hash[EDTHASHVAL((hba), (scsi_id), (lun))]
#define EDTHASHVAL(hba, scsi_id, lun)		\
		( ( (unsigned int)(((hba)<<6) | ((scsi_id)<<3) | (lun)) % \
			(unsigned int)EDT_HASH_LEN ))
struct hbadata {
	struct xsb *sb;
	/* addition driver dependent stuff here */
};

struct hbagetinfo {	/* structure used to pass up hba-specific data from
			   hba to sdi, using p##getinfo() function. */
	char *name;	/* Name */
	char iotype;	/* Specifies the type of I/O device is capable of */
			/* i.e. DMA, PIO, scatter/gather, etc */
};

void sdi_callback();

#define	HBA_INFO(p, f, x)	\
	long		p##freeblk();	\
	struct hbadata	* p##getblk();	\
	long		p##icmd();	\
	void		p##getinfo();	\
	long		p##send();	\
	void		p##xlat();	\
	int		p##open();	\
	int		p##close();	\
	int		p##ioctl();	\
	static	struct	hba_info	p##hba_info	= { \
		f, x,	\
		p##freeblk,	\
		p##getblk,	\
		p##icmd,	\
		p##getinfo,	\
		p##send,	\
		p##xlat,	\
		p##open,	\
		p##close,	\
		p##ioctl	\
	}

/*
 * Per-module HBA information.
 */
struct	hba_info	{
	int	*hba_flag;
	ulong	max_xfer;
	/*
	 * Entry points.
	 */
	long		 (*hba_freeblk)();
	struct	hbadata	*(*hba_getblk)();
	long		 (*hba_icmd)();
	void		 (*hba_getinfo)();
	long		 (*hba_send)();
	void		 (*hba_xlat)();
	int		 (*hba_open)();
	int		 (*hba_close)();
	int		 (*hba_ioctl)();
};

/*
 * Per-instance HBA information.
 */
struct	hba_idata	{
	int		version_num;	/* version num to determine contents */
					/* of idata struct by sdi driver     */
	char		*name;
	unsigned char	ha_id;
	ulong		ioaddr1;
	int		dmachan1;
	int		iov;
	int		cntlr;
	int		active;
};

#define	IDP(p)	((struct hba_idata *)(p))
#define	HIP(p)	((struct hba_info *)(p))

struct	hba_cfg	{
	struct	hba_info	*info;
	struct	hba_idata	*idata;
	ulong	active;
};

#define HBA_END_SENTINAL	"end sentinal"
extern struct hba_cfg HBA_tbl[];
extern int sdi_hacnt;

struct dev_cfg {
	ulong	match_type;		/* action to take on a match */
	ushort	hba_no;			/* match HBA, -1 if not used */
	unsigned char	scsi_id;	/* match SCSI id, 0xff if not used */
	unsigned char	lun;		/* match LUN, 0xff if not used */
	unsigned char devtype;		/* target device type */
	int	inq_len;		/* length of inquiry string to match */
	char	inquiry[INQ_LEN];	/* inquiry string to match */
};

struct dev_spec {
	char	inquiry[INQ_LEN];	/* device specification */
	int	(*first_open)();	/* first open support */
	int	(*last_close)();	/* last close support */
	void	(*intr)();		/* interrupt support */
	ulong	cmd_sup[8];		/* bitmap of recognized commands */
	ulong	cmd_chk[8];		/* bitmap of cmds passed to *command */
	void	(*command)();		/* SCSI command helper */
};

#define CMD_CHK(scsi_cmd, dev_specp)				\
		((dev_specp)->cmd_chk[ ((unsigned char)scsi_cmd)>>5 ] &	\
			(1 << ((((unsigned char)scsi_cmd) & 0x1f) -1) ))

#define CMD_SUP(scsi_cmd, dev_specp)				\
		((dev_specp)->cmd_sup[ ((unsigned char)scsi_cmd)>>5 ] &	\
			(1 << ((((unsigned char)scsi_cmd) & 0x1f) -1) ))

struct owner	*sdi_doconfig();	/* configure devices		*/
struct dev_spec *sdi_findspec();	/* find a dev_spec		*/

/*
 * the host adapter minor device number is interpreted as follows:
 *
 *           MAJOR           MINOR      
 *      -------------------------------
 *      |  mmmmmmmm  |  ccc  ttt ll   |
 *      -------------------------------
 *      
 *         m = major number assigned by idinstall
 *	   c = Host Adapter Card number (0-7)
 *         t = target controller ID (0-7)
 *         l = logical unit number (0-3)
 *
 */

extern int sdi_major;

/* These macros take a dev_t as an argument */
#if	(_SYSTEMENV == 4)
#define SC_HAN(dev)	((getminor(dev) >> 5) & 0x07)
#define SC_TCN(dev)	((getminor(dev) >> 2) & 0x07)
#define SC_LUN(dev)	((getminor(dev) & 0x03))
#else
#define SC_HAN(dev)	((minor(dev) >> 5) & 0x07)
#define SC_TCN(dev)	((minor(dev) >> 2) & 0x07)
#define SC_LUN(dev)	((minor(dev) & 0x03))
#endif

#define	SDI_HAN(x)	(((x)->sa_fill >> 3) & 0x07)
#define	SDI_TCN(x)	((x)->sa_fill & 0x07)
#define SDI_LUN(x)	((x)->sa_lun)

#define	SDI_ILLEGAL(c,t,l)    ((c >= sdi_hacnt) || (!sdi_redt(c,t,l)) )

/*
 *	message format types for sdi_errmsg
 */
#define SDI_SFB_ERR		0
#define SDI_CKCON_ERR	1
#define SDI_CKSTAT_ERR	2
#define SDI_DEFAULT_ERR	3
#define NAMESZ	49

#endif /* _IO_TARGET_SDI_H */
