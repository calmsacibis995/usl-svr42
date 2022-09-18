/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idcheck.c	1.5"
#ident	"$Header:"

/*
 * This command is used by Installable Drivers to return selected
 * information about the system configuration. The various forms are:
 *    "idcheck -p device-name [-R dir]"
 *    "idcheck -y device-name [-R dir]"
 *    "idcheck -v vector [-R dir] [-r]"
 *    "idcheck -d dma-channel [-R dir] [-r]"
 *    "idcheck -a -l lower_address -u upper_address [-R dir] [-r]"
 *    "idcheck -c -l lower_address -u upper_address [-R dir] [-r]"
 * This command scans sdevice and mdevice and returns:
 *    100 if an error occurs.
 *    0	  if no conflict exists.
 *    non-zero exit codes, detailed below, if there is a conflict.
 * Options:
 * '-r' report name of conflicting device on stdout.
 * '-p' returns a value from 1 to 31 if the package exists.
 *	The exit code is calculated from the following.
 * 	add 1 if the directory '/etc/conf/pack.d/DSP' exists.
 *	add 2 if an entry can be found in mdevice.
 *	add 4 if the System Module is in '/etc/conf/sdevice.d'.
 *	add 8 if an entry can be found in sdevice.
 *	add 16 if a Driver.o file can be found for the device.
 * '-y' returns a 1 if the specified device exists in sdevice, and the
 *      "Configure" field contains a 'Y'. If the device "Configure" field
 *      is 'N' a 0 is returned. (change made 1/27/88).
 * '-v' returns 'itype' field of device that is using the vector specified.
 *	(i.e. Non-zero means another DSP is already using the vector.)
 * '-d' returns non-zero if the dma channel specified is being used.
 * '-a' returns non-zero if the IOA region bounded by (lower, upper)
 *	conflicts with another device.
 *	The exit code is calculated from the following.
 *	add 1 if a device using this IOA exists and does not
 *		contain the 'O' option in the flags field of mdevice.
 *	add 2 if a device using this IOA exists and does
 *		contain the 'O' option in the flags field of mdevice.
 * '-c' returns 1 if the CMA region bounded by (lower, upper) conflicts
 *	with another device.
 * '-l' address - lower bound of adress range specified in hex.
 * '-u' address - upper bound of address range specified in hex.
 *	The leading 0x is unnecessary for the '-l' and '-u' options.
 * '-R' dir - specifies the configuration "root" directory (/etc/conf).
 *
 * Note: If the device's sdevice entry contains an 'N' (device not
 *       configured), the entire line is ignored when doing the -v, -d
 *       -a, and -c checks. (change made 1/27/88).
 */

#include "inst.h"
#include "defines.h"
#include "mdep.h"
#include <ctype.h>
#include <sys/stat.h>

/* diagnostics */
#define OPT	0
#define SYS	1
#define MAST	2

/* messages */
#define	IONEOF	1
#define IUSAGE	2

int search_type;  /* fields or elements being searched */
#define DSP	1	/* check if DSP exists				*/
#define YN	2	/* check if DSP sdevice conf field == 'Y'	*/
#define VECTOR	3	/* search for identical interrupt vector	*/
#define	IOA	4	/* search for overlapping I/O addresses		*/
#define CMA	5	/* search for overlapping memory addresses	*/
#define DMA	6	/* search for identical DMA controller		*/

#define SET_SEARCH_TYPE(typ) \
	if (search_type != 0) \
		message(IONEOF); \
	search_type = (typ)

/* search return codes */
#define IERROR		-1
#define IEOF		-2
#define IFOUND		-3

/* exit codes */
#define EERROR		100
#define ENOCONFLICT	0
#define	EEXIST		1

int verbose;
int uflag, lflag, iflag, Rflag, debug;
char buf[LINESZ];
struct sdev sys, sd_check;
struct mdev mas;
struct stat st_stat;
extern char *optarg;
extern char pathinst[];
extern char instroot[];
extern int noloadable;

