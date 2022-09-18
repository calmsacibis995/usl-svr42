/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _COPYRIGHT_H
#define _COPYRIGHT_H

#ident	"@(#)cmd-inet:i386/cmd/cmd-inet/usr.sbin/copyright.h	1.1"
#ident	"$Header: copyright.h 1.1 91/04/10 $"

/*
 * copyright for i386 machines
 */

#define	COPYRIGHT(un) \
        (void) printf("UNIX System V/386 Release %s Version %s\n%s\n\
Copyright (c) 1992 UNIX System Laboratories, Inc.\n\
Copyright (c) 1987, 1988 Microsoft Corp. All rights reserved.\n",\
	un.release, un.version, un.nodename)

#endif
