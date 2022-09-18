/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5util:setvgamode/setvgamode.c	1.12"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#include <stdio.h>
#include <sys/stat.h>

/*
 * The comment character is '#', and causes the rest of the line to
 * be ignored.  Blank lines are allowed.  Whitespace within strings
 * may be quoted within pairs of either single or double quotes, and
 * a single character may be escaped with a backslash (\) character.
 */

#define	SKIPSPACE(bp)	while (*(bp) != '\0' && isspace (*(bp))) (bp)++
#define	FINDSPACE(bp)	while (*(bp) != '\0' && !isspace (*(bp))) (bp)++
#define	MAXLINE	150 /* max length of a single line in file */
#define	MAX_TOKENSIZE	64  /* max length of a single token in a line */
#define	MAXARGS		16  /* max number of arguments on one line */
#define	MAXVENDORS	128 /* max number of vendors */
#define	MAXENTRIES	256 /* max number of entries */
#define	MAX_CFGFILES	4   /* max number of config files */

#define SUCCESS		1
#define FAIL		0

/*
 * This is appended to either /usr/X or $XWINHOME
 */
#define CONFIGFILE "/defaults/Xwinconfig"

/* MISC NOTES:
 *	/usr/X/lib/vgainit/VGAVendorDB : this file has the info about the
 *	available 'initialization' drivers for various VGA boards
 *
 *	In /usr/X/lib/vgainit directory, each 'init' driver has a 
 *	corresponding configuration file and an initialization library;
 * 	For example, Tseng Labs ET4000, 16 color driver has 
 *	the following two files:
 *		et4k_16i.so and et4k.16cfg
 *
 *	Once the user selects the appropriate vendor and the resolution for
 *	that board, the corresponding init library is copied as libv16i.so.1
 *	Ex:
 *		cp /usr/X/lib/vgainit/et4k_16i.so /usr/X/lib/libv16i.so.1
 *		OR
 *		cp /usr/X/lib/vgainit/et4k_256i.so /usr/X/lib/libv256i.so.1
 */
/* vendor data */
typedef struct _vdata {
	char *vendor;
	char *chipset;
	char *cfgfiles;
	char *idrivers;
} vdata;

/* board data for each mode supported */
typedef struct _bdata {
	char	*class;
	char	*cmap_name;
	char 	*entry;
	char 	*monitor;
	int	xmax;
	int	ymax;
	int	colors;
	char	*initdriver;
} bdata;

/* #define DEBUG	1 */
#ifdef DEBUG
FILE	*tmp_fp;

char *
MALLOC (size)
int size;
{
	char *p;
	
	p = (char *)malloc(size);
	fprintf (tmp_fp, "MALLOC: %x\n", p);
	return (p);
}

FREE (ptr)
char *ptr;
{
	fprintf (tmp_fp, "FREE : %x\n", ptr);
	free (ptr);
}
#else
#define MALLOC malloc
#define FREE free
#endif

#define TRUE	1
#define FALSE	0

/*
 * If you want to use the functions in this file with a xclient uncomment the
 * next line.
 */
/* #define XCLIENT */

static char *envp, envpath[MAXLINE];

