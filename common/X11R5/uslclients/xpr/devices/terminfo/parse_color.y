%{
#ident	"@(#)xpr:devices/terminfo/parse_color.y	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "Xlib.h"

#include "xpr.h"

#include "xpr_term.h"

static int		yylex();

static int		*local_pncolors,
			last_band;

static XColor		**local_pcolors;

static char		*color_string;

static void		yyerror();

struct string {
	char			*start;
	int			len;
};

static unsigned long	band_mask;

/**
 ** parse_color()
 **/

void			parse_color (string, pncolors, pcolors)
	char			*string;
	int			*pncolors;
	XColor			**pcolors;
{
	if (!*pcolors) {
		/*
		 * There are "colors" primary colors plus black.
		 * This means there are 2-to-the-N possible color
		 * combinations, where N is "colors" + 1.
		 * Here is one reason why we can't deal with too
		 * many colors!
		 */
		register long		two_to_the_N = 1 << (colors + 1);


		*pcolors = (XColor *)Malloc(two_to_the_N * sizeof(XColor));

		/*
		 * A pixel value of 0 is white.
		 */
		(*pcolors)[0].pixel = 0;
		(*pcolors)[0].red = 65535;
		(*pcolors)[0].green = 65535;
		(*pcolors)[0].blue = 65535;

		/*
		 * A pixel value of 1 is black (i.e. band 1 is defined to
		 * be black in Terminfo).
		 */
		(*pcolors)[1].pixel = 1;
		(*pcolors)[1].red = 0;
		(*pcolors)[1].green = 0;
		(*pcolors)[1].blue = 0;

		last_band = 1;
		band_mask = two_to_the_N - 1;

		*pncolors = 2;
	}

	color_string = string;
	local_pncolors = pncolors;
	local_pcolors = pcolors;

	yyparse ();

	return;
}

/**
 ** YACC GRAMMER
 **/
%}

%union {
	char			*pointer;
	int			integer;
	unsigned long		bandmask;
	struct string		string;
	XColor			color;
	int			none;
}

%start	colorlist

%token		<pointer>	LETTER
%token		<integer>	DIGIT

%type		<integer>	integer
%type		<string>	string colorname
%type		<color>		rgblist simplecolor
%type		<bandmask>	colorsum
%type		<none>		color colorlist

%%

color
:	simplecolor
	{
		add_primary_color ($1);
	}
|	simplecolor '=' colorsum
	{
		add_overstruck_color ($1, $3);
	}
;

colorlist
:	colorlist ',' color
|	color
;

colorname
:	string
	{
		$$ = $1;
	}
;

colorsum
	/*
	 * By the time we start parsing ``color sums'' we should
	 * have seem all the primary colors and assigned them to
	 * ribbon bands; this is necessary because we now need to
	 * convert each color in the sum to a band.
	 */
:	simplecolor
	{
		$$ = lookup_band($1);
	}
|	colorsum '+' simplecolor
	{
		$$ = $1 | lookup_band($3);
	}
;

rgblist
:	'(' integer ',' integer ',' integer ')'
	{
#define TI_RGB_SCALE(X)	((long)X * 65535) / 1000;

		$$.red = TI_RGB_SCALE($2);
		$$.green = TI_RGB_SCALE($4);
		$$.blue = TI_RGB_SCALE($6);
	}
;

simplecolor
:	colorname
	{
		int			c = $1.start[$1.len];


		$1.start[$1.len] = 0;
		$$ = *(lookup_color($1.start));
		$1.start[$1.len] = c;
	}
|	rgblist
	{
		$$ = $1;
	}
;

string
:	LETTER
	{
		$$.start = $1;
		$$.len = 1;
	}
|	string LETTER
	{
		$$.len++;
	}
;

integer
:	DIGIT
	{
		$$ = $1;
	}
|	integer DIGIT
	{
		$$ = $1 * 10 + $2;
	}
;

%%

/**
 ** yylex()
 **/

