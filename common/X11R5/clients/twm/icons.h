/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5twm:icons.h	1.1"
/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

/**********************************************************************
 *
 * $XConsortium: icons.h,v 1.4 89/07/18 17:16:24 jim Exp $
 *
 * Icon releated definitions
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef ICONS_H
#define ICONS_H

typedef struct IconRegion
{
    struct IconRegion	*next;
    int			x, y, w, h;
    int			grav1, grav2;
    int			stepx, stepy;	/* allocation granularity */
    struct IconEntry	*entries;
} IconRegion;

typedef struct IconEntry
{
    struct IconEntry	*next;
    int			x, y, w, h;
    TwmWindow		*twm_win;
    short 		used;
}IconEntry;

#endif /* ICONS_H */
