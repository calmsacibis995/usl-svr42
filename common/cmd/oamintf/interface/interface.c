/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/interface/interface.c	1.9.14.5"
#ident  "$Header: interface.c 2.0 91/07/12 $"

#include <stdio.h> 
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "local.h"
#include "intf.h"

#define PL_LEN 	(sizeof(PHOLDER)-1)

/*
 * this program inteprets OAM interface tables and outputs
 * commands for ViewMaster to use to display menus.
 * Invocation:
 * "object_gen path_name file_name"
 */

extern int 	get_desc(),
		getopt();
extern char 	*read_item(),
		*getenv(),
	     	*menutok();
		
static int 	init(),
		process_obj();

#ifdef DEBUG
	FILE *debug;
#endif


/* global variables */
FILE *descfile;			/* description file pointer */
char inpath[PATH_MAX];		/* input path name */
char svpath[PATH_MAX];		/* path for run command */
char infile[PATH_MAX];		/* input file name */
char pkgbase[PATH_MAX];		/* package base location directory */
char env_var[PATH_MAX+7];	/* environment variable */
char menupfx[] = MENUPFX;	/* menu file prefix */
char formpfx[] = FORMPFX;	/* form file prefix */
char textpfx[] = TEXTPFX;	/* text file prefix */
int pfxlen = PFXSIZ;		/* prefix length */
char dir_delimit = '/';		/* directory delimiter */
char continuation = '\\';	/* continuation char for end of line */
char *itemname;			/* item name if -t present */
char *path;			/* full path name in argv */
char *lessmenu;			/* inpath less 'menu' */
char oamset[] = "/usr/sadm/sysadm";	

char *oambase;			/* oambase from env */
/* global variables */

main(argc, argv)
int argc;
char **argv;
{
	int tflag;		/* type of invocation: -t or not */

	tflag = init(argc, argv);
	(void) process_obj(tflag);
	return(0);

}

static int
init(argc, argv)
int argc;
char **argv;
{
	register char *argptr;	/* pointer into argument array */
	register char *pathptr;	/* pointer to path array */
	register char *fileptr;	/* pointer to file name */
	extern char *optarg;		/* used by getopt */
	extern int optind, opterr;	/* used by getopt */
	int ch;				/* return from getopt */
	int errflg;			/* error flag */
	int opt;			/* set if option present */
	int tflag;			/* set if -t option present */

	opterr = 0;			/* no print errors from getopt */

	tflag = errflg = opt = 0;			/* init*/

	while((ch = getopt(argc, argv, "t")) != EOF) {
		switch(ch) {
			case 't':
				tflag++;
				break;
			case '?':
				errflg++;
		}

		if(*optarg == '-') errflg++;
		opt = 1;
	}

	if((errflg || ((!opt)) && (optind >= argc))) {
		(void) fprintf(stderr, "Error:  argument parsing error\n");
		return(-1);
	}
	if(tflag) itemname = argv[optind++];

	/*
	 * sets "inpath" array to contain the pathname passed in through
	 * the first argument, excluding OAMBASE.
	 * "infile" is set to contain the filename passed
	 * in through the second argument.
	 */

	if(tflag) 
		oambase = getenv(TESTBASE);
	else
		oambase = getenv(OAMBASE);
		if (oambase == NULL)
			oambase = oamset;
	
	(void) sprintf(pkgbase, "%s%s", oambase, PKGBASE);


	/*
	 * set argptr to point to directory argument, set pathptr to point
	 * to inpath array, then copy the argument into "inpath" using
	 * the pointers.
	 */
	
	path = argv[optind];
	argptr = argv[optind++];
	if(strncmp(argptr, oambase, strlen(oambase)) == 0)
		argptr += (strlen(oambase) + 1);
	pathptr = &inpath[0];
	while(*pathptr++ = *argptr++);

	/* save inpath in svpath for later use when inpath gets overwritten */
	(void) strcpy(svpath, inpath);

	/*
	 * set argptr to point to file name argument, set fileptr to point
	 * to infile array, then copy the argument into "infile" using
	 * the pointers.
	 */
	argptr = argv[optind];
	fileptr = infile;
	while(*fileptr++ = *argptr++);
	return(tflag);
}

