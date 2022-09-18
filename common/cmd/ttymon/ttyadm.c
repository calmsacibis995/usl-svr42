/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttyadm.c	1.10.12.2"
#ident  "$Header: ttyadm.c 1.2 91/06/24 $"

/***************************************************************************
 * Command: ttyadm
 * Inheritable Privileges: P_DEV,P_MACWRITE,P_MACREAD,P_DACREAD
 *       Fixed Privileges: None
 *
 * Notes: format and output port monitor-specific information and print it
 * to stdout.
 *
 *	Usage: 	ttyadm [options] -d device -s service -l ttylabel
 *		ttyadm -V
 *			
 *		valid options are:
 *		-c
 *		-h
 *		-b
 *		-r count
 *		-t timeout
 *		-p prompt
 *		-m modules
 *		-i msg
 *	if enhanced security is installed
 *		-k SAKtype
 *		-K SAK
 *		-x
 *
 *
 ***************************************************************************/

# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <sys/types.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/termios.h>
# include <sys/stream.h>
# include <sys/tp.h>
# include <locale.h>
# include <pfmt.h>
# include <errno.h>
# include <priv.h>
# include "tmstruct.h"
# include "ttymon.h"

char	Scratch[BUFSIZ];


int	EnhancedSecurityInstalled;
int	B2Running;

static void usage();
static	int	check_label();

extern	int	is_enhancedsecurityinstalled();	/* see tmutil.c */
extern	int	check_device();
extern	int	check_cmd();
extern	int	vml();
extern	int	check_sak();

static const char badnumber[] = ":697:Invalid argument for \"-%c\" -- positive number expected.\n";
const char badopen[] = ":396:Cannot open %s: %s";

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 pfmt: None
                 strerror: None
                 getopt: None
                 fprintf: None
*/
main(argc, argv)
int argc;
const char *argv[];
{
	int c;			/* option letter */
	int errflg = 0;		/* error indicator */
	int ret;
	char *options;		/* for getops(3C) option string */

	struct pmtab *ptr;
	char *timeout = "";
	char *count = "";
	char prompt[BUFSIZ];
	char dmsg[BUFSIZ];
	char ttyflags[BUFSIZ], *tf;
	char *saktypep = "";
	char *sakdefp = "";
	char *saksecp = "";

	int  dflag = 0;		/* -d seen */
	int  sflag = 0;		/* -s seen */
	int  lflag = 0;		/* -l seen */
	int  mflag = 0;		/* -m seen */

	extern	void 	copystr();
	extern	char	*optarg;
	extern	int	optind;
	extern	int	strcheck();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:ttyadm");

	/* -check if enhanced security is installed
	*/
	EnhancedSecurityInstalled = is_enhancedsecurityinstalled();

	if (argc == 1)
		usage(1);
	if ((ptr = ALLOC_PMTAB) == PNULL) {
		(void)pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n",
			strerror(errno));
		exit(1);
	}

	ptr->p_modules = "";
	ptr->p_dmsg = "";
	ptr->p_prompt = "login\\: ";
	ttyflags[0] = '\0';
	tf = ttyflags;

	if (EnhancedSecurityInstalled == TRUE)
		options = "Vd:s:chbr:t:l:m:p:i:k:K:x";
	else
		options = "Vd:s:chbr:t:l:m:p:i:";

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'V':
			if ((argc > 2) || (optind < argc))
				usage(1);
			(void)fprintf(stdout,"%d", PMTAB_VERS);
			exit(0);
			break;	/*NOTREACHED*/
		case 'd':
			ptr->p_device = optarg;
			dflag = 1;
			break;
		case 'c':
			tf = strcat(tf,"c");
			break;
		case 'h':
			tf = strcat(tf,"h");
			break;
		case 'b':
			tf = strcat(tf,"b");
			break;
		case 'r':
			tf = strcat(tf,"r");
			count = optarg;
			if (strcheck(optarg,NUM) != 0) {
				(void)pfmt(stderr, MM_ERROR, badnumber, 'r');
				usage(0);
			}
			break;
		case 's':
			ptr->p_server = optarg;
			sflag = 1;
			break;
		case 't':
			timeout = optarg;
			if (strcheck(optarg,NUM) != 0) {
				(void)pfmt(stderr, MM_ERROR, badnumber, 't');
				usage(0);
			}
			break;
		case 'l':
			ptr->p_ttylabel = optarg;
			lflag = 1;
			break;
		case 'm':
			ptr->p_modules = optarg;
			mflag = 1;
			break;
		case 'p':
			ptr->p_prompt = prompt;
			copystr(ptr->p_prompt,optarg);
			break;
		case 'i':
			ptr->p_dmsg = dmsg;
			copystr(ptr->p_dmsg,optarg);
			break;
		case 'k':
			saktypep = optarg;
			break;
		case 'K':
			sakdefp = optarg;
			break;
		case 'x':
			saksecp = "drop";
			break;
		case '?':
			usage(0);
			break;
		}
	}
	if (optind < argc)
		usage(1);

	if ((!dflag) || (!sflag) || (!lflag))
		usage(1);

	if (check_device(ptr->p_device) != 0)
		errflg++;
	if (check_cmd(ptr->p_server) != 0)
		errflg++;
	if (check_label(ptr->p_ttylabel) != 0)
		errflg++;
	if (mflag && (vml(ptr->p_modules) != 0))
		errflg++;
	if ((ret = check_sak(saktypep, sakdefp, saksecp)) != 0){
		errflg++;
		if (ret == 1)
			usage(1);
	}
	if (errflg)
		exit(1);
	(void)fprintf(stdout, "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:",
			ptr->p_device, ttyflags, count, ptr->p_server,
			timeout, ptr->p_ttylabel, ptr->p_modules,
			ptr->p_prompt, ptr->p_dmsg, saktypep, sakdefp,saksecp);
	exit(0);
	/*NOTREACHED*/
}

