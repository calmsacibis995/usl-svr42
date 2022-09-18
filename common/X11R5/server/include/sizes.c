/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/sizes.c	1.1"
#include <stdio.h>
#include "Xos.h"
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "keynames.h"			/* AT&T */
#include "i386.h"			/* AT&T */
#include <sys/xque.h>			/* AT&T */
#define  XTestSERVER_SIDE
#include "xtestext1.h"	
#include "xtestqueue.h"	
#include "miscstruct.h"	

main ()
{

	printf ("size of XTestQueueNode : %d\n", sizeof(XTestQueueNode));
	printf ("size of KeySym         : %d\n", sizeof(KeySym));
	printf ("size of DDXPointRec    : %d\n", sizeof(DDXPointRec));

}