main(argc, argv)
int argc;
char *argv[];
{
	int s, m;
	int c, rc, p, f, d_o;

	while ((c = getopt(argc, argv, "#?rp:y:v:i:R:acl:u:d:")) != EOF)
		switch(c) {
		case 'p':
			SET_SEARCH_TYPE(DSP);
			sprintf(buf, "%%%ds", NAMESZ - 1);
			sscanf(optarg, buf, sd_check.name);
			break;
		case 'v':
			SET_SEARCH_TYPE(VECTOR);
			sscanf(optarg, "%hd", &sd_check.vector);
			break;
		case 'd':
			SET_SEARCH_TYPE(DMA);
			sscanf(optarg, "%hd", &sd_check.dmachan);
			break;
		case 'y':
			SET_SEARCH_TYPE(YN);
			sprintf(buf, "%%%ds", NAMESZ - 1);
			sscanf(optarg, buf, sd_check.name);
			break;
		case 'r':
			verbose++;
			break;
		case 'a':
			SET_SEARCH_TYPE(IOA);
			break;
		case 'c':
			SET_SEARCH_TYPE(CMA);
			break;
		case 'l':
			lflag++;
			sscanf(optarg, "%lx", &sd_check.sioa);
			sd_check.scma = sd_check.sioa;
			break;
		case 'u':
			uflag++;
			sscanf(optarg, "%lx", &sd_check.eioa);
			sd_check.ecma = sd_check.eioa;
			break;
		case 'i':
			iflag++;
			strcpy(pathinst, optarg);
			sprintf(instroot, "%s/..", pathinst);
			break;
		case 'R':
			Rflag++;
			strcpy(instroot, optarg);
			sprintf(pathinst, "%s/%s", instroot, CFDIR);
			break;
		case '?':
			message(IUSAGE);
		case '#':
			debug++;
			break;
		}

	if ((search_type == IOA || search_type == CMA) &&
	    (!lflag || !uflag))
		message(IUSAGE);

	if (iflag && Rflag)
		message(IUSAGE);

	if (debug)
		diag(OPT, 0);

	noloadable = 1;

	switch (search_type) {
	case VECTOR:
	case CMA:
	case IOA:
	case DMA:
		s = sdev_search(SDEV_D);
		if (s == IEOF)
			exit( ENOCONFLICT );
		if (s == IERROR)
			exit( EERROR );
		prdev(sys.name);
		exit(s == IFOUND ? EEXIST : s);

	case YN:
		s = sdev_search(SDEV_D);
		if (s == IFOUND)
			exit( EEXIST );
		else if (s == IEOF)
			exit( ENOCONFLICT );
		else
			exit( EERROR );

	case DSP:
		s = sdev_search(SDEV);
		m = mdev_search(sd_check.name);
		if (s == IERROR || m == IERROR)
			exit( EERROR );

		/* check if DSP exists under /etc/conf/pack.d */
		sprintf(buf, "%s/pack.d/%s", instroot, sd_check.name);
		p = (stat(buf, &st_stat) == 0);

		/* check if Driver.o Module exists under /etc/conf/pack.d */
		strcat(buf, "/Driver.o");
		d_o = (stat(buf, &st_stat) == 0);

		/* check if System Module exists under /etc/conf/sdevice.d */
		sprintf(buf, "%s/sdevice.d/%s", instroot, sd_check.name);
		f = (stat(buf, &st_stat) == 0);

		rc = p ? 1 : 0;
		rc = (m == IFOUND) ? rc + 2 : rc;
		rc = f ? rc + 4 : rc;
		rc = (s == IFOUND) ? rc + 8 : rc;
		rc = d_o ? rc + 16 : rc;

		/* In case ENOCONFLICT not 0 */
		if (rc == 0)
			exit( ENOCONFLICT );
		exit(rc);
	}
}



/* search System file */

