/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/maplocale/maplocale.c	1.2.1.2"
#ident  "$Header: maplocale.c 1.1 91/07/04 $"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <nl_types.h>
#include <langinfo.h>
#include <ctype.h>
#include <unistd.h>
#include "maplocale.h"

extern int optind;

extern char *optarg;

int abbrev_name;

char locale_name[STR_SIZE];

main(argc, argv)
int argc;
char *argv[];
{
	int ch;		/*  Arg returned from getopt  */
	int mode = 0;

	char sysv_locale[STR_SIZE];
	static char territory[STR_SIZE];
	static char codeset[STR_SIZE];

	struct stat loc_stat;	/*  Stat struct for sysv locale  */

	/*  Process arguments  */
	while ((ch = getopt(argc, argv, "f:t:c:")) != EOF)
	{
		switch(ch) {
			case 'f':
				/*  Only 'XENIX' is currently valid with -f  */
				if (strcmp(optarg, SCO_CONV_ARG) == 0)
					mode = SCO_CONV_MODE;
				else if (strcmp(optarg, ISC_CONV_ARG) == 0)
					mode = ISC_CONV_MODE;
				else
					error("Illegal conversion option, valid ones are: XENIX, ISC");
				break;

			case 't':
				strcpy(territory, optarg);
				break;

			case 'c':
				strcpy(codeset, optarg);
				break;

			default:
				usage();
		}
	}

	/*  Check that the mode was specified  */
	if (mode == 0)
		usage();

	/*  Move to non-flag arguments  */
	argc -= optind;
	argv += optind;

	/*  Must have a sysv locale name  */
	if (argc < 1)
		usage();

	/*  Convert the arguments to full pathnames  */
	sprintf(sysv_locale, "%s%s", SYSV_LOC_DIR, argv[0]);
	if (mode == SCO_CONV_MODE)
		conv_sco_locale(argv[0], territory, codeset);
	else
		conv_isc_locale(argv[0]);

	/*  Check that the SYSV locale exists and is a directory  */
	if (stat(sysv_locale, &loc_stat) < 0)
		error("cannot access System V locale");

	if ((loc_stat.st_mode & S_IFDIR) == 0)
		error("System V locale is not a directory");

	/*  Set the locale to that selected by the user  */
	if (setlocale(LC_ALL, argv[0]) == NULL)
		error("Failed to set the locale, conversion aborted");

	if (mode == SCO_CONV_MODE)
	{
		/*  Map the different categories of the locale  */
		sco_map_ctype();
		sco_map_numeric();
		sco_map_date();
		sco_map_collate(sysv_locale);
		sco_map_messages();
		sco_map_currency();

		if (abbrev_name == 1)
			sco_update_default();

	}
	else
	{
		isc_map_collate(sysv_locale);
		isc_map_ctype(sysv_locale);
		isc_map_numeric();
		isc_map_date();
		isc_map_messages();
		isc_map_currency();
	}

	exit(SUCCESS);
}

/**
 *  Usage of this utility
 **/
void
usage()
{
	fprintf(stderr, "Usage: maplocale -f<format> <input_locale> <output_locale>\n");
	exit(ERROR);
}

/**
 *  Output error string and exit.
 **/
void
error(err_str)
char *err_str;
{
	cleanup(locale_name);
	fprintf(stderr, "maplocale: %s\n", err_str);
	exit(ERROR);
}

/**
 *  Check pathname - if it does not exist try to create it as a directory.
 *  If it exists and is not a directory report an error
 **/
void
path_create(pathname)
char *pathname;
{
	char error_str[STR_SIZE];

	struct stat loc_stat;

	/*  If we can't stat it thenwe guess it doesn't exist  */
	if (stat(pathname, &loc_stat) < 0)
	{
		/*  Try to create the directory  */
		if (mkdir(pathname, DMODES) < 0)
		{
			sprintf(error_str, "Error - Unable to create %s", pathname);
			error(error_str);
		}
	}
	else
	{
		/*  Check that it is a directory and not a file  */
		if ((loc_stat.st_mode & S_IFDIR) == 0)
		{
			sprintf(error_str, "Error - %s exists, but is not a directory", pathname);
			error(error_str);
		}
	}
}

/**
 *  Function to ensure that no half completed locales are created
 **/
void
cleanup()
{
	char fname[STR_SIZE];

	/*  We try to delete all files that could have been created, if the
	 *  files have not yet been created unlink will fail, but we don't 
	 *  bother to check the return value, so who cares?
	 */
	sprintf(fname, "%s/%s", locale_name, CTYPE);
	unlink(fname);
	sprintf(fname, "%s/%s", locale_name, NUMERIC);
	unlink(fname);
	sprintf(fname, "%s/%s", locale_name, TIME);
	unlink(fname);
	sprintf(fname, "%s/%s", locale_name, COLLATE);
	unlink(fname);
	sprintf(fname, "%s/%s", locale_name, MESSAGES);
	unlink(fname);
	sprintf(fname, "%s/%s", locale_name, CURRENCY);
	unlink(fname);
}