#ifndef XCLIENT
main(argc, argv)
 int argc;
 char **argv;
{
	vdata	*vendordata[MAXVENDORS];
	bdata	*boarddata[MAXENTRIES];
	int	vendornum, mode;
	int	num_entries;		/* number of entries per board */
	int	num_vendors;		/* total number of vendors */
	int	monitor_size;		/* monitor size in inches */
	double  monitor_width;
	double	monitor_height;
	unsigned char	defaultFlg;	/* to indicate default settings */
	unsigned short myid, getuid ();
	char	tmpbuf[MAXLINE];
	int	len;
	double	atof ();
	unsigned char OK;
	extern char *getenv(const char *);

#ifdef DEBUG
	if ((tmp_fp = fopen ("/tmp/malloc.debug", "w")) == (FILE *)0)
		printf ("Error opening /tmp/malloc.debug file \n");
#endif
	defaultFlg = FALSE;

	if ( (myid = getuid()) != 0) {
		printf ("You must be super user to run this utility.\n");
		exit ();
	}

	/*
	 * if XWINHOME is not set, set it to default (ie: /usr/X)
	 */
	if ( (envp = getenv("XWINHOME")) != NULL)
	{
		strcpy (envpath, envp);
	}
	else
	{
		/* default */
		strcpy (envpath, "/usr/X/");
		envp = envpath;
	}

	/* default monitor size 14inch monitor */
	monitor_width =  9.75;
	monitor_height = 7.32; 

	switch (argc) {
	   case 1:
		break;
	   case 2:
		if (!strcmp(argv[1], "-default"))
		{
			vendornum = 0;
			mode = 0;
			defaultFlg = TRUE;
		}
		else
			printUsg ();

		break;
	   case 4:
		if (!strcmp(argv[1], "-msize"))
		{
			monitor_width = atof(argv[2]);
			monitor_height = atof(argv[3]);
			/*
			 * check for some sanity; the range that is being
			 * checked here is arbitrary
			 */
			if ( (monitor_width<6.0) || (monitor_height<5.0) ||
			     (monitor_width>20.0) || (monitor_height>20.0) )
			{
				printf ("Warning: Invalid entry for either monitor width or height.\n");
				printf ("selecting default width and height, 9.75x7.32 inches.\n");
				monitor_width = 9.75;	/* default values for 14 inches. */
				monitor_height = 7.32;
			}
		}
		else
			printUsg ();

		break;
	   default:
		printUsg ();
		break;
	}

	/*
	 * Read the vendor data
	 *	output: vendordata	; returns data for all the supported
	 *				; vendors found in database
	 * 	returns number of vendors found in DB
	 */

	/* vendordata[0] = getCurrentVendorInfo (); */

	if ( (num_vendors = getVendorData (vendordata)) == 0) {
		printf ("Error reading Vendor database file.\n");
		exit ();
	}

	for (;;) {
	   if ( !defaultFlg ) {
		/*
		 * present the data to the user to make the selection
		 */
		printCurrentModeData ();
		printVendorData (vendordata, num_vendors); 
		printf ("\nEnter vendor choice (default 0) => ");
		len = getline(tmpbuf);
		vendornum = atoi(tmpbuf);
	   } /* !defaultFlg */

	   if ( (vendornum < 0) || (vendornum >= num_vendors) ) {
		printf ("\nInvalid vendor number: %d. Try again.\n", vendornum);
	   }
	   else
		break;
	}

	/*
	 * Once a vendor's video board is selected, get the various modes
	 * supported by that board; This info is in the corresponding config
	 * file
	 * input:
	 *	vendordata
	 *	vendornum
	 * on return:
	 *	boarddata has info for all the supported modes.
	 */
	if ( (num_entries = getBoardData(vendordata, vendornum, boarddata)) == 0)
	{
		printf ("Error reading Vendor's data config file : %s\n",
					vendordata[vendornum]->cfgfiles);
		return;
	}

      OK = FALSE;
      if (!defaultFlg) {
        while (!OK ) {
	/*
	 * present various modes supported, for the selected vendor's
	 * VGA board
	 */
	for (;;) {
	   printBoardData (boarddata, num_entries);

	   printf ("\nEnter mode (default 0) => ");
	   len = getline(tmpbuf);
	   mode = atoi(tmpbuf);

	   if ( (mode < 0) || (mode >= num_entries) ) {
		printf ("\nInvalid mode: %d. Try again\n", mode);
	   }
	   else
		break;
	}

#ifdef NOTNEEDED
	/*
	 * Now get the monitor size
	 */
	printf ("Enter Monitor width and height (default: 9.75 7.32): "); 
	len = getline(tmpbuf);
	sscanf (tmpbuf, "%f %f", &monitor_width, &monitor_height);

	/*
	 * check for some sanity; the range that is being checked here is
	 * arbitrary
	 */
	if ( (monitor_width<6.0) || (monitor_height<5.0) ||
	     (monitor_width>20.0) || (monitor_height>20.0) )
	{
		printf ("Warning: Invalid entry for either monitor width or height.\n");
		printf ("selecting default width and height, 9.75x7.32 inches.\n");
		monitor_width = 9.75;	/* default values for 14 inches. */
		monitor_height = 7.32;
	}
#endif

	printf ("\nYou have selected the following:\n\n");
	printf ("	Vendor       : %s \n", vendordata[vendornum]->vendor);
	printf ("	Chipset      : %s \n", vendordata[vendornum]->chipset);
	printf ("	Monitor      : %s \n", boarddata[mode]->monitor);
	printf ("	Resolution   : %dx%d \n", boarddata[mode]->xmax,
					boarddata[mode]->ymax);
	printf ("	Colors       : %d \n\n", boarddata[mode]->colors); 

	printf ("Accept(y),  Quit(q),  or any other key to repeat the options: ");
	len = getline(tmpbuf);
	if ( tmpbuf[0] == 'y' )
		OK = TRUE;
	else if ( tmpbuf[0] == 'q' )
		exit ();
       } /* while !OK */
      } /* !defaultFlg */
      else {
	printf ("Restored default configuration file.\n");
	printf ("default mode : Standard VGA, 640x480 with 16 colors.\n");
      } /* !defaultFlg */

	/*
	 * Required data is here; so now
	 * 1.  generate the config file
	 * 2.  copy the corresponding init driver to the system dir
	 * (ex: cp /usr/X/lib/vgainit/t89_16i.so /usr/X/lib/libv16i.so.1
	 */
	if ( !generateCFGfile (vendordata, vendornum, boarddata, mode, 
			monitor_width, monitor_height) ) {
		printf ("Error generating configuration file.\n");
		return;
	}
	else if ( !defaultFlg )
		printNewEntry ();

	/*
	 * Now, free up all the allocated memory
	 */
	freememory (vendordata, num_vendors, boarddata, num_entries);

#ifdef DEBUG
	fclose (tmp_fp);
#endif
}

