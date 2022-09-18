/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:track.c	1.1"
#include "cvttam.h"
#include "track.h"

int
TAMtrack (w)
short w;
{
  if (w != TamWin2int(CurrentWin)) {
    return TERR_SYS;
  }
  return TAMwgetc (w);
}
