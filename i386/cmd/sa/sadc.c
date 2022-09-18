/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sa:i386/cmd/sa/sadc.c	1.11"
#ident  "$Header: $"

/***************************************************************************
 * Command: sadc
 *
 * Inheritable Privileges: P_DEV
 *       Fixed Privileges: None
 *
 * Notes:
 *
 *	sadc -	writes system activity binary data from /dev/kmem to a
 *		file or stdout.
 *	Usage:	sadc [t n] [file]
 *		if t and n are not specified, it writes a dummy record to 
 *		data file. This usage is particularly used at system booting.
 *		If t and n are specified, it writes system data n times to
 *		file every t seconds. In both cases, if file is not specified, 
 *		it writes data to stdout.
 *
 ***************************************************************************/


#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/var.h>
#include <sys/iobuf.h>
#include <sys/stat.h>
#include <sys/elog.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/vtoc.h>
#include <sys/immu.h>
#include <sys/ksym.h>

#include <sys/sdi_edt.h>
#include <sys/altsctr.h>
#include <sys/scsi.h>
#include <sys/sdi.h>
#include <sys/sd01.h> 

#include <sys/sysinfo.h>
#include <sys/fs/rf_acct.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/flock.h>
#include <sys/tuneable.h>
#include <sys/ksym.h>
#include <errno.h>
#include <sys/fsinode.h>
#include "sa.h"

#define MAX_LUN	8
#define NDRV    2       /* maximum number of drives on 386 (2 per 546 board) */
#define NBRD    1       /* maximum number of 546 boards on 386 */

int 	sd01_diskcnt = 0;
static 	struct	disk	**sd01_dp;
static	struct 	disk 	*dsd01;
static	struct	stat 	ubuf,syb;
static	struct	var 	tbl;
static	struct	syserr 	err;

static	struct 	fshead	s5fshead;
static	struct 	fshead	sfs_fshead;
static	struct 	fshead	vxfs_fshead;
static	struct	sa 	d;
struct	flckinfo flckinfo;
struct	tune 	tune;
static	char	*flnm = "/tmp/sa.adrfl";
static	int	fa = 0;
static	int	tblmap[SINFO];
static	int	recsz;
extern	time_t	time();
static	int	f,i,k;

extern	void	*malloc();
extern	gid_t	getegid();
void	perrexit();
long	lseek();


#define GETINFO(fd,name,buf,buflen)	ginfo(fd,name,buf,buflen,MIOC_READKSYM)
#define GETIINFO(fd,name,buf,buflen)	ginfo(fd,name,buf,buflen,MIOC_IREADKSYM)

/*
 * Procedure:     main
 *
 * Restrictions:
 *                open(2):none
 *                read(2):none
 *                write(2):none
 * Notes:
*/

