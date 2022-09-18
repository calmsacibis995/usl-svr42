/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _PDF_H
#define _PDF_H

#ident	"@(#)pm_cmds:pdf.h	1.12.2.3"
#ident  "$Header: pdf.h 1.2 91/06/27 $"

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<pfmt.h>
#include	<priv.h>
#include	<deflt.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<limits.h>
#include	<locale.h>
#include	<signal.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/mac.h>
#include	<sys/time.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<sys/secsys.h>
#include	<sys/signal.h>
#include	<sys/resource.h>

struct pdf {
	unsigned long	pf_size;
		 long	pf_cksum;
	unsigned long	pf_validity;
		 char	*pf_privs;
		 char	*pf_filep;
};

#define	PRIV_DEF	"privcmds"

#define	FP_BSIZ	(BUFSIZ + PATH_MAX + 1)
#define	ABUFSIZ	(((NPRIVS + 1) * PRVNAMSIZ) + (NPRIVS + 1))

#define	pm_prid(a, b)	(((a) << 24) | (b))

#define	FAILURE		  -1
#define	SUCCESS		   0
#define	BADSYN		   1
#define	SOMEBAD		   2
#define	NOPKG		   3
#define	INVARG		   4

static	char	PDF[] =	   "/etc/security/tcb/privs";

#if defined(__STDC__)

extern struct pdf *getpfent(void);
extern void setpfent(void);
extern void endpfent(void);
extern struct pdf *fgetpfent(FILE *);
extern int putpfent(const struct pdf *, FILE *);

#else
extern struct pdf *getpfent();
extern void setpfent();
extern void endpfent();
extern struct pdf *fgetpfent();
extern int putpfent();

#endif	/* defined(__STDC__) */

#endif /* _PDF_H */
