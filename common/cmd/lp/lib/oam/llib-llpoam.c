/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/oam/llib-llpoam.c	1.9.1.3"
#ident	"$Header: $"
/*LINTLIBRARY*/

#include "oam.h"

/*	from file agettxt.c */

char * agettxt ( long msg_id, char *buf, int buflen)
{
    static char			* _returned_value;
    return _returned_value;
}

/*	from file fmtmsg.c */

/**
 ** fmtmsg()
 **/
int fmtmsg ( char *label, int severity, int seqnum,
	     long int arraynum, int logind, ...)
{
    static int			 _returned_value;
    return _returned_value;
}
