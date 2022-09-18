/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/data.c	1.1.5.2"
#ident  "$Header: data.c 1.2 91/06/25 $"
/* LINTLIBRARY */

/*
 *  data.c
 *
 *	This file contains global data for the library "liboam".
 *
 *	FILE   *oam_devtab	The open file descriptor for the current
 *				device table.
 *	FILE   *oam_dgroup	The open file descriptor for the current
 *				device-group table.
 */


/*
 *  Header files needed:
 *	<stdio.h>	Standard I/O definitions
 */

#include	<stdio.h>



FILE	       *oam_devtab = (FILE *) NULL;
FILE	       *oam_dgroup = (FILE *) NULL;
