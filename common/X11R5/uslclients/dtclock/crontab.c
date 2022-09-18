/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:crontab.c	1.3"
#endif 

/*
 * crontab.c
 *
 * Desktop UNIX(r) System crontab edit functions
 *
 */

#include <stdio.h>

#include <crontab.h>

static int EditCrontabEntry(char, char *, char *);

#ifdef MAKE_MAIN

main(argc, argv)
int argc;
char * argv[];
{
   switch (*argv[1])
   {
      case 'a':
         AddCrontabEntry(argv[2]);
         break;
      case 'd':
         DeleteCrontabEntry(argv[2]);
         break;
      case 'r':
         ReplaceCrontabEntry(argv[2], argv[3]);
         break;
      default:
         fprintf(stderr, "usage: %s [a | r | d] [keyword] [entry]\n", argv[0]);
         break;
   }
} /* end of main */
#endif
/*
 * AddCrontabEntry
 *
 */

extern int
AddCrontabEntry(char * entry)
{

   EditCrontabEntry('a', "", entry);

} /* end of AddCrontabEntry */
/*
 * DeleteCrontabEntry
 *
 */

extern int
DeleteCrontabEntry(char * keyword)
{

   EditCrontabEntry('d', keyword, "");

} /* end of DeleteCrontabEntry */
/*
 * ReplaceCrontabEntry
 *
 */

extern int
ReplaceCrontabEntry(char * keyword, char * entry)
{

   EditCrontabEntry('r', keyword, entry);

} /* end of ReplaceCrontabEntry */
/*
 * EditCrontabEntry
 *
 */

static int
EditCrontabEntry(char option, char * keyword, char * entry)
{
/*
 * FIX: is croncmd big enough?
 */
   char croncmd[4096];

   (void)sprintf(croncmd, "/usr/X/bin/cronedit.sh -%c \"%s\" \"%s\"", 
      option, keyword, entry);

   system(croncmd);

} /* end of EditCrontabEntry */
