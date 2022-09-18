/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SYSTEMINFO_H	/* wrapper symbol for kernel use */
#define _SVC_SYSTEMINFO_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/systeminfo.h	1.4"
#ident	"$Header: $"

extern char srpc_domain[];

/*
 * Commands to sysinfo()
 */

#define SI_SYSNAME		1	/* return name of operating system */
#define SI_HOSTNAME		2	/* return name of node */
#define SI_RELEASE 		3	/* return release of operating system */
#define SI_VERSION		4	/* return version field of utsname */
#define SI_MACHINE		5	/* return kind of machine */
#define SI_ARCHITECTURE		6	/* return instruction set arch */
#define SI_HW_SERIAL		7	/* return hardware serial number */
#define SI_HW_PROVIDER		8	/* return hardware manufacturer */
#define SI_SRPC_DOMAIN		9	/* return secure RPC domain */
#define SI_INITTAB_NAME	       10	/* return name of inittab file used */
/*
 * These commands are unpublished interfaces to sysinfo().
 */
#define SI_SET_HOSTNAME		258	/* set name of node */
					/*  -unpublished option */
#define SI_SET_SRPC_DOMAIN	265	/* set secure RPC domain */
					/* -unpublished option */
#define	SI_SET_KERB_REALM	266	/* set Kerberos realm */
					/* -unpublished option */
#define	SI_KERB_REALM		267	/* return Kerberos realm */
					/* -unpublished option */

#define SI_SET_HW_PROVIDER	268	/* set hardware provider name */

#define SI_SET_HW_SERIAL	269	/* set serial number */
					

#if defined(__STDC__) && !defined(_KERNEL)
int sysinfo(int, char *, long);
#endif

#endif /* _SVC_SYSTEMINFO_H */
