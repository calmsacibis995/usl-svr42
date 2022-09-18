/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)nfs.cmds:nfs/nfsstat/nfsstat.c	1.1.4.3"
#ident	"$Header: $"


/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
		  All rights reserved.
 
*/
/* 
 * nfsstat: Network File System statistics
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <nlist.h>
#include <sys/ksym.h>


struct nlist nl[] = {
#define	X_RCSTAT	0
	{ "rcstat" , 0, 0, 0, 0, 0 },
#define	X_CLSTAT	1
	{ "clstat", 0, 0, 0, 0, 0  },
#define	X_RSSTAT	2
	{ "rsstat", 0, 0, 0, 0, 0  },
#define	X_SVSTAT	3
	{ "svstat", 0, 0, 0, 0, 0  },
	"",
};
int kmemfd;			/* fd for core file (kmem) */
char *core   = "/dev/kmem";	/* name of core file (kmem) */
char *vmunix = "/stand/unix";
char *progname;			/* argv[0] */

/*
 * client side rpc statistics
 */
struct {
	int     rccalls;
	int     rcbadcalls;
	int     rcretrans;
	int     rcbadxids;
	int     rctimeouts;
	int     rcwaits;
	int     rcnewcreds;
	int     rcbadverfs;
	int     rctimers;
	int     rctoobig;
	int     rcnomem;
	int     rccantsend;
	int     rcbufulocks;
} rcstat;

/*
 * client side nfs statistics
 */
struct {
	int	nclsleeps;		/* client handle waits */
	int	nclgets;		/* client handle gets */
	int	ncalls;			/* client requests */
	int	nbadcalls;		/* rpc failures */
	int	reqs[32];		/* count of each request */
} clstat;

/*
 * Server side rpc statistics
 */
struct {
	int     rscalls;
	int     rsbadcalls;
	int     rsnullrecv;
	int     rsbadlen;
	int     rsxdrcall;
} rsstat;

/*
 * server side nfs statistics
 */
struct {
	int     ncalls;         /* number of calls received */
	int     nbadcalls;      /* calls that failed */
	int     reqs[32];       /* count for each request */
} svstat;

static boolean_t	ccode, scode; 	/* Identify server and client code present */
static boolean_t	memflg = B_FALSE; /* set to true if user supplied core */

main(argc, argv)
	char *argv[];
{
	char *options;
	int	cflag = 0;		/* client stats */
	int	sflag = 0;		/* server stats */
	int	nflag = 0;		/* nfs stats */
	int	rflag = 0;		/* rpc stats */
	int	zflag = 0;		/* zero stats after printing */


	progname = argv[0];

	if (argc >= 2 && *argv[1] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'c':
				cflag++;
				break;
			case 'n':
				nflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'z':
				zflag++;
				break;
			default:
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}
	if (argc >= 2) {
		vmunix = argv[1];
		argv++;
		argc--;
		if (argc == 2) {
			core = argv[1];
			memflg = B_TRUE;
			argv++;
			argc--;
		}
	}
	if (argc != 1) {
		usage();
	}


	setup(zflag);
	getstats();
	if (sflag && (!scode)) {
		fprintf(stderr,"nfsstat: kernel is not configured with the server nfs and rpc code.\n");
	}
	if ((sflag || (!sflag && !cflag)) && scode) {
		if (rflag || (!rflag && !nflag)) {
			sr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			sn_print(zflag);
		}
	}
	if (cflag && (!ccode)) {
		fprintf(stderr,"nfsstat: kernel is not configured with the client nfs and rpc code.\n");
	}
	if ((cflag || (!sflag && !cflag)) && ccode) {
		if (rflag || (!rflag && !nflag)) {
			cr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			cn_print(zflag);
		}
	}
	if (zflag) {
		putstats();
	}
	exit(0);
}

getstats()
{
	if(ioinfo(X_RCSTAT,&rcstat,sizeof(rcstat),MIOC_READKSYM) != 0 ||
	   ioinfo(X_CLSTAT,&clstat,sizeof(clstat), MIOC_READKSYM) != 0)
		ccode = B_FALSE;
	else
		ccode = B_TRUE;

	if(ioinfo(X_RSSTAT,&rsstat,sizeof(rsstat),MIOC_READKSYM) != 0 ||
	   ioinfo(X_SVSTAT,&svstat,sizeof(svstat), MIOC_READKSYM) != 0)
		scode = B_FALSE;
	else
		scode = B_TRUE;

}

putstats()
{
	if (ccode) {
		if(ioinfo(X_RCSTAT,&rcstat,sizeof(rcstat),MIOC_WRITEKSYM) != 0 ||
	   	   ioinfo(X_CLSTAT,&clstat,sizeof(clstat), MIOC_WRITEKSYM) != 0) {
			fprintf(stderr,"%s: Cannot write statistics to kernel\n",progname);
			exit(-1);
		}
	}
	if (scode) {
		if(ioinfo(X_RSSTAT,&rsstat,sizeof(rsstat),MIOC_READKSYM) != 0 ||
	   	   ioinfo(X_SVSTAT,&svstat,sizeof(svstat), MIOC_READKSYM) != 0) {
			fprintf(stderr,"%s: Cannot write statistics to kernel\n",progname);
			exit(-1);
		}

	}
}

