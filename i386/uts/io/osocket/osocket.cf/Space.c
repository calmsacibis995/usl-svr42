/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/osocket/osocket.cf/Space.c	1.2"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <netinet/in.h>
#include <sys/tiuser.h>
#include <sys/sockmod.h>
#include <sys/osocket.h>

#define ONSOCK 100

int num_osockets = ONSOCK;
struct osocket *osocket_tab[ONSOCK];
char osoc_domainbuf[OMAXHOSTNAMELEN] = { 0 };

/* Enhanced Application Compatibility Support */
