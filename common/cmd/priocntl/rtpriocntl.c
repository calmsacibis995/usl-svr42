/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)priocntl:common/cmd/priocntl/rtpriocntl.c	1.7.12.6"
#ident  "$Header: rtpriocntl.c 1.2 91/06/27 $"

/***************************************************************************
 * Command: RTpriocntl
 *
 * Inheritable Privileges: P_OWNER,P_RTIME,P_MACREAD,P_MACWRITE
 *       Fixed Privileges: None
 *
 * Notes: This file contains the class specific code implementing
 * 	  the real-time priocntl sub-command.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>
#include	<sys/rtpriocntl.h>
#include	<sys/param.h>
#include	<sys/hrtcntl.h>
#include	<limits.h>
#include	<errno.h>
#include	<priv.h>
#include	<sys/secsys.h>
#include	"priocntl.h"
#include	<pfmt.h>
#include	<locale.h>

#define RT_TQINF_STRING	"RT_TQINF"

static void	print_rtinfo(), print_rtprocs(), set_rtprocs(), exec_rtcmd();

static char usage[] =
	":1039:Usage:	priocntl -l\n\tpriocntl -d [-i idtype] [idlist]\n\tpriocntl -s -c RT [-p rtpri] [-t tqntm [-r res]] [-i idtype] [idlist]\n\tpriocntl -e -c RT [-p rtpri] [-t tqntm [-r res]] command [argument(s)]\n";

static char
	badclid[] = ":1040:Cannot get RT class ID, errno = %d\n",
	badquantum[] = ":1041:Invalid time quantum; time quantum must be positive\n",
	badrange[] = ":1042:Specified real time priority %d out of configured range\n",
	badres[] = ":1043:Invalid resolution; resolution must be between 1 and 1,000,000,000\n",
	badreset[] = ":1044:Cannot reset real time parameters, errno = %d\n";
	
static char	cmdpath[256];

/*
 * Procedure: main - process options & call appropriate subroutines.
 */