static int
process_obj(tflag)
int tflag;		/* type of processing -t or not */
{
	/*
	 * process_obj() - processes the object specified by the path
	 * and file name arguments.  The first thing process_obj() does
	 * is to build the path and file names in "inpath" and "infile"
	 * into the local variable "file".  It then opens the file, and
	 * looks at the name in "infile" to see if it is an FMLI object
	 * (begins with "Menu.*", "Text.*", or "Form.*").  If so, it will
	 * simply read and write the file one line at a time.
	 * 
	 * All output goes to stdout.
	 * 
	 * If the file is a menu description file, process_obj() processes
	 * it line by line.  A line beginning with the prefix "#menu#"
	 * is the menu title, and the line beginning with "#help#" 
	 * has two fields: (1) file name of help text for the menu, and
	 * (2) help title.
	 * All other lines are menu items and have three fields:
	 * (1) menu item name, (2) menu item description, and 
	 * (3) action to take upon menu item selection.  
	 * The first field is translated into a "name=" statement
	 * for FMLI.  The second field is translated into a "description="
	 * statement for FMLI.  The third field is translated into a
	 * "lininfo=" statement for FMLI help menu messages processing.
	 * The fourth field is translated into an
	 * "action=" statement for FMLI and specifies a menu file,
	 * or a FMLI object (Form, Text, or Menu file).  If
	 * "placeholder" appears in the action field, then the menu
	 * item is not processed and will not appear (the "name=", 
	 * "description=" and "action=" statements aren't written).
	 */

	register char *item_next;	/* where to go when item chosen */

	char file[PATH_MAX];	/* array with complete path to file */
	char *instr;		/* pointer to input line */
	char *item_name;	/* item name */
	char *item_descr;	/* item description */
	char *rename;		/* item rename (orig name) */
	char *end_dirname;	/* end of directory name */
	char *end_item;		/* end of item */
	char *endpos;		/* position of \n in item_next, if any */
	char *itemptr;		/* item pointer */
	char *desc;		/* pointer to object description */
	char *title;		/* pointer to help title */
	char *save_full;	/* pointer to full action field */
	char *dot;		/* position of "dot" in item_next */
	char *pkginst;		/* package instance, if present on line */
	char *basedir;		/* base directory to use */
	char *next;		/* next field after item_next */
	char *pos;		/* position in pkginst field */
	int pr_out;		/* flag to print file directly out */
	int type;		/* type of obj in descriptor line */
	int i,  mp;		/* misc */
	int abs;		/* flag if absolute path */
	int loc_xqtable;	/* flag if local executable present (*LOC*) */

	struct menu_line *ptr_menu;	/* pointer to first line of menu item */
	struct menu_line m_line;	/* first line of menu item */

	char *menu_name;	/* pointer to <name> portion of <name>.menu */
	char target[MAXNAM];	/* holds name of Menu.<name> file */
	char *Menu;
	char helpfile[124];	/* used to construct potential path to help file */

	ptr_menu = &m_line;		/* init pointer */

#ifdef DEBUG
	if((debug = fopen("/tmp/OG.debug", "w")) == (FILE *)0) {
		fprintf(stderr, "Could not open file /tmp/OG.debug\n");
		exit(1);
	}
#endif

	/*
	 * initialize object - menu or form
	 */

	pr_out = 0;
	mp = strlen(MAIN_PATH);


	/* build path name to file from arguments */
	(void) sprintf(file, "%s/%s/%s", oambase, inpath, infile);

	/* open file */
	
	if((descfile = fopen(file, "r")) == NULL) {
		/* failed to open */
		(void) fprintf(stderr, "Error:  unable to open %s\n", file);
		return(-1);
	}
	/*
	 * if file name denotes a ViewMaster object (begins with
	 * "Menu.*", "Text.*" or "Form.*") then just print it out. 
 	 * This will make the virtual object take on the contents of the 
	 * file.  This idea was borrowed from the OAM ViewMaster 
	 * prototype, developed by ViewMaster developers in Summit.
	 */
	if((strncmp(infile, menupfx, pfxlen) == 0) ||
		(strncmp(infile, textpfx, pfxlen) == 0) ||
		(strncmp(infile, formpfx, pfxlen) == 0)) {
			pr_out = 1;

			/* generate $OBJ_DIR setting */
			(void) sprintf(env_var, "#extra line\n`set -e OBJ_DIR=%s`\n", path);
			(void) printf("%s", env_var);
	}
	if(tflag) 
		(void) printf("#extra line\n`message \"press <CANCEL> to return to edsysadm\"`\n\n");
			

	while((instr = read_item(ptr_menu, descfile, FULL_LINE)) != NULL) {
		if(pr_out) (void) printf("%s", instr);

		else if(*instr == '#') {  /* it's an object title 
					     or name of a help text
					     file */

			type = get_desc(instr, &desc, &title);
			if(type == ERR_RET) continue;
			if(type == STAR) continue;
			if(type == MENU) {
				(void) printf("menu=%s\n", desc);
				(void) printf("framemsg=`readfile $INTFBASE/ssmenu.msg`\n");
				(void) printf("help=OPEN TEXT $INTFBASE/Text.oamhelp $!LININFO \n\n");
			} else if (type == HELP) {
				/* (void) printf("help=OPEN TEXT $INTFBASE/Text.oamhelp %s/%s/HELP/%s '%s'\n\n", oambase, inpath, desc, title); */
			} else if (type == ROWS) {
				(void) printf("rows=%s\n", desc);
			} else if(!tflag) (void) printf("lifetime=%s\n", desc);

		}

		else {	/* it's an item */

			rename = NULL;
			abs = 0;
			loc_xqtable = 0;
			/* 
			 * get item name
			 * if it's NULL, continue
			 */
			if((item_name = menutok(instr)) == NULL) continue;

			/* 
			 * get item description
			 * if it's NULL, continue
			 */
			if((item_descr = menutok(NULL)) == NULL) 
				continue;

			/* 
			 * get pointer to next - if placeholder, skip 
			 * and go on to next item
			 * if it's NULL, continue 
			 */

			if((item_next = menutok(NULL)) 
				== NULL) continue;
			next = menutok(NULL);
			if(strncmp(next, PHOLDER, PL_LEN) == 0) 
					next = menutok(NULL);
			if(*next == '[') { 
				/* rename present, get rid of brackets */
				rename = next + 1;
				*(rename + strlen(rename) - 1) = '\0';
				next = menutok(NULL);
			} else {
				rename = item_name;
			}
			if(*next == '#') { /* pkginst present */
				pkginst = next + 1;
				pos = strrchr(pkginst, (int) '#');
				if(pos != NULL) *pos = NULL;
			}
			else pkginst = "";

			if(strncmp(item_next, PHOLDER, PL_LEN) == 0) 
				continue;
			/* not a placeholder, real item */
			/* 
			 * find out if item is in a directory
			 * If so, strip out directory name and
			 * append it to the inpath so 
			 * subsequent directory changes will
			 * work.
			 */

			if(*item_next == '/') { 
				/* absolute path */
				abs = 1;
			}
			else if(strncmp(item_next, LOC_IDENT, strlen(LOC_IDENT))
				== 0) {
				loc_xqtable = 1;
			}
			itemptr = item_next;

			if((!loc_xqtable) && ((end_dirname = strrchr(itemptr, 
				(int) dir_delimit)) != NULL)) {
				if(!abs) {
					*end_dirname = NULL;
					/* append dir name to inpath */
					(void) strcat(inpath, "/");
					(void) strcat(inpath, item_next);

				} /* end if ! abs */
				else {
				      (void) strcpy(inpath, item_next);
				      save_full = item_next;
				      *(strrchr(inpath, (int)
					 dir_delimit)) = NULL;
				}
				item_next = end_dirname+1;
			} /* end if */

			if((end_item = strchr(item_next, continuation)) 
				!= NULL) *end_item = NULL;
			else if((end_item = strchr(item_next, TAB_CHAR)) 
				!= NULL) *end_item = NULL;

			if((endpos = strrchr(item_next, (int) '\n')) != NULL) {
				/* strip out extra nl */
					*endpos = NULL;
			}
					
			(void) printf("name=%s\n", item_name);
			(void) printf("description=%s\n", item_descr);

			if(strncmp(item_next, menupfx, pfxlen) == 0) 
				type = FML_MENU;
			else if(strncmp(item_next,formpfx,pfxlen) == 0) 
				type = FML_FORM;
			else if(strncmp(item_next,textpfx,pfxlen) == 0) 
				type = FML_TEXT;
			else if(((dot = strrchr(item_next, (int) '.')) != NULL)
				&& (strncmp(dot, MENU_SFX, strlen(MENU_SFX)) 
				== 0) ) {
				type = IS_MENU;
				menu_name = strdup(strtok(item_next, "."));
				sprintf(target, "Menu.%s", menu_name);
				Menu = strdup(target);
			}
			else type = OTHER;

			if(((type != OTHER) && (type != IS_MENU)) && (*pkginst != '\0')) {
				i = mp;	/* skip main in path */
				basedir = pkgbase;
			}
			else {
				i = 0;
				basedir = oambase;
			}

			if(tflag) {
				if(strcmp(item_name, itemname) == 0)
					(void) printf("action=OPEN MENU $INTFBASE/Menu.testmenu /%s %s\n\n", inpath, item_next);
				else (void) printf("action=done\n\n");
				(void) strcpy(inpath, svpath);
				continue;
			}

			if ((lessmenu=strstr(inpath, "menu/")) != NULL)
				lessmenu = lessmenu + 5;
			else {
				lessmenu = strchr(inpath, '/');
				lessmenu++;
			}
			
			if ((type == FML_MENU) || (type == FML_FORM) || (type == FML_TEXT) || (type == IS_MENU)
			  ) {
				if (*pkginst == '\0') {
					(void) printf("lininfo=\"%s/%s \\\"%s\\\"\" %s\n",
				   		basedir, inpath+i, item_descr, rename);
				} else {
					sprintf(helpfile, "%s/%s/Help", basedir, inpath);
					if (access(helpfile, R_OK) == 0)
						(void) printf("lininfo=\"%s/%s \\\"%s\\\"\" %s\n",
						  basedir, inpath, item_descr, rename);
					else
						(void) printf("lininfo=\"%s%s/%s \\\"%s\\\"\" %s\n",
						  pkgbase, pkginst, lessmenu, item_descr, rename);
				}
			}
			if(type == FML_MENU) {
				(void) printf("action=`set -l OBJ_DIR=%s%s/%s \n", basedir, pkginst, inpath+i);
				(void) printf("`OPEN MENU %s%s/%s/%s\n\n", basedir, pkginst, inpath+i, item_next);
			}
			else if(type == FML_FORM) {
				(void) printf("action=`set -l OBJ_DIR=%s%s/%s \n", basedir, pkginst, inpath+i);
				(void) printf("`OPEN FORM %s%s/%s/%s\n\n", basedir, pkginst, inpath+i, item_next);
			}
			else if(type == FML_TEXT) {
				(void) printf("action=`set -l OBJ_DIR=%s%s/%s \n", basedir, pkginst, inpath+i);
				(void) printf("`OPEN TEXT %s%s/%s/%s\n\n", basedir, pkginst, inpath+i, item_next);
			}
			else if(type == IS_MENU) {
					(void) printf("action=`set -l OBJ_DIR=%s/%s \n", basedir, inpath+i);
					(void) printf("`OPEN MENU  %s/%s/%s\n\n", basedir, inpath+i, target);
			} else {
				if(loc_xqtable) {
					item_next += strlen(LOC_IDENT);
					(void) printf(
				   	   "lininfo=\"%s%s/%s \\\"%s\\\"\" %s\n",
				   	   pkgbase, pkginst, item_name, 
					   item_descr, rename);
					(void) printf("action=`run %s%s%s` NOP\n\n", 
						pkgbase, pkginst, item_next);
				}
				else if(!abs) {
					{
					register int	i,
						lim = strcspn(lessmenu, "/");
					char menu_nm[16];

					for (i=0; i < lim; i++)
						menu_nm[i] = lessmenu[i];
					menu_nm[i] = '\0';

					(void) printf(
					"lininfo=\"%s%s/%s/%s \\\"%s\\\" %s\"\n",
					pkgbase, pkginst, menu_nm, rename,
					item_descr, rename);
					}
					(void) printf("action=`run %s` NOP\n\n",
					   item_next);
				} else {
					(void) printf(
				   	"lininfo=\"%s%s/%s \\\"%s\\\"\" %s\n",
				   	pkgbase, pkginst, rename, item_descr,
					rename);
					(void) printf("action=`run %s` NOP\n\n",
					   save_full);
				}
			}

			(void) strcpy(inpath, svpath);

		}  /* end else */
	} /* end while */
	return(0);
} /* end process_obj */
