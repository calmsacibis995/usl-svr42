#ident	"@(#)xpr:parse.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "stdio.h"

#include "xpr.h"
#include "xgetopt.h"

extern double		atof();

extern int		atoi(),
			getopt(),
			optind,
			opterr,
			optopt;

extern char		*optarg;

static void		usage ()
{
#define	P	(void) printf ("%s\n",
#define	X	);
P "usage:"								X
P "    xpr [options] [file]"						X
P "	[-o output-file]	(output to output-file (overwrite))"	X
P "	[-a output-file [-n]]	(append to output-file [no page break])"X
P "	[-d device-name]	(prepare output for device-name)"	X
P "	[-h string]		(center string 1/4 inch above image)"	X
P "	[-t string]		(center string 1/4 inch below image)"	X
P "	[-W n-inches]		(maximum width of image is n-inches)"	X
P "	[-H n-inches]		(maximum height of image is n-inches)"	X
P "	[-l]			(rotate 90 degrees left (landscape))"	X
P "	[-p]			(don't rotate (portrait) (default))"	X
P "	[-L n-inches]		(left margin of n-inches, else center)"	X
P "	[-T n-inches]		(top margin of n-inches, else center)"	X
P "	[-s n-pages]		(split output into n-pages)"		X
P "	[-S n-dots]		(scale each pixel to n-dots)"		X
P "	[-c]			(compact output (default, so obsolete))"X
P "	[-r]			(reverse \"video\")"			X
#if	defined(FOCUS)
P "	[-f location]		(focus in on large images)"		X
#endif
#if	defined(IMPROVE)
P "	[-i]			(improve image quality (runs slower))"	X
#endif
P "	[-C color-list]		(list ribbon colors)"			X
P "	[file]			(get input from file, else stdin)"	X
#undef P
#undef X
}

struct xoptions			xoptions[] = {
	'o', "output",		1,
	'a', "append",		1,
	'n', "noff",		0,
	'd', "device",		1,
	'h', "header",		1,
	't', "trailer",		1,
	'w', "width",		1,
	'W', 0,			1,	/* synonym for -w */
	'H', "height",		1,
	'l', "landscape",	0,
	'p', "portrait",	0,
	'L', "left",		1,
	'T', "top",		1,
	's', "split",		1,
	'S', "scale",		1,
	'c', "compact",		0,
	'r', "rv",		0,
#if	defined(FOCUS)
	'f', "focus",		1,
#endif
#if	defined(IMPROVE)
	'i', "improve",		0,
#endif
	'C', "color",		1,

	/*
	 * The following are here for compatibility with earlier
	 * versions of the xpr program. These were undocumented
	 * options, so did not show up in the requirements.
	 */
	'N', "nosixopt",	0,
	'R', "report",		0,

	/*
	 * The following is an undocumented option for changing
	 * the color mapping scheme. It is here until someone decides
	 * to take it out.
	 */
	'b', "bw",		1,

	0,   0,			0
};

static int		isterminfo();

/**
 ** parse_args()
 **/

