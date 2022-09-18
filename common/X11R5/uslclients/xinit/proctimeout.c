/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xinit:proctimeout.c	1.2"
#endif
/*
 proctimeout.c (C source file)
	Acc: 575322353 Fri Mar 25 14:45:53 1988
	Mod: 573840597 Tue Mar  8 11:09:57 1988
	Sta: 573840597 Tue Mar  8 11:09:57 1988
	Owner: 2011
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* ProcessTimeout                                                     */
/*                                                                    */
/* This function is used to determine when a given pid becomes either */
/* valid (if waitingfor is passed as 0) or invalid (if waitingfor is  */
/* not 0).  It loops checking and sleeping the for the specified      */
/* number of seconds.  If string is not NULL the message "waiting     */
/* for <string>" is output to stderr before the first check is made   */
/* and " done!\n" is output when before the routine returns.  If      */
/* blip is also not null it is output before each sleep of 1 second.  */
/*                                                                    */
/* The return value is interpreted as:                                */
/*                                                                    */
/* waitingfor was      return value is     meaning                    */
/*                                                                    */
/*        0                  0             the pid did not become     */
/*                                         valid during the specified */
/*                                         time (timeout).            */
/*                                                                    */
/*        0                > 0             the pid was found to be    */
/*                                         valid at time return value */
/*                                                                    */
/*       -1                  0             the pid remained valid     */
/*                                         during the time period     */
/*                                                                    */
/*       -1                > 0             the pid became invalid at  */
/*                                         the time return value.     */

#include <stdio.h>
#define TESTFORVALIDPID    0

#define OPENLOOK
#ifdef OPENLOOK
extern char * basename;
#endif

ProcessTimeout( pid, timeout, pause, string, blip, donestring, waitingfor)
int             pid;
int		timeout;
char          * string;
char          * blip;
char          * donestring;
int		waitingfor;
{
int KillReturned;

if (string) 
#ifdef OPENLOOK
   {
   fprintf(stderr, "\n%s: waiting for %s", basename, string);
   fflush(stderr);
   }
#else
   { fprintf(stderr, "\nxinit: waiting for %s", string); fflush(stderr); }
#endif

while ( timeout > 0 )
   {
   if ( (KillReturned = kill(pid, TESTFORVALIDPID)) == waitingfor )
      break;

   if (blip) { fprintf(stderr, "%s", blip); fflush(stderr); }

   if (--timeout) 
      sleep (pause);
   }

if ( donestring ) { fprintf(stderr, "%s", donestring); fflush(stderr); }

return timeout;

} /* end of processTimeout */
