/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:tam.h	1.1"
/* user-includeable tam definition file		*/
#ifndef tam_h

#define tam_h

#include	"sys/window.h"
#include	"kcodes.h"

#include	"chartam.h"

#define NWINDOW 20	/* max # of windows in a single process	*/
			/* must be >= NOFILE in <sys/param.h>	*/

struct wstat { short begy,begx,height,width; unsigned short uflags; };
typedef struct wstat WSTAT;

extern short wncur;		/* current window */
extern int LINES, COLS;

#define A_STANDOUT A_REVERSE

#endif
