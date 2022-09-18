/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)nonexclus:Nonexclusi.h	1.10"
#endif

#ifndef _OlNonexclusives_h
#define _OlNonexclusives_h

/*
 * Author:	Karen S. Kendler
 * Date:	16 August 1988
 * File:	Nonexclusives.h - Public definitions for Nonexclusives widget
 *	Copyright (c) 1989 AT&T		
 *
 */

#include <Xol/Manager.h>	/* include superclasses' header */

extern WidgetClass     nonexclusivesWidgetClass;

typedef struct _NonexclusivesClassRec   *NonexclusivesWidgetClass;
typedef struct _NonexclusivesRec        *NonexclusivesWidget;

#endif /*  _OlNonexclusives_h  */

/* DON'T ADD STUFF AFTER THIS */
