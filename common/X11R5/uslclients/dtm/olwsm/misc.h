/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#pragma ident	"@(#)dtm:olwsm/misc.h	1.16"
#endif

#ifndef _MISC_H
#define _MISC_H

typedef char *			ADDR;

/*
 *	useful defines
 */
#define ASSERT(x)		_ASSERT((char *)(x),__LINE__,__FILE__)
#define DIMENSION(x)		(sizeof(x)/sizeof(x[0]))
#define ELEMENT(x)		(x *)MALLOC(sizeof(x))
#define ARRAY(x, n)		(x *)MALLOC(sizeof(x) * (n))
#define MATCH(p, q)		(strcmp((p), (q)) == 0)
#define MATCHN(p, q, n)		(strncmp((p), (q), (int)(n)) == 0)
#define MIN(i, j)		((i) < (j) ? (i) : (j))
#define MAX(i, j)		((i) > (j) ? (i) : (j))
#define ABS(i)			((i) > 0 ? (i) : -(i))
#define BCOPY(p, q, n)		(bcopy((char *)(p), (char *)(q), (int)(n)))
#define STRNDUP(p, n)		(_strndup(MALLOC((n) + 1), (p), (n)))
#define CONCAT(p, q) \
	(strcat(strcpy(MALLOC(strlen(p) + strlen(q) + 1), (p)), (q)))
#define SWITCH(p)		{ char *_s_ = p;  if (!_s_) { ;
#define CASE(p)			} else if (MATCH(_s_, p)) {
#define DEFAULT			} else {
#define ENDSWITCH		} }

#define ERROR(x)		(void)fprintf x
/*
 *	debugging stuff
 */
#ifdef DEBUG
#define debug(x)		(void)fprintf x
#define trace(x)		(void)fprintf(stderr,\
					"%s: line = %d, file = %s\n",\
					x, __LINE__, __FILE__)
#else
#define debug(x)
#define trace(x)
#endif

/*
 *	externs in misc.c
 */
extern char *			_ASSERT();
extern char *			_strndup();

#ifdef NEED_BCOPY
extern int			bcopy();
extern int			bcmp();
extern int			bzero();
extern int			ffs();
#endif

#if	defined(__Ol_OpenLook_h__)
extern Widget		CreateCaption OL_ARGS((
	String			name,
	String			label,
	Widget			parent
));
extern void		BusyPeriod OL_ARGS((
	Widget			w,
	Boolean			busy
));
#endif

#endif
