/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sa:common/cmd/sa/sar.c	1.56.2.7"
#ident  "$Header: $"

/***************************************************************************
 * Command : sar
 * Inheritable Privileges : P_DEV
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************/
/*
	sar.c - It generates a report either
		from an input data file or
		by invoking sadc to read system activity counters 
		at the specified intervals.
	usage: sar [-ubdycwaqvtmpgrkxACDS] [-o file] t [n]    or
	       sar [-ubdycwaqvtmpgrkxACDS][-s hh:mm][-e hh:mm][-i ss][-f file]
*/
#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>
#include <time.h>
#include <priv.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/fcntl.h>
#include <sys/flock.h>
#include <sys/fs/rf_acct.h>
#include "sa.h"

struct	sa	nx,ox,ax,bx;
struct	tm	*localtime();
static	struct	tm *curt, args, arge;
static	struct	utsname name;
static	int	sflg, eflg, iflg, oflg, fflg;
static	int	dflg = 0;
static	float	Isyscall, Isysread, Isyswrite, Isysexec, Ireadch, Iwritech;
static	float	Osyscall, Osysread, Osyswrite, Osysexec, Oreadch, Owritech;
static	float 	Lsyscall, Lsysread, Lsyswrite, Lsysexec, Lreadch, Lwritech;

static	int	realtime;
static	int	passno;
static	int	t = 0;
static	int	n = 0;
int	lines = 0;

int 	recsz, tblmap[TBLSIZ];	/* for the u370 tblmap is a kludge - not really used */

static	int	tabflg;
static	char	options[30],fopt[30];
static	char	cc;
float	tdiff;
float	sec_diff, totsec_diff=0.0;	/* number of times one second boundary occurred in clock.c for interval and total sample*/
static	time_t	ts, te;			 	/* time interval start and end */
static	float	stime, etime, isec;
static	int	fin, fout;
static	pid_t	childid;
static	int	pipedes[2];
static	char	arg1[10], arg2[10];
int	strlen();
void	prttim(), prthdg(), prtopt(), prtavg();
void	pillarg(), perrexit(), pmsgexit(), prpass();

extern	char	*strcpy(),*strncat(), *strncpy(), *strchr();
extern  int optind;
extern  char *optarg;

/*
*Procedure:     main
*
* Restrictions:
                 open(2): none
                 read(2): none
                 write(2): none
*/
main (argc,argv)
char	**argv;
int	argc;
{
	char    flnm[50], ofile[50];
	char	i, ccc;
	long    temp;
	int	jj=0;
/*
	long	lseek();
	extern time_t time();
*/

/*      process options with arguments and pack options 
	without arguments  */
	while ((i = getopt(argc,argv,"uybdvtcwaqmpgrkxACDSo:s:e:i:f:")) != EOF)
		switch(ccc = i){
		case 'D':
			dflg++;
			break;
		case 'o':
			oflg++;
			sprintf(ofile,"%s",optarg);
			break;
		case 's':
			if (sscanf(optarg,"%d:%d:%d",
			&args.tm_hour, &args.tm_min, &args.tm_sec) < 1)
				pillarg();
			else {
				sflg++,
				stime = args.tm_hour*3600.0 +
					args.tm_min*60.0 +
					args.tm_sec;
			}
			break;
		case 'e':
			if(sscanf(optarg,"%d:%d:%d",
			&arge.tm_hour, &arge.tm_min, &arge.tm_sec) < 1)
				pillarg();
			else {
				eflg++;
				etime = arge.tm_hour*3600.0 +
					arge.tm_min*60.0 +
					arge.tm_sec;
			}
			break;
		case 'i':
			if(sscanf(optarg,"%f",&isec) < 1)
				pillarg();
			else{
			if (isec > 0.0)
				iflg++;
			}
			break;
		case 'f':
			fflg++;
			sprintf(flnm,"%s",optarg);
			break;
		case '?':
			fprintf(stderr,"usage: sar [-ubdycwaqvtmpgrkxACDS][-o file] t [n]\n");
			fprintf(stderr,"       sar [-ubdycwaqvtmpgrkxACDS][-s hh:mm][-e hh:mm][-i ss][-f file]\n");
			exit(2);
			break;
		case 'u':
			strncat (options,&ccc,1);
			strncat (options,"U",1);
			break;
		default:
			strncat (options,&ccc,1);
			break;
		}

	/*   Are starting and ending times consistent?  */
	if ((sflg) && (eflg) && (etime <= stime))
		pmsgexit("etime <= stime");

	/*   Determine if t and n arguments are given,
	and whether to run in real time or from a file   */
	switch(argc - optind) {
	case 0:		/*   Get input data from file   */
		if(fflg == 0) {
			temp = time((long *) 0);
			curt = localtime(&temp);
			sprintf(flnm,"/var/adm/sa/sa%.2d", curt->tm_mday);
		}
		if((fin = open(flnm, 0)) == -1) {
			fprintf(stderr, "sar:Can't open %s\n", flnm);
			exit(1);
		}
		break;
	case 1:		/*   Real time data; one cycle   */
		realtime++;
		t = atoi(argv[optind]);
		n = 2;
		break;
	case 2:		/*   Real time data; specified cycles   */
	default:
		realtime++;
		t = atoi(argv[optind]);
		n = 1 + atoi(argv[optind+1]);
		break;
	}

	/*	"u" is default option to display cpu utilization   */
	if(strlen(options) == 0)
		strcpy(options, "uU");
	/*    'A' means all data options   */

	if(strchr(options, 'A') != NULL) {
		strcpy(options, "uUdqbwcayvtmpgrkxCS");
		dflg++;
	}
	else if ( (dflg)
	     && (strchr(options,'u') == NULL)
	     && (strchr(options,'U') == NULL)
	     && (strchr(options,'b') == NULL)
	     && (strchr(options,'c') == NULL) ) strcat(options,"uU");

	if(realtime) {
	/*	Get input data from sadc via pipe   */
		if((t <= 0) || (n < 2))
			pmsgexit("args t & n <= 0");
		sprintf(arg1,"%d", t);
		sprintf(arg2,"%d", n);
		if (pipe(pipedes) == -1)
			perrexit();
		if ((childid = fork()) == 0){	/*  child:   */
			close(1);       /*  shift pipedes[write] to stdout  */
			dup(pipedes[1]);
			if (execlp ("/usr/lib/sa/sadc","/usr/lib/sa/sadc",arg1,arg2,(char*)0) == -1)
				perrexit();
		}else if (childid == -1) {
			pmsgexit("Could not fork to exec sadc");
		}		/*   parent:   */
		fin = pipedes[0];
		close(pipedes[1]);	/*   Close unused output   */
	}

	if(oflg) {
		if(strcmp(ofile, flnm) == 0)
			pmsgexit("ofile same as ffile");
		fout = creat(ofile, 00644);
	}

/*      read the header record and compute record size */
	if (read(fin, tblmap, sizeof tblmap) < 0)
		perrexit ();
	for (i = 0; i < SINFO; i++)
		recsz += tblmap[i];
	recsz = sizeof (struct sa) - sizeof nx.devio +
		recsz * sizeof nx.devio[0];

	if(oflg)	write(fout, tblmap, sizeof tblmap);

	if(realtime) {
		/*   Make single pass, processing all options   */
		strcpy(fopt, options);
		passno++;
		prpass();
		kill(childid, 2);
		wait((int *) 0);
	}
	else {
		/*   Make multiple passes, one for each option   */
		while(strlen(strncpy(fopt,&options[jj++],1)) > 0) {
			lseek(fin, (long)(sizeof tblmap), 0);
			passno++;
			prpass();
		}
	}
	exit(0);
}

