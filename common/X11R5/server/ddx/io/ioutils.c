/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/io/ioutils.c	1.15"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

#include	<stdio.h>
#include	<ctype.h>
#include    "X.h"
#include    "misc.h"
#include    <servermd.h>
#include    "dixstruct.h"
#include    "dix.h"
#include    "opaque.h"

#if defined(USG) || defined(SVR4)
# include	<string.h>
#else
# include	<strings.h>
#endif
#include    "siconfig.h"
#include    "site.h"

extern char *getenv(const char *);

extern	char	*display;		/* in server/os/386ix/connection.c */
extern Bool	screen_inited;	/* initialized in init.c */

extern char *strncpy();
extern GCPtr CreateScratchGC();

/*------------------------------------------------------------------------
 * RestoreOutput --
 *	Things which must be done before exiting the server.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Each display should be restored to the same as when the server
 *	was started.
 *
 *----------------------------------------------------------------------- */
RestoreOutput ()
{
	if (screen_inited)
		si_Restore ();
}

/*
 *	Routines for reading the configuration file.  The file should look
 *	something like this:

RESOURCE  CLASS  CMAP 		SDD_INFO   	    	      SCREEN  DEVICE LIBRARY
display	  VGA16	 StaticColor	"ET4000 MULTISYNC 800x600 .." 0.0  /dev/console /usr/X/lib/libvga16.so
display	  VGA256 PseudoColor	"ET4000 MULTISYNC 1024x768 .." 0.0    /dev/console /usr/X/lib/libvga256.so
display	  MF-T8  PseudoColor    'Sony 17"' 0.2	/dev/mft8	/lib/libt8.so

 *
 *	The comment character is '#', and causes the rest of the line to
 *	be ignored.  Blank lines are allowed.  Whitespace within strings
 *	may be quoted within pairs of either single or double quotes, and
 *	a single character may be escaped with a backslash (\) character.
 */

#define	SKIPSPACE(bp)		while (*(bp) != '\0' && isspace (*(bp))) (bp)++
#define	FINDSPACE(bp)		while (*(bp) != '\0' && !isspace (*(bp))) (bp)++

#define	MAX_LINESIZE	150	/* maximum length of a single line in file */
#define	MAXARGS	16	/* maximum number of arguments on one line */

#ifndef	CONFIG_FILE
#define	CONFIG_FILE	"Xwinconfig"
#endif

#ifndef	COLOR_FILE
#define	COLOR_FILE	"Xwincmaps"
#endif

static	FILE	*config_fp	= (FILE *)0;	/* open file pointer    */
char	*config_file	= NULL;		/* configuration file to use 	*/
char	*color_file	= NULL;		/* color file to use   		*/
char	*display_lib	= NULL;		/* useful only for SVR4: ptr to */
					/* the name of the dynamic lib	*/
static	SIConfig config_entry;		/* actual entry returned 	*/

/*
 * The following colors are in the same order as defined in X.h. For example,
 *
 *	StaticGray		0
 *	GrayScale		1
 *	StaticColor		2
 *	PseudoColor		3
 *	TrueColor		4
 *	DirectColor		5
 */
static	char *display_defaults[] = {
	"StaticGray",
	"GrayScale",
	"StaticColor",
	"PseudoColor",
	"TrueColor",
	"DirectColor",
	""
};

