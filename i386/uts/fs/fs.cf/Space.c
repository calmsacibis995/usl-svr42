/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/fs.cf/Space.c	1.6"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/buf.h>
#include <sys/flock.h>
#include <sys/dnlc.h>
#include <sys/conf.h>

/* Default fstype for root is "s5".  This can be overridden with
   the "rootfstype" parameter in /stand/boot. */
#define	ROOTFSTYPE	"s5"
char rootfstype[ROOTFS_NAMESZ+1] = ROOTFSTYPE;

/* Directory name lookup cache size */
int	ncsize = DNLCSIZE;
int	nchash_size = NC_HASH_SIZE;
struct	nc_hash	nc_hash[NC_HASH_SIZE];

/* multiple groups and chown(2) restrictions */
int	rstchown = RSTCHOWN;

struct	buf	pbuf[NPBUF];
struct	hbuf	hbuf[NHBUF];

struct	flckinfo flckinfo = {0, 0};

/* Enhanced Application Compatibility Support */

#ifdef ACAD_CMAJOR_0
	int dev_autocad_major = ACAD_CMAJOR_0;
#else
     	int dev_autocad_major = -1;
#endif

/* End Enhanced Application Compatibility Support */

