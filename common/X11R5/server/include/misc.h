/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/misc.h	1.5"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: misc.h,v 1.54 90/03/08 14:14:45 keith Exp $ */

#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions 
 *
 */


extern unsigned long globalSerialNumber;
extern unsigned long serverGeneration;

#ifndef NULL
#if __STDC__ && !NOSTDHDRS
#include <stddef.h>
#else
#define NULL            0
#endif
#endif

#define MAXSCREENS	3
#define MAXCLIENTS	128
#define MAXFORMATS	8
#define MAXVISUALS_PER_SCREEN 50

typedef unsigned char *pointer;
typedef int Bool;
typedef unsigned long PIXEL;
typedef unsigned long ATOM;


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#include "os.h" 	/* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */

#define NullBox ((BoxPtr)0)
#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)

    /* this next is used with None and ParentRelative to tell
       PaintWin() what to use to paint the background. Also used
       in the macro IS_VALID_PIXMAP */

#define USE_BACKGROUND_PIXEL 3
#define USE_BORDER_PIXEL 3


/* byte swap a long literal */
#define lswapl(x) ((((x) & 0xff) << 24) |\
		   (((x) & 0xff00) << 8) |\
		   (((x) & 0xff0000) >> 8) |\
		   (((x) >> 24) & 0xff))

/* byte swap a short literal */
#define lswaps(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define abs(a) ((a) > 0 ? (a) : -(a))
#define fabs(a) ((a) > 0.0 ? (a) : -(a))	/* floating absolute value */
#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
/* this assumes b > 0 */
#define modulus(a, b)	((a) < 0 ? ((b) - (-(a) % (b))) : ((a) % (b)))
/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#if ! UTSXENV
#if #machine(i386)                     /* sure, I'll bite.  -jbm */
#define lowbit(x) ((x) & (-(x)))
#else
#define lowbit(x) ((x) & (~(x) + 1))
#endif
#else /* UTSXENV */
#define lowbit(x) ((x) & (~(x) + 1))
#endif /* UTSXENV */

#define MAXSHORT 32767
#define MINSHORT -MAXSHORT 


/* some macros to help swap requests, replies, and events */

#define LengthRestB(stuff) \
    (((unsigned long)stuff->length << 2) - sizeof(*stuff))

#define LengthRestS(stuff) \
    (((unsigned long)stuff->length << 1) - (sizeof(*stuff) >> 1))

#define LengthRestL(stuff) \
    ((unsigned long)stuff->length - (sizeof(*stuff) >> 2))

#define SwapRestS(stuff) \
    SwapShorts((short *)(stuff + 1), LengthRestS(stuff))

#define SwapRestL(stuff) \
    SwapLongs((long *)(stuff + 1), LengthRestL(stuff))

/* byte swap a long */
#define swapl(x, n) n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[3];\
		 ((char *) (x))[3] = n;\
		 n = ((char *) (x))[1];\
		 ((char *) (x))[1] = ((char *) (x))[2];\
		 ((char *) (x))[2] = n;

/* byte swap a short */
#define swaps(x, n) n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[1];\
		 ((char *) (x))[1] = n

/* copy long from src to dst byteswapping on the way */
#define cpswapl(src, dst) \
                 ((char *)&(dst))[0] = ((char *) &(src))[3];\
                 ((char *)&(dst))[1] = ((char *) &(src))[2];\
                 ((char *)&(dst))[2] = ((char *) &(src))[1];\
                 ((char *)&(dst))[3] = ((char *) &(src))[0];

/* copy short from src to dst byteswapping on the way */
#define cpswaps(src, dst)\
		 ((char *) &(dst))[0] = ((char *) &(src))[1];\
		 ((char *) &(dst))[1] = ((char *) &(src))[0];

extern void SwapLongs();
extern void SwapShorts();

typedef struct _DDXPoint *DDXPointPtr;
typedef struct _Box *BoxPtr;

/*
 * running the server in sdb for debugging is a pain; to disable mouse
 * interrupts, define SDB_MOUSE, and after the debugging session is over
 * comment this line.
 * Also see dix/dispatch.c, os/sysV/WaitFor.c ddx/io/xwin_io.c
 * ddx/io/init.c
 */
/* #define SDB_MOUSE 1 */

#endif /* MISC_H */