/*
 * Procedure:     usage
 *
 * Restrictions:
 *               pfmt: None
 *
 * usage - print out a usage message
 */

static void
usage(complain)
int complain;
{
	if (complain)
		(void)pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
	(void)pfmt(stderr, MM_ACTION, ":698:Usage:\n\tttyadm [ options ] -d device -s service -l ttylabel\n");
	(void)pfmt(stderr, MM_NOSTD, ":699:\tttyadm -V\n");
	(void)pfmt(stderr, MM_NOSTD, ":700:\n\tValid options are:\n");
	(void)pfmt(stderr, MM_NOSTD, ":701:\t-c\n");
	(void)pfmt(stderr, MM_NOSTD, ":702:\t-h\n");
	(void)pfmt(stderr, MM_NOSTD, ":703:\t-b\n");
	(void)pfmt(stderr, MM_NOSTD, ":704:\t-r count\n");
	(void)pfmt(stderr, MM_NOSTD, ":705:\t-t timeout\n");
	(void)pfmt(stderr, MM_NOSTD, ":706:\t-p prompt\n");
	(void)pfmt(stderr, MM_NOSTD, ":707:\t-m modules\n");
	(void)pfmt(stderr, MM_NOSTD, ":708:\t-i msg\n");
	if (EnhancedSecurityInstalled == TRUE){
		(void)pfmt(stderr, MM_NOSTD, ":874:\t-k SPECSAKtype\n");
		(void)pfmt(stderr, MM_NOSTD, ":875:\t-k SAKtype -K SAK [-x]\n");
	}
	exit(1);
}

/*
 * Procedure:     check_label
 *
 * Restrictions:
 *               pfmt: None
 *               fopen: P_MACREAD
 *               fclose: None
 *	Notes - if ttylabel exists in /etc/ttydefs, return 0
 *			- otherwise, return -1
 */

static int
check_label(ttylabel)
char	*ttylabel;
{
	FILE *fp;
	extern	int	find_label();

	if ((ttylabel == NULL) || (*ttylabel == '\0')) {
		(void)pfmt(stderr, MM_ERROR, ":709:ttylabel is missing\n");
		return(-1);
	}
	/* TTYDEFS should be set at SYS_PRIVATE, so make sure it is */
	procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
	if ((fp = fopen(TTYDEFS, "r")) == NULL) {
		(void)pfmt(stderr, MM_ERROR, ":710:\"%s\" does not exist, cannot verify ttylabel <%s>\n", TTYDEFS, ttylabel);
		procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
		return(-1);
	}
	procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
	if (find_label(fp,ttylabel)) {
		(void)fclose(fp);
		return(0);
	}	
	(void)fclose(fp);
	(void)pfmt(stderr, MM_ERROR, ":711:Cannot find ttylabel <%s> in \"%s\"\n",
		ttylabel, TTYDEFS);
	return(-1);
}

/*
 * Procedure:     log
 *
 * Restrictions:
 *               pfmt: None
 *               putc: None
 *	log	- print a message to stderr
 */

void
log(sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int	sev;
char	*msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	pfmt(stderr, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	putc('\n', stderr);
}

#ifdef DEBUG
/*
 * Procedure:     dlog
 *
 * Restrictions:
 *               pfmt: None 
 *               putc: None
 *
 *	dlog	- print a debugging message to stderr
 */

void
dlog(msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
char	*msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	pfmt(stderr, MM_INFO|MM_NOGET, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	putc('\n', stderr);
}
#endif
