/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_DCD_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DCD_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/dcd.h	1.8"
#ident	"$Header: $"


#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif	/* _UTIL_TYPES_H */

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define DCD_HA 7	/* the number of the dcd Host Adapter */

typedef struct dcdblk {
	struct xsb *sb;
	struct dcdblk *av_forw;
	char *dcd_priv;  	/* private ptr for dyn alloc routines DON'T USE*/
	int b_flags;		/* holders buffer flags */
	struct   proc	*b_procp;
	struct drq_entry *drq_srt;
	struct drq_entry *drq_end;
} dcdblk_t;


struct blk_desc{
	unchar bd_dencode;
	unchar bd_nblks1;
	unchar bd_nblks2;
	unchar bd_nblks3;
	unchar bd_res;
	unchar bd_blen1;
	unchar bd_blen2;
	unchar bd_blen3;
};


extern int dcd_init();
extern void dcd_intr();
struct hbadata * dcd_getblk();
extern void dcd_xlat();
extern void dcd_getinfo();
extern long dcd_freeblk();
extern void dcd_getdev();
extern long dcd_send();
extern long dcd_icmd();
extern int dcd_open();
extern int dcd_close();
extern int dcd_ioctl();

#define SCM_RAD(x) ((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */

/*****************************************************************************
 *
 *	The reminder of this file is taken from io/sdo01/sd01.h
 *	Including all of sd01.h doesn't seem to be the right thing
 *	to do, but neither is duplicating this stuff here.
 ****************************************************************************/

/*
 * Define for Reassign Blocks defect list size.
 */

#define RABLKSSZ	8	/* Defect list in bytes		*/

/*
 * Define for Read Capacity data size.
 */

#define RDCAPSZ 	8	/* Length of data area		*/

/*
 * Defines for Mode sense data command.
 */

#define FPGSZ 		0x1C	/* Length of page 3 data area	*/
#define RPGSZ 		0x18	/* Length of page 4 data area	*/
#define	SENSE_PLH_SZ	4	/* Length of page header	*/

/*  
 * Define the Read Capacity Data Header format.
 */

typedef struct capacity {
	int cd_addr;		/* Logical Block Address	*/
	int cd_len;		/* Block Length			*/
} CAPACITY_T;

/*
 *  Define the Mode Sense Parameter List Header format.
 */

typedef struct sense_plh {
	unchar 	plh_len;	/* Data Length			*/
	unchar 	plh_type;	/* Medium Type			*/
	uint  	plh_res : 7;	/* Reserved			*/
	uint  	plh_wp : 1;	/* Write Protect		*/
	unchar 	plh_bdl;	/* Block Descriptor Length	*/
} SENSE_PLH_T;

/*  
 * Define the Direct Access Device Format Parameter Page format.
 */

typedef struct dadf {
	int pg_pc	: 6;	/* Page Code			*/
	int pg_res1	: 2;	/* Reserved			*/
	unchar pg_len;		/* Page Length			*/
	int pg_trk_z	: 16;	/* Tracks per Zone		*/
	int pg_asec_z	: 16;	/* Alternate Sectors per Zone	*/
	int pg_atrk_z	: 16;	/* Alternate Tracks per Zone	*/
	int pg_atrk_v	: 16;	/* Alternate Tracks per Volume	*/
	int pg_sec_t	: 16;	/* Sectors per Track		*/
	int pg_bytes_s	: 16;	/* Bytes per Physical Sector	*/
	int pg_intl	: 16;	/* Interleave Field		*/
	int pg_trkskew	: 16;	/* Track Skew Factor		*/
	int pg_cylskew	: 16;	/* Cylinder Skew Factor		*/
	int pg_res2	: 27;	/* Reserved			*/
	int pg_ins	: 1;	/* Inhibit Save			*/
	int pg_surf	: 1;	/* Allocate Surface Sectors	*/
	int pg_rmb	: 1;	/* Removable			*/
	int pg_hsec	: 1;	/* Hard Sector Formatting	*/
	int pg_ssec	: 1;	/* Soft Sector Formatting	*/
} DADF_T;

/*  
 * Define the Rigid Disk Drive Geometry Parameter Page format.
 */

typedef struct rddg {
	int pg_pc	: 6;	/* Page Code			 */
	int pg_res1	: 2;	/* Reserved			 */
	unchar pg_len;		/* Page Length			 */
	int pg_cylu	: 16;	/* Number of Cylinders (Upper)	 */
	unchar pg_cyll;		/* Number of Cylinders (Lower)	 */
	unchar pg_head;		/* Number of Heads		 */
	int pg_wrpcompu	: 16;	/* Write Precompensation (Upper) */
	unchar pg_wrpcompl;	/* Write Precompensation (Lower) */
	int pg_redwrcur	: 24;	/* Reduced Write Current	 */
	int pg_drstep	: 16;	/* Drive Step Rate		 */
	int pg_landu	: 16;	/* Landing Zone Cylinder (Upper) */
	unchar pg_landl;	/* Landing Zone Cylinder (Lower) */
	int pg_res2	: 24;	/* Reserved			 */
} RDDG_T;

struct mdata{
	SENSE_PLH_T plh;
	struct blk_desc blk_desc;
	union {
		struct pdinfo pg0;
		DADF_T	pg3;
		RDDG_T  pg4;
	}pdata;
};

struct scs_format{
	unsigned char fmt_op;		/* Opcode              */
	unsigned char fmt_defectlist : 3;
	unsigned char fmt_cmplst : 1;
	unsigned char fmt_fmtdata : 1;
	unsigned char fmt_lun : 5;
	unsigned char reserv;
	unsigned char fmt_intlmsb;
	unsigned char fmt_intllsb;
	unsigned char fmt_cont;		/* Control field       */
};

#define	SDI_HAN(x)	(((x)->sa_fill >> 3) & 0x07)
#define	SDI_TCN(x)	((x)->sa_fill & 0x07)
#define SDI_LUN(x)	((x)->sa_lun)

#define	SC_CONTROL(x)	dcd_ctoi[((x>>5) & 0x7)]/* C# from minor number */
#define	SC_TARGET(x)	((x >> 2) & 0x7)	/* TC# from minor number */
/*
#define SC_LUN(x)	((x) & 0x03)
*/

#define	SC_ILLEGAL(c,t)	(c < 0  ||  dcd_ftoi[ c*MAX_TCS + t] < 0 )
#define MAX_CMDSZ	12
#define FILL2DEVT(sa)	(makedev((sa).sa_major, ((sa).sa_fill << 2) & (sa).sa_lun) )

#endif /* _IO_HBA_DCD_H */