/*
*Procedure:     prpass
*
* Restrictions:
                 localtime:none
                 write(2):none
                 printf:none
*/
/*****************************************************/

/*	Read records from input, classify, and decide on printing   */
void
prpass(){
	int recno=0;
	float tnext=0;
	float trec;

	if(sflg)	tnext = stime;
	while(read(fin, &nx, (unsigned)recsz) > 0) {
		curt = localtime(&nx.ts);
		trec =    curt->tm_hour * 3600.0
			+ curt->tm_min * 60.0
			+ curt->tm_sec;
		if((recno == 0) && (trec < stime))
			continue;
		if((eflg) && (trec > etime))
			break;
		if((oflg) && (passno == 1))
			write(fout, &nx, (unsigned)recsz);
		if(recno == 0) {
			if(passno == 1) {
				uname(&name);
				printf("\n%s %s %s %s %s    %.2d/%.2d/%.2d\n",
					name.sysname,
					name.nodename,
					name.release,
					name.version,
					name.machine,
					curt->tm_mon + 1,
					curt->tm_mday,
					curt->tm_year);
			}
			prthdg();
			recno = 1;
			if((iflg) && (tnext == 0))
				tnext = trec;
		}
		if ((nx.si.cpu[0] + nx.si.cpu[1] + nx.si.cpu[2] + nx.si.cpu[3]) < 0) {
		/*  This dummy record signifies system restart.
		 *  New initial values of counters follow in next record 
		 *  prevent printing if this is for option 'U' and no other
		 *  processor exists
		 */
		   	if (!((fopt[0] == 'U') && (!BPB_UTIL) && (!realtime))) {
				prttim();
				printf("\tunix restarts\n");
				recno = 1;
				continue;
		   	}
		}
		if((iflg) && (trec < tnext))
			continue;
		if(recno++ > 1) {
			ts = ox.si.cpu[0] + ox.si.cpu[1] + ox.si.cpu[2] + ox.si.cpu[3];
			te = nx.si.cpu[0] + nx.si.cpu[1] + nx.si.cpu[2] + nx.si.cpu[3];
			tdiff = (float) (te - ts);
			sec_diff = (float) (te/HZ - ts/HZ);
			if (nx.apstate) {
				tdiff = tdiff/2;
				sec_diff = sec_diff/2;
			}
			if(tdiff <= 0) 
				continue;
			
			prtopt();	/*  Print a line of data  */
			if (passno == 1) {
				totsec_diff += sec_diff;
				lines++;
			}
		}
		ox = nx;		/*  Age the data	*/
		if(isec > 0)
			while(tnext <= trec)
				tnext += isec;
	}
	if(lines > 1) {
		tabflg = 1;
		prtavg();
	}
	ax = bx;		/*  Zero out the accumulators   */
}

/*
*Procedure:     prttim
*
* Restrictions:
                 localtime:none
                 printf:none
*/
/************************************************************/

/*      print time label routine	*/
void
prttim()
{
	curt = localtime(&nx.ts);
	printf("%.2d:%.2d:%.2d",
		curt->tm_hour,
		curt->tm_min,
		curt->tm_sec);
	tabflg = 1;
}

/*
*Procedure:     tsttab
*
* Restrictions:
                 printf:none
*/
/***********************************************************/

/*      test if 8-spaces to be added routine    */
void
tsttab()
{
	if (tabflg == 0) 
		printf("        ");
	else
		tabflg = 0;
}

/*
*Procedure:     prthdg
*
* Restrictions:
                 printf:none
*/
/************************************************************/

/*      print report heading routine    */
void
prthdg()
{
	int	jj=0;
	char	ccc;

	printf("\n");
	/* Stop prttim for opt U if not utilized */
	if(fopt[jj] == 'U') {
		if(BPB_UTIL)
			prttim();
	}
	else {
		prttim();
	}
	while((ccc = fopt[jj++]) != NULL)
	switch(ccc){
	case 'u':
		tsttab();
		if (dflg) {
			printf(" %7s %7s %7s %7s %7s\n",
				"%usr",
				"%sys",
				"%sys",
				"%wio",
				"%idle");
			tsttab();
			printf(" %7s %7s %7s\n",
				"",
				"local",
				"remote");
			break;
		}
		printf(" %7s %7s %7s %7s\n",
			"%usr",
			"%sys",
			"%wio",
			"%idle");
		break;
	case 'y':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s\n",
			"rawch/s",
			"canch/s",
			"outch/s",
			"rcvin/s",
			"xmtin/s",
			"mdmin/s");
		break;
	case 'b':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s %7s %7s\n",
			"bread/s",
			"lread/s",
			"%rcache",
			"bwrit/s",
			"lwrit/s",
			"%wcache",
			"pread/s",
			"pwrit/s");
		break;
	case 'd':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s %7s\n",
			"device",
			"%busy",
			"avque",
			"r+w/s",
			"blks/s",
			"avwait",
			"avserv");
		break;
	case 'v':
		tsttab();
		printf(" %s %s %s %s\n",
			"proc-sz ov",
			"inod-sz ov",
			"file-sz ov",
			"lock-sz");
		break;
	case 't':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s\n",
			"fstype",
			"inodes",
			"inodes",
			"inodes",
			"inodes",
			"%ipf");
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s\n",
			"",
			"inuse",
			"alloc",
			"limit",
			"fail",
			"");
		break;
	case 'c':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s %7s\n",
			"scall/s",
			"sread/s",
			"swrit/s",
			"fork/s",
			"exec/s",
			"rchar/s",
			"wchar/s");
		break;
	case 'w':
		tsttab();
		printf(" %7s %7s %7s %7s %7s\n",
			"swpin/s",
			"pswin/s",
			"swpot/s",
			"pswot/s",
			"pswch/s");
		break;
	case 'a':
		tsttab();
		printf(" %7s %7s %7s\n",
			"iget/s",
			"namei/s",
			"dirbk/s");
		break;
	case 'q':
		tsttab();
		printf(" %7s %7s %7s %7s\n",
			"runq-sz",
			"%runocc",
			"swpq-sz",
			"%swpocc");
		break;
	default:
		prtspechdg(ccc);	/* prints machine special headings */
		break;
	}
	if(jj > 2)
		/* don't print newline if this is the "uU" case */
		if( !((jj==3)  && ((fopt[0]=='U') || (fopt[1]=='U'))))
			printf("\n");
}

