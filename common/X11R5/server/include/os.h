/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/os.h	1.5"

/*copyright     "%c%"*/

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: os.h,v 1.39 89/12/06 16:41:26 rws Exp $ */

#ifndef OS_H
#define OS_H
#include "misc.h"

#ifdef INCLUDE_ALLOCA_H
#include <alloca.h>
#endif

#define NullFID ((FID) 0)

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2

#if defined(stellar)
#define MAX_REQUEST_SIZE 65535
#else
#define MAX_REQUEST_SIZE 16384
#endif

typedef pointer	FID;
typedef struct _FontPathRec *FontPathPtr;

typedef struct _NewClientRec *NewClientPtr;

#ifndef NO_ALLOCA
/*
 * os-dependent definition of local allocation and deallocation
 * If you want something other than Xalloc/Xfree for ALLOCATE/DEALLOCATE
 * LOCAL then you add that in here.
 */
#if defined(__HIGHC__)

extern char *alloca();

#if HCVERSION < 21003
#define ALLOCATE_LOCAL(size)	alloca((int)(size))
pragma on(alloca);
#else /* HCVERSION >= 21003 */
#define	ALLOCATE_LOCAL(size)	_Alloca((int)(size))
#endif /* HCVERSION < 21003 */

#define DEALLOCATE_LOCAL(ptr)  /* as nothing */

#endif /* defined(__HIGHC__) */


#ifdef __GNUC__
#define alloca __builtin_alloca
#define ALLOCATE_LOCAL(size) alloca((int)(size))
#define DEALLOCATE_LOCAL(ptr)  /* as nothing */
#else /* ! __GNUC__ */

#if defined(SYSV) || defined(SVR4)
extern char *allocz();
/* NOTE: allocz() is like alloca(), in that it allocates local storage
 *       on the stack, but it also null-fills it.  This addition is
 *       required in our current server implementation.  alloca() is
 *       much faster than malloc(), and allocz() null-fills about as
 *       inexpensively as you could wish, but you might could get away
 *       with defining the macro to be alloca() followed by a memset().
 *       (Just don't make it a procedure!)
 */

#define ALLOCATE_LOCAL(size) (allocz(size))
#define DEALLOCATE_LOCAL(ptr)   /* as nothing */
#define NULL_DEALLOCATE_LOCAL   /* allow optimizations which assume this */

#else /* !SYSV || !SVR4 */

/*
 * warning: mips alloca is unsuitable in the server, do not use.
 */
#if defined(vax) || defined(sun) || defined(apollo) || defined(stellar)
/*
 * Some System V boxes extract alloca.o from /lib/libPW.a; if you
 * decide that you don't want to use alloca, you might want to fix 
 * ../os/4.2bsd/Imakefile
 */
char *alloca();
#define ALLOCATE_LOCAL(size) alloca((int)(size))
#define DEALLOCATE_LOCAL(ptr)  /* as nothing */
#endif /* who does alloca */
#endif /* SYSV || USL */
#endif /* __GNUC__ */

#endif /* NO_ALLOCA */

#ifndef ALLOCATE_LOCAL
#define ALLOCATE_LOCAL(size) Xalloc((unsigned long)(size))
#define DEALLOCATE_LOCAL(ptr) Xfree((pointer)(ptr))
#endif /* ALLOCATE_LOCAL */


#define xalloc(size) Xalloc((unsigned long)(size))
#define xrealloc(ptr, size) Xrealloc((pointer)(ptr), (unsigned long)(size))
#define xfree(ptr) Xfree((pointer)(ptr))

int		ReadRequestFromClient();
char		*strcat();
char		*strncat();
char		*strcpy();
char		*strncpy();
Bool		CloseDownConnection();
FontPathPtr	ExpandFontNamePattern();
FID		FiOpenForRead();
void		CreateWellKnownSockets();
int		SetDefaultFontPath();
void		FreeFontRecord();
int		SetFontPath();
void		ErrorF();
void		Error();
void		FatalError();
void		ProcessCommandLine();
void		Xfree();
void		FlushAllOutput();
void		FlushIfCriticalOutputPending();
unsigned long	*Xalloc();
unsigned long	*Xrealloc();
long		GetTimeInMillis();

#endif /* OS_H */