printUsg ()
{
	printf ("Usage: setvgamode \n");
	printf ("	-default\n");
	printf ("	-msize width height\n");
	exit ();
}

int
getline (buf)
 char *buf;
{
  int c, i;
  char	*p = buf;

	for (i = 0; (i < MAXLINE-1) &&
			((c=getchar()) != EOF) && c != '\n'; ++i) {
		p[i] = c;
	}
	if (c == '\n') {
		p[i] = c;
		++i;
	}
	p[i] = '\0';
	return i;
}

printNewEntry ()
{
	char	tbuf[MAXLINE];

	strcpy (tbuf, "cat ");
	if (envp)
	{
		strcat (tbuf, envp);
		strcat (tbuf, "/defaults/Xwinconfig");
	}
	else
	{
		strcat (tbuf, "/usr/X/defaults/Xwinconfig");
	}

	printf ("The New Entry is: \n\n");
	system (tbuf);
}

#endif /* ! XCLIENT */

/*
 *	Parse a string into an argument vector, handling quoted tokens
 *	properly.  The input string is modified in place (quoted strings
 *	and escaped characters are collapsed in place), and the argument
 *	vector is set to point to substrings in the input buffer, so argv[]
 *	will be incorrect if the input buffer is changed.
 */
static
line_parse (str, argv, maxargs)
register char	*str;
char		**argv;
int		maxargs;
{
	register char	*cp;
	register char	quotec = '\0';
	register char	**argp = argv;


	while (*str != '\0') {
		SKIPSPACE (str);		/* skip leading whitespace */
		if (*str == '\0')		/* end of input string */
			break;
		
		if (*str == '\n')	/* comment, eol */
			break;

		if (--maxargs <= 0)		/* out of space in arglist */
			break;

		*argp++ = str;			/* save ptr to start of token */

		/*
		 *	Now collect the token contents, collapsing quotes
		 *	in the process.
		 */
		for (cp = str; *str != '\0'; str++) {
			/*
			 *	Deal with open/close quote characters.
			 */
			if ((*str == '"' || *str == '\'') &&
			    (quotec == '\0' || *str == quotec)) {
				if (quotec == '\0')	/* opening quote */
					quotec = *str;
				else if (*str == quotec) /* closing quote */
					quotec = '\0';
			} else if (*str == '\\')
				*cp++ = *++str;
			else if (quotec == '\0' &&
				 (isspace(*str) || *str == '\n'))
				break;
			else
				*cp++ = *str;
		}

		/*
		 *	If we found a comment character or newline, quit.
		 */
		if (*str == '\n') {
			*cp = '\0';	/* ensure last token was null-term'd */
			break;
		}

		/*
		 *	Weirdness time: if there weren't any quoted or escaped
		 *	characters/strings in the input, then "cp" and "str"
		 *	should point to the same place.  On the other hand,
		 *	if quoted/escaped stuff, then "cp" will be less than
		 *	"str".  We need to be sure to check and advance "str"
		 *	before null-terminating the token ended at "cp".
		 */
		if (*str != '\0')	/* skip past the end of this word */
			str++;

		*cp = '\0';	/* null-terminate the current word. */
	}

	if (maxargs >= 0)	/* null-terminate the argument vector */
		*argp = (char *)0;

	return argp - argv;	/* returns total number of tokens found */
}

