/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)libDtI:HyperText0.c	1.10"

/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperText0.c
**      Date:   08/06/90
**
****************************************************************/

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the hypertext widget.
 *
 ******************************file*header********************************
 */

                        /* #includes go here    */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>

#include "HyperTextP.h"

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define EATSPACE(P, C)	while (*(P) == C) (P)++
#define str_len(s) ((s) ? (int)strlen((s)) : 0)

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *      1. Private Procedures
 *      2. Public  Procedures
 *
 **************************forward*declarations***************************
 */
                    /* private procedures       */


static char * strn_copy();

static HyperLine *    HyperLineNew();
static HyperSegment * HyperSegmentNew();
static void           HyperSegmentFree();

static Widget h_widget;
static void h_start();
static void h_append_start();
static void h_push();
static void h_newline();
static void h_tab();
static char * h_key();
static void h_scan();
static void h_flush();
static char * h_parse();
static void do_command();
static void do_color();
static void do_key();


/* ALL variables below has to be reset by h_start */

/* hyper related variables */
static HyperSegment	*h_segment;
static HyperLine	*h_first_line;
static HyperLine	*h_line;

/* buffer related variables */
static char	h_buf[200];
static int	h_buf_i;

/* color related variables */
static Boolean	h_use_color;
static Pixel	h_color;

/* tabs count */
static int	h_tabs;


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/

