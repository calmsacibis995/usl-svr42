/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/dlpi_ether/wd.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <sys/wd.h>
#include <config.h>

/*	INTEL CORPORATION PROPRIETARY INFORMATION
 *	
 *	This software is suppolied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 * 	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 *	Copyright 1987, 1988 Intel Corporation
 */

#define	NSAPS		8
#define MAXMULTI	8
#define INETSTATS	1
#define STREAMS_LOG	0
#define IFNAME		"wd"

int wd_multisize = MAXMULTI;
struct wdmaddr wdmultiaddrs[ WD_CNTLS * MAXMULTI ];

int wdinetstats = INETSTATS;
int wdboards = WD_CNTLS;
int wdstrlog = STREAMS_LOG;
char *wd_ifname = IFNAME;
DL_sap_t  wdsaps[NSAPS * WD_CNTLS];

DL_bdconfig_t wdconfig[ WD_CNTLS ] = {
#ifdef	WD_0
    {
	WD_CMAJOR_0,
	WD_0_SIOA,
	WD_0_EIOA,
	WD_0_SCMA,
	WD_0_ECMA,
	WD_0_VECT,
	NSAPS,
	0
     },
#endif
#ifdef	WD_1
    {
	WD_CMAJOR_1,
	WD_1_SIOA,
	WD_1_EIOA,
	WD_1_SCMA,
	WD_1_ECMA,
	WD_1_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	WD_2
    {
	WD_CMAJOR_2,
	WD_2_SIOA,
	WD_2_EIOA,
	WD_2_SCMA,
	WD_2_ECMA,
	WD_2_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	WD_3
    {
	WD_CMAJOR_3,
	WD_3_SIOA,
	WD_3_EIOA,
	WD_3_SCMA,
	WD_3_ECMA,
	WD_3_VECT,
	NSAPS,
	0
    },
#endif
};