/*
 *	Simple fgets() replacement to read a single line from a file,
 *	allowing for "\"-newline escapes on long lines.
 */
config_fgets (buf, len, fp)
char	*buf;
int	len;
FILE	*fp;
{
	char	*cp = buf;
	int	c, c2;

	while (len > 1) {
		if ((c = getc (fp)) == EOF)
			break;
		else if (c == '\\') {	/* peek at next -- is it \n? */
			if ((c2 = getc (fp)) == '\n')
				continue;
			ungetc (c2, fp);
		}
		*cp++ = c;
		len--;
		if (c == '\n')
			break;
	}
	if (len > 0)
		*cp = '\0';
	return (cp != buf);
}

/*
 *	Given vendor data and a vendor number, reads all the config files
 *	found for the requested vendor and returns all the supported
 *	modes
 */
int
getBoardData (vd, vnum, bd)
  vdata **vd;		/* vendor data */
  int	vnum;		/* vendor number */
  bdata **bd;		/* to return data for various supported modes */
{
	char		*argv[MAXARGS];
	int		argc;
	bdata		*bp;
	char		inbuf[MAXLINE]; 	/* TEMP buffer for misc reads */
	char		inbuf2[MAX_TOKENSIZE];
	char		inbuf3[MAX_TOKENSIZE];
	char		tmp1[MAX_TOKENSIZE], tmp2[MAX_TOKENSIZE];
	int		num;
    	FILE 		*cfg_fp = (FILE *)0;
	char		*envp = NULL;	/* save the ptr to env var, XWINHOME */
	char		vendor_cfgfile[MAX_TOKENSIZE];
	char		path[MAX_TOKENSIZE];
	int		num_cfgfiles;
	char		*configfiles[MAX_CFGFILES];
	char		*initdrivers[MAX_CFGFILES];
	int		i;

	/*
	 * if XWINHOME is not set, set it to default (ie: /usr/X)
	 */
	if (envp)
	{
		strcpy (path, envp);
		strcat (path, "/lib/vgainit/");
		/* the actual file name is appended later */
	}
	else
	{
		strcpy (path, "/usr/X/lib/vgainit/");
		/* the actual file name is appended later */
	}

	strcpy (inbuf2, vd[vnum]->cfgfiles);
	num_cfgfiles = line_parse (inbuf2, configfiles, 
			sizeof(configfiles) / sizeof(configfiles[0]));

	strcpy (inbuf3, vd[vnum]->idrivers);
	i = line_parse (inbuf3, initdrivers, 
			sizeof(initdrivers) / sizeof(initdrivers[0]));

	/*
	 * For each config file found in VGAVendorDB, there must be a
	 * corresponding initialization driver; if not return error.
	 */
	if ( i != num_cfgfiles )
		return (0);

	num = 0;
	for (i=0; i<num_cfgfiles; i++) {
		strcpy (vendor_cfgfile, path);
		strcat (vendor_cfgfile, configfiles[i]);
		if ((cfg_fp = fopen (vendor_cfgfile, "r")) == (FILE *)0) {
			return (0);
		}

		/*
		 *	Read lines from the file.
		 */
		while (config_fgets (inbuf, sizeof inbuf, cfg_fp)) {
			/*
			 *	Split the line into tokens.
			 */
			argc = line_parse (inbuf, argv, 
				sizeof(argv) / sizeof(argv[0]));

			/*
			 * if the first should be either "display" or "#display"
			 * or if the number of tokens in the line is < 6 error
			 */
			if ((strcmp(argv[0],"display") && 
				(strcmp(argv[0],"#display"))) || (argc<6) )
				continue;

			/*
			 * found a valid line
			 * Now, figure out if it is an entry for 16 or 256 driver
			 */

			bp = (bdata *)MALLOC(sizeof(bdata));
			if ( !strcmp(argv[1],"VGA256") ) {
				/* 256 colors */
				sscanf (argv[3],"%s %s %dx%d", tmp1, tmp2, 
						&bp->xmax, &bp->ymax);
				bp->colors = 256;
			}
			else {
				/* 2 or 4 or 16 colors */
				sscanf (argv[3],"%s %s %dx%d %d", tmp1, tmp2, 
					&bp->xmax, &bp->ymax, &bp->colors);
			}

			bp->class =  (char *)MALLOC(strlen(argv[1])+1);
			bp->cmap_name =  (char *)MALLOC(strlen(argv[2])+1);
			bp->entry = (char *)MALLOC(strlen(tmp1)+1);
			bp->monitor = (char *)MALLOC(strlen(tmp2)+1);
			bp->initdriver = (char *)MALLOC(strlen(initdrivers[i])+1);
			strcpy (bp->class, argv[1]);
			strcpy (bp->cmap_name, argv[2]);
			strcpy (bp->entry, tmp1);
			strcpy (bp->monitor, tmp2);
			strcpy (bp->initdriver, initdrivers[i]);

			bd[num] = bp;
			num++;
		}
		fclose (cfg_fp);
	}
	return (num);
}