/*
*Procedure:     prtopt
*
* Restrictions:
                 printf:none
*/
/**********************************************************/

/*      print options routine   */
void
prtopt()
{
	register int ii;
	int jj=0;
	char	ccc;

	if ((strcmp(fopt, "d") == 0) || (strcmp(fopt, "t") == 0))
		printf("\n");
	/* Stop prttim for opt U if not utilized */
	if(fopt[jj] == 'U'){
		if(BPB_UTIL)
			prttim();
	}
	else {
	prttim();
	}
	for(ii=0;ii<4;ii++)
		ax.si.cpu[ii] += nx.si.cpu[ii] - ox.si.cpu[ii];
	if (dflg) ax.rf_srv.rfsi_serve += nx.rf_srv.rfsi_serve   - ox.rf_srv.rfsi_serve;

	while((ccc = fopt[jj++]) != NULL)
	switch(ccc){
	case 'u':
		tsttab();
	if (dflg) {
		if (nx.apstate) {
			printf(" %7.0f %7.0f %7.0f %7.0f %7.0f\n",
			(float)(nx.si.cpu[1] - ox.si.cpu[1])/(2*tdiff) * 100.0,

			(float)(nx.si.cpu[2] - ox.si.cpu[2] -
			       (nx.rf_srv.rfsi_serve - ox.rf_srv.rfsi_serve) )/(2*tdiff) * 100.0,
			(float)(nx.rf_srv.rfsi_serve - ox.rf_srv.rfsi_serve)/(2*tdiff) * 100.0,

			(float)(nx.si.cpu[3] - ox.si.cpu[3])/(2*tdiff) * 100.0,
			(float)(nx.si.cpu[0] - ox.si.cpu[0])/(2*tdiff) * 100.0);
		} else {
			printf(" %7.0f %7.0f %7.0f %7.0f %7.0f\n",
			(float)(nx.si.cpu[1] - ox.si.cpu[1])/tdiff * 100.0,

			(float)(nx.si.cpu[2] - ox.si.cpu[2] -
			       (nx.rf_srv.rfsi_serve - ox.rf_srv.rfsi_serve) )/tdiff * 100.0,
			(float)(nx.rf_srv.rfsi_serve - ox.rf_srv.rfsi_serve)/tdiff * 100.0,

			(float)(nx.si.cpu[3] - ox.si.cpu[3])/tdiff * 100.0,
			(float)(nx.si.cpu[0] - ox.si.cpu[0])/tdiff * 100.0);
		}
		break;
	}
		if (nx.apstate)
			printf(" %7.0f %7.0f %7.0f %7.0f\n",
			(float)(nx.si.cpu[1] - ox.si.cpu[1])/(2*tdiff) * 100.0,
			(float)(nx.si.cpu[2] - ox.si.cpu[2])/(2*tdiff) * 100.0,
			(float)(nx.si.cpu[3] - ox.si.cpu[3])/(2*tdiff) * 100.0,
			(float)(nx.si.cpu[0] - ox.si.cpu[0])/(2*tdiff) * 100.0);
		else
			printf(" %7.0f %7.0f %7.0f %7.0f\n",
			(float)(nx.si.cpu[1] - ox.si.cpu[1])/tdiff * 100.0,
			(float)(nx.si.cpu[2] - ox.si.cpu[2])/tdiff * 100.0,
			(float)(nx.si.cpu[3] - ox.si.cpu[3])/tdiff * 100.0,
			(float)(nx.si.cpu[0] - ox.si.cpu[0])/tdiff * 100.0);
		break;
	case 'y':
		tsttab();
		printf(" %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
		(float)(nx.si.rawch - ox.si.rawch)/tdiff * HZ,
		(float)(nx.si.canch - ox.si.canch)/tdiff * HZ,
		(float)(nx.si.outch - ox.si.outch)/tdiff * HZ,
		(float)(nx.si.rcvint - ox.si.rcvint)/tdiff * HZ,
		(float)(nx.si.xmtint - ox.si.xmtint)/tdiff * HZ,
		(float)(nx.si.mdmint - ox.si.mdmint)/tdiff * HZ);

		ax.si.rawch += nx.si.rawch - ox.si.rawch;
		ax.si.canch += nx.si.canch - ox.si.canch;
		ax.si.outch += nx.si.outch - ox.si.outch;
		ax.si.rcvint += nx.si.rcvint - ox.si.rcvint;
		ax.si.xmtint += nx.si.xmtint - ox.si.xmtint;
		ax.si.mdmint += nx.si.mdmint - ox.si.mdmint;
		break;
	case 'b':
		tsttab();
		if (dflg) {
			printf("\n   local  %4.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
				(float)(nx.si.bread - ox.si.bread)/tdiff * HZ,
				(float)(nx.si.lread - ox.si.lread)/tdiff * HZ,
				((nx.si.lread - ox.si.lread) <= 0) ? 100 :
				(((float)(nx.si.lread - ox.si.lread) -
				  (float)(nx.si.bread - ox.si.bread))/
				  (float)(nx.si.lread - ox.si.lread) * 100.0),
				(float)(nx.si.bwrite - ox.si.bwrite)/tdiff * HZ,
				(float)(nx.si.lwrite - ox.si.lwrite)/tdiff * HZ,
				((nx.si.lwrite - ox.si.lwrite) <= 0) ? 100 :
				(((float)(nx.si.lwrite - ox.si.lwrite) -
				  (float)(nx.si.bwrite - ox.si.bwrite))/
				  (float)(nx.si.lwrite - ox.si.lwrite) * 100.0),
				(float)(nx.si.phread - ox.si.phread)/tdiff * HZ,
				(float)(nx.si.phwrite - ox.si.phwrite)/tdiff * HZ);

			ax.si.bread += nx.si.bread - ox.si.bread;
			ax.si.bwrite += nx.si.bwrite - ox.si.bwrite;
			ax.si.lread += nx.si.lread - ox.si.lread;
			ax.si.lwrite += nx.si.lwrite - ox.si.lwrite;
			ax.si.phread += nx.si.phread - ox.si.phread;
			ax.si.phwrite += nx.si.phwrite - ox.si.phwrite;
			printf("   remote %4.0f %7.0f %7.0f %7.0f %7.0f %7.0f \n",
				(float)((nx.rfc.rfci_pmread - ox.rfc.rfci_pmread)
					/tdiff * HZ) * BLKPERPG,
				(float)((nx.rfc.rfci_ptread - ox.rfc.rfci_ptread)
					/tdiff * HZ) * BLKPERPG,
				(((float)(nx.rfc.rfci_ptread - ox.rfc.rfci_ptread) -
	         		(float)(nx.rfc.rfci_pmread - ox.rfc.rfci_pmread))/
				(float)((nx.rfc.rfci_ptread - ox.rfc.rfci_ptread)?
				(nx.rfc.rfci_ptread - ox.rfc.rfci_ptread):1) * 100.0),
				(float)((nx.rfc.rfci_pmwrite - ox.rfc.rfci_pmwrite)
					/tdiff * HZ) * BLKPERPG,
				(float)((nx.rfc.rfci_ptwrite - ox.rfc.rfci_ptwrite)
					/tdiff * HZ) * BLKPERPG,
				(((float)(nx.rfc.rfci_ptwrite - ox.rfc.rfci_ptwrite) -
				(float)(nx.rfc.rfci_pmwrite - ox.rfc.rfci_pmwrite))/
				(float)((nx.rfc.rfci_ptwrite - ox.rfc.rfci_ptwrite)?
				(nx.rfc.rfci_ptwrite - ox.rfc.rfci_ptwrite):1) * 100.0));

			ax.rfc.rfci_pmread += nx.rfc.rfci_pmread - ox.rfc.rfci_pmread;
			ax.rfc.rfci_ptread += nx.rfc.rfci_ptread - ox.rfc.rfci_ptread;
			ax.rfc.rfci_pmwrite += nx.rfc.rfci_pmwrite - ox.rfc.rfci_pmwrite;
			ax.rfc.rfci_ptwrite += nx.rfc.rfci_ptwrite - ox.rfc.rfci_ptwrite;
			break;
		}
		printf(" %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
			(float)(nx.si.bread - ox.si.bread)/tdiff * HZ,
			(float)(nx.si.lread - ox.si.lread)/tdiff * HZ,
			((nx.si.lread - ox.si.lread) <= 0) ? 100 :
			(((float)(nx.si.lread - ox.si.lread) -
			  (float)(nx.si.bread - ox.si.bread))/
			  (float)(nx.si.lread - ox.si.lread) * 100.0),
			(float)(nx.si.bwrite - ox.si.bwrite)/tdiff * HZ,
			(float)(nx.si.lwrite - ox.si.lwrite)/tdiff * HZ,
			((nx.si.lwrite - ox.si.lwrite) <= 0) ? 100 :
			(((float)(nx.si.lwrite - ox.si.lwrite) -
			  (float)(nx.si.bwrite - ox.si.bwrite))/
			  (float)(nx.si.lwrite - ox.si.lwrite) * 100.0),
			(float)(nx.si.phread - ox.si.phread)/tdiff * HZ,
			(float)(nx.si.phwrite - ox.si.phwrite)/tdiff * HZ);

		ax.si.bread += nx.si.bread - ox.si.bread;
		ax.si.bwrite += nx.si.bwrite - ox.si.bwrite;
		ax.si.lread += nx.si.lread - ox.si.lread;
		ax.si.lwrite += nx.si.lwrite - ox.si.lwrite;
		ax.si.phread += nx.si.phread - ox.si.phread;
		ax.si.phwrite += nx.si.phwrite - ox.si.phwrite;
		break;
	case 'd':
		prtoptdsk();	/* prints activities of block devices */
		break;
	case 'v':
		tsttab();
		printf(" %3d/%3d%3ld %3d/%3d%3ld %3d/%3d%3ld %3d/%3d\n",
			nx.szproc, nx.mszproc, (nx.procovf - ox.procovf),
			nx.szinode, nx.mszinode, (nx.inodeovf - ox.inodeovf),
			nx.szfile, nx.mszfile, (nx.fileovf - ox.fileovf),
			nx.szlckr, nx.mszlckr);
		break;

	case 't':
		if (nx.s5cszinode > 0) {
			long	s5ipagediff, s5inopagediff;
			s5ipagediff = nx.si.s5ipage - ox.si.s5ipage;
			s5inopagediff = nx.si.s5inopage - ox.si.s5inopage;
			tsttab();
			printf(" %7s %7d %7d %7d %7d %7.2f\n",
				"s5",
				nx.s5szinode, nx.s5cszinode, nx.s5mszinode, 
				(nx.s5inodeovf - ox.s5inodeovf),
				((s5ipagediff + s5inopagediff) <= 0) ? 0 :
			    		((double)s5ipagediff/
					(double)(s5ipagediff + s5inopagediff))
					* 100);
			ax.si.s5ipage += s5ipagediff;
			ax.si.s5inopage += s5inopagediff;
			ax.s5cszinode += nx.s5cszinode;
		}
				
		if (nx.sfscszinode > 0) {
			long	sfsipagediff, sfsinopagediff;
			sfsipagediff = nx.si.sfsipage - ox.si.sfsipage;
			sfsinopagediff = nx.si.sfsinopage - ox.si.sfsinopage;
			tsttab();
			printf(" %7s %7d %7d %7d %7d %7.2f\n",
				"ufs/sfs",
				nx.sfsszinode, nx.sfscszinode, nx.sfsmszinode, 
				(nx.sfsinodeovf - ox.sfsinodeovf),
				((sfsipagediff + sfsinopagediff) <= 0) ? 0 :
			    		((double)sfsipagediff/
					(double)(sfsipagediff + sfsinopagediff))
					* 100);
			ax.si.sfsipage += sfsipagediff;
			ax.si.sfsinopage += sfsinopagediff;
			ax.sfscszinode += nx.sfscszinode;
		}
		if (nx.vxfscszinode > 0) {
			long	vxfsipagediff, vxfsinopagediff;
			vxfsipagediff = nx.si.vxfsipage - ox.si.vxfsipage;
			vxfsinopagediff = nx.si.vxfsinopage - ox.si.vxfsinopage;
			tsttab();
			printf(" %7s %7d %7d %7d %7d %7.2f\n",
				"vxfs", nx.vxfsszinode,
				nx.vxfscszinode, nx.vxfsmszinode, 
				(nx.vxfsinodeovf - ox.vxfsinodeovf),
				((vxfsipagediff + vxfsinopagediff) <= 0) ? 0 :
				    ((double)vxfsipagediff/
				    (double)(vxfsipagediff + vxfsinopagediff))
				    * 100);
			ax.si.vxfsipage += vxfsipagediff;
			ax.si.vxfsinopage += vxfsinopagediff;
			ax.vxfscszinode += nx.vxfscszinode;
		}
		break;
	case 'c':
		tsttab();
	if (dflg) {
		Isyscall = ((float)(nx.rfs_in.fsivop_open - ox.rfs_in.fsivop_open)
		 	+ (float)(nx.rfs_in.fsivop_close - ox.rfs_in.fsivop_close)
		 	+ (float)(nx.rfs_in.fsivop_read - ox.rfs_in.fsivop_read)
		 	+ (float)(nx.rfs_in.fsivop_write - ox.rfs_in.fsivop_write)
		 	+ (float)(nx.rfs_in.fsivop_lookup - ox.rfs_in.fsivop_lookup)
		 	+ (float)(nx.rfs_in.fsivop_create - ox.rfs_in.fsivop_create)
		 	+ (float)(nx.rfs_in.fsivop_readdir - ox.rfs_in.fsivop_readdir))
					/tdiff * HZ;

		Osyscall = ((float)(nx.rfs_out.fsivop_open - ox.rfs_out.fsivop_open)
			+ (float)(nx.rfs_out.fsivop_close - ox.rfs_out.fsivop_close)
			+ (float)(nx.rfs_out.fsivop_read - ox.rfs_out.fsivop_read)
			+ (float)(nx.rfs_out.fsivop_write - ox.rfs_out.fsivop_write)
		 	+ (float)(nx.rfs_out.fsivop_lookup - ox.rfs_out.fsivop_lookup)
		 	+ (float)(nx.rfs_out.fsivop_create - ox.rfs_out.fsivop_create)
		 	+ (float)(nx.rfs_out.fsivop_readdir - ox.rfs_out.fsivop_readdir))
					/tdiff * HZ;

		Lsyscall = (float)(nx.si.syscall - ox.si.syscall )/tdiff * HZ -
				(Osyscall);

		Isysread = (float)(nx.rfs_in.fsivop_read - ox.rfs_in.fsivop_read)
					/tdiff * HZ;
		Osysread = (float)(nx.rfs_out.fsivop_read - ox.rfs_out.fsivop_read)
					/tdiff * HZ;
		Lsysread = (float)(nx.si.sysread - ox.si.sysread )/tdiff * HZ -
				(Osysread);

		Isyswrite = (float)(nx.rfs_in.fsivop_write - ox.rfs_in.fsivop_write)
					/tdiff * HZ;
		Osyswrite = (float)(nx.rfs_out.fsivop_write - ox.rfs_out.fsivop_write)
					/tdiff * HZ;
		Lsyswrite = (float)(nx.si.syswrite - ox.si.syswrite )/tdiff * HZ -
				(Osyswrite);

		Isysexec = 0;
		Osysexec = 0;
		Lsysexec = (float)(nx.si.sysexec - ox.si.sysexec )/tdiff * HZ;

		Ireadch = (float)(nx.rfs_in.fsireadch - ox.rfs_in.fsireadch)
					/tdiff * HZ;
		Oreadch = (float)(nx.rfs_out.fsireadch - ox.rfs_out.fsireadch)
					/tdiff * HZ;
		Lreadch = (float)(nx.si.readch - ox.si.readch )/tdiff * HZ -
				(Oreadch);

		Iwritech = (float)(nx.rfs_in.fsiwritech - ox.rfs_in.fsiwritech)
					/tdiff * HZ;
		Owritech = (float)(nx.rfs_out.fsiwritech - ox.rfs_out.fsiwritech)
					/tdiff * HZ;
		Lwritech = (float)(nx.si.writech - ox.si.writech )/tdiff * HZ -
				(Owritech);

		printf("\n%-8s %7.0f %7.0f %7.0f %7s %7.2f %7.0f %7.0f\n",
			"   in", Isyscall,Isysread,Isyswrite,"",Isysexec,
			Ireadch,Iwritech);
		printf("%-8s %7.0f %7.0f %7.0f %7s %7.2f %7.0f %7.0f\n",
			"   out",Osyscall,Osysread,Osyswrite,"",Osysexec,
			Oreadch,Owritech);
		printf("%-8s %7.0f %7.0f %7.0f %7.2f %7.2f %7.0f %7.0f\n",
			"   local",
			Lsyscall >= 0.0 ? Lsyscall : 0.0,
			Lsysread >= 0.0 ? Lsysread : 0.0,
			Lsyswrite >= 0.0 ? Lsyswrite : 0.0,
			(float)(nx.si.sysfork - ox.si.sysfork)/tdiff * HZ,
			Lsysexec,
			Lreadch >= 0.0 ? Lreadch : 0.0,
			Lwritech >= 0.0 ? Lwritech : 0.0);
 
		ax.rfs_in.fsivop_open += nx.rfs_in.fsivop_open 
					- ox.rfs_in.fsivop_open;
		ax.rfs_in.fsivop_close += nx.rfs_in.fsivop_close 
					- ox.rfs_in.fsivop_close;
		ax.rfs_in.fsivop_lookup += nx.rfs_in.fsivop_lookup 
					- ox.rfs_in.fsivop_lookup;
		ax.rfs_in.fsivop_create += nx.rfs_in.fsivop_create 
					- ox.rfs_in.fsivop_create;
		ax.rfs_in.fsivop_readdir += nx.rfs_in.fsivop_readdir 
					- ox.rfs_in.fsivop_readdir;

		ax.rfs_out.fsivop_open += nx.rfs_out.fsivop_open 
					- ox.rfs_out.fsivop_open;
		ax.rfs_out.fsivop_close += nx.rfs_out.fsivop_close 
					- ox.rfs_out.fsivop_close;
		ax.rfs_out.fsivop_lookup += nx.rfs_out.fsivop_lookup 
					- ox.rfs_out.fsivop_lookup;
		ax.rfs_out.fsivop_create += nx.rfs_out.fsivop_create 
					- ox.rfs_out.fsivop_create;
		ax.rfs_out.fsivop_readdir += nx.rfs_out.fsivop_readdir 
					- ox.rfs_out.fsivop_readdir;
		ax.si.syscall += nx.si.syscall - ox.si.syscall;
 
		ax.rfs_in.fsivop_read += nx.rfs_in.fsivop_read 
					- ox.rfs_in.fsivop_read;
		ax.rfs_out.fsivop_read += nx.rfs_out.fsivop_read 
					- ox.rfs_out.fsivop_read;
		ax.si.sysread += nx.si.sysread - ox.si.sysread;
 
		ax.rfs_in.fsivop_write += nx.rfs_in.fsivop_write 
					- ox.rfs_in.fsivop_write;
		ax.rfs_out.fsivop_write += nx.rfs_out.fsivop_write 
					- ox.rfs_out.fsivop_write;
		ax.si.syswrite += nx.si.syswrite - ox.si.syswrite;
 
		ax.si.sysexec += nx.si.sysexec - ox.si.sysexec;
 
		ax.rfs_in.fsireadch += nx.rfs_in.fsireadch - ox.rfs_in.fsireadch;
		ax.rfs_out.fsireadch += nx.rfs_out.fsireadch - ox.rfs_out.fsireadch;
		ax.si.readch += nx.si.readch - ox.si.readch;
	 
		ax.rfs_in.fsiwritech += nx.rfs_in.fsiwritech - ox.rfs_in.fsiwritech;
		ax.rfs_out.fsiwritech += nx.rfs_out.fsiwritech - ox.rfs_out.fsiwritech;
		ax.si.writech += nx.si.writech - ox.si.writech;

		ax.si.sysfork += nx.si.sysfork - ox.si.sysfork;

		break;
	}
		printf(" %7.0f %7.0f %7.0f %7.2f %7.2f %7.0f %7.0f\n",
			(float)(nx.si.syscall - ox.si.syscall)/tdiff *HZ,
			(float)(nx.si.sysread - ox.si.sysread)/tdiff *HZ,
			(float)(nx.si.syswrite - ox.si.syswrite)/tdiff *HZ,
			(float)(nx.si.sysfork - ox.si.sysfork)/tdiff *HZ,
			(float)(nx.si.sysexec - ox.si.sysexec)/tdiff *HZ,
			(float)(nx.si.readch - ox.si.readch)/tdiff * HZ,
			(float)(nx.si.writech - ox.si.writech)/tdiff * HZ);

		ax.si.syscall += nx.si.syscall - ox.si.syscall;
		ax.si.sysread += nx.si.sysread - ox.si.sysread;
		ax.si.syswrite += nx.si.syswrite - ox.si.syswrite;
		ax.si.sysfork += nx.si.sysfork - ox.si.sysfork;
		ax.si.sysexec += nx.si.sysexec - ox.si.sysexec;
		ax.si.readch += nx.si.readch - ox.si.readch;
		ax.si.writech += nx.si.writech - ox.si.writech;
		break;
	case 'w':
		tsttab();
		printf(" %7.2f %7.1f %7.2f %7.1f %7.0f\n",
			(float)(nx.vmi.v_swpin - ox.vmi.v_swpin)/tdiff * HZ,
			(float)(nx.vmi.v_pswpin - ox.vmi.v_pswpin)/tdiff * HZ,
			(float)(nx.vmi.v_swpout - ox.vmi.v_swpout)/tdiff * HZ,
			(float)(nx.vmi.v_pswpout - ox.vmi.v_pswpout)/tdiff * HZ,
			(float)(nx.si.pswitch - ox.si.pswitch)/tdiff * HZ);

		ax.vmi.v_swpin += nx.vmi.v_swpin - ox.vmi.v_swpin;
		ax.vmi.v_pswpin += nx.vmi.v_pswpin - ox.vmi.v_pswpin;
		ax.vmi.v_swpout += nx.vmi.v_swpout - ox.vmi.v_swpout;
		ax.vmi.v_pswpout += nx.vmi.v_pswpout - ox.vmi.v_pswpout;
		ax.si.pswitch += nx.si.pswitch - ox.si.pswitch;
		break;
	case 'a':
		tsttab();
		printf(" %7.0f %7.0f %7.0f\n",
			(float)(nx.si.iget - ox.si.iget)/tdiff * HZ,
			(float)(nx.si.namei - ox.si.namei)/tdiff * HZ,
			(float)(nx.si.dirblk - ox.si.dirblk)/tdiff * HZ);

		ax.si.iget += nx.si.iget - ox.si.iget;
		ax.si.namei += nx.si.namei - ox.si.namei;
		ax.si.dirblk += nx.si.dirblk - ox.si.dirblk;
		break;
	
	case 'q':
		tsttab();
		if ((nx.si.runocc - ox.si.runocc) == 0)
			printf(" %7s %7s", "  ", "  ");
		else {
			printf(" %7.1f %7.0f",
			(float)(nx.si.runque -ox.si.runque)/
				(float)(nx.si.runocc - ox.si.runocc),
			sec_diff <= 0.0 ? 0.0 :
				(float)(nx.si.runocc -ox.si.runocc)/
				sec_diff * 100.0);
			ax.si.runque += nx.si.runque - ox.si.runque;
			ax.si.runocc += nx.si.runocc - ox.si.runocc;
		}
		if ((nx.si.swpocc - ox.si.swpocc) == 0)
			printf(" %7s %7s\n","  ","  ");
		else {
			printf(" %7.1f %7.0f\n",
			(float)(nx.si.swpque -ox.si.swpque)/
				(float)(nx.si.swpocc - ox.si.swpocc),
			sec_diff <= 0.0 ? 0.0 :
				(float)(nx.si.swpocc -ox.si.swpocc)/
				sec_diff *100.0);
			ax.si.swpque += nx.si.swpque - ox.si.swpque;
			ax.si.swpocc += nx.si.swpocc - ox.si.swpocc;

		}
		break;
	case 'm':
		tsttab();
		printf(" %7.2f %7.2f\n",
			(float)(nx.si.msg - ox.si.msg)/tdiff * HZ,
			(float)(nx.si.sema - ox.si.sema)/tdiff * HZ);

		ax.si.msg += nx.si.msg - ox.si.msg;
		ax.si.sema += nx.si.sema - ox.si.sema;
		break;

	default:
		prtspecopt(ccc);
		break;
	}
	if(jj > 2)
		/* don't print newline if this is the "uU" case */
		if( !((jj==3)  && ((fopt[0]=='U') || (fopt[1]=='U'))))
			printf("\n");
}

