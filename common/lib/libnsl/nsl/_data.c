/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/nsl/_data.c	1.3.4.2"
#ident "$Header: _data.c 1.2 91/03/06 $"
#include "sys/types.h"
#include "sys/stream.h"
#include "sys/timod.h"
#include "stdio.h"
#include "_import.h"


/*
 * ti_user structures get allocated
 * the first time the user calls t_open or t_sync
 */
struct _ti_user *_ti_user = 0;

/*
 * This must be here to preserve compatibility
 */
struct _oldti_user _old_ti = { 0, 0, NULL,0,NULL,NULL,NULL,0,0,0,0,0 };