/*
 * returns num of vendors and the corresponding data in vendordata
 */
int
getVendorData (vendordata)
  vdata **vendordata;
{
    char 	*ptrs[MAXARGS];
    int  	num; 
    vdata	*vp;
    char	*buf;
    int		nvendors = 0;
    char	vendor_dbfile[MAXLINE];
    FILE 	*db_fp = (FILE *)0;
    char	inbuf[MAXLINE]; 	/* TEMP buffer for misc reads */

   /*
    * if XWINHOME is not set, set it to default (ie: /usr/X)
    */
   if (envp)
    {
	strcpy (vendor_dbfile, envp);
	strcat (vendor_dbfile, "/lib/vgainit/VGAVendorDB");
    }
    else {
	strcpy (vendor_dbfile, "/usr/X/lib/vgainit/VGAVendorDB");
    }

    if ((db_fp = fopen (vendor_dbfile,"r")) == (FILE *)0) {
	printf ("Cannot open Vendor database file : %s\n", vendor_dbfile);
	return (0);
    }

    while (config_fgets (inbuf, sizeof(inbuf), db_fp) ) {
	/*
	 *	Split the line into tokens.
	 */
	num = line_parse (inbuf, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));
	if ( (num != 4) || (inbuf[0] == '#') )
		continue;
	vp = (vdata *)MALLOC(sizeof(vdata));
	vp->vendor = (char *)MALLOC(strlen(ptrs[0])+1);
	vp->chipset = (char *)MALLOC(strlen(ptrs[1])+1);
	vp->cfgfiles = (char *)MALLOC(strlen(ptrs[2])+1);
	vp->idrivers = (char *)MALLOC(strlen(ptrs[3])+1);
	strcpy(vp->vendor, ptrs[0]);
	strcpy(vp->chipset, ptrs[1]);
	strcpy(vp->cfgfiles, ptrs[2]);
	strcpy(vp->idrivers, ptrs[3]);

	vendordata[nvendors++] = vp;
	vendordata[nvendors] = NULL;
    }

    fclose (db_fp);
    return (nvendors);
}

printVendorData (vendordata, num_vendors)
  vdata **vendordata;
  int	num_vendors;
{
	int i = 0;

	printf ("\n%4s %-20s %-15s\n",   "#id", "Vendor", "Chipset" );
	printf ("%4s %-20s %-15s\n", "===", "======", "=======");
 
	for (i=0; i<num_vendors; i++) {
	   printf ("%4d %-20s %-15s\n", 
		i,
		vendordata[i]->vendor,
		vendordata[i]->chipset);
	}
}

