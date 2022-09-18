/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)libnsl:common/lib/libnsl/des/intl_crypt.c	1.1.6.3"
#ident  "$Header: intl_crypt.c 1.2 91/06/26 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_crypt.c 1.2 89/03/10 Copyr 1986 Sun Micro";
#endif
/*
 * des_crypt.c, DES encryption library routines
 */


#include <sys/types.h>
#include <rpc/des_crypt.h>
#include <des/des.h>

/*
 * To see if chip is installed 
 */
#define UNOPENED (-2)
static int g_desfd = UNOPENED;


/*
 * Copy 8 bytes
 */
#define COPY8(src, dst) { \
}
 
/*
 * Copy multiple of 8 bytes
 */
#define DESCOPY(src, dst, len) { \
}

/*
 * CBC mode encryption
 */
cbc_crypt(key, buf, len, mode, ivec)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
	char *ivec;	
{
	return(DESERR_NONE);
}


/*
 * ECB mode encryption
 */
ecb_crypt(key, buf, len, mode)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
{
	return(DESERR_NONE);
}



/*
 * Common code to cbc_crypt() & ecb_crypt()
 */
static
common_crypt(key, buf, len, mode, desp)	
	char *key;	
	char *buf;
	register unsigned len;
	unsigned mode;
	register struct desparams *desp;
{
	return(DESERR_NONE);
}