main(argc, argv)
char **argv;
int	argc;
{
	int	ct;
	unsigned ti;
	int	fp;
	long 	min;
	mode_t 	u;
	struct 	stat 	buf;
	char 	*fname;
	char	*ptr;
	register struct disk *dp;

	ct = argc >= 3? atoi(argv[2]): 0;
	min = time((long *) 0);
	ti = argc >= 3? atoi(argv[1]): 0;

	/*	open /dev/kmem	*/
	if((f = open("/dev/kmem", 0)) == -1)
		perrexit("sadc cannot open /dev/kmem");

 	/*    get number of scsi drives for 386	*/
	if (GETINFO(f,SD01,&sd01_diskcnt,sizeof(sd01_diskcnt)) == 0) {
		sd01_dp = (struct disk **) malloc(sizeof(struct disk *) * sd01_diskcnt);
		if (sd01_dp == NULL)
			perrexit("sadc can't malloc");

		dsd01 = (struct disk *) malloc(sizeof(struct disk) * sd01_diskcnt);
		if (dsd01 == NULL)
			perrexit("sadc can't malloc");
	}

	/*	construct tblmap and compute record size	*/
	/* only one disk  as far as sadc is concerned (i.e. SINFO == 1)*/
	tblmap[0] = sd01_diskcnt;
	recsz += tblmap[0];
	recsz = sizeof (struct sa) - sizeof d.devio + recsz * sizeof d.devio[0];

	if (argc == 3 || argc == 1){
		/*	no data file is specified, direct data to stdout */
		fp = 1;
		/*	write header record	*/
		write(fp,tblmap,sizeof tblmap);
	}
	else {
		fname = argc==2? argv[1]: argv[3];
		/*
		 *	check if the data file is there
		 *	check if data file is too old
		 */
		if ((stat(fname,&buf) == -1) || ((min - buf.st_mtime) > 86400))
			goto credfl;
		if ((fp = open(fname,2)) == -1){
credfl:
			/*
			 *	data file does not exist:
			 *	create one and write the header record.	
			 */
			if ((fp = creat(fname,00644)) == -1)
				perrexit("sadc");
			close(fp);
			fp = open (fname,2);
			lseek(fp,0L,0);
			write (fp,tblmap,sizeof tblmap);
		}
		else {
			/*
			 *	data file exist: position the write pointer 
			 *	to the last good record.  
			 */
			lseek(fp,-(long)((buf.st_size - sizeof tblmap) % recsz),2);
		}
	}

	/*	if n =0 , write the additional dummy record	*/
	if (ct == 0){
		for (i=0;i<4;i++)
			d.si.cpu[i] = -300;
		d.ts = min;
		write(fp,&d,(unsigned int)recsz);
	}

	/*	get memory for tables	*/
	if(GETINFO(f,V,&tbl,sizeof(tbl)) != 0)
		perrexit("sadc: cannot get V");

	for(;;) {
		/*	read data from /dev/kmem to structure d	*/

		if(GETINFO(f,SYSINFO,&d.si,sizeof(d.si)) != 0)
			perrexit("sadc: cannot get SINFO");

		/*	Distributed Unix info	*/
		(void) GETINFO(f,RCINFO,&d.rfc,sizeof(d.rfc));
		(void) GETINFO(f,RF_SRV,&d.rf_srv,sizeof(d.rf_srv));
		(void) GETINFO(f,RFSR,&d.rfs_in,sizeof(d.rfs_in));
		(void) GETINFO(f,RFCL,&d.rfs_out,sizeof(d.rfs_out));
		(void) GETINFO(f,MINSERVE,&d.minserve,sizeof(d.minserve));
		(void) GETINFO(f,MAXSERVE,&d.maxserve,sizeof(d.maxserve));

		/*	Client Caching info	*/
		(void) GETINFO(f,RFC_INFO,&d.rfc,sizeof(d.rfc));

		/*	Kernel Memory Allocation info  */
		(void) GETINFO(f,KMEMINFO,&d.km,sizeof(d.km));

		/*	virtual memory	*/
		(void) GETINFO(f,MINFO,&d.mi,sizeof(d.mi));
		(void) GETINFO(f,VMINFO,&d.vmi,sizeof(d.vmi));

		/*	translate from clicks to disk blocks */
		d.si.bswapin = ctob(d.si.bswapin) / NBPSCTR; /* BSIZE to NBPSCTR */
		d.si.bswapout = ctob(d.si.bswapout) / NBPSCTR; 


                if (sd01_diskcnt > 0) {
			if(GETINFO(f,SD01_D,sd01_dp, sizeof(struct disk *) * sd01_diskcnt) != 0) {
				perror("sadc can't read SD01_D");
				exit(2);
			}
			for (i=0; i < sd01_diskcnt; i++) {
				lseek(f, *(sd01_dp + i), 0);
				if (read(f, dsd01 + i, sizeof(struct disk)) == -1) {
					perror("sadc can't read disk structure");
				}
			}
		}

		/*	compute size of system tables	*/
		d.mszinode = 0;
		d.cszinode = 0;
		d.szinode = 0;
		d.inodeovf = 0;

		if (GETINFO(f,S5FSHEAD,&s5fshead,sizeof(struct fshead)) == 0) {
			d.mszinode += s5fshead.f_max;
			d.cszinode += s5fshead.f_curr;
			d.szinode += s5fshead.f_inuse;
			d.inodeovf += s5fshead.f_fail;

			d.s5mszinode = s5fshead.f_max;
			d.s5cszinode = s5fshead.f_curr;
			d.s5szinode = s5fshead.f_inuse;
			d.s5inodeovf = s5fshead.f_fail;
		}

		if (GETINFO(f,SFS_FSHEAD,&sfs_fshead,sizeof(struct fshead)) == 0) {
			d.mszinode += sfs_fshead.f_max;
			d.cszinode += sfs_fshead.f_curr;
			d.szinode += sfs_fshead.f_inuse;
			d.inodeovf += sfs_fshead.f_fail;

			d.sfsmszinode = sfs_fshead.f_max;
			d.sfscszinode = sfs_fshead.f_curr;
			d.sfsszinode = sfs_fshead.f_inuse;
			d.sfsinodeovf = sfs_fshead.f_fail;
		}
		if (GETINFO(f,VXFS_FSHEAD,&vxfs_fshead,sizeof(struct fshead)) == 0) {
			d.mszinode += vxfs_fshead.f_max;
			d.cszinode += vxfs_fshead.f_curr;
			d.szinode += vxfs_fshead.f_inuse;
			d.inodeovf += vxfs_fshead.f_fail;

			d.vxfsmszinode = vxfs_fshead.f_max;
			d.vxfscszinode = vxfs_fshead.f_curr;
			d.vxfsszinode = vxfs_fshead.f_inuse;
			d.vxfsinodeovf = vxfs_fshead.f_fail;
		}

		(void) GETINFO(f,FILCT,&d.szfile,sizeof(d.szfile));

		(void) GETINFO(f,PRO,&d.szproc,sizeof(d.szproc));

		(void) GETINFO(f,FLCK,&flckinfo,sizeof(flckinfo));

		d.szlckr = flckinfo.reccnt;

		(void) GETINFO(f,TUNE,&tune,sizeof(tune));

		/* 	record maximum sizes of system tables */
		d.mszlckr = tune.t_flckrec;
		d.mszfile = 0;
		d.mszproc = tbl.v_proc;

		/*	record system tables overflows	*/
		if(GETINFO(f,SERR,&err, sizeof(err)) != 0)
			perrexit("sadc: cannot read SERR");
		d.procovf = err.procovf;
		d.fileovf = 0;
		
		/*	get time stamp	*/
		d.ts = time ((long *) 0);


                 for (dp = dsd01, k = 0; k < sd01_diskcnt; k++) {
                        d.devio[k][IO_OPS] =
				dp->dk_stat.tnrreq +
				dp->dk_stat.tnwreq;
                        d.devio[k][IO_BCNT] = dp->dk_stat.io_bcnt;
                        d.devio[k][IO_ACT] = dp->dk_stat.io_act;
                        d.devio[k][IO_RESP] = dp->dk_stat.io_resp;
			dp++;
                }

		/*	write data to data file from structure d	*/
		write(fp,&d,(unsigned int)recsz);
		if(--ct > 0)
			sleep(ti);
		else {
			close(fp);
			exit(0);

		}
	}
}


/*
 * Procedure:     perrexit
 *
 * Restrictions:
 *                 perror: none
*/

void
perrexit(s)
char *s;
{
	perror(s);
	exit(2);
}


/*
 * ginfo: read info from kernel using ioctl on /dev/kmem
 * Restrictions:
 *		ioctl(2) none
 */
int
ginfo(fd,name,buf,buflen,cmd)
int fd,cmd;
size_t buflen;
void *buf;
char *name;
{
	struct mioc_rksym rks;
	rks.mirk_symname = name;
	rks.mirk_buf = buf;
	rks.mirk_buflen = buflen;
	return(ioctl(fd,cmd,&rks));
}
