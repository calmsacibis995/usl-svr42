/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/oam/buffers.c	1.2.5.3"
#ident	"$Header: $"
/* LINTLIBRARY */

#include "oam.h"

char			_m_[MSGSIZ],
			_a_[MSGSIZ],
			_f_[MSGSIZ],
			*_t_	= "999999";
