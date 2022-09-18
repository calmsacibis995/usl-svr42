/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:svc/name.cf/Space.c	1.10"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/utsname.h>

#define SYS	"UNIX_SV"
#define XSYS	"UNIX_Sys"
#define NODE	"unix"
#define REL	"4.2"
#define VER	"1"
#define CPU	"i386"

#define MACH		"i386at"
#define ARCHITECTURE	"386/AT"
#define HW_SERIAL	""
#define HW_PROVIDER	"AT&T"
#define SRPC_DOMAIN	""
#define INITFILE	"/etc/inittab"

#define RESRVD	""
#define ORIGIN	1
#define OEMNUM	0
#define SERIAL	0

struct utsname	utsname = {
	SYS,
	NODE,
	REL,
	VER,
	CPU
};

struct	xutsname xutsname = {
		XSYS,
		NODE,
		REL,
		VER,
		MACH,
		RESRVD,
		ORIGIN,
		OEMNUM,
		SERIAL
};

/* sysinfo information */
char architecture[SYS_NMLN] = ARCHITECTURE;
char hw_serial[SYS_NMLN] = HW_SERIAL;
char hw_provider[SYS_NMLN] = HW_PROVIDER;
char srpc_domain[SYS_NMLN] = SRPC_DOMAIN;
char initfile[SYS_NMLN] = INITFILE;
char bustype[] = "AT";
