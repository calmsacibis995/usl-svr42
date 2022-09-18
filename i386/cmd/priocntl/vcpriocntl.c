/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)priocntl:i386/cmd/priocntl/vcpriocntl.c	1.1"
#ident	"$Header: $"

#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>
#include	<sys/vcpriocntl.h>
#include	<errno.h>

#include	"priocntl.h"

/*
 * This file contains the class specific code implementing
 * the time-sharing priocntl sub-command.
 */

#define BASENMSZ	16

static void	print_vcinfo(), print_vcprocs(), set_vcprocs(), exec_vccmd();

static char usage[] =
"usage:	priocntl -l\n\
	priocntl -d [-d idtype] [idlist]\n\
	priocntl -s [-c VC] [-m vcuprilim] [-p vcupri] [-i idtype] [idlist]\n\
	priocntl -e [-c VC] [-m vcuprilim] [-p vcupri] command [argument(s)]\n";

static char	cmdpath[256];
static char	basenm[BASENMSZ];


main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;
	extern int	optind;

	int		c;
	int		lflag, dflag, sflag, mflag, pflag, eflag, iflag;
	short		vcuprilim;
	short		vcupri;
	char		*idtypnm;
	idtype_t	idtype;
	int		idargc;

	strcpy(cmdpath, argv[0]);
	strcpy(basenm, basename(argv[0]));
	lflag = dflag = sflag = mflag = pflag = eflag = iflag = 0;
	while ((c = getopt(argc, argv, "ldsm:p:ec:i:")) != -1) {
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

		case 'm':
			mflag++;
			vcuprilim = (short)atoi(optarg);
			break;

		case 'p':
			pflag++;
			vcupri = (short)atoi(optarg);
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			if (strcmp(optarg, "VC") != 0) {
				fprintf(stderr,"error: %s executed for %s class, \
%s is actually sub-command for VC class\n", cmdpath, optarg, cmdpath);
				exit(1);
			}
			break;

		case 'i':
			iflag++;
			idtypnm = optarg;
			break;

		case '?':
			fprintf(stderr, usage);
			exit(1);

		default:
			break;
		}
	}

	if (lflag) {
		if (dflag || sflag || mflag || pflag || eflag || iflag) {
			fprintf(stderr,usage);
			exit(1);
		}

		print_vcinfo();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || mflag || pflag || eflag) {
			fprintf(stderr,usage);
			exit(1);
		}

		print_vcprocs();
		exit(0);

	} else if (sflag) {
		if (lflag || dflag || eflag) {
			fprintf(stderr,usage);
			exit(1);
		}
	
		if (iflag) {
			if (str2idtyp(idtypnm, &idtype) == -1) {
				fprintf(stderr,"%s: Bad idtype %s\n", basenm,
				    idtypnm);
				exit(1);
			}
		} else
			idtype = P_PID;

		if (mflag == 0)
			vcuprilim = VC_NOCHANGE;

		if (pflag == 0)
			vcupri = VC_NOCHANGE;

		if (optind < argc)
			idargc = argc - optind;
		else
			idargc = 0;

		set_vcprocs(idtype, idargc, &argv[optind], vcuprilim, vcupri);
		exit(0);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag) {
			fprintf(stderr,usage);
			exit(1);
		}

		if (mflag == 0)
			vcuprilim = VC_NOCHANGE;

		if (pflag == 0)
			vcupri = VC_NOCHANGE;

		exec_vccmd(&argv[optind], vcuprilim, vcupri);

	} else {
		fprintf(stderr,usage);
		exit(1);
	}
}


/*
 * Print our class name and the configured user priority range.
 */
static void
print_vcinfo()
{
	pcinfo_t	pcinfo;

	strcpy(pcinfo.pc_clname, "VC");

	printf("VC (VP/ix-like class)\n");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		fprintf(stderr,"\tCan't get configured VC user priority range\n");
		exit(1);
	}

	printf("\tConfigured VC User Priority Range: -%d through %d\n",
	    ((vcinfo_t *)pcinfo.pc_clinfo)->vc_maxupri,
	    ((vcinfo_t *)pcinfo.pc_clinfo)->vc_maxupri);
}


/*
 * Read a list of pids from stdin and print the user priority and user
 * priority limit for each of the corresponding processes.
 */
static void
print_vcprocs()
{
	pid_t		pidlist[NPIDS];
	int		numread;
	int		i;
	id_t		vccid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;

	numread = fread(pidlist, sizeof(pid_t), NPIDS, stdin);

	printf("TIME SHARING PROCESSES:\n    PID    VCUPRILIM    VCUPRI\n");

	strcpy(pcinfo.pc_clname, "VC");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		fprintf(stderr,"%s: Can't get VC class ID\n", basenm);
		exit(1);
	}

	vccid = pcinfo.pc_cid;

	if (numread <= 0) {
		fprintf(stderr,"%s: No pids on input\n", basenm);
		exit(1);
	}


	pcparms.pc_cid = PC_CLNULL;
	for (i = 0; i < numread; i++) {
		printf("%7ld", pidlist[i]);
		if (priocntl(P_PID, pidlist[i], PC_GETPARMS, &pcparms) == -1) {
			printf("\tCan't get VC user priority\n");
			continue;
		}

		if (pcparms.pc_cid == vccid) {
			printf("    %5d       %5d\n",
			    ((vcparms_t *)pcparms.pc_clparms)->vc_uprilim,
			    ((vcparms_t *)pcparms.pc_clparms)->vc_upri);
		} else {

			/*
			 * Process from some class other than time sharing.
			 * It has probably changed class while priocntl
			 * command was executing (otherwise we wouldn't
			 * have been passed ivc pid).  Print the little
			 * we know about it.
			 */
			pcinfo.pc_cid = pcparms.pc_cid;
			if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) != -1)
				printf("%ld\tChanged to class %s while priocntl \
command executing\n", pidlist[i], pcinfo.pc_clname);

		}
	}
}


