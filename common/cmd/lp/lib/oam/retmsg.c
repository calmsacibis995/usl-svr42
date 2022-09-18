/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/oam/retmsg.c	1.3.1.3"
#ident	"$Header: $"
/* LINTLIBRARY */

#include <stdio.h>
#include <string.h>
#include "oam.h"
#include <locale.h>

/**
 ** retmsg()
 **/

char *
#if	defined(__STDC__)
retmsg (
	int                     seqnum,
	long int                arraynum
)
#else
retmsg ( seqnum, arraynum )
	int                     seqnum;
	long int                arraynum;
#endif
{
        static char             buf[MSGSIZ];
        char                    msg_text[MSGSIZ];
	char			*msg;

        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        sprintf(msg_text,":%d",seqnum);
        msg = gettxt(msg_text,agettxt(arraynum,buf,MSGSIZ));
        return (msg);
}
