/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:dm_exit.h	1.1"
#endif

/*
 * dm_exit.h
 *
 */

#ifndef _dm_exit_h
#define _dm_exit_h

typedef struct _SessionManagementInfo
   {
      Window   SessionLeader;       /* base window that should be left alone */
      Window   SessionLeaderIcon;   /* icon window that should be left alone */
      int      ProcessTerminating;  /* flag indicating desktop exit state    */
      Window * WindowKillList;      /* windows pending WM_COMMAND state chg  */
      int      WindowKillCount;     /* number of windows in the kill list    */
      int      xerror;              /* error flag tracking X protocol errors */
      int      (*_XDefaultError)(); /* error function temporary storage      */
   } SessionManagementInfo;

extern SessionManagementInfo Session;

extern int  QueryAndKillWindowTree(Display *); /* RC == 0 ---> exit() */
extern void HandleWindowDeaths(XEvent *);
extern void RealExit(int);

#endif
