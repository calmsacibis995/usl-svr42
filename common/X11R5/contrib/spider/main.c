/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:main.c	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)main.c	2.1	90/04/25
 *
 */

/*
 * Spider solitaire
 */

#include	"defs.h"
#include	"globals.h"

static char	*display = NULL;
extern char	*getenv();

static void
usage(arg)
char	*arg;
{
	if (arg)
		(void) fprintf(stderr,"spider: illegal argument %s\n", arg);
	(void) fprintf(stderr,
	"usage: -display <display> -geometry < geometry> -save <save_file>\n");
}

main(argc, argv)
int	argc;
char	**argv;
{
int	i;
char	*save_file = NULL;
char	*geometry = NULL;

	/* argument processing */
	/* display, save file */
	for (i = 1; i < argc; i++)	{
		if (strncmp(argv[i], "-d", 2) == 0)	{
			display = argv[++i];
		} else if (strncmp(argv[i], "-g", 2) == 0)	{
			if (argv[i+1])	{
				geometry = argv[++i];
			} else	{
				usage(NULL);
				exit(-1);
			}
		} else if (strncmp(argv[i], "-s", 2) == 0)	{
			if (argv[i+1])	{
				save_file = argv[++i];
			} else	{
				usage(NULL);
				exit(-1);
			}
		} else	{
			usage(argv[i]);
			exit(-1);
		}
	}


	display_init();
	gfx_init(dpy, screen);
	window_init(argc, argv, geometry);
	card_init();
	if (save_file)
		read_file(save_file);
	event_loop();
	exit(0);
}

display_init()
{
	if ((dpy = XOpenDisplay(display)) == NULL)	{
		(void) fprintf(stderr,"Can't open display %s\n", 
			(display ? display : getenv("DISPLAY")));
		exit(-1);
	}
	screen = DefaultScreen(dpy);
#ifdef DEBUG
	XSynchronize(dpy, True);
#endif
}
