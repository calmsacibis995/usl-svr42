/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libspeedo:speedo/sperr.c	1.1"
/* $XConsortium: sperr.c,v 1.3 91/07/15 18:15:26 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *
 */

#include	"spint.h"

/* VARARGS */
void
SpeedoErr(str, a1)
    char       *str;
    char       *a1;
{
    ErrorF("Speedo: %s %d\n", str, a1);
}


/*
 * Called by Speedo character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
void
sp_report_error(n)
    fix15       n;
{
    switch (n) {
    case 1:
	SpeedoErr("Insufficient font data loaded\n");
	break;
    case 3:
	SpeedoErr("Transformation matrix out of range\n");
	break;
    case 4:
	SpeedoErr("Font format error\n");
	break;
    case 5:
	SpeedoErr("Requested specs not compatible with output module\n");
	break;
    case 7:
	SpeedoErr("Intelligent transformation requested but not supported\n");
	break;
    case 8:
	SpeedoErr("Unsupported output mode requested\n");
	break;
    case 9:
	SpeedoErr("Extended font loaded but only compact fonts supported\n");
	break;
    case 10:
	SpeedoErr("Font specs not set prior to use of font\n");
	break;
    case 12:
	break;
    case 13:
	SpeedoErr("Track kerning data not available()\n");
	break;
    case 14:
	SpeedoErr("Pair kerning data not available()\n");
	break;
    default:
	SpeedoErr("report_error(%d)\n", n);
	break;
    }
}
