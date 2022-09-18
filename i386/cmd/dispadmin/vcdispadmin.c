/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dispadmin:i386/cmd/dispadmin/vcdispadmin.c	1.2"
#ident	"$Header: $"

#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/priocntl.h>
#include	<sys/vcpriocntl.h>
#include	<sys/param.h>
#include	<sys/hrtcntl.h>
#include	<sys/vc.h>

/*
 * This file contains the class specific code implementing
 * the time-sharing dispadmin sub-command.
 */

#define	BASENMSZ	16

extern int	errno;
extern char	*basename();
extern void	fatalerr();
extern long	hrtconvert();

static void	get_vcdptbl(), set_vcdptbl();

static char usage[] =
"usage:	dispadmin -l\n\
	dispadmin -c VC -g [-r res]\n\
	dispadmin -c VC -s infile\n";

static char	basenm[BASENMSZ];
static char	cmdpath[256];


main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;

	int		c;
	int		lflag, gflag, rflag, sflag;
	ulong		res;
	char		*infile;

	strcpy(cmdpath, argv[0]);
	strcpy(basenm, basename(argv[0]));
	lflag = gflag = rflag = sflag = 0;
	while ((c = getopt(argc, argv, "lc:gr:s:")) != -1) {
		switch (c) {

		case 'l':
			lflag++;
			break;

		case 'c':
			if (strcmp(optarg, "VC") != 0)
				fatalerr("error: %s executed for %s class, \
%s is actually sub-command for VC class\n", cmdpath, optarg, cmdpath);
			break;

		case 'g':
			gflag++;
			break;

		case 'r':
			rflag++;
			res = strtoul(optarg, (char **)NULL, 10);
			break;

		case 's':
			sflag++;
			infile = optarg;
			break;

		case '?':
			fatalerr(usage);

		default:
			break;
		}
	}

	if (lflag) {
		if (gflag || rflag || sflag)
			fatalerr(usage);

		printf("VC\t(Time Sharing)\n");
		exit(0);

	} else if (gflag) {
		if (lflag || sflag)
			fatalerr(usage);

		if (rflag == 0)
			res = 1000;

		get_vcdptbl(res);
		exit(0);

	} else if (sflag) {
		if (lflag || gflag || rflag)
			fatalerr(usage);

		set_vcdptbl(infile);
		exit(0);

	} else {
		fatalerr(usage);
	}
}


/*
 * Retrieve the current vc_dptbl from memory, convert the time quantum
 * values to the resolution specified by res and write the table to stdout.
 */
