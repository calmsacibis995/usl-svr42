/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libspeedo:speedo/spint.h	1.2"
/* $XConsortium: spint.h,v 1.4 91/07/16 20:20:32 keith Exp $ */
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
 * $NCDId: @(#)spint.h,v 4.6 1991/06/12 13:17:23 lemke Exp $
 *
 */

#ifndef _SPINT_H_
#define _SPINT_H_

#ifndef DEBUG
#define NDEBUG
#endif

#include	<stdio.h>
#include	"fontfilest.h"
#include	"speedo.h"

#define	SaveMetrics	0x1

#define GLWIDTHBYTESPADDED(bits,nbytes) \
        ((nbytes) == 1 ? (((bits)+7)>>3)        /* pad to 1 byte */ \
        :(nbytes) == 2 ? ((((bits)+15)>>3)&~1)  /* pad to 2 bytes */ \
        :(nbytes) == 4 ? ((((bits)+31)>>3)&~3)  /* pad to 4 bytes */ \
        :(nbytes) == 8 ? ((((bits)+63)>>3)&~7)  /* pad to 8 bytes */ \
        : 0)

#define GLYPH_SIZE(ch, nbytes)          \
        GLWIDTHBYTESPADDED((ch)->metrics.rightSideBearing - \
                        (ch)->metrics.leftSideBearing, (nbytes))

#define	MasterFileOpen	0x1

typedef struct _sp_master {
    FILE       *fp;
    char       *fname;
    ufix8      *f_buffer;
    ufix8      *c_buffer;
    char       *copyright;
    ufix8      *key;
    buff_t      font;
    buff_t      char_data;
    ufix16      mincharsize;
    int         first_char_id;
    int         num_chars;
    int         max_id;
    int         state;		/* open, closed */
    int         refcount;	/* number of instances */
    int        *enc;
    int         enc_size;
}           SpeedoMasterFontRec, *SpeedoMasterFontPtr;

typedef struct _cur_font_stats {
    fsBitmapFormat format;
    /* current glyph info */
    ufix16      char_index;
    ufix16      char_id;

    fix15       bit_width,
                bit_height;
    fix15       cur_y;
    int         bpr;

    /*
     * since Speedo returns extents that are not identical to what it feeds to
     * the bitmap builder, and we want to be able to use the extents for
     * preformance reasons, some of the bitmaps require padding out.  the next
     * two flags keep track of this.
     */
    fix15       last_y;
    int         trunc;

    pointer     bp;
    int         scanpad;
}           CurrentFontValuesRec, *CurrentFontValuesPtr;


typedef struct _ScaleInfo {
	unsigned short exists:1;
	unsigned short scaled:1;
	unsigned short allocated:1;
	unsigned short cached:1;
	short glyphsize;
} ScaleInfoRec, *ScaleInfoPtr;


typedef struct _sp_font {
    struct _sp_master *master;
    specs_t     specs;

    FontScalableRec vals;

    /* char & metric data */
    char *filename;
    ScaleInfoPtr scaled;
    CharInfoPtr encoding;
    CharInfoPtr pDefault;
    pointer     bitmaps;

#ifdef DEBUG
    unsigned long bitmap_size;
#endif

}           SpeedoFontRec, *SpeedoFontPtr;

extern SpeedoFontPtr cur_spf;

extern int  open_sp_font();
extern void close_sp_font();
extern void close_master_file();
extern void sp_reset_master();
extern void SpeedoErr();

extern void make_sp_standard_props();
extern void make_sp_header();
extern void compute_sp_bounds();
extern void compute_sp_props();
extern int  build_all_sp_bitmaps();
extern unsigned long compute_sp_data_size();

extern int  bics_map[];
extern int  bics_map_size;

#ifdef EXTRAFONTS
extern int  adobe_map[];
extern int  adobe_map_size;

#endif

#endif				/* _SPINT_H_ */
