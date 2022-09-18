/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fmli:inc/inc.types.h	1.1.3.3"

#ifndef PRE_CI5_COMPILE		/* then assume EFT types are defined
				 * in system header files.
				 */
#include <sys/types.h>
#else
#include "eft.types.h"		/* EFT defines for pre SVR4.0 systems */
#endif