/*
 *	Search for the next entry for resource "resource", matching display
 *	"display".  Note that the search starts from the current file position,
 *	and that a 'config_setent()' should be done before starting a scan.
 */

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
		
		if (*str == '#' || *str == '\n')	/* comment, eol */
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
				 (isspace(*str) || *str == '#' || *str == '\n'))
				break;
			else
				*cp++ = *str;
		}

		/*
		 *	If we found a comment character or newline, quit.
		 */
		if (*str == '#' || *str == '\n') {
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
 *	Read the next line from the configuration file and process it.
 *	Opens the file if needed, returns null at end of file.
 */
SIConfigP
config_getent ()
{
	static	char	line[MAX_LINESIZE];
	char		*argv[MAXARGS];
	int		argc;
	register char	*cp;
	int		bad,
			val;
	static char *dlib 	= NULL;

	/*
	 *	Read lines from the file.
	 */
	while (config_fgets (line, sizeof line, config_fp)) {
		/*
		 *	Split the line into tokens.
		 */
		argc = line_parse (line, argv, sizeof(argv) / sizeof(argv[0]));

		/*
		 *	Need to have atleast 6 tokens on the line.
		 *	(resource, type, info, display, devname, [display lib])
		 */
		if (argc < 6)
			continue;

		if (line[0] == '#')
			continue;
		
		/*
		 *	Save resource, type, info, and devname.
		 */
		config_entry.resource = argv[0];
		config_entry.type     = argv[1];
		config_entry.deflt    = -1;
		config_entry.info     = argv[3];
		config_entry.display  = argv[4];
		config_entry.device   = argv[5];

		for (val=0, cp = display_defaults[val]; *cp;) {
		    if (strcmp(argv[2], cp) == 0) {
			config_entry.deflt = val;
			break;
		    }
		    val++;
		    cp = display_defaults[val];
		}
  
		/*
		 * The 7th arg is the dynamic library and is used in SVR4;
		 * if XWINDISPLIB is set, ignore the entry in the config file.
		 * if a 7th arg is given in a SVR32, it is not used anywhere....
		 */
		if ( (display_lib = getenv("XWINDISPLIB")) == NULL) {
		    if (argc == 7) {
			cp = (char *)GetXWINHome(argv[6]);
			if (dlib != NULL)
				Xfree (dlib);
			dlib = (char *)Xalloc(strlen(cp) + 1);
			strcpy (dlib, cp);
			display_lib = dlib;
		    }
		}

		/*
		 *	Now deal with the "display[.screen]" string.
		 */
		config_entry.displaynum = -1;
		config_entry.screen     = -1;	/* this part is optional */

		for (bad = val = 0, cp = argv[4]; *cp; cp++) {
			if (*cp == '.') {	/* between display & screen */
				if (config_entry.displaynum == -1)
					config_entry.displaynum = val;
				else		/* "nn.nn." ?? */
					bad = 1;
				val = 0;
			} else if (! isdigit (*cp))
				bad = 1;
			else
				val = val * 10 + *cp - '0';
		}

		if (bad)	/* display[.screen] spec was malformed */
			continue;

		if (config_entry.displaynum == -1)
			config_entry.displaynum = val;
		else		/* is this right? */
			config_entry.screen = val;

		return &config_entry;	/* it worked */
	}

	return (SIConfigP)0;	/* must have reached EOF */
}

#ifndef MAIN
SIConfigP
config_getresource (resource, display)
char	*resource,
	*display;
{
	SIConfigP	cf;
	char	*tfp, *cp;
	char	path[MAX_LINESIZE];
	unsigned short *prgb;

	/* extern char *getenv (); */

	/*
	 * Open the configuration file if not already open, else rewind it.
	 */
	if ( (tfp = getenv("XWINCONFIG")) != NULL)
		config_file = tfp;

	if (config_fp == (FILE *)0) {	/* file not open yet */
	   /*
	    * if configuration file name is given on command line, don't bother
	    */
	   if (!config_file) {
		cp = (char *) GetXWINHome ("defaults");
		if (cp != (char *) 0) {
			strcpy (path, cp);
			strcat (path, "/");
			strcat (path, CONFIG_FILE);
			config_file = path;
		}
		else
			config_file = "/usr/X/defaults/Xwinconfig";
	   }

	   if ((config_fp = fopen (config_file, "r")) == (FILE *)0) {
	    	    FatalError("Cannot open config file: %s.\n", config_file);
	   }
	   else {
		/*
		 * if the config_file is not user defined, set it to NULL,
		 * or else, when the server restarts (when last client disconnects)
		 * config_file points to junk....
		 */
		if (cp) 
			config_file = NULL;
	   }
	} else
		rewind (config_fp);	/* file already open, just rewind */

	/*
	 * read the data from the config file and initialize  config_entry
	 * data structure
	 */
	if ((cf = config_getent ()) != (SIConfigP)0) {
	     /* (void) fclose (config_fp);
	      config_fp = (FILE *)0; */
	}
	else 
		FatalError("Invalid Xwinconfig file.\n");

	return (&config_entry);
}

/*
 *	Routines for reading the colormap file.  The file should look
 *	like this:

#RESOURCE	TYPE   		SCREEN	SIZE
colormap  	StaticColor	0.0	16
	0, 0, 0,
	blue,
	red,
	0x3400, 0x3400, 0x3400,
	etc... (for a total of SIZE colors)
 */

/*
 *	Parse the colormap file to find a suitable colormap.
 */
io_readcmapdata(cmap)
siCmapP cmap;
{
	char		line[MAX_LINESIZE];
	char		*argv[MAXARGS];
	int		argc;
	int		i, j;
	int		bad,
			val,
			screen,
			display;
	unsigned short	*pshort, red, green, blue;
	register char	*cp = NULL;
	FILE    *color_fp       = (FILE *)0;    /* open file pointer     */

	extern unsigned short numtos();

	cmap->sz = 0;

	/*
	 *	Open the colormap and read the data
	 *	if color_file is already set (ie: via: command line option)
	 *	use that file name,
	 *	else if XWINHOME is set, get the path to $XWINHOME/defaults
	 *	else use /usr/X/defaults/Xwincmaps
	 */
	if (!color_file) {
		cp = (char *) GetXWINHome ("defaults");
		if (cp != (char *) 0) {
			strcpy (line, cp);
			strcat (line, "/");
			strcat (line, COLOR_FILE);
			color_file = line;
		}
		else
			color_file = "/usr/X/defaults/Xwincmaps";
	}

	if ((color_fp = fopen (color_file, "r")) == (FILE *)0)
			return -1;	/* fail */
	else {
		if (cp)
			color_file = NULL;
	}

	/*
	 *	Search for a colormap matching the requirements:
	 *	display, screen, and visual type must match.
	 */
	while (config_fgets (line, sizeof line, color_fp)) {
		/*
		 *	Split the line into tokens.
		 */
		argc = line_parse (line, argv, sizeof(argv) / sizeof(argv[0]));

		/*
		 * Need a line with 4 tokens.  That's the start of a 
		 * colormap description.  Individual colors have either
		 * 1 or 3 tokens (color name or hex values)
		 *
		 *	(colormap, type, size, display )
		 */
		if (argc == 4) {
			if (strcmp(argv[0], "colormap") != 0)
				continue;

			for (val=0, cp = display_defaults[val]; *cp;) {
				if (strcmp(argv[1], cp) == 0)
					break;
				val++;
				cp = display_defaults[val];
			}

			if (cmap->visual != val)
				continue;

			screen = 0;
			for (cp = argv[2]; *cp; cp++) {
				if (*cp == '.') {
					*cp = '\0';
					screen = atoi(cp+1);
					break;
				}
			}

			display = atoi(argv[2]);
			if ((display!=cmap->display) || (screen!=cmap->screen))
				continue;
		
			cmap->sz = atoi(argv[3]);

			/*
			 * If we make it here, we've found a colormap
			 */
			break;
		}
	}

	if (!cmap->sz) {
		(void) fclose (color_fp);
		return;
	}

	/*
	 * At this point, we know we've got a colormap that matches our needs.
	 * Allocate the storage for the colormap and read it in.
	 */
	if (cmap->colors)
		xfree(cmap->colors);
	cmap->colors = (unsigned short *)Xalloc(cmap->sz * sizeof(short) * 3);
	if (!cmap->colors) {
		cmap->sz = 0;
		(void) fclose (color_fp);
		return;
	}

	pshort = cmap->colors;
	for (i = 0; i < cmap->sz; i++) {
		if (config_fgets(line, sizeof line, color_fp)) {
			argc = line_parse(line, argv, 
					  sizeof(argv) / sizeof(argv[0]));

			/*
			 * if the first token is a number assume that it's
			 * RGB values, else get the ascii name and look in
			 * system RGB database and get the R, G and B values.
			 */
			if ( isdigit(**argv) ) {
			     if (argc == 3) {
				/*
				 * if the data is numerical RGB values, 
				 * read the data directly
				 */
				red = numtos(argv[0]);
				green = numtos(argv[1]);
				blue = numtos(argv[2]);
			      }
			}
			else {
				/*
				 * it's an ascii name; get the R, G and B values
				 * the name could be different tokens, concat
				 * them before calling OsLookupColor....
				 */
					
				strcpy (line, argv[0]);
				for (j=1; j<argc; j++)
					strcat (line, argv[j]);

				if (!OsLookupColor(0,line,strlen(line)-1,
						   &red, &green, &blue)) {
					ErrorF ("ioutils.c: OsLookupColor failed for color : %s, using white for this entry.\n", argv[0]);
					red = green = blue = ~0;
				}
			}
			*pshort++ = red;
			*pshort++ = green;
			*pshort++ = blue;
		} 
	}
	(void) fclose (color_fp);
}

/*
 * numtos(str)	-- convert and return the string passed in into a short value
 *			The string may be of the form:
 *				dddd	for a decimal number
 *				0xdddd	for a hex number
 *				0dddd	for an octal number
 */
unsigned short
numtos(str)
char *str;
{
	int val;

	if (str[0] == '0') {
		if (str[1] == 'x')			/* hex */
			sscanf(str, "0x%x", &val);
		else					/* octal */
			sscanf(str, "0%o", &val);
	}
	else						/* decimal */
		sscanf(str, "%d", &val);

	return ((unsigned short) val);
}
		
#else  /* if MAIN */
main (argc, argv)
int	argc;
char	**argv;
{
	SIConfigP	cf;

	if (argc > 2) {
		fprintf (stderr, "Usage: %s [config_file]\n", *argv);
		exit (1);
	}

	/*
	 * Open the configuration file if not already open, else rewind it.
	 */
	if (config_fp == (FILE *)0) {	/* file not open yet */
		if ((config_fp = fopen (config_file, "r")) == (FILE *)0)
		        return 0;
	} else
		rewind (config_fp);	/* file already open, just rewind */

	while ((cf = config_getent ()) != (SIConfigP)0) {
		printf ("resource {%s}\t", cf->resource);
		printf ("type {%s}\t", cf->type);
		printf ("info {%s}\t", cf->info);
		printf ("display {%d.%d}\t", cf->displaynum, cf->screen);
		printf ("device {%s}\n", cf->device);
	}

	if (config_fp != (FILE *)0) {
		(void) fclose (config_fp);
		config_fp = (FILE *)0;
	}

	exit (0);
}
#endif	/* MAIN */

/*
 * support functions for vt switching
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <signal.h>
#include "opaque.h"
#include "xwin_io.h"

/* Sun River Work */
extern Bool SunRiverStation;
char	    *phyaddr;

int waiting_for_acquire = -1;	/*
				 * Global that indicates state of VT switching
				 * out of the server:
				 *
				 * -1 = off (VT switching not allowed)
				 *  0 = not waiting for a VT switch (normal)
				 *  1 = waiting for a VT switch (after SIGUSR1)
				 *  2 = switch is being serviced (after RELDISP)
				 */

extern int	condev;
extern unchar	ledsdown, orgleds;

void
sigusr2(dummy)
int dummy;
{
	waiting_for_acquire = 0;	/* Set to not-waiting */
	signal(SIGUSR2, SIG_IGN);
}

void
sigusr1()
{
	if (waiting_for_acquire == 0)
		waiting_for_acquire = 1;	/* Set to waiting-for-service */
	signal(SIGUSR2, sigusr2);	/* This signal comes from the driver
					 * when it eventually switches back */
}

void
waitForSiguser2()
{
	void		(*hxsig)(); 
	extern int	xsig;

	sighold (SIGUSR1);	/* Don't want another one of these now... */
	SetKbdLeds(orgleds);
	waiting_for_acquire = 2;	/* Set to being-serviced;
					 * This must be above the ioctl().
					 */

	if ( si_savescreen() == -1 || ioctl(condev, VT_RELDISP, 1) == -1)
	{
		Error("Unable to release vt");
		si_restorescreen();
		SetKbdLeds(ledsdown);	/* Restore server's LED settings */
		sigusr2(0);
		sigrelse (SIGUSR1);
		return;
	}

	hxsig = signal(xsig, SIG_IGN);

	/* Now sleep while waiting for SIGUSR2 */
	while (waiting_for_acquire) {
		pause();
	}
	/* Acknowledge the acquire */
	ioctl(condev, VT_RELDISP, VT_ACKACQ);

	orgleds = GetKbdLeds();		/* get latest console setting */
	SetKbdLeds(ledsdown);		/* Restore server's LED settings */

	if ( si_restorescreen() == SI_FATAL )
		FatalError("Error restoring screen");

	signal(xsig, hxsig);
	sigrelse (SIGUSR1);	/* OK, critical section is through. */
}

/* Clean up what the server's done */
void
resetmodes()
{
	struct vt_mode	vtmode;

	sigignore (SIGUSR1);		/* Tough luck, going bye-bye */
	ioctl(condev, KDQUEMODE, 0);

	/*
	 * If VT switching isn't turned off, and one isn't currently being
	 * serviced, then reset the keyboard LEDs to their original state and
	 * unmap the display.
	 */
	if (waiting_for_acquire != -1 && waiting_for_acquire != 2)
		SetKbdLeds(orgleds);

	if (HWroutines) {
		si_Restore ();
		if (ioctl(condev, KDSETMODE, KD_TEXT) == -1)
			Error("KDSETMODE KD_TEXT failed");
		vtmode.mode = VT_AUTO;
		vtmode.relsig = SIGUSR1;
		vtmode.acqsig = SIGUSR2;
		vtmode.frsig = SIGUSR2;		/* rjk */
		vtmode.waitv = 0;		/* rjk */

		if (ioctl(condev, VT_SETMODE, &vtmode) == -1) 
			Error("VT_SETMODE VT_AUTO failed");
	}
	attclean ();
}

/* Sun River work: use different ioctl for Sun River Board */
int
SunRivGetVDCInfo(pvdcInfo, fd)
	struct kd_vdctype *pvdcInfo;
	int	fd;
{
    struct kd_disparam dispParam;

    if (ioctl (fd, KDDISPTYPE, &dispParam) == -1)
    {
     	ErrorF ("SunRivGetVDCInfo:  Can't determine display controller type\n");	attexit (1);
    }

    phyaddr = dispParam.addr;

    switch (dispParam.type) {
      case KD_VGA:
	pvdcInfo->cntlr = KD_VGA;
	pvdcInfo->dsply = KD_STAND_C;
	break;

      case KD_EGA:
	pvdcInfo->cntlr = KD_EGA;
	pvdcInfo->dsply = KD_STAND_C;
	break;

      default:
	ErrorF ("Unsupported display controller\n");
	attexit (1);
    }
}
