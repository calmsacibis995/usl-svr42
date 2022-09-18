/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libspeedo:speedo/spencode.c	1.1"
/* $XConsortium: spencode.c,v 1.4 91/07/15 18:15:22 keith Exp $ */
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
 * $NCDId: @(#)spencode.c,v 4.3 1991/05/29 17:29:22 lemke Exp $
 *
 */

#include	"spint.h"

#include	"bics-iso.h"

int         bics_map_size = (sizeof(bics_map) / (sizeof(int) * 2));

#ifdef EXTRAFONTS
#include	"adobe-iso.h"

int         adobe_map_size = (sizeof(adobe_map) / (sizeof(int) * 2));

#endif				/* EXTRAFONTS */
