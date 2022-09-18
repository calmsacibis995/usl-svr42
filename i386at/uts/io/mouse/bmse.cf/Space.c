/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/mouse/bmse.cf/Space.c	1.3"
#ident	"$Header: $"
#include "config.h"
#include <sys/param.h>
#include <sys/types.h>
#include <sys/termio.h>
#include <sys/proc.h>
#include <sys/stream.h>
#include <sys/xque.h>
#include <sys/mouse.h>
#include <sys/mse.h>

struct mouseconfig mse_config = {
	BMSE_0_SIOA, BMSE_0_VECT
};

#if defined(BMSE_0) && BMSE_0_VECT
int mse_nbus = BMSE_UNITS;
#else
int mse_nbus = 0;
#endif