setup(zflag)
	int zflag;
{
	if ((kmemfd = open(core, zflag ? O_RDWR : O_RDONLY)) < 0) {
		fprintf (stderr, "%s: ", progname);
		perror ("kmem open");
		exit(1);
	}
	if(!memflg)
		return;

	if (nlist(vmunix, nl) < 0) {
		fprintf (stderr, "%s: ", progname);
		perror ("no namelist: nlist failed");
		exit(1);
	}
	if (nl[0].n_value == 0 && nl[1].n_value == 0 &&
	    nl[2].n_value == 0 && nl[3].n_value == 0) {
		fprintf (stderr, "%s: ", progname);
		perror ("no namelist");
		exit(1);
	}
}

cr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nClient rpc:\n");
	fprintf(stdout,
	 "calls      badcalls   retrans    badxid     timeout    wait       newcred\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d%-11d%-11d\n",
	    rcstat.rccalls,
	    rcstat.rcbadcalls,
	    rcstat.rcretrans,
	    rcstat.rcbadxids,
	    rcstat.rctimeouts,
	    rcstat.rcwaits,
	    rcstat.rcnewcreds);
	if (zflag) {
		memset((char *)&rcstat, 0, sizeof rcstat);
	}
}

sr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer rpc:\n");
	fprintf(stdout,
	    "calls      badcalls   nullrecv   badlen     xdrcall\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d\n",
	   rsstat.rscalls,
	   rsstat.rsbadcalls,
	   rsstat.rsnullrecv,
	   rsstat.rsbadlen,
	   rsstat.rsxdrcall);
	if (zflag) {
		memset((char *)&rsstat, 0, sizeof rsstat);
	}
}

#define RFS_NPROC       19
char *nfsstr[RFS_NPROC] = {
	"null",
	"getattr",
	"setattr",
	"root",
	"lookup",
	"readlink",
	"read",
	"wrcache",
	"write",
	"create",
	"remove",
	"rename",
	"link",
	"symlink",
	"mkdir",
	"rmdir",
	"readdir",
	"fsstat",
	"access" };

cn_print(zflag)
	int zflag;
{
	int i;

	fprintf(stdout, "\nClient nfs:\n");
	fprintf(stdout,
	    "calls      badcalls   nclget     nclsleep\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d\n",
	    clstat.ncalls,
	    clstat.nbadcalls,
	    clstat.nclgets,
	    clstat.nclsleeps);
	req_print((int *)clstat.reqs, clstat.ncalls);
	if (zflag) {
		memset((char *)&clstat, 0, sizeof clstat);
	}
}

sn_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer nfs:\n");
	fprintf(stdout, "calls      badcalls\n");
	fprintf(stdout, "%-11d%-11d\n", svstat.ncalls, svstat.nbadcalls);
	req_print((int *)svstat.reqs, svstat.ncalls);
	if (zflag) {
		memset((char *)&svstat, 0, sizeof svstat);
	}
}

#define min(a,b)	((a) < (b) ? (a) : (b))
req_print(req, tot)
	int	*req;
	int	tot;
{
	int	i, j;
	char	fixlen[128];

	for (i=0; i<=RFS_NPROC / 7; i++) {
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			fprintf(stdout, "%-11s", nfsstr[j]);
		}
		fprintf(stdout, "\n");
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			if (tot) {
				sprintf(fixlen,
				    "%d %2d%% ", req[j], (req[j]*100)/tot);
			} else {
				sprintf(fixlen, "%d 0%% ", req[j]);
			}
			fprintf(stdout, "%-11s", fixlen);
		}
		fprintf(stdout, "\n");
	}
}

usage()
{
	fprintf(stderr, "nfsstat [-cnrsz] [unix] [core]\n");
	exit(1);
}

int
ioinfo(index, buf, buflen, rw)
int index, rw;
void *buf;
size_t buflen;
{

	struct mioc_rksym rks;


	if(memflg) {
		if(nl[index].n_value == 0)
			return(-1);
		if (rw == MIOC_READKSYM) {
			if (lseek(kmemfd, nl[index].n_value, 0) != nl[index].n_value ||
		    	read(kmemfd, buf, buflen) != buflen) {
				fprintf(stderr, "%s: ", progname);
				perror("kernel seek/read error");
				exit(1);
			}
		} else { 
			if (lseek(kmemfd, nl[index].n_value, 0) != nl[index].n_value ||
		    	write(kmemfd, buf, buflen) != buflen) {
				fprintf(stderr, "%s: ", progname);
				perror("kernel seek/write error");
				exit(1);
			}
		}
		return(0);
	}
	else {
		rks.mirk_symname = nl[index].n_name;
		rks.mirk_buf = buf;
		rks.mirk_buflen = buflen;
		return(ioctl(kmemfd, rw, &rks));
	}
}
