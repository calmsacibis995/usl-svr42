/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)iconv:iconv.c	1.2.8.3"

/*
 * iconv.c	code set conversion
 */

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>
#include <iconv.h>

#define DIR_DATABASE 		"/usr/lib/iconv"	/* default database */
#define FILE_DATABASE 		"iconv_data"	/* default database */
#define MAXLINE 	1282			/* max chars in database line */
#define MINFLDS 	4			/* min fields in database */
#define FLDSZ 		257			/* max field size in database */

char	*mode;					/* Optional argument mode */
char	tmode[FLDSZ];				/* Database field variable */

extern int optind;
extern int opterr;
extern char *optarg;

extern struct kbd_tab *gettab();
extern const char badopen[];

main(argc, argv)
int argc;
char **argv;
{
register int c;
char *fcode;
char *tcode;
char *d_data_base = DIR_DATABASE;
char *f_data_base = FILE_DATABASE;
char from[FLDSZ];
char to[FLDSZ];
char table[FLDSZ];
char file[FLDSZ];
struct kbd_tab *t;
int fd;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxmesg");
	(void)setlabel("UX:iconv");

	fcode = (char*)NULL;
	tcode = (char*)NULL;
	mode  = (char*)NULL;
	c = 0;

	/*
	 * what about files
	 */
	while ((c = getopt(argc, argv, "f:t:m:")) != EOF) {

		switch (c) {

			case 'f':
				fcode = optarg;
				break;	

			case 't':
				tcode = optarg;
				break;

			/* New case to provide ES 3.2 compatability */
			case 'm':
				mode = optarg;
				break;

			default:
				usage_iconv(0);
		}

	}

	/* required arguments */
	if (!fcode || !tcode)
		usage_iconv(1);

	if (optind < argc) {

		/*
		 * there is a file
		 */
		fd = open(argv[optind],0);
		if (fd < 0) {
			pfmt(stderr, MM_ERROR, badopen, argv[optind],
				strerror(errno));
			exit(1);
		}
	} else
		fd = 0;

	if (search_dbase(file,table,d_data_base,f_data_base,(char*)0,fcode,tcode)) {

		/*
		 * got it so set up tables
		 */
		t = gettab(file,table,d_data_base,f_data_base,0);
		if (!t) {
		    pfmt(stderr, MM_ERROR, 
		    	":54:Cannot access conversion table (%s): %s\n", table,
		    	strerror(errno));
			exit(1);
		}
		process(t,fd,0);

	} else {
		if (mode)
			pfmt(stderr, MM_ERROR,
				":55:Not supported %s to %s: mode %s\n",
				fcode, tcode, mode);
		else
			pfmt(stderr, MM_ERROR,
				":56:Not supported %s to %s\n",
				fcode, tcode);
		exit(1);
	}
		
	exit(0);
}


usage_iconv(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":26:Incorrect usage\n");
	pfmt(stderr, MM_ACTION,
		":57:Usage: iconv -f fromcode -t tocode [-m mode] [file]\n");
	exit(1);
}


search_dbase(o_file, o_table, d_data_base, f_data_base, this_table, fcode, tcode)
char        *o_file,*o_table,*d_data_base,*f_data_base,*this_table,*fcode,*tcode;
{
int fields;
int row;
char buff[MAXLINE];
FILE *dbfp;
char from[FLDSZ];
char to[FLDSZ];
char data_base[MAXNAMLEN];

	fields = 0;

	from[FLDSZ-1] = '\0';
	to[FLDSZ-1] = '\0';
	o_table[FLDSZ-1] = '\0';
	o_file[FLDSZ-1] =  '\0';
	buff[MAXLINE-2] = '\0';
	tmode[FLDSZ-1] = '\0';

	sprintf(data_base,"%s/%s",d_data_base,f_data_base);

	/* open database for reading */
	if ((dbfp = fopen(data_base, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":58:Cannot access data base %s: %s\n", 
			data_base, strerror(errno));
			exit(1);
	}

	/* start the search */

	for (row=1; fgets(buff, MAXLINE, dbfp) != NULL ; row++) {

		if (buff[MAXLINE-2] != NULL) {
			pfmt(stderr, MM_ERROR, 
				":59:Database Error : row %d has more than %d characters\n",
				row, MAXLINE-2);
			exit(1);
		}

		if (!mode)
			fields = sscanf(buff,
				"%s %s %s %s", from, to, o_table, o_file);
		else
			fields = sscanf(buff,
				"%s %s %s %s %s",
				from, to, o_table, o_file, tmode);
		if (fields < MINFLDS) {
			pfmt(stderr, MM_ERROR, 
				":60:Database Error : row %d cannot retrieve required %d fields\n",
				row, MINFLDS);
			exit(1);
		}

		if ( (from[FLDSZ-1] != NULL) || (to[FLDSZ-1] != NULL) ||
		     (o_table[FLDSZ-1] != NULL) || (o_file[FLDSZ-1] != NULL) ||
		     (tmode[FLDSZ-1] != NULL)) {
			pfmt(stderr, MM_ERROR, 
				":61:Database Error : row %d has a field with more than %d characters\n",
				row, FLDSZ-1);
			exit(1);
		}

		if (this_table) {

			if (strncmp(this_table,o_table,KBDNL - 1) == 0) {

				fclose(dbfp);
				return 1;

			}
		} else
		if (strcmp(fcode, from) == 0 && strcmp(tcode, to) == 0)
		{
			if ((!mode) || (strcmp(mode, tmode) == NULL))
			{
				fclose(dbfp);
				return 1;
			}
		}
	}

	fclose(dbfp);
	return 0;
}