/*
*Procedure:     prtavg
*
* Restrictions:
                 printf:none
*/
/**********************************************************/

/*      print average routine  */
void
prtavg()
{
	int	jj = 0;
	char	ccc;

	tdiff = ax.si.cpu[0] + ax.si.cpu[1] + ax.si.cpu[2] + ax.si.cpu[3];
	if (nx.apstate)
		tdiff = tdiff/2;
	if (tdiff <= 0.0)
		return;
	printf("\n");

	while((ccc = fopt[jj++]) != NULL)
	switch(ccc){
	case 'u':
		if (dflg) {
			if(nx.apstate)
				printf("Average  %7.0f %7.0f %7.0f %7.0f %7.0f\n",
				(float)ax.si.cpu[1]/(2*tdiff) * 100.0,
				(float)(ax.si.cpu[2] - ax.rf_srv.rfsi_serve)/(2*tdiff) * 100.0,
				(float)(ax.rf_srv.rfsi_serve)/(2*tdiff) * 100.0,
				(float)ax.si.cpu[3]/(2*tdiff) * 100.0,
				(float)ax.si.cpu[0]/(2*tdiff) * 100.0);
			else
				printf("Average  %7.0f %7.0f %7.0f %7.0f %7.0f\n",
				(float)ax.si.cpu[1]/tdiff * 100.0,
				(float)(ax.si.cpu[2] - ax.rf_srv.rfsi_serve)/tdiff * 100.0,
				(float)(ax.rf_srv.rfsi_serve)/tdiff * 100.0,
				(float)ax.si.cpu[3]/tdiff * 100.0,
				(float)ax.si.cpu[0]/tdiff * 100.0);

		break;
		}
		if(nx.apstate)
			printf("Average  %7.0f %7.0f %7.0f %7.0f\n",
			(float)ax.si.cpu[1]/(2*tdiff) * 100.0,
			(float)ax.si.cpu[2]/(2*tdiff) * 100.0,
			(float)ax.si.cpu[3]/(2*tdiff) * 100.0,
			(float)ax.si.cpu[0]/(2*tdiff) * 100.0);
		else
			printf("Average  %7.0f %7.0f %7.0f %7.0f\n",
			(float)ax.si.cpu[1]/tdiff * 100.0,
			(float)ax.si.cpu[2]/tdiff * 100.0,
			(float)ax.si.cpu[3]/tdiff * 100.0,
			(float)ax.si.cpu[0]/tdiff * 100.0);
		break;
	case 'y':
		printf("Average  %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
			(float)ax.si.rawch/tdiff *HZ,
			(float)ax.si.canch/tdiff *HZ,
			(float)ax.si.outch/tdiff *HZ,
			(float)ax.si.rcvint/tdiff *HZ,
			(float)ax.si.xmtint/tdiff *HZ,
			(float)ax.si.mdmint/tdiff *HZ);
		break;
	case 'b':
		if (dflg) {
			printf("Average\n   local  %4.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
				(float)ax.si.bread/tdiff *HZ,
				(float)ax.si.lread/tdiff *HZ,
				((ax.si.lread == 0) ? 100 :
					((float)(ax.si.lread - ax.si.bread)/
					(float)(ax.si.lread) * 100.0)),
				(float)ax.si.bwrite/tdiff *HZ,
				(float)ax.si.lwrite/tdiff *HZ,
				((ax.si.lwrite == 0) ? 100.0 :
					((float)(ax.si.lwrite - ax.si.bwrite)/
					(float)(ax.si.lwrite) * 100.0)),
				(float)ax.si.phread/tdiff *HZ,
				(float)ax.si.phwrite/tdiff *HZ);
			printf("   remote %4.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
				(float)(ax.rfc.rfci_pmread/tdiff *HZ) * BLKPERPG,
				(float)(ax.rfc.rfci_ptread/tdiff *HZ) * BLKPERPG,
				(float)(ax.rfc.rfci_ptread - ax.rfc.rfci_pmread)/
				(float)(ax.rfc.rfci_ptread?(ax.rfc.rfci_ptread):1) * 100.0,
				(float)(ax.rfc.rfci_pmwrite/tdiff *HZ) * BLKPERPG,
				(float)(ax.rfc.rfci_ptwrite/tdiff *HZ) * BLKPERPG,
				(float)(ax.rfc.rfci_ptwrite - ax.rfc.rfci_pmwrite)/
					(float)(ax.rfc.rfci_ptwrite?(ax.rfc.rfci_ptwrite):
					1) * 100.0);
				break;
		}
		printf("Average  %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f %7.0f\n",
			(float)ax.si.bread/tdiff *HZ,
			(float)ax.si.lread/tdiff *HZ,
			((ax.si.lread == 0) ? 100.0 :
				((float)(ax.si.lread - ax.si.bread)/
				(float)(ax.si.lread) * 100.0)),
			(float)ax.si.bwrite/tdiff *HZ,
			(float)ax.si.lwrite/tdiff *HZ,
			((ax.si.lwrite == 0) ? 100.0 :
				((float)(ax.si.lwrite - ax.si.bwrite)/
				(float)(ax.si.lwrite) * 100.0)),
			(float)ax.si.phread/tdiff *HZ,
			(float)ax.si.phwrite/tdiff *HZ);
		break;
	case 'd':
		prtavgdsk();
		break;
	case 'v':
		break;
	case 't':
		if ((ax.s5cszinode == 0) && (ax.sfscszinode == 0) &&
		    (ax.vxfscszinode == 0))
			break;

		printf("Average ");
		if (ax.s5cszinode != 0) {
			tsttab();
			printf(" %7s %7s %7s %7s %7s %7.2f\n",
				"s5", "", "", "", "",
				(ax.si.s5ipage + ax.si.s5inopage <= 0) ? 0 :
			    	   ((double)ax.si.s5ipage /
				   (double)(ax.si.s5ipage + ax.si.s5inopage))
				   * 100);
		}
		if (ax.sfscszinode != 0) {
			tsttab();
			printf(" %7s %7s %7s %7s %7s %7.2f\n",
				"ufs/sfs", "", "", "", "",
				(ax.si.sfsipage + ax.si.sfsinopage <= 0) ? 0 :
			    	   ((double)ax.si.sfsipage /
				   (double)(ax.si.sfsipage + ax.si.sfsinopage))
				   * 100);
		}
		if (ax.vxfscszinode != 0) {
			tsttab();
			printf(" %7s %7s %7s %7s %7s %7.2f\n",
				"vxfs", "", "", "", "",
				(ax.si.vxfsipage + ax.si.vxfsinopage <= 0) ? 0 :
			    	  ((double)ax.si.vxfsipage /
				  (double)(ax.si.vxfsipage + ax.si.vxfsinopage))
				  * 100);
		}
		break;
	case 'c':
		if (dflg) {
			Isyscall = (float)((ax.rfs_in.fsivop_open
					+ ax.rfs_in.fsivop_close 	
					+ ax.rfs_in.fsivop_read 	
					+ ax.rfs_in.fsivop_write 	
					+ ax.rfs_in.fsivop_lookup 	
					+ ax.rfs_in.fsivop_create 	
					+ ax.rfs_in.fsivop_readdir) 	
					/tdiff * HZ);
			Osyscall = (float)((ax.rfs_out.fsivop_open
					+ ax.rfs_out.fsivop_close 	
					+ ax.rfs_out.fsivop_read 	
					+ ax.rfs_out.fsivop_write 	
					+ ax.rfs_out.fsivop_lookup 	
					+ ax.rfs_out.fsivop_create 	
					+ ax.rfs_out.fsivop_readdir) 	
					/tdiff * HZ);
			Lsyscall = (float)(ax.si.syscall/tdiff * HZ) -
					(Osyscall);

			Isysread = (float)(ax.rfs_in.fsivop_read/tdiff * HZ);
			Osysread = (float)(ax.rfs_out.fsivop_read/tdiff * HZ);
			Lsysread = (float)(ax.si.sysread/tdiff * HZ) -
					(Osysread);

			Isyswrite = (float)(ax.rfs_in.fsivop_write/tdiff * HZ);
			Osyswrite = (float)(ax.rfs_in.fsivop_write/tdiff * HZ);
			Lsyswrite = (float)(ax.si.syswrite/tdiff * HZ) -
					(Osyswrite);

			Isysexec = 0;
			Osysexec = 0;
			Lsysexec = (float)(ax.si.sysexec/tdiff * HZ);

			Ireadch = (float)(ax.rfs_in.fsireadch/tdiff * HZ);
			Oreadch = (float)(ax.rfs_out.fsireadch/tdiff * HZ);
			Lreadch = (float)(ax.si.readch/tdiff * HZ) -
					(Oreadch);

			Iwritech = (float)(ax.rfs_in.fsiwritech/tdiff * HZ);
			Owritech = (float)(ax.rfs_out.fsiwritech/tdiff * HZ);
			Lwritech = (float)(ax.si.writech/tdiff * HZ) -
					(Owritech);

			printf("Average\n%-8s %7.0f %7.0f %7.0f %7s %7.2f %7.0f %7.0f\n",
				"   in", Isyscall,Isysread,Isyswrite,"",Isysexec,
				Ireadch,Iwritech);
			printf("%-8s %7.0f %7.0f %7.0f %7s %7.2f %7.0f %7.0f\n",
				"   out",Osyscall,Osysread,Osyswrite,"",Osysexec,
				Oreadch,Owritech);
			printf("%-8s %7.0f %7.0f %7.0f %7.2f %7.2f %7.0f %7.0f\n",
				"   local",
				Lsyscall >= 0.0 ? Lsyscall : 0.0,
				Lsysread >= 0.0 ? Lsysread : 0.0,
				Lsyswrite >= 0.0 ? Lsyswrite : 0.0,
				(float)ax.si.sysfork/tdiff *HZ,
				Lsysexec,
				Lreadch >= 0.0 ? Lreadch : 0.0,
				Lwritech >= 0.0 ? Lwritech : 0.0);
			break;
		}

		printf("Average  %7.0f %7.0f %7.0f %7.2f %7.2f %7.0f %7.0f\n",
			(float)ax.si.syscall/tdiff *HZ,
			(float)ax.si.sysread/tdiff *HZ,
			(float)ax.si.syswrite/tdiff *HZ,
			(float)ax.si.sysfork/tdiff *HZ,
			(float)ax.si.sysexec/tdiff *HZ,
			(float)ax.si.readch/tdiff * HZ,
			(float)ax.si.writech/tdiff * HZ);
		break;
	case 'w':
		printf("Average  %7.2f %7.1f %7.2f %7.1f %7.0f\n",
			(float)ax.vmi.v_swpin/tdiff * HZ,
			(float)ax.vmi.v_pswpin /tdiff * HZ,
			(float)ax.vmi.v_swpout/tdiff * HZ,
			(float)ax.vmi.v_pswpout/tdiff * HZ,
			(float)ax.si.pswitch/tdiff * HZ);
		break;
	case 'a':
		printf("Average  %7.0f %7.0f %7.0f\n",
			(float)ax.si.iget/tdiff * HZ,
			(float)ax.si.namei/tdiff * HZ,
			(float)ax.si.dirblk/tdiff * HZ);
		break;
	case 'q':
		if (ax.si.runocc == 0)
			printf("Average  %7s %7s ","  ","  ");
		else {
			printf("Average  %7.1f %7.0f ",
			(float)ax.si.runque /
				(float)ax.si.runocc,
			totsec_diff <= 0.0 ? 0.0 :
				(float)ax.si.runocc /totsec_diff * 100.0);
		}
		if (ax.si.swpocc == 0)
			printf("%7s %7s\n","  ","  ");
		else {
			printf("%7.1f %7.0f\n",
			(float)ax.si.swpque/
				(float)ax.si.swpocc,
			totsec_diff <= 0.0 ? 0.0 :
				(float)ax.si.swpocc/totsec_diff *100.0);

		}
		break;
	case 'm':
		printf("Average  %7.2f %7.2f\n",
			(float)ax.si.msg/tdiff * HZ,
			(float)ax.si.sema/tdiff * HZ);
		break;
	default:
		prtspecavg(ccc);
		break;
	}
}

/*
*Procedure:     pillarg
*
* Restrictions:
                 fprintf:none
*/
/**********************************************************/

/*      error exit routines  */
void
pillarg()
{
	fprintf(stderr,"%s -- illegal argument for option  %c\n",
		optarg,cc);
	exit(1);
}

/*
*Procedure:     perrexit
*
* Restrictions:
                 perror:none
*/
void
perrexit()
{
	perror("sar");
	exit(1);
}

/*
*Procedure:     pmsgexit
*
* Restrictions:
                 fprintf:none
*/
void
pmsgexit(s)
char	*s;
{
	fprintf(stderr, "%s\n", s);
	exit(1);
}