main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;
	extern int	optind;

	int		c;
	int		lflag, dflag, sflag, pflag, tflag, rflag, eflag, iflag;
	short		rtpri;
	long		tqntm;
	long		res;
	char		*idtypnm;
	idtype_t	idtype;
	int		idargc;

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	setlabel("UX:priocntl");
	
	strcpy(cmdpath, argv[0]);
	lflag = dflag = sflag = pflag = tflag = rflag = eflag = iflag = 0;
	while ((c = getopt(argc, argv, "ldsp:t:r:ec:i:")) != -1) {
		switch(c) {

		case 'l':
			lflag++;
			break;

		case 'd':
			dflag++;
			break;

		case 's':
			sflag++;
			break;

		case 'p':
			pflag++;
			rtpri = (short)atoi(optarg);
			break;

		case 't':
			tflag++;
			tqntm = atol(optarg);
			break;

		case 'r':
			rflag++;
			res = atol(optarg);
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			if (strcmp(optarg, "RT") != 0) {
				pfmt(stderr, MM_ERROR, ":1045:%s executed for %s class, %s is actually sub-command for RT class\n", cmdpath, optarg, cmdpath);
				exit(1);
			}
			break;

		case 'i':
			iflag++;
			idtypnm = optarg;
			break;

		case '?':
			pfmt(stderr, MM_ERROR, usage);
			exit(1);

		}
	}

	if (pflag && rtpri < 0) {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}

	if (lflag) {
		if (dflag || sflag || pflag || tflag || rflag || eflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_rtinfo();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || pflag || tflag || rflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_rtprocs();
		exit(0);

	} else if (sflag) {
		if (lflag || dflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (iflag) {
			if (str2idtyp(idtypnm, &idtype) == -1) {
				pfmt(stderr, MM_ERROR, ":1024:Bad idtype %s\n",idtypnm);
				exit(1);
			}
		} else
			idtype = P_PID;

		if (pflag == 0)
			rtpri = RT_NOCHANGE;

		if (tflag == 0)
			tqntm = RT_NOCHANGE;
		else if (tqntm < 1) {
			pfmt(stderr, MM_ERROR, badquantum);
			exit(1);
		}
		if (rflag == 0)
			res = 1000;

		if (optind < argc)
			idargc = argc - optind;
		else
			idargc = 0;

		set_rtprocs(idtype, idargc, &argv[optind], rtpri, tqntm, res);
		exit(0);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (pflag == 0)
			rtpri = RT_NOCHANGE;

		if (tflag == 0)
			tqntm = RT_NOCHANGE;
		else if (tqntm < 1) {
			pfmt(stderr, MM_ERROR, badquantum);
			exit(1);
		}
		if (rflag == 0)
			res = 1000;

		exec_rtcmd(&argv[optind], rtpri, tqntm, res);

	} else {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}
	/* NOTREACHED */
}


/*
 * Procedure:     print_rtinfo
 *
 * Notes: Print our class name and the maximum configured real-time
 *	  priority. 
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_rtinfo()
{
	pcinfo_t	pcinfo;

	strcpy(pcinfo.pc_clname, "RT");

	pfmt(stdout, MM_NOSTD, ":1046:RT (Real Time)\n");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, ":1047:\tCannot get maximum configured RT priority\n");
		exit(1);
	}
	pfmt(stdout, MM_NOSTD, ":1048:\tMaximum Configured RT Priority: %d\n", 
		((rtinfo_t *)pcinfo.pc_clinfo)->rt_maxpri);
}



/*
 * Procedure:     print_rtprocs
 * 
 * Notes: Read a list of pids from stdin and print the real-time
 *	  priority and time quantum (in millisecond resolution) for
 *	  each of the corresponding processes.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_rtprocs()
{
	pid_t		pidlist[NPIDS];
	int		numread;
	int		i;
	id_t		rtcid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	ulong		tqsecs;
	long		tqnsecs;

	numread = fread(pidlist, sizeof(pid_t), NPIDS, stdin);

	pfmt(stdout, MM_NOSTD, ":1049:REAL TIME PROCESSES:\n    PID    RTPRI       TQNTM\n");

	strcpy(pcinfo.pc_clname, "RT");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	rtcid = pcinfo.pc_cid;

	if (numread <= 0) {
		pfmt(stderr, MM_ERROR, ":1050:No pids on input\n");
		exit(1);
	}
	pcparms.pc_cid = PC_CLNULL;
	for (i = 0; i < numread; i++) {
		printf("%7ld", pidlist[i]);
		if (priocntl(P_PID, pidlist[i], PC_GETPARMS, &pcparms) == -1) {
			pfmt(stdout, MM_WARNING, ":1051:\tCannot get real time parameters\n");
			continue;
		}

		if (pcparms.pc_cid == rtcid) {
			printf("   %5d", ((rtparms_t *)pcparms.pc_clparms)->rt_pri);
			tqsecs = ((rtparms_t *)pcparms.pc_clparms)->rt_tqsecs;
			tqnsecs = ((rtparms_t *)pcparms.pc_clparms)->rt_tqnsecs;
			if (tqsecs > LONG_MAX / 1000 - 1)
			pfmt(stdout, MM_WARNING, ":1052:    Time quantum too large to express in millisecond resolution.\n");
			else {
				if (tqnsecs == RT_TQINF)
					printf(" %11s\n", RT_TQINF_STRING);
				else
					printf(" %11ld\n", tqsecs * 1000 + tqnsecs / 1000000);
			}
		} else {

			/*
			 * Process from some class other than real time.
			 * It has probably changed class while priocntl
			 * command was executing (otherwise we wouldn't
			 * have been passed its pid).  Print the little
			 * we know about it.
			 */
			pcinfo.pc_cid = pcparms.pc_cid;
			if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) != -1)
			pfmt(stdout, MM_WARNING, ":1053:%ld\tChanged to class %s while priocntl command executing\n", pidlist[i], pcinfo.pc_clname);

		}
	}
}