printBoardData (mdata, num)
  bdata **mdata;
  int	num;
{
	int 	i = 0;
	char 	c;
    	FILE	*fp;

	if ( (fp = fopen("/tmp/boardlist", "w")) != NULL) {
	    fprintf (fp, "\n%4s %10s %13s %6s %12s\n", "mode",
			"resolution", "Monitor", "colors", "entry");
	    fprintf (fp, "%4s %10s %13s %6s %12s\n", "====",
			"==========", "=======", "======", "=====");

	    for (i=0; i<num; i++) {
		fprintf (fp, "%4d %4dx%d %15s %6d %12s \n", i, 
			mdata[i]->xmax, mdata[i]->ymax, 
			mdata[i]->monitor,
			mdata[i]->colors,
			mdata[i]->entry);
	    }
	    fclose (fp);
	    system ("pg -p 'Press `'-'` for previous page; `'ENTER/RETURN'` for more choices .... ' /tmp/boardlist");
	    printf ("The list presented here is also saved in :  /tmp/boardlist\n");
	}
}

int
generateCFGfile (vendordata, vendornum, boarddata, mode, monitor_width, monitor_height)
  vdata **vendordata;
  int	vendornum;
  bdata **boarddata;
  int	mode;
  float	monitor_width;
  float	monitor_height;
{
    FILE	*fp;
    FILE	*from, *to;
    char	tbuf[MAX_TOKENSIZE];
    char	tname[MAX_TOKENSIZE];
    char	initdriver[MAX_TOKENSIZE];
    char	xwincfgfile[MAX_TOKENSIZE];
    struct	stat buf;
    extern char *getenv(const char *);
    extern int  getpid ();

	/*
	 * if XWINHOME is not set, set it to default (ie: /usr/X)
	 */
	if (envp)
		strcpy (initdriver, envp);
	else
		strcpy (initdriver, "/usr/X");

	strcat (initdriver, "/lib/");

	strcpy (xwincfgfile, envp);
	strcat (xwincfgfile, CONFIGFILE);

	/*
	 * generate the config file
	 * default config file : /usr/X/defaults/Xwinconfig
	 * if XWINHOME env is set, xwincfgfile will be $XWINHOME/defaults/Xwinconfig
	 */
	/* strcpy (tname, envp); */
	if ( (fp = fopen("/tmp/XTMPCFGFILE", "w")) != NULL) {
		if (boarddata[mode]->colors == 16) {
			strcpy (tname, "lib/libvga16.so");
			fprintf (fp, "display %s %s %c%s %s %dx%d %d %.2fx%.2f%c 0 /dev/console %s\n",
				boarddata[mode]->class,
				boarddata[mode]->cmap_name,
				'"',
				boarddata[mode]->entry,
				boarddata[mode]->monitor,
				boarddata[mode]->xmax,
				boarddata[mode]->ymax,
				boarddata[mode]->colors,
				monitor_width,
				monitor_height,
				'"',
				tname);
		}
		else {
			strcpy (tname, "lib/libvga256.so");
			fprintf (fp, "display %s %s %c%s %s %dx%d %.2fx%.2f%c 0 /dev/console %s\n",
				boarddata[mode]->class,
				boarddata[mode]->cmap_name,
				'"',
				boarddata[mode]->entry,
				boarddata[mode]->monitor,
				boarddata[mode]->xmax,
				boarddata[mode]->ymax,
				monitor_width,
				monitor_height,
				'"',
				tname);
		}
		fclose (fp);
	
		/*
		 * Now, copy the appropriate init driver to the system dir.
		 * Before copying, we need to back up the existing one 
		 * because if it is in use and we overwrite on it, the next
		 * time a Vt switch is done, the server will core dump.
		 * So, move it to some unique name into /tmp directory.
		 * ex:
		 *     mv /usr/X/lib/libv16i.so.1 /tmp/v16i.so.pid 
		 * OR  mv /usr/X/lib/libv256i.so.1 /tmp/v256i.so.pid
		 *     cp /usr/X/lib/vgainit/t89_16i.so /usr/X/lib/libv16i.so.1
		 */

		/*
		 * if ( /usr/X/lib/libv??i.so.1 exists) then
		 * 	move it, ie:
		 * mv /usr/X/lib/libv??i.so /tmp/??i.so.pid
		 */
		sprintf (tname, "%s/libv%di.so.1", initdriver,
					boarddata[mode]->colors);
		if (!stat(tname, &buf)) {
		    sprintf (tname, "mv %s/libv%di.so.1 /tmp/v%d.so.%d", 
				initdriver,
				boarddata[mode]->colors,boarddata[mode]->colors,
				getpid());
		    system (tname);
		}

		/* cp /usr/X/lib/vgainit/??.so /usr/X/lib/libv??i.so.1 */

		sprintf (tbuf, "cp %svgainit/%s %slibv%di.so.1", 
			initdriver, boarddata[mode]->initdriver,
			initdriver, boarddata[mode]->colors);

		if ( !system (tbuf) ) {
			/*
			 * successfully copied the init driver, now mv the
			 * /tmp/XTMPCFGFILE to /usr/X/defaults/Xwinconfig
			 */
			strcpy (tbuf, "mv /tmp/XTMPCFGFILE ");
			strcat (tbuf, xwincfgfile);
			if ( system(tbuf) )
				return (FAIL);
		}
		else
			return (FAIL);
	}
	else {
		return (FAIL);
	}

	return (SUCCESS);
}