Boolean
HyperSegmentRV(hs)
HyperSegment * hs;
{
    return(hs ? hs->reverse_video : FALSE);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
char *
HyperSegmentKey(hs)
HyperSegment * hs;
{
    return(hs ? hs->key : NULL);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
char *
HyperSegmentScript(hs)
HyperSegment * hs;
{
    return(hs ? hs->script : NULL);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
char *
HyperSegmentText(hs)
HyperSegment * hs;
{
    return(hs ? hs->text : NULL);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
HyperLine *
HyperLineFromString(widget, str)
Widget widget;
char * str;
{
    h_widget = widget;

    h_start();
    h_scan(str);
    return(h_first_line);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
HyperLine *
HyperLineFromFile(widget, file_name)
Widget widget;
char * file_name;
{
    FILE * f;
    char buf[200];
    char * str;
    char * s;

    h_widget = widget;

    if ((f = fopen(file_name, "r")) == NULL) {
	fprintf(stderr, "Cannot open hypertext file %s.\n", file_name);
	return(NULL);
    }

    h_start();
    for (str = fgets(buf, sizeof(buf)-1, f);
	 str;
	 str = fgets(buf, sizeof(buf)-1, f)) {
	/* append \0 */
	s = strchr(str, '\n');
	if (s) *++s = '\0';
	h_scan(str);
    }
    fclose(f);
    return(h_first_line);
}

/*--------------------- HyperSegment * Functions --------------------*/

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static HyperSegment *
HyperSegmentNew(text, len, key, script, tabs, use_color, color)
char * text;
int len;
char * key;
char * script;
int tabs;
Boolean use_color;
Pixel color;
{
    register HyperSegment * hs;

    hs = (HyperSegment *)calloc(1, sizeof(struct hyper_segment));

    hs->next = NULL;
    hs->prev = NULL;
    hs->text = HtNewString(text);
    hs->len = len;
    hs->tabs = tabs;
    hs->key = HtNewString(key);
    hs->script = HtNewString(script);
    hs->use_color = use_color;
    hs->color = color;
    hs->x = hs->y = hs->y_text = 0;
    hs->w = hs->h = 0;
    hs->reverse_video = FALSE;

    return(hs);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
HyperSegmentFree(hs)
HyperSegment * hs;
{
	if (hs == NULL)
		return;

	if (hs->text)
		free(hs->text);

	if (hs->key)
		free(hs->key);

	if (hs->script)
		free(hs->script);

	hs->text = hs->key = hs->script = NULL;
	free(hs);

} /* end of HyperSegmentFree */


/*
 *************************************************************************
 *
 * Private Procedures - HyperLine functions.
 *
 ***************************private*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static HyperLine *
HyperLineNew()
{
    register HyperLine * hl;

    hl = (HyperLine *)calloc(1, sizeof(struct hyper_line));

    hl->next = NULL;
    hl->first_segment = NULL;
    hl->last_segment  = NULL;

    return(hl);
} /* end of HyperLineNew */

/*
 ****************************procedure*header*****************************
 * Frees resources allocated for a HyperLine.
 *************************************************************************
*/
void
HyperLineFree(widget, hl)
Widget	widget;
HyperLine	*hl;
{
	HyperLine		*hl1;
	HyperSegment	*hs;
	HyperSegment	*hs1;

	if (hl == NULL)
		return;

	for (; hl; hl = hl1) {

		hl1 = hl->next;

		for (hs = hl->first_segment; hs; hs = hs1) {
			hs1 = hs->next;
			HyperSegmentFree(hs);
		}
		hl->first_segment = NULL;
		hl->last_segment = NULL;
		free(hl);
	}
} /* end of HyperLineFree */


/*---------------------- convert -------------------------*/

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
h_start()
{
    h_segment = NULL;
    h_first_line = h_line = NULL;
    h_buf_i = 0;
    h_use_color = FALSE;
    h_tabs = 0;
}

/*
 ****************************procedure*header*****************************
 * (hl could be NULL)
 *************************************************************************
*/
static void
h_append_start(hl)
HyperLine * hl;
{
    h_segment = NULL;

    h_first_line = hl;
    /* set h_line to the last line of hl */
    for ( ; hl && hl->next; hl = hl->next);
    h_line = hl; /* h_line is now either NULL or ->next is NULL */

    h_buf_i = 0;
    h_use_color = FALSE;
    h_tabs = 0;
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
h_new_segment(str, len, key, script)
char		*str;
int		len;
char		*key;
char		*script;
{
	HyperSegment * hs1;

	hs1 = HyperSegmentNew(str, len, key, script, h_tabs,
			h_use_color, h_color);

	/* have to update h_segment, h_line, and also reset h_tabs */
	if (h_segment == NULL) {
		HyperLine * hl1;

		hl1 = HyperLineNew();

		if (h_line != NULL)
			h_line->next = hl1;
		else
			h_first_line = hl1;

		h_line = hl1;
		h_line->first_segment = hs1;

	} else {
		hs1->prev = h_segment;
		h_segment->next = hs1;
	}

	h_segment = hs1;
	h_tabs = 0;

	/* keep updating last segment in hyper line for each new
	 * segment that's added so that it eventually contains the
	 * ptr to the last segment in the line.
	 */
	h_line->last_segment = hs1;

} /* end of h_new_segment */


/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
h_flush()
{
    if (h_buf_i == 0)
	return;

    h_buf[h_buf_i] = '\0';
    h_new_segment(h_buf, h_buf_i, NULL, NULL);
    h_buf_i = 0;
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
h_push(ch)
int ch;
{
    /* flush it first if buf is full */
    if (h_buf_i >= sizeof(h_buf)-1)
	h_flush();
    h_buf[h_buf_i++] = ch;
}

/*
 ****************************procedure*header*****************************
 * Since h_flush() may not call h_new_segment(), which will set h_tabs to 0,
 * tabs may grow larger than 1 under such condition.
 *************************************************************************
*/
static void
h_tab()
{
    h_flush();
    h_tabs++;
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
h_newline()
{
    h_flush();
    h_segment = NULL;
    h_tabs = 0;

    /* need a place holder so empty lines can be properly handled */
    h_new_segment(NULL, 0, NULL, NULL);
}

/*
 ****************************procedure*header*****************************
    escape commands can be
	\n  : new line
	\t  : tab
	\   : line continuation
	\c(red)  : color
	\b(red)  : blinking color
	\k(...)  : keyword
*/
static void
h_scan(s)
register char *s;
{
    register char c;

    if (s == NULL) {
	h_push(' ');
	h_flush();
	return;
    }

    while ((c = *s++)) {
	switch (c) {

	case '\\':                  /* escpace character */
	    switch (c = *s++) {
	    case '\0':              /* end of string */
		goto done;
	    case '\\':              /* normal \ character */
		h_push(c);
		break;
	    case '\n':              /* ignore new line character after \ */
		break;
	    case 'n':               /* new line */
		h_newline();
		break;
	    case 't':               /* tab */
		h_tab();
		break;
	    default: {               /* \cmd(arg) */
		char * s1;
		s1 = h_parse(s-1);
		if (s1 == s-1) {
		    h_push('\\');
		    h_push(c);
		    s1++;
		}
		s = s1;
		}
		break;
	    }
	    break;

	case '\n':                  /* newline */
	    h_newline();
	    break;

	case '\t':                  /* tab */
	    h_tab();
	    break;

	default:                    /* normal character */
	    h_push(c);
	    break;
	}
    }
done:
    h_flush();
}

/*
 ****************************procedure*header*****************************
 * Input: s0 pointto the character after '\'
 *************************************************************************
*/
static char *
h_parse(s0)
char * s0;
{
    char * s;
    char * s1;
    char * cmd;
    char * arg;
    int delimiter;
    register int len;

    s = s0;

    /* find the command name first */
    len = 0;
    s1 = s;
    while (isalnum(*s)) s++;
    len = s - s1;

    /* no name follow the \ character, give up */
    if (len == 0)
	return(s0);

    cmd = strn_copy(s1, len);
    arg = HtNewString("");

    /* parse argument, first we get the delimiter */
    if (isprint(*s)) {  /* but we know is not alnum */
	delimiter = *s;

	/* adjust delimiter */
	switch (delimiter) {
	case '(': delimiter = ')'; break;
	case '<': delimiter = '>'; break;
	case '{': delimiter = '}'; break;
	/* this one is used in setting up links in a table of contents */
	case '$': delimiter = '$'; break;
	default: break;
	}

	s1 = ++s;
	s = strchr(s1, delimiter);
	if (s == NULL) { /* no matching delimiter, give up */
	    fprintf(stderr,
		    "can not found matching delimiter '%c' for command '%s'\n",
		    delimiter, cmd);
		if (cmd)
			free(cmd);
		if (arg)
			free(arg);
	    return(s1-1);
	}
	len = s - s1; /* len may be 0 */
	if (arg)
		free(arg);
	arg = strn_copy(s1, len);
	++s;
	}
    else {
		if (cmd)
			free(cmd);
		if (arg)
			free(arg);
		return(s0);
    }

    do_command(cmd, arg);
	if (cmd)
		free(cmd);
	if (arg)
		free(arg);

    return(s);
    }

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
do_command(cmd, arg)
register char * cmd;
register char * arg;
{
    h_flush();

    if (strcmp(cmd, "c") == 0 || strcmp(cmd, "color") == 0) {
	do_color(arg, FALSE);
    }
    else if (strcmp(cmd, "k") == 0 || strcmp(cmd, "key") == 0) {
	do_key(arg, FALSE);
    }
    else if (strcmp(cmd, "d") == 0 || strcmp(cmd, "def") == 0) {
	do_key(arg, TRUE);
    }
    else if (strcmp(cmd, "b") == 0 || strcmp(cmd, "blink") == 0) {
	do_color(arg, TRUE);
    }
}

/*
 ****************************procedure*header*****************************
 * Return color pixel given a color name.
 *************************************************************************
*/
static Pixel
get_color_pixel(widget, colorname)
Widget widget;
char * colorname;
{
    Pixel color;
    XrmValue  from_value, to_value;

    from_value.size = strlen(colorname);
    from_value.addr = colorname;

    XtConvert(widget, XtRString, &from_value, XtRPixel, &to_value );

    if (to_value.addr == NULL) {
/*
     fprintf(stderr, "Warning: Couldn't allocate color %s\n", colorname);
*/
     return(BlackPixelOfScreen(XtScreen(widget)));
    }
    else
     return(*(Pixel *) to_value.addr);
} /* end of get_color_pixel */

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
do_color(arg, blink_flag)
char * arg;
Boolean blink_flag;
{
    if (arg[0] == '\0')
		h_use_color = FALSE;
    else {
		h_use_color = TRUE;
		h_color = get_color_pixel(h_widget, arg);
    }
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
do_key(arg, reverse_video)
char * arg;
Boolean reverse_video;
{
    char * name;
    char * label;
    char * script;


    label = name = arg;

    /* find the first non-escaped caret */
    script = strchr(name, '^');

    while (script && *(script-1) == '\\') {
		script = strcpy(script-1, script);
		script = strchr(script+1, ',');
    }

    if (script)
	*script++ = '\0';

	/* eat leading spaces in script */
	EATSPACE(script, ' ');

    h_new_segment(label, strlen(label), name, script);
    h_segment->reverse_video = reverse_video;
}


/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static char *
strn_copy(s, l)
register char * s;
register int l;
{
    register char * s1 = (char *)calloc(1, l + 1);
    register char * s2 = s1;
    int l1 = str_len(s);

    if (l1 < l) l = l1;
    if (l < 0) l = 0;
    if (s) while (l--) *s2++ = *s++;
    *s2 = '\0';
    return (s1);
}

#define stipple_width 16
#define stipple_height 16
static unsigned char stipple_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
void
x_fill_grids(display, window, color, x, y, width, height)
Display * display;
Window window;
long color;
int x, y;
int width, height;
{
    static GC gc;

    if (gc == 0) {
	Pixmap bm;
	Window rootwin;
	XGCValues values;
	bm = XCreatePixmapFromBitmapData(display,
					 rootwin = DefaultRootWindow(display),
					 (char *)stipple_bits,
					 stipple_width,
					 stipple_height,
					 0, /* foreground always off */
					 1, /* background always on  */
					 (unsigned int)1);
	values.stipple = bm;
	values.fill_style = FillStippled;
	gc = XCreateGC(display, rootwin,
		       (unsigned) GCStipple | GCFillStyle,
		       &values);
	XFreePixmap(display, bm);
    }
    XSetForeground(display, gc, color);
    XFillRectangle(display, window, gc, x, y, width, height);

} /* end of x_fill_grids */
