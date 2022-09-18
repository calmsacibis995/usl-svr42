/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:main.c	1.2"
#ident	"$Header: $"

#define MAIN
#include "menu.h"
#include "locale.h"

static	void usage();
extern char	*optarg;	/* for getopt() cmd line parsing */
char	*output_file;		/* output (response) file (-o opt) */
char	*menu_file;		/* menu description file (-f opt) */

void
main(argc, argv)
int	argc;
char	*argv[];
{
	int	arg;			/* for getopt() cmd line parsing */
	char	*ptr;			/* for getopt() cmd line parsing */
	int	rc;			/* (tmp) for holding return codes */
	static	int fflag,oflag;	/* flags for cmd line */
	
	(void) setlocale(LC_ALL,"");	/* turn locale processing on */
	
	while ( EOF != ( arg = getopt( argc, argv, "crf:o:" ) ) ) {
		switch (arg) {
			/*
			 *   After menu -r, use -c to 'c'lear the screen...
			 */
			case 'c':
				start_curses();
				curs_set(0);
				refresh();
				curs_set(1);
				refresh();
				end_curses(rflag);
				exit(0);
				break;
			
			/*
			 *   Allow screen to be 'r'etained at end...
			 */
			case 'r':
				rflag++;
				break;
			case 'o':
				oflag++;
				ptr = optarg;
				if ((output_file =
					(char *)malloc(strlen(ptr)+1))
					== (char *)NULL) {
						(void)fprintf(stderr, "Couldnt malloc\n");
						exit(ENOMEM);
				}
				strcpy(output_file, ptr);
				break;
			case 'f':
				fflag++;
				ptr = optarg;
				if ( access(ptr, 04) < 0) {
					(void)fprintf(stderr, "%s: cannot open %s\n",
						argv[0], ptr);
					exit(EACCES);
				}
				if ((menu_file =
					(char *)malloc(strlen(ptr)+1))
					== (char *)NULL) {
						(void)fprintf(stderr, "Couldnt malloc\n");
						exit(ENOMEM);
				}
				strcpy(menu_file, ptr);
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
	if (!fflag) {
		fprintf(stderr, "No form description file specified.\n");
		usage(argv[0]);
	}
	if (!oflag) {
		fprintf(stderr, "No output file specified.\n");
		usage(argv[0]);
	}

	parse();
	start_curses();
	rc = do_form(output_file);
	wrefresh(w1);
	curs_set(0);
	refresh();
	curs_set(1);
	refresh();
	end_curses(rflag);
	exit(rc);
}

void
usage(prog_name)
char	*prog_name;
{
	(void)fprintf(stderr, "Usage: %s [-c]\n", prog_name);
	(void)fprintf(stderr, "   or: %s [-r(etain)] -f form_description_file -o output_file\n", prog_name);
	exit(1);
}