static void
get_vcdptbl(res)
ulong	res;
{
	int		i;
	int		vcdpsz;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	vcadmin_t	vcadmin;
	vcdpent_t	*vc_dptbl;
	hrtime_t	hrtime;

	strcpy(pcinfo.pc_clname, "VC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get VC class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&vcadmin;
	vcadmin.vc_cmd = VC_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get vc_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	vcdpsz = vcadmin.vc_ndpents * sizeof(vcdpent_t);
	if ((vc_dptbl = (vcdpent_t *)malloc(vcdpsz)) == NULL)
		fatalerr("%s: Can't allocate memory for vc_dptbl\n", basenm);

	vcadmin.vc_dpents = vc_dptbl;

	vcadmin.vc_cmd = VC_GETDPTBL;
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get vc_dptbl, priocntl system call \
call failed with errno %d\n", basenm, errno);

	printf("# Time Sharing Dispatcher Configuration\n");
	printf("RES=%ld\n\n", res);
	printf("# vc_quantum  vc_tqexp  vc_slpret  vc_maxwait vc_lwait  \
PRIORITY LEVEL\n");

	for (i = 0; i < vcadmin.vc_ndpents; i++) {
		if (res != HZ) {
			hrtime.hrt_secs = 0;
			hrtime.hrt_rem = vc_dptbl[i].vc_quantum;
			hrtime.hrt_res = HZ;
			if (_hrtnewres(&hrtime, res, HRT_RNDUP) == -1)
				fatalerr("%s: Can't convert to requested \
resolution\n", basenm);
			if ((vc_dptbl[i].vc_quantum = hrtconvert(&hrtime))
			    == -1)
				fatalerr("%s: Can't express time quantum in \
requested resolution,\ntry coarser resolution\n", basenm);
		}
		printf("%10d%10d%10d%12d%10d        #   %3d\n",
		    vc_dptbl[i].vc_quantum, vc_dptbl[i].vc_tqexp,
		    vc_dptbl[i].vc_slpret, vc_dptbl[i].vc_maxwait,
		    vc_dptbl[i].vc_lwait, i);
	}
}


/*
 * Read the vc_dptbl values from infile, convert the time quantum values
 * to HZ resolution, do a little sanity checking and overwrite the table
 * in memory with the values from the file.
 */
static void
set_vcdptbl(infile)
char	*infile;
{
	int		i;
	int		nvcdpents;
	char		*tokp;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	vcadmin_t	vcadmin;
	vcdpent_t	*vc_dptbl;
	int		linenum;
	ulong		res;
	hrtime_t	hrtime;
	FILE		*fp;
	char		buf[512];
	int		wslength;

	strcpy(pcinfo.pc_clname, "VC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get VC class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&vcadmin;
	vcadmin.vc_cmd = VC_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get vc_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	nvcdpents = vcadmin.vc_ndpents;
	if ((vc_dptbl =
	    (vcdpent_t *)malloc(nvcdpents * sizeof(vcdpent_t))) == NULL)
		fatalerr("%s: Can't allocate memory for vc_dptbl\n", basenm);

	if ((fp = fopen(infile, "r")) == NULL)
		fatalerr("%s: Can't open %s for input\n", basenm, infile);

	linenum = 0;

	/*
	 * Find the first non-blank, non-comment line.  A comment line
	 * is any line with '#' as the first non-white-space character.
	 */
	do {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			fatalerr("%s: Too few lines in input table\n",basenm);
		linenum++;
	} while (buf[0] == '#' || buf[0] == '\0' ||
	    (wslength = strspn(buf, " \t\n")) == strlen(buf) ||
	    strchr(buf, '#') == buf + wslength);

	if ((tokp = strtok(buf, " \t")) == NULL)
		fatalerr("%s: Bad RES specification, line %d of input file\n",
		    basenm, linenum);
	if (strlen(tokp) > 4) {
		if (strncmp(tokp, "RES=", 4) != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (tokp[4] == '-')
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		res = strtoul(&tokp[4], (char **)NULL, 10);
	} else if (strlen(tokp) == 4) {
		if (strcmp(tokp, "RES=") != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if ((tokp = strtok(NULL, " \t")) == NULL)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (tokp[0] == '-')
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		res = strtoul(tokp, (char **)NULL, 10);
	} else if (strlen(tokp) == 3) {
		if (strcmp(tokp, "RES") != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if ((tokp = strtok(NULL, " \t")) == NULL)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (strlen(tokp) > 1) {
			if (strncmp(tokp, "=", 1) != 0)
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			if (tokp[1] == '-')
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			res = strtoul(&tokp[1], (char **)NULL, 10);
		} else if (strlen(tokp) == 1) {
			if ((tokp = strtok(NULL, " \t")) == NULL)
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			if (tokp[0] == '-')
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			res = strtoul(tokp, (char **)NULL, 10);
		}
	} else {
		fatalerr("%s: Bad RES specification, line %d of input file\n",
		    basenm, linenum);
	}

	/*
	 * The remainder of the input file should contain exactly enough
	 * non-blank, non-comment lines to fill the table (vc_ndpents lines).
	 * We assume that any non-blank, non-comment line is data for the
	 * table and fail if we find more or less than we need.
	 */
	for (i = 0; i < vcadmin.vc_ndpents; i++) {

		/*
		 * Get the next non-blank, non-comment line.
		 */
		do {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				fatalerr("%s: Too few lines in input table\n",
				    basenm);
			linenum++;
		} while (buf[0] == '#' || buf[0] == '\0' ||
		    (wslength = strspn(buf, " \t\n")) == strlen(buf) ||
		    strchr(buf, '#') == buf + wslength);

		if ((tokp = strtok(buf, " \t")) == NULL)
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);

		if (res != HZ) {
			hrtime.hrt_secs = 0;
			hrtime.hrt_rem = atol(tokp);
			hrtime.hrt_res = res;
			if (_hrtnewres(&hrtime, HZ, HRT_RNDUP) == -1)
				fatalerr("%s: Can't convert specified \
resolution to ticks\n", basenm);
			if((vc_dptbl[i].vc_quantum = hrtconvert(&hrtime)) == -1)
				fatalerr("%s: vc_quantum value out of \
valid range; line %d of input,\ntable not overwritten\n", basenm, linenum);
		} else {
			vc_dptbl[i].vc_quantum = atol(tokp);
		}
		if (vc_dptbl[i].vc_quantum <= 0)
			fatalerr("%s: vc_quantum value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) == NULL || tokp[0] == '#')
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);
		vc_dptbl[i].vc_tqexp = (short)atoi(tokp);
		if (vc_dptbl[i].vc_tqexp < 0 ||
		    vc_dptbl[i].vc_tqexp > vcadmin.vc_ndpents)
			fatalerr("%s: vc_tqexp value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) == NULL || tokp[0] == '#')
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);
		vc_dptbl[i].vc_slpret = (short)atoi(tokp);
		if (vc_dptbl[i].vc_slpret < 0 ||
		    vc_dptbl[i].vc_slpret > vcadmin.vc_ndpents)
			fatalerr("%s: vc_slpret value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) == NULL || tokp[0] == '#')
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);
		vc_dptbl[i].vc_maxwait = (short)atoi(tokp);
		if (vc_dptbl[i].vc_maxwait < 0)
			fatalerr("%s: vc_maxwait value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) == NULL || tokp[0] == '#')
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);
		vc_dptbl[i].vc_lwait = (short)atoi(tokp);
		if (vc_dptbl[i].vc_lwait < 0 ||
		    vc_dptbl[i].vc_lwait > vcadmin.vc_ndpents)
			fatalerr("%s: vc_lwait value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) != NULL && tokp[0] != '#')
			fatalerr("%s: Too many values, line %d of input file\n",
			    basenm, linenum);
	}

	/*
	 * We've read enough lines to fill the table.  We fail
	 * if the input file contains any more.
	 */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] != '#' && buf[0] != '\0' &&
		    (wslength = strspn(buf, " \t\n")) != strlen(buf) &&
		    strchr(buf, '#') != buf + wslength)
			fatalerr("%s: Too many lines in input table\n",
			    basenm);
	}

	vcadmin.vc_dpents = vc_dptbl;
	vcadmin.vc_cmd = VC_SETDPTBL;
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't set vc_dptbl, priocntl system call \
failed with errno %d\n", basenm, errno);
}
