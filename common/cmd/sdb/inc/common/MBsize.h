/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/MBsize.h	1.1"

/* MB_LEN_MAX is defined in limits.h too. However, cfront won't accept
 * limits.h because "long double" type is used in the header and cfront
 * doesn't support "long double" type yet.
 *
 * MB_LEN_MAX is used for internationationalization feature. It should
 * be updated as long as MB_LEN_MAX in limits.h is updated.
 */

#define MB_LEN_MAX	5
