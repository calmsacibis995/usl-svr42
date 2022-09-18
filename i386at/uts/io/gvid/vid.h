/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_GVID_VID_H	/* wrapper symbol for kernel use */
#define _IO_GVID_VID_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/gvid/vid.h	1.6"
#ident	"$Header: $"

#define ATYPE(x, y)	(x.w_vstate.v_cmos == y)
#define DTYPE(x, y)	(x.w_vstate.v_type == y)
#define CMODE(x, y)	(x.w_vstate.v_cvmode == y)	/* Current disp. mode */
#define DMODE(x, y)	(x.w_vstate.v_dvmode == y)

#define KD_RDVMEM	0
#define KD_WRVMEM	1

struct modeinfo {
	ushort	m_cols,		/* Number of character columns */
		m_rows,		/* Number of character rows */
		m_xpels,	/* Number of pels on x axis */
		m_ypels;	/* Number of pels on y axis */
	unchar	m_color;	/* Non-zero value indicates color mode */
	paddr_t	m_base;		/* Physical address of screen memory */
	ulong	m_size;		/* Size of screen memory */
	unchar	m_font,		/* Default font (0 indicates grahpics mode) */
		m_params,	/* Parameter location: BIOS or static table */
		m_offset,	/* offset with respect to m_params */
		m_ramdac;	/* RAMDAC table offset */
};

#endif /* _IO_GVID_VID_H */