freememory ( vendordata, num_vendors, boarddata, num_entries )
  vdata **vendordata;
  int	num_vendors;
  bdata **boarddata;
  int	num_entries;
{
	int i;

	for (i=0; i<num_vendors; i++) {
	    /* free the individual strings */
	    FREE (vendordata[i]->vendor);
	    FREE (vendordata[i]->chipset);
	    FREE (vendordata[i]->cfgfiles);
	    FREE (vendordata[i]->idrivers);
	    /* now, free the actual data structure, ie: vdata */
	    FREE (vendordata[i]);
	}

    	for (i=0; i<num_entries; i++) {
		/* free the individual strings */
		FREE (boarddata[i]->entry);
		FREE (boarddata[i]->monitor);
		FREE (boarddata[i]->initdriver);
		/* now, free the actual data structure, ie: bdata */
		FREE (boarddata[i]);
    	}
}

printCurrentModeData ()
{
    char 	*ptrs[MAXARGS];
    char	*buf;
    int		num = 0;
    char	cfgfile[MAXLINE];
    FILE 	*fp = (FILE *)0;
    char	inbuf[MAXLINE]; 	/* TEMP buffer for misc reads */

   /*
    * if XWINHOME is not set, set it to default (ie: /usr/X)
    */
   if (envp)
    {
	strcpy (cfgfile, envp);
	strcat (cfgfile, "/defaults/Xwinconfig");
    }
    else {
	strcpy (cfgfile, "/usr/X/defaults/Xwinconfig");
    }

    if ((fp = fopen (cfgfile,"r")) == (FILE *)0) {
	printf ("There is no current Xwinconfig file.\n");
	return (0);
    }

    while (config_fgets (inbuf, sizeof(inbuf), fp) ) {
	/*
	 * Split the line into tokens.
	 */
	num = line_parse (inbuf, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));
	if ( (num != 7) || (inbuf[0] == '#') )
		continue;

	printf ("You can quit anytime by hitting the DEL key. The current Entry is: \n\n");
	printf ("%s %s %s %s %s %s %s\n", 
			ptrs[0], ptrs[1], ptrs[2], ptrs[3], 
			ptrs[4], ptrs[5], ptrs[6]);
	fclose (fp);
	return (1);
    }

    printf ("There are no valid entris in the current Xwinconfig file.\n");
    fclose (fp);
    return (0);
}

#ifdef XCLIENT
main(argc, argv)
 int argc;
 char **argv;
{
	char 	ret;
	vdata	*vendordata[MAXVENDORS];
	bdata	*boarddata[MAXENTRIES];
	int	vendornum, mode;
	char	ok;
	extern char *getenv(const char *);


	printf ("NOT IMPLEMENTED YET !!\n");
}

#endif
