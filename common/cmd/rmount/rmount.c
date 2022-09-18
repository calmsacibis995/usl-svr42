/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rmount:rmount.c	1.1.13.2"
#ident  "$Header: rmount.c 1.2 91/06/27 $"
#include <sys/types.h>
#include <nserve.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mnttab.h>
#include <stdlib.h>
#include <string.h>
#include <mac.h>
#include "rmount.h"
#include "rmnttab.h"

static int	renter();
static int	rshow();

char		*cmd;

static char		*usage_sec	= "usage: %s [-F fstype] [-drc] [-o options] [-l macceiling] resource directory\n";
static char		*usage_nonsec	= "usage: %s [-F fstype] [-drc] [-o options] resource directory\n";

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char	*optarg;
	extern int	optind;
	int	c;
	int	err	= 0; 
	int	dflg	= 0;	/* command line flags */
	int	rflg	= 0;		
	int	Fflg	= 0;
	int	oflg	= 0;
	int	cflg	= 0;
	int	lflg	= 0;
	char	*fstype	= NULL;
	char	*lvl	= NULL;
	char 	*usage	= ismac() ? usage_sec	: usage_nonsec;
	char	*optlist= ismac() ? "F:drco:l:"	: "F:drco:";
	char	opt_buf[50];	/* large enough for all options
				 * Note: repeating an option is an error */

	cmd = argv[0];
	if (argc == 1) {		/* output contents of rmnttab */
		return rshow();
	}
#ifdef   OLDSEC
	else if (geteuid() != 0) {
		Fprintf(stderr, "%s: must be super-user\n", cmd);
		return 1;
	}
#endif /*OLDSEC*/
	opt_buf[0] = NULL;
	while ((c = getopt(argc, argv, optlist)) != -1)
		switch (c) {
		case 'F':
			if (Fflg) 
				err = 1;
			else {
				fstype = optarg;
				Fflg = 1;
				dflg = 1;
			}
			break;
		case 'd':
			if (Fflg|dflg)
				err = 1;
			else {
				fstype = "rfs";
				Fflg = 1;
				dflg = 1;
			}
			break;
		case 'c':
			if (cflg)
				err = 1;
			else {
				cflg = 1;
				if (opt_buf[0]) 
					Strcat(opt_buf,",nocaching");
				else
					Strcpy(opt_buf,"nocaching");
			}
			break;
		case 'r':
			if (rflg)
				err = 1;
			else {
				rflg = 1;
				if (opt_buf[0]) 
					Strcat(opt_buf,",ro");
				else
					Strcpy(opt_buf,"ro");
			}
			break;
		case 'o':
			if (oflg)
				err = 1;
			else {
				oflg = 1;
				if (opt_buf[0]) {
					Strcat(opt_buf,",");
					Strcat(opt_buf,optarg);
				}
				else
					Strcpy(opt_buf,optarg);
			}
			break;
		case 'l':
			if (lflg) 
				err = 1;
			else {
				level_t	level;

				lvl  = optarg;
				if( lvlin(lvl, &level) == -1){
					if(errno == EINVAL){
						Fprintf(stderr,
							"%s: bad level: %s\n",
							cmd, lvl);
						return 1;
					} else {
						perror("lvlin");
						return 1;
					}
				}
				lflg = 1;
			}
			break;
		case '?':
			err = 1;
			break;
		}

	if (err || optind+2 != argc) {
		Fprintf(stderr, usage, cmd);
		return 1;
	}
	else if (!fstype) {
		Fprintf(stderr, "%s: file system type not specified\n", cmd);
		Fprintf(stderr, usage, cmd);
		return 1;
		}
		else  {
			if (!opt_buf[0])
				Strcpy(opt_buf, "rw");
			return renter(	argv[optind],
					argv[optind+1],
					opt_buf,
					fstype,
					lvl);
		}
}


/*
	renter - enter a request onto rmnttab
*/
int
renter(resource, dir, opt, fs, lvl)
	char	*resource;
	char	*dir;
	char	*opt;
	char	*fs;
	char	*lvl;
{
	char	*fqn();	/* convert a name into a fully qualified domain.name */
	char	fres[MAXDNAME];	/* fully qualified names */
	char	fdev[MAXDNAME];	/* fully qualified names */
	int	ret;
	FILE	*rp;
	struct stat	stbuf;
	struct rmnttab	rmtab;

	lock();

	(void)fqn(resource, fres);	/* generate a fully qualified name */
	/* look for conflicts between rmnttab entries and the new one */

	switch( ret = rd_rmnttab(&rp, &stbuf) ) {
		case 1:   /* rmnttab does not exist */
			if (!wr_rmnttab(fres, dir, opt, fs, lvl)) {
				unlock();
				return ret;
			}
			else {
				unlock();
				return 0;
			}
		case 2:
			unlock();
			return ret;
		case 0:
			break;
		default:
			unlock();
			Fprintf(stderr, "%s: error in reading rmnttab\n", cmd);
			return 0;
	}

	while (getrmntent(rp, &rmtab) == 0) {
		if (strcmp(fqn(rmtab.rmnt_special, fdev), fres) == 0) {
			unlock();
			Fprintf(stderr,"%s: %s already queued for mount\n",
				cmd, resource);
			Fclose(rp);
			return 1;
		}
	}
	Fclose(rp);

	/* write out the rmnttab file */
	if (!wr_rmnttab(fres, dir, opt, fs, lvl))
		ret = 1;
	unlock();
	return ret;
}


/*
	rshow - show pending mounts (display rmnttab)
*/

int
rshow()
{
	struct rmnttab	rmtab;
	char		ptime[BUFSIZ];
	FILE		*rfp;
	long		ltime;
	int		ret;

	if ((rfp = fopen (RMNTTAB, "r")) == NULL) 
		return 0;
	while ((ret = getrmntent(rfp, &rmtab)) == 0) {
		ltime = atol(rmtab.rmnt_time);
		if (cftime(ptime, (char *)0, &ltime)+1 > sizeof(ptime)){
			Fprintf (stderr, "%s: overflow ptime[]\n", cmd);
			Fclose(rfp);
			return 1;
		}
		rmtab.rmnt_time = ptime;

		putrmntent(stdout, &rmtab);
	}
	if (ret > 0) {
		Fprintf (stderr, "%s: error in %s\n", cmd, RMNTTAB);
		Fclose(rfp);
		return 1;
	}
	Fclose(rfp);
	return 0;
}