static int		yylex ()
{
	int			c;

	for (;;) {
		switch ((c = *color_string++)) {
		case 0:
			return (0);

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			yylval.integer = c - '0';
			return (DIGIT);

		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'i': case 'I':
		case 'j': case 'J':
		case 'k': case 'K':
		case 'l': case 'L':
		case 'm': case 'M':
		case 'n': case 'N':
		case 'o': case 'O':
		case 'p': case 'P':
		case 'q': case 'Q':
		case 'r': case 'R':
		case 's': case 'S':
		case 't': case 'T':
		case 'u': case 'U':
		case 'v': case 'V':
		case 'w': case 'W':
		case 'x': case 'X':
		case 'y': case 'Y':
		case 'z': case 'Z':
		case ' ':
			yylval.pointer = color_string - 1;
			return (LETTER);

		case '(':
		case ')':
		case '+':
		case '=':
		case ',':
			return (c);

		default:
			break;
		}

	}
}

/**
 ** lookup_color() - CONVERT COLOR NAME TO RGB TRIPLE
 **/

static XColor		*lookup_color (name)
	char			*name;
{
	static XColor		nocolor	= { 0, 0, 0, 0 },
				ret;

	register struct colorname *p;


#define RGB_SCALE(X)	((long)X * 65535) / 255

	for (p = colornames; p->name1; p++)
		if (STREQU(name, p->name1) || STREQU(name, p->name2)) {
			ret.red = RGB_SCALE(p->red);
			ret.green = RGB_SCALE(p->green);
			ret.blue = RGB_SCALE(p->blue);
			return (&ret);
		}

	fprintf (
		stderr,
		"xpr: Warning: \"%s\" is not a recognized color.\n",
		name
	);
	return (&nocolor);
}

extern char		*lsearch(),
			*lfind();

/**
 ** lookup_band() - CONVERT RGB TRIPLE TO RIBBON BAND NUMBER
 ** add_primary_color() - ADD PRIMARY (RIBBON BAND) COLOR TO LIST
 ** add_overstruck_color() - ADD OVERSTRUCK (BUILT UP) COLOR TO LIST
 **/

/*
 * SURPRISE! ``Bands'' are pixel values, so the ``list'' is the
 * final "XColor" list we need!
 */

static int		color_cmp (pa, pb)
	XColor			*pa,
				*pb;
{
	/*
	 * Return 0 if the same, non 0 otherwise.
	 */
	return (!(
		pa->red == pb->red
	     && pa->green == pb->green
	     && pa->blue == pb->blue
	));
}

static unsigned long	lookup_band (rgb)
	XColor			rgb;
{
	XColor			key,
				*p;


	key.pixel = 0;
	key.red = rgb.red;
	key.green = rgb.green;
	key.blue = rgb.blue;

	p = (XColor *)lfind(
		(char *)&key,
		(char *)(*local_pcolors),
		local_pncolors,
		sizeof(key),
		color_cmp
	);

	return (p? p->pixel : 0);
}

static void		add_primary_color (rgb)
	XColor			rgb;
{
	XColor			*p;


	rgb.pixel = 0;

	p = (XColor *)lsearch(
		(char *)&rgb,
		(char *)(*local_pcolors),
		local_pncolors,
		sizeof(rgb),
		color_cmp
	);

	if (!p->pixel) {
		/*
		 * If we attempt to define more colors than given
		 * in the Terminfo "colors" number, then "last_band"
		 * will shift out to zero, meaning that the pixel
		 * value stored will be zero: This will cause image
		 * colors that are close to the RGB value to map
		 * into white on the printer--no ink!
		 */
		last_band <<= 1;
		last_band &= band_mask;
		p->pixel = last_band;
	}

	return;
}

static void		add_overstruck_color (rgb, band_mask)
	XColor			rgb;
	unsigned int		band_mask;
{
	rgb.pixel = band_mask;

	(void)lsearch (
		(char *)&rgb,
		(char *)(*local_pcolors),
		local_pncolors,
		sizeof(rgb),
		color_cmp
	);
	return;
}

/**
 ** yyerror() - HANDLE SYNTAX ERRORS
 **/

static void		yyerror (s)
	char			*s;
{
	fprintf (
		stderr,
		"xpr: Error: Syntax error in ribbon color specification\n",
		s
	);
	exit (1);
	return;
}
