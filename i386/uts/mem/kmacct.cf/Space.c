/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:mem/kmacct.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/kmacct.h>

int nkmasym = KMARRAY;
int kmadepth = SDEPTH;
int nkmabuf = NKMABUF;

kmasym_t kmasymtab[KMARRAY];
caddr_t kmastack[KMARRAY * SDEPTH];
kmabuf_t kmabuf[NKMABUF];
