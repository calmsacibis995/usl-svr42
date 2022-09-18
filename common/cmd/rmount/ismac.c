/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rmount:ismac.c	1.1.1.2"
#ident  "$Header: ismac.c 1.1 91/06/28 $"
#include        <errno.h>
#include        <sys/types.h>
#include        <mac.h>
int
ismac()
{
        level_t level;

        return lvlproc(MAC_GET, &level) == -1 && errno == ENOPKG ? 0 : 1;
}
