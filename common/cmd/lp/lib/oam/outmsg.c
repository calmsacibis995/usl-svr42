/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/oam/outmsg.c	1.2.1.4"
#ident	"$Header: $"

/* LINTLIBRARY */

#include "stdio.h"
#include "string.h"
#include "oam.h"
#include <locale.h>

/**
 ** outmsg()
 **/

void
#if	defined(__STDC__)
outmsg (
	char *			label,
	int			severity,
	int                     seqnum,
	long int                arraynum,
        int                     logind,
                                ...
)
#else
outmsg ( label, severity, seqnum, arraynum, logind, va_alist)
	char			*label;
	int			severity;
	int                     seqnum;
	long int                arraynum;
        int                     logind;
        va_dcl
#endif
{
        va_list                 args;
        char                    buf[MSGSIZ];
        char                    text[MSGSIZ];
        char                    msg_text[MSGSIZ];

#ifdef  __STDC__
        va_start(args, logind);
#else
        va_start(args);
#endif
        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        setlabel(label);
        sprintf(msg_text,":%d:%s\n",seqnum,agettxt(arraynum,buf,MSGSIZ));
        if (logind != LOG)
           vpfmt(stdout,severity,msg_text,args);
        else
           vlfmt(stdout,severity,msg_text,args);
        sprintf(text,"%s",agettxt(arraynum + 1,buf,MSGSIZ));
        if (strncmp(text, "", 1) != 0)  {
           sprintf(msg_text,":%d:%s\n",seqnum + 1,text);
           pfmt(stdout,MM_ACTION,msg_text);
        }
}