/*
 * Set all processes in the set specified by idtype/idargv to time-sharing
 * (if they aren't already time-sharing) and set their user priority limit
 * and user priority to those specified by vcuprilim and vcupri.
 */
static void
set_vcprocs(idtype, idargc, idargv, vcuprilim, vcupri)
idtype_t	idtype;
int		idargc;
char		**idargv;
short		vcuprilim;
short		vcupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;
	char		idtypnm[12];
	int		i;
	id_t		vccid;

	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (vcuprilim == VC_NOCHANGE && vcupri == VC_NOCHANGE)
		vcuprilim = vcupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "VC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		fprintf(stderr,"%s: Can't get VC class ID, priocntl system call \
failed with errno %d\n", basenm, errno);
		exit(1);
	}
	maxupri = ((vcinfo_t *)pcinfo.pc_clinfo)->vc_maxupri;

	/*
	 * Validate the vcuprilim and vcupri argumenvc.
	 */
	if ((vcuprilim > maxupri || vcuprilim < -maxupri) &&
	    vcuprilim != VC_NOCHANGE) {
		fprintf(stderr,"%s: Specified user priority limit %d out of \
configured range\n", basenm, vcuprilim);
		exit(1);
	}

	if ((vcupri > maxupri || vcupri < -maxupri) &&
	    vcupri != VC_NOCHANGE) {
		fprintf(stderr,"%s: Specified user priority %d out of \
configured range\n", basenm, vcupri);
		exit(1);
	}

	pcparms.pc_cid = pcinfo.pc_cid;
	((vcparms_t *)pcparms.pc_clparms)->vc_uprilim = vcuprilim;
	((vcparms_t *)pcparms.pc_clparms)->vc_upri = vcupri;

	if (idtype == P_ALL) {
		if (priocntl(P_ALL, 0, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM)
				fprintf(stderr, "Permissions error \
encountered on one or more processes.\n");
			else {
				fprintf(stderr,"%s: Can't reset time sharing \
parameters\npriocntl system call failed with errno %d\n", basenm, errno);
				exit(1);
			}
		}
	} else if (idargc == 0) {
		if (priocntl(idtype, P_MYID, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM) {
				(void)idtyp2str(idtype, idtypnm);
				fprintf(stderr, "Permissions error \
encountered on current %s.\n", idtypnm);
			} else {
				fprintf(stderr,"%s: Can't reset time sharing \
parameters\npriocntl system call failed with errno %d\n", basenm, errno);
				exit(1);
			}
		}
	} else {
		(void)idtyp2str(idtype, idtypnm);
		for (i = 0; i < idargc; i++) {
			if ( idtype == P_CID ) {
				vccid = clname2cid(idargv[i]);
				if (priocntl(idtype, vccid, PC_SETPARMS, &pcparms) == -1) {
					if (errno == EPERM)
						fprintf(stderr, "Permissions error \
encountered on %s %s.\n", idtypnm, idargv[i]);
					else {
						fprintf(stderr,"%s: Can't reset \
time sharing parameters\npriocntl system call failed with errno %d\n", basenm, errno);
						exit(1);
					}
				}
			} else if (priocntl(idtype, (id_t)atol(idargv[i]),
			    PC_SETPARMS, &pcparms) == -1) {
				if (errno == EPERM)
					fprintf(stderr, "Permissions error \
encountered on %s %s.\n", idtypnm, idargv[i]);
				else {
					fprintf(stderr,"%s: Can't reset time sharing \
parameters\npriocntl system call failed with errno %d\n", basenm, errno);
					exit(1);
				}
			}
		}
	}
		
}


/*
 * Execute the command pointed to by cmdargv as a time-sharing process
 * with the user priority limit given by vcuprilim and user priority vcupri.
 */
static void
exec_vccmd(cmdargv, vcuprilim, vcupri)
char	**cmdargv;
short	vcuprilim;
short	vcupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;

	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (vcuprilim == VC_NOCHANGE && vcupri == VC_NOCHANGE)
		vcuprilim = vcupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "VC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		fprintf(stderr,"%s: Can't get VC class ID, priocntl system call \
failed with errno %d\n", basenm, errno);
		exit(1);
	}
	maxupri = ((vcinfo_t *)pcinfo.pc_clinfo)->vc_maxupri;

	if ((vcuprilim > maxupri || vcuprilim < -maxupri) &&
	    vcuprilim != VC_NOCHANGE) {
		fprintf(stderr,"%s: Specified user priority limit %d out of \
configured range\n", basenm, vcuprilim);
		exit(1);
	}

	if ((vcupri > maxupri || vcupri < -maxupri) &&
	    vcupri != VC_NOCHANGE) {
		fprintf(stderr,"%s: Specified user priority %d out of \
configured range\n", basenm, vcupri);
		exit(1);
	}

	pcparms.pc_cid = pcinfo.pc_cid;
	((vcparms_t *)pcparms.pc_clparms)->vc_uprilim = vcuprilim;
	((vcparms_t *)pcparms.pc_clparms)->vc_upri = vcupri;
	if (priocntl(P_PID, P_MYID, PC_SETPARMS, &pcparms) == -1) {
		fprintf(stderr,"%s: Can't reset time sharing parameters\n\
priocntl system call failed with errno %d\n", basenm, errno);
		exit(1);
	}

	(void)execvp(cmdargv[0], cmdargv);
	fprintf(stderr,"%s: Can't execute %s, exec failed with errno %d\n",
	    basenm, cmdargv[0], errno);
	exit(1);
}
