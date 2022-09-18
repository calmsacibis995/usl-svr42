/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)slider:GaugeP.h	1.1"
#endif

/* 
 * Gauge.h - Private definitions for Gauge widget
 * 
 */

#ifndef _GaugeP_h
#define _GaugeP_h

/***********************************************************************
 *
 * Gauge Widget Private Data
 *
 ***********************************************************************/

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Gauge.h>

#include <Xol/SliderP.h>

extern SliderClassRec gaugeClassRec;

#endif /* _GaugeP_h */
