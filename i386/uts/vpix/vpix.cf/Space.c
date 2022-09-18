/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:vpix/vpix.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/v86.h>


v86_t *v86tab[NV86TASKS];                
char v86procflag;                       /* ldt-switch flag for ttrap.s */
int v86timer = V86TIMER;               /* Current ticks to go for timer */

/*
 * The following is required for the CGA Status Port Register
 * extension in ml/v86gptrap.s
*/
unchar cs_table[CS_MAX] = {		/* table of status states */
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
	0x01, 0x01, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x01, 0x00, 0x01,
	0x00, 0x01,
};	
unchar	*cs_table_beg = cs_table;
unchar	*cs_table_ptr = cs_table;
unchar	*cs_table_end = &cs_table[CS_MAX];