/*
 * Procedure:     set_rtprocs
 *
 * Notes: Set all processes in the set specified by idtype/idargv to
 *	  real time (if they aren't already real time) and set their
 *	  real-time priority and quantum to those specified by rtpri
 *	  and tqntm/res.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_rtprocs(idtype, idargc, idargv, rtpri, tqntm, res)
idtype_t	idtype;
int		idargc;
char		**idargv;
short		rtpri;
long		tqntm;
long		res;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxrtpri;
	hrtime_t	hrtime;
	char		idtypnm[12];
	int		i;
	id_t		rtcid;


	/*
	 * Get the real time class ID and max configured RT priority.
	 */
	strcpy(pcinfo.pc_clname, "RT");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxrtpri = ((rtinfo_t *)pcinfo.pc_clinfo)->rt_maxpri;

	/*
	 * Validate the rtpri and res arguments.
	 */
	if ((rtpri > maxrtpri || rtpri < 0) && rtpri != RT_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange, rtpri);
		exit(1);
	}
	if (res > 1000000000 || res < 1) {
		pfmt(stderr, MM_ERROR, badres);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((rtparms_t *)pcparms.pc_clparms)->rt_pri = rtpri;
	if (tqntm == RT_NOCHANGE) {
		((rtparms_t *)pcparms.pc_clparms)->rt_tqnsecs = RT_NOCHANGE;
	} else {
		hrtime.hrt_secs = 0;
		hrtime.hrt_rem = tqntm;
		hrtime.hrt_res = res;
		if (_hrtnewres(&hrtime, NANOSEC, HRT_RNDUP) == -1) {
			pfmt(stderr, MM_ERROR, ":1054:Cannot convert resolution.\n");
			exit(1);
		}
		((rtparms_t *)pcparms.pc_clparms)->rt_tqsecs = hrtime.hrt_secs;
		((rtparms_t *)pcparms.pc_clparms)->rt_tqnsecs = hrtime.hrt_rem;
	}

	if (idtype == P_ALL) {
		if (priocntl(P_ALL, 0, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM)
				pfmt(stderr, MM_ERROR, ":1055:Permissions error encountered on one or more processes.\n");
			else {
				pfmt(stderr, MM_ERROR, badreset, errno);
				exit(1);
			}
		}
	} else if (idargc == 0) {
		if (priocntl(idtype, P_MYID, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM) {
				(void)idtyp2str(idtype, idtypnm);
				pfmt(stderr, MM_ERROR, ":1056:Permissions error encountered on current %s.\n", idtypnm);
			} else {
				pfmt(stderr, MM_ERROR, badreset, errno);
				exit(1);
			}
		}
	} else {
		(void)idtyp2str(idtype, idtypnm);
		for (i = 0; i < idargc; i++) {
			if ( idtype == P_CID ) {
				rtcid = clname2cid(idargv[i]);
				if (priocntl(idtype, rtcid, PC_SETPARMS, &pcparms) == -1) {
					if (errno == EPERM)
						pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
					else {
						pfmt(stderr, MM_ERROR, badreset, errno);
						exit(1);
					}
				}
			} else if (priocntl(idtype, (id_t)atol(idargv[i]),
			    PC_SETPARMS, &pcparms) == -1) {
				if (errno == EPERM)
					pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
				else {
					pfmt(stderr, MM_ERROR, badreset, errno);
					exit(1);
				}
			}
		}
	}
}


/*
 * Procedure:     exec_rtcmd
 *
 * Notes: Execute the command pointed to by cmdargv as a real-time
 *	  process with real time priority rtpri and quantum tqntm/res.
 *
 * Restrictions:  priocntl(2): <none>
 *                execvp(2): P_MACREAD
 */
static void
exec_rtcmd(cmdargv, rtpri, tqntm, res)
char	**cmdargv;
short	rtpri;
long	tqntm;
long	res;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxrtpri;
	hrtime_t	hrtime;
	uid_t		priv_id;	/* hold privileged ID if any */
	
	/*
	 * Get the real time class ID and max configured RT priority.
	 */
	strcpy(pcinfo.pc_clname, "RT");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxrtpri = ((rtinfo_t *)pcinfo.pc_clinfo)->rt_maxpri;

	if ((rtpri > maxrtpri || rtpri < 0) && rtpri != RT_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange, rtpri);
		exit(1);
	}
	if (res > 1000000000 || res < 1) {
		pfmt(stderr, MM_ERROR, badres);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((rtparms_t *)pcparms.pc_clparms)->rt_pri = rtpri;
	if (tqntm == RT_NOCHANGE) {
		((rtparms_t *)pcparms.pc_clparms)->rt_tqnsecs = RT_NOCHANGE;
	} else {
		hrtime.hrt_secs = 0;
		hrtime.hrt_rem = tqntm;
		hrtime.hrt_res = res;
		if (_hrtnewres(&hrtime, NANOSEC, HRT_RNDUP) == -1) {
			pfmt(stderr, MM_ERROR, ":1054:Cannot convert resolution.\n");
			exit(1);
		}
		((rtparms_t *)pcparms.pc_clparms)->rt_tqsecs = hrtime.hrt_secs;
		((rtparms_t *)pcparms.pc_clparms)->rt_tqnsecs = hrtime.hrt_rem;
	}
	
	if (priocntl(P_PID, P_MYID, PC_SETPARMS, &pcparms) == -1) {
		pfmt(stderr, MM_ERROR, badreset, errno);
		exit(1);
	}

	/*
	 * Check if the current system is privileged ID based or 
	 * not by calling secsys().
	 *
	 * If this is NOT a privileged ID based system i.e (this is lpm), then
	 * clear maximum privileges before executing specified
	 * command.  If the specified command needs privilege, it
	 * should use tfadmin(1M).
	 *
	 * If this IS a privileged ID based system, then
	 * check if the current UID is equal to the privileged ID.
	 * If current UID is not equal to the privileged ID, then 
	 * also clear all privileges.
	 */
	
	{
	int privid;
  privid = secsys(ES_PRVID, 0);

  if (privid < 0 ||  privid != getuid()) 
    procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
  }

	(void) execvp(cmdargv[0], cmdargv);
	pfmt(stderr, MM_ERROR, ":1058:Cannot execute %s, errno = %d\n", cmdargv[0], errno);
	exit(1);
}
