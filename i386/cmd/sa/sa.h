/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sa:i386/cmd/sa/sa.h	1.9"
#ident  "$Header: $"
/*	sa.h contains struct sa and defines variable used 
 *		in sadc.c and sar.c.
 *	For i386 machines,
 *		- winchester disk driver status system name (wnstat).
 *		- symbol name for sysinfo.
 *		- symbol names for system tables: inode, file,
 *			text and process.
 *		- symbol name for var.
 *		- symbol name for wn_cnt.
 *		- symbol name for system error table:
 *
 */
 
#define SINFO   	1

#define SD01		"sd01_diskcnt"
#define SYSINFO		"sysinfo"
#define INO     	"inode"
#define FILCT		"filecnt"
#define TXT     	"text"
#define PRO     	"nproc"
#define FLCK    	"flckinfo"
#define V       	"v"
#define SERR    	"syserr"
#define DINFO   	"rf_srv_info"
#define MINSERVE        "minserve"
#define MAXSERVE        "maxserve"
#define RCINFO        	"rfc_info"
#define	NINODE		"ninode"
#define	RF_SRV		"rf_srv_info"
#define	RFCL		"rfcl_fsinfo"
#define	RFSR		"rfsr_fsinfo"
#define	RFC_INFO	"rfs_info"
#define	KMEMINFO	"kmeminfo"
#define	VMINFO		"vminfo"
#define BRD    		"i214bdd"
#define TUNE		"tune"
#define SD01_D		"Sd01_dp"
#define MINFO		"minfo"
#define S5FSHEAD	"s5fshead"
#define SFS_FSHEAD	"sfs_fshead"
#define VXFS_FSHEAD	"vxfs_fshead"

#define TBLSIZ		SINFO

#define BLKPERPG 	4
#define NDEVS 60  	/* 2 ESDI/St506 hard disks, tape drive and floppy drive */
		  	/* plus lots scsi drives approx. 50 */

/*	iotbsz, devnm tables define the initial value of number of drives
	and name of devices.
*/
static	int	iotbsz[SINFO] = {
	0
};

 
/*	structure sa defines the data structure of system activity data file
*/
 
struct sa {
	struct	sysinfo si;	/* defined in /usr/include/sys/sysinfo.h  */
	struct	minfo	mi;	/* defined in /usr/include/sys/sysinfo.h */
	struct	vminfo	vmi;	/* defined in /usr/include/sys/sysinfo.h */
	rf_srv_info_t	rf_srv;	/* defined in /usr/include/sys/fs/rf_acct.h */
	fsinfo_t	rfs_in;
	fsinfo_t	rfs_out; /* defined in /usr/include/sys/sysinfo.h */
	rfc_info_t	rfc;     /* defined in /usr/include/sys/fs/rf_acct.h */

	struct  kmeminfo km;	/* defined in /usr/include/sys/sysinfo.h */
	int	minserve;
	int	maxserve;
	int	szinode;	/* in use size of inode table  */
	uint	szfile;		/* in use size of file table  */
	uint	szproc;		/* in use size of proc table  */
	int	szlckf;		/* in use size of file record header table */
	int	szlckr;		/* in use size of file record lock table */
	int	cszinode;	/* current size of inode table */
	int	mszinode;	/* maximum size of inode table  */
	int	mszfile;	/* maximum size of file table  */
	int	mszproc;	/* maximum size of proc table  */
	int	mszlckf;	/* maximum size of file record header table */
	int	mszlckr;	/* maximum size of file record lock table */
	long	inodeovf;	/* cumulative overflows of inode table
					since boot  */
	long	fileovf;	/* cumulative overflows of file table
					since boot  */
	long	procovf;	/* cumulative overflows of proc table
					since boot  */
	int	s5szinode;	/* inuse size of s5 inode table */
	int	s5cszinode;	/* current size of s5 inode table */
	int	s5mszinode;	/* maximum size of s5 inode table */
	int	s5inodeovf;	/* cumulative overflows of s5 inode table */
	int	sfsszinode;	/* inuse size of ufs/sfs inode table */
	int	sfscszinode;	/* current size of ufs/sfs inode table */
	int	sfsmszinode;	/* maximum size of ufs/sfs inode table */
	int	sfsinodeovf;	/* cumulative overflows of ufs/sfs inode table*/
	int	vxfsszinode;	/* inuse size of vxfs inode table */
	int	vxfscszinode;	/* current size of vxfs inode table */
	int	vxfsmszinode;	/* maximum size of vxfs inode table */
	int	vxfsinodeovf;	/* cumulative overflows of vxfs inode table*/
	time_t	ts;		/* time stamp  */
	int apstate;
	ulong	devio[NDEVS][4]; /* device unit information  */

#define	IO_OPS	0  /* number of I /O requests since boot  */
#define	IO_BCNT	1  /* number of blocks transferred since boot */
#define	IO_ACT	2  /* cumulative time in ticks when drive is active  */
#define	IO_RESP	3  /* cumulative I/O response time in ticks since boot  */
};
extern struct sa sa;

#define BPB_UTIL  0		  /* no Co-processor utilize */