void			parse_args (argc, argv, scale, width, height, left, top, device, flags, split, header, trailer, color_list)
	int			argc;
	char			**argv;
	int			*scale;
	double			*width,
				*height,
				*left,
				*top;
	Device			**device;
	int			*flags,
				*split;
	char			**header,
				**trailer,
				**color_list;
{
	register char		*output_filename	= 0,
				*s_device		= DEFAULT_DEVICE;

	int			optlet;
	int			a_option = 0, o_option = 0;


	*flags = 0;
	*split = 1;
	*width = -1;
	*height = -1;
	*top = -1;
	*left = -1;
	*header = 0;
	*trailer = 0;
	
	opterr = 0;
	while ((optlet = xgetopt(argc, argv, xoptions)) != -1)

		switch (optlet) {

		case 'a':
			if (o_option) {
				fprintf ( 
					stderr,
		"xpr: Error: The options -a/-append and -o/-output cannot be used together.\n"
				);
				exit (1);
			}
			a_option = 1;
			output_filename = optarg;
			/* See if file exists, else treat as -o option */
			if (!access(output_filename, 00))
				*flags |= F_APPEND;
			break;

		case 'd':
			s_device = optarg;
			break;

		case 'h':
			if (strlen (optarg))
				*header = optarg;
			break;

		case 'H':
			MyAtof(optarg,height,"height");
			CheckArgBad(*height, "height");
			CheckArgZero(*height, "height");
			break;

		case 'l':
			*flags |= F_LANDSCAPE;
			break;

		case 'L':
			MyAtof(optarg,left,"left");
			CheckArgBad(*left, "left");
			break;

		case 'n':
			*flags |= F_NOFF;
			break;

		case 'N':
			fprintf (
				stderr,
		"xpr: Warning: The -nosixopt option is obsolete.\n"
			);
			break;

		case 'o':
			if (a_option) {
				fprintf ( 
					stderr,
		"xpr: Error: The options -a/-append and -o/-output cannot be used together.\n"
				);
				exit (1);
			}
			o_option = 1;
			output_filename = optarg;
			break;

		case 'p':
			*flags |= F_PORTRAIT;
			break;

		case 'c':
			fprintf (
				stderr,
		"xpr: Warning: The -compact option is obsolete.\n"
			);
			break;

		case 'R':
			*flags |= F_REPORT;
			break;

		case 's':
			MyAtoi(optarg,split,"split");
			CheckArgBad((double) *split, "split");
			CheckArgZero((double) *split, "split");
			break;

		case 'S':
			MyAtoi(optarg,scale,"scale");
			CheckArgBad((double) *scale, "scale");
			CheckArgZero((double) *scale, "scale");
			break;

		case 't':
			if (strlen (optarg))
				*trailer = optarg;
			break;

		case 'T':
			MyAtof(optarg,top,"top");
			CheckArgBad(*top, "top");
			break;

		case 'w':
		case 'W':
			MyAtof(optarg,width,"width");
			CheckArgBad(*width, "width");
			CheckArgZero(*width, "width");
			break;

		case 'r':
			*flags |= F_INVERT;
			break;

#if	defined(FOCUS)
		case 'f':
			break;
#endif

#if	defined(IMPROVE)
		case 'i':
			break;
#endif

		case 'C':
			*color_list = optarg;
			break;

			/*
			 * This option is not supported, and is here
			 * for testing different color to black-and-white
			 * mappings. The argument should be the set of
			 * coefficients used to convert RGB into a single
			 * gray-scale value.
			 */
		case 'b':
			{
				extern double		atod();

#define DSTRTOK(S,D)	atof(strtok(S,D))
				rgbmap.red = DSTRTOK(optarg, ",");
				rgbmap.green = DSTRTOK((char *)0, ",");
				rgbmap.blue = DSTRTOK((char *)0, ",");
			}
			break;

		case '?':
			/*
			 * "optopt" is an undocumented feature of
			 * "getopt()". It appears to contain the option
			 * letter where things went wrong. If it's an
			 * option we recognize, the problem must be a
			 * missing argument.
			 */
			if (optopt == '?') {
				usage ();
				exit (0);
			} else {
				struct xoptions		*po;


				for (po = xoptions; po->letter; po++)
					if (po->letter == optopt)
						break;
				if (po->letter)
					fprintf (
						stderr,
		"xpr: Error: The option -%c requires an argument.\n",
						optopt
					);
				else
					fprintf (
						stderr,
		"xpr: Error: The option -%c is not recognized.\n",
						optopt
					);
			}
			exit (1);
			/*NOTREACHED*/

		}

	if (*flags & F_NOFF && !(*flags & F_APPEND))
		fprintf (
			stderr,
"xpr: Warning: The -n option is ignored unless the -a option is used.\n"
		);

	for (*device = device_list; (*device)->name; (*device)++)
		if (
			STREQU((*device)->name, s_device)
		     || (*device)->terminfo && isterminfo(s_device)
		)
			break;
	if (!(*device)->name) {
		fprintf (
			stderr,
			"xpr: Device \"%s\" not supported.\n",
			s_device
		);
		exit(1);
	}

	/*
	 * Other routines want to know if the output device
	 * is a postscript printer, thus the following:
	 */
	if (
		STREQU(s_device, "ps")
	     || STREQU(s_device, "lw")
	     || STREQU(s_device, "laserwriter")
	)
		s_device = "postscript";
	if ((*device)->terminfo)
		(*device)->name = strdup(s_device);

	if (optind < argc) {
		input_filename = argv[optind];
		if (!freopen(input_filename, "r", stdin)) {
			fprintf (
				stderr,
				"xpr: Error opening \"%s\" for input (%s).\n",
				argv[optind],
				PERROR (errno)
			);
			exit (1);
		}
	}

	if (output_filename) {
		if (!freopen(
			output_filename,
			(*flags & F_APPEND? "a+" : "w"),
			stdout
		)) {
			fprintf (
				stderr,
				"xpr: Error opening \"%s\" for output (%s).\n", 
				output_filename,
				PERROR (errno)
			);
			exit (1);
		}
		if (*flags & F_APPEND)
			fseek (stdout, 0L, 2);
		/*
		 * The device dependent routine(s) are responsible for
		 * backing up over a terminating ``formfeed'' in an
		 * appended file (if -noff), because a ``formfeed'' is
		 * not the same for all devices.
		 */
	}

	return;
}

/**
 ** isterminfo() - SEE IF NAME IS VALID AND USABLE TERMINFO ENTRY
 **/

static int		isterminfo (name)
	char			*name;
{
/*
 * MORE: This probably belongs in the terminfo library
 */
	/*
	 * "ps" is already used for a different device.
	 */
	if (STREQU(name, "ps"))
		name = "postscript";

	return (tidbit(name, "porder", (short *)0) != -1);
}

#define	HUGE	99999

CheckArgZero(value,name)
double	value;
char	*name;
{
	if ( value == 0) {
		fprintf (stderr,"xpr:  Error:  Value for %s out of range\n",name);
		exit (1);
	}
	return (0);
}

CheckArgBad(value,name)
double	value;
char	*name;
{
	if ( (value < 0) || (value > HUGE) ) {
		fprintf (stderr,"xpr:  Error:  Value for %s out of range\n",name);
		exit (1);
	}
	return (0);
}

MyAtof(token,variable,name)
char	*token;
double	*variable;
char	*name;
{
	if (*token != '.' && *token != '-' && ( *token < '0' || *token > '9')) {
		fprintf (stderr,"xpr:  Error:  Value for %s is non-numeric\n",name);
		exit(1);
	}
	*variable = atof(token);

}

MyAtoi(token,variable,name)
char	*token;
int	*variable;
char	*name;
{
	if (*token != '-' && (*token < '0' || *token > '9')) {
		fprintf (stderr,"xpr:  Error:  Value for %s must be an integer\n",name);
		exit(1);
	}
	*variable = atoi(token);
}
