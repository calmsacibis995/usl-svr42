/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_ST01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_ST01_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/target/st01.h	1.4"
#ident	"$Header: $"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/* THESE SHOULD BE DEFINED IN sys/scsi.h	*/
/* Operational codes for group zero commands which are used for control */
#ifndef SS_RDBLKLEN
#define	SS_RDBLKLEN	0X05		/* Read block length limits	   */
#endif
/* Block length limit data */
struct blklen {
	unsigned res1:8;	   /* Reserved			*/
	unsigned max_blen:24;	   /* maximum block length	*/
	unsigned min_blen:16;	   /* minimum block length	*/
};

#define RDBLKLEN_SZ       6 
#define RDBLKLEN_AD(x)     ((char *)(x))
/* END sys/scsi.h DEFINES	*/

/*
 * the minor device number is interpreted as follows:
 * 
 *     bits:
 *	  7   6   5   4   3   2   1   0
 * 	+---+-----------+-------+---+---+
 * 	| 0 |    unit   | 0   0 | r | n |
 * 	+---+-----------+-------+---+---+
 *
 *     codes:
 *	unit  - sequential device no. (0 - 7)
 *	r     - retension on open (0 or 1, 1 = True)
 *	n     - no rewind at close (0 or 1, 1 = True)
 */
#define UNIT(x)		((getminor(x) >> 4) & 0x07)
#define NOREWIND(x)		(getminor(x) & 0x01)
#define RETENSION_ON_OPEN(x)	(getminor(x) & 0x02)

#define	IMMEDIATE	0x01		/* Status returned immediately	*/
#define	VARIABLE	0x00		/* Variable block mode		*/
#define	FIXED		0x01		/* Fixed block mode		*/
#define	LOAD		0x01		/* Medium to be loaded		*/
#define	UNLOAD		0x00		/* Medium to be unloaded	*/
#define	LOCK		0x01		/* Lock medium in drive		*/
#define	UNLOCK		0x00		/* Unlock medium in drive	*/
#define	RETENSION	0x03		/* Retension (and load) tape	*/
#define BLOCKS		0x00		/* Space blocks			*/
#define FILEMARKS	0x01		/* Space file marks		*/
#define SEQFLMRKS	0x02		/* Space sequential file marks	*/
#define EORD		0x03		/* Space to end-of-recorded-data */
#define SHORT		0x00		/* Short erase (not supported)	*/
#define	LONG		0x01		/* Long erase			*/
#define BUFFERED	0x01		/* Buffered mode		*/
#define UNBUFFERED	0x00		/* Unbuffered mode		*/
#define DEFAULT		0x00		/* Use default value		*/

#define ONE_SEC		1000		/* # of msec in one second	*/
#define ONE_MIN		60000		/* # of msec in one minute	*/
#define JTIME		30 * ONE_SEC	/* 30 sec for an I/O job	*/
#define MAX_RETRY	2		/* Max number of retries	*/
#define	ST01_MAXSIZE	60*1024		/* Max ST01 job size		*/

/*
 * Mode data structure
 */
struct mode {
	unsigned md_len   :8;		/* Sense data length		*/
	unsigned md_media :8;		/* Medium type			*/
	unsigned md_speed :4;		/* Tape speed			*/
	unsigned md_bm    :3;		/* Buffering mode		*/
	unsigned md_wp    :1;		/* Write protected		*/
	unsigned md_bdl   :8;		/* Block descriptor length	*/
	unsigned md_dens  :8;		/* Density code			*/
	unsigned md_nblks :24;		/* Number of blocks		*/
	unsigned md_res   :8;		/* Reserved field		*/
	unsigned md_bsize :24;		/* Block size			*/
};

/*
 * Job structure
 */
struct job {
	struct job     *j_next;	   	/* Points to next job on list	*/
	struct job     *j_prev;	   	/* Points to prev job on list	*/
	struct job     *j_priv;	   	/* private pointer for dynamic  */
					/* alloc routines DON'T USE IT  */
	struct sb      *j_sb;		/* SCSI block for this job	*/
	struct buf     *j_bp;		/* Pointer to buffer header	*/
	struct tape    *j_tp;		/* Device to be accessed	*/
	time_t 		j_time;		/* Time limit for job		*/
	union sc {
		struct scs  ss;		/* Group 0,6 command - 6 bytes	*/
		struct scm  sm;		/* Group 1,7 command - 10 bytes */
	} j_cmd;
};

/*
 * Device information structure
 */
struct tape {
	struct scsi_ad	t_addr;		/* SCSI address			*/
	unsigned  	t_state;	/* Operational state flags	*/ 
	unsigned	t_lastop;	/* Last command completed	*/ 
	unsigned  	t_bsize;	/* Block size			*/ 
	unsigned	t_fltcnt;	/* Retry count (for recovery)	*/ 
	struct job     *t_fltjob;	/* Job associated with fault	*/
	struct sb      *t_fltreq;	/* SCSI block for Request Sense */
	struct sb      *t_fltres;	/* SCSI block for resume job	*/
	struct sb      *t_fltrst;	/* SCSI block for reset job	*/
	struct scs	t_fltcmd;	/* Request Sense command	*/
	struct sense	t_sense;	/* Request Sense data		*/
	struct mode	t_mode;		/* Mode Sense/Select data	*/
	struct blklen	t_blklen;	/* Tape block length limit	*/
	struct ident	t_ident;	/* Inquiry ident data		*/
	struct dev_spec *t_spec;
};

/* Values of t_state */
#define	T_OPENED	0x01		/* Tape is open			*/
#define	T_WRITTEN	0x02		/* Tape has been written 	*/
#define	T_SUSPEND	0x04		/* Tape LU Q suspended by HA	*/
#define	T_FILEMARK	0x08		/* File mark encountered	*/
#define	T_TAPEEND	0x10		/* End of media 		*/
#define	T_PARMS		0x20		/* Tape parms set and valid	*/
#define	T_READ		0x40		/* Tape has been read	 	*/
#define	T_RESERVED	0x80		/* Tape has been reserved 	*/
#define	T_OPENING	0x100		/* Tape is being opened		*/

extern struct dev_spec *st01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg ST01_dev_cfg[];	/* configurable devices struct	*/
extern int ST01_dev_cfg_size;		/* number of dev_cfg entries	*/

/*
 *	Debug ioctls -- specific to this driver
*/
#define	T_ERRMSGON	(T_BASE | 040)	/* System error message ON	*/
#define	T_ERRMSGOFF	(T_BASE | 041)	/* System error message OFF	*/

#endif /* _IO_TARGET_ST01_H */