sdev_search(sdevtype)
int sdevtype;
{
	register int rc = IEOF, r;
	struct sdev sav_sys;

	if (getinst(sdevtype, RESET, &sys) == -1)
		return(IERROR);

	for (;;) {

		r = getinst(sdevtype, NEXT, &sys);
		if (debug)
			diag(SYS, r);
		switch (r) {

		default:		/* getinst error */
			return(IERROR);

		case 0:			/* getinst EOF */
			if (rc != IEOF)
				sys = sav_sys;
			return(rc);

		case 1:
			switch (search_type) {

			case DSP:
			case YN:
				if (strcmp(sd_check.name, sys.name) == 0 &&
				    (search_type != YN || sys.conf == 'Y'))
					return(IFOUND);
				break;

			case VECTOR:
				if (sys.conf == 'N')
					break;
				if (sd_check.vector == sys.vector &&
				    sys.itype > 0)
					return(sys.itype);
				break;

			case IOA:
				if (sys.conf == 'N' || sys.eioa == 0)
					break;
				if (OVERLAP(sd_check.sioa, sd_check.eioa,
					    sys.sioa, sys.eioa)) {
					/* check if overlapping IOA is OK
					 * for this device */
					switch (mdev_search(sys.name)) {
					case IFOUND:
						if (!IO_OVERLAP_OK(&mas))
							return(1);
						break;
					default:
						return(IERROR);
					}
					rc = 1;
					sav_sys = sys;
				}
				break;

			case CMA:
				if (sys.conf == 'N' || sys.ecma == 0)
					break;
				if (OVERLAP(sd_check.scma, sd_check.ecma,
					    sys.scma, sys.ecma))
					return(IFOUND);
				break;

			case DMA:
				if (sys.conf == 'N')
					break;
				if (sd_check.dmachan == sys.dmachan)
					return(IFOUND);
				break;

			}	/* end of inner switch */

			break;	/* end default case of outer switch */
		}		/* end of outer switch */
	}			/* end of loop */
}



/* search Master file */

mdev_search(name)
char *name;
{
	register int r;

	r = getinst(MDEV_D, name, &mas);

	if (debug)
		diag(MAST, r);

	switch (r) {

	default:		/* getinst error */
		return(IERROR);

	case 0:			/* getinst EOF */
		return(IEOF);

	case 1:
		return(IFOUND);
	}
}



/* print error messages */

message(n)
int n;
{
	fprintf(stderr, "idcheck: ");

	switch(n) {
	case IONEOF:
		fprintf(stderr,
			"Only one of -p, -y, -v, -d, -a or -c allowed\n");
		break;
	case IUSAGE:
		fprintf(stderr, "Usage: idcheck [-r] [-R dir]\n");
		fprintf(stderr, "\t\t\t[-p module_name]\n");
		fprintf(stderr, "\t\t\t[-y module_name]\n");
		fprintf(stderr, "\t\t\t[-v vector]\n");
		fprintf(stderr, "\t\t\t[-d dma_channel]\n");
		fprintf(stderr, "\t\t\t[-a -l lower_address -u upper_address]\n");
		fprintf(stderr, "\t\t\t[-c -l lower_address -u upper_address]\n");
		break;
	}

	exit(100);
}



/* print diagnostics */

diag(type, rc)
int type, rc;
{
	switch (type) {
	case SYS:
		if (rc <= 0) {
			fprintf(stderr, "System getinst rc=%d\n", rc);
			return;
		}
		fprintf(stderr, "System getinst: flag=%d rc=%d\n",
			search_type, rc);
		fprintf(stderr, "\tdevice=%s vec=%hd type=%hd\n",
			sys.name, sys.vector, sys.itype);
		fprintf(stderr, "\tsioa=0x%lx eioa=0x%lx scma=0x%lx ecma=0x%lx\n",
			sys.sioa, sys.eioa, sys.scma, sys.ecma);
		break;

	case MAST:
		if (rc <= 0) {
			fprintf(stderr, "Master getinst rc=%d\n", rc);
			return;
		}
		fprintf(stderr, "Master getinst: flag=%d rc=%d\n",
			search_type, rc);
		fprintf(stderr, "\tdevice=%s characteristics=%s\n",
			mas.name, mas.mflags);
		break;

	case OPT:
		if (iflag)
			fprintf(stderr,"input=%s\t", pathinst);
		if (search_type == DSP)
			fprintf(stderr, "package=%s\n", sd_check.name);
		else if (search_type == VECTOR)
			fprintf(stderr, "vector=%d\n", sd_check.vector);
		else if (search_type == DMA)
			fprintf(stderr, "dma-channel=%d\n", sd_check.dmachan);
		else if (search_type == IOA)
			fprintf(stderr, "IOA: lbound=0x%lx ubound=0x%lx\n",
				sd_check.sioa, sd_check.eioa);
		else if (search_type == CMA)
			fprintf(stderr, "CMA: lbound=0x%lx ubound=0x%lx\n",
				sd_check.scma, sd_check.ecma);
		break;
	}
}

prdev(dev)
char *dev;
{
	if (verbose)
		fprintf(stdout, "%s\n", dev);
}
