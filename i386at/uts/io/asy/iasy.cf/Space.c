/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)uts-x86at:io/asy/iasy.cf/Space.c	1.4"
#ident	"$Header: $"

/*
 *	Reserve storage for Generic serial driver
*/
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termio.h>
#include <sys/strtty.h>
#include <sys/cred.h>
#include <sys/ddi.h>
#include <sys/iasy.h>
#ifdef VPIX
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/v86.h>
#endif

#define IASY_UNITS	128
int	iasy_num = IASY_UNITS;

#define	IASY_CONSOLE	0
int	iasy_console = IASY_CONSOLE;

struct	strtty asy_tty[IASY_UNITS];	/* tty structs for each device */
					/* iasy_tty is changed to asy_tty for merge */
struct iasy_hw iasy_hw[IASY_UNITS];		/* Hardware support routines */

#ifdef VPIX
v86_t *iasystash[IASY_UNITS];
int iasyintmask[IASY_UNITS];
int iasy_v86_prp_pd[2*IASY_UNITS];
struct termss iasyss[IASY_UNITS];
int iasy_closing[IASY_UNITS];
char iasy_opened[IASY_UNITS];
#endif /* VPIX */

