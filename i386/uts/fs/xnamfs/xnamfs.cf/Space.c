/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/xnamfs/xnamfs.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/xnamnode.h>
#include <sys/sd.h>

struct	xsem xsem[XSEMMAX];
int	nxsem = XSEMMAX;
struct	xsd xsd[XSDSEGS];
int	nxsd = XSDSEGS;
struct  sd sdtab[XSDSEGS * XSDSLOTS];
