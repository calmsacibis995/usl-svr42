/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/fp/fpsymbols.c	1.2"
#ident	"$Header: $"

#include <proc/user.h>
#include <proc/proc.h>
#include <util/fp/sizes.h>

#define offsetof(s, m)	(size_t)(&(((s *)0)->m))

size_t __SYMBOL__USER_FPSTK = USER_FPSTK;
size_t __SYMBOL__USER_FP = USER_FP;
size_t __SYMBOL__GRSL = GRSL;

size_t __SYMBOL__u_fpvalid = offsetof(struct user, u_fpvalid);
size_t __SYMBOL__u_fps = offsetof(struct user, u_fps);

size_t __SYMBOL__p_sysid = offsetof(struct proc, p_sysid);
