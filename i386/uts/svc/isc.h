/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_ISC_H
#define _SVC_ISC_H

#ident	"@(#)uts-x86:svc/isc.h	1.3"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */

#define ISC_SIGCONT	23
#define ISC_SIGSTOP	24
#define ISC_SIGTSTP	25

/* POSIX waitpid() ISC Defines */
#define ISC_WNOHANG	1
#define ISC_WUNTRACED	2

/* POSIX TIOC  ISC Defines */
#define ISC_TIOC	('T'<<8)
#define ISC_TCSETPGRP	(ISC_TIOC|20)
#define ISC_TCGETPGRP	(ISC_TIOC|21)


/* End Enhanced Application Compatibility Support */

#endif	/* _SVC_ISC_H */
