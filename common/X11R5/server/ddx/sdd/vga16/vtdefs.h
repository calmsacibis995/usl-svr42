/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vtdefs.h	1.3"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#ifndef NULL
#define NULL 	0
#endif

#define PG_SIZE		4096	/* page size in 386/ix */

#define	VT_EGA		0	/* EGA 		640x350 16 colors */
#define	VT_PEGA		1	/* PEGA2 	640x480 16 colors*/
#define	VT_VGA		2	/* VGA 		640x480 16 colors*/
#define	VT_VEGA720	3	/* VEGA VGA 	720x540	16 colors */
#define	VT_VEGA800	4	/* VEGA VGA 	800x600	16 colors */
#define	VT_TSL8005_16	5	/* Tseng Labs	800x560	16 colors */
#define	VT_TSL8006_16	6	/* Tseng Labs	800x600	16 colors */
#define	VT_TSL960	7	/* Tseng Labs	960x720	16 colors */
#define	VT_TSL1024	8	/* Tseng Labs  1024x768 16 colors */
#define VT_TSL1024ni	9	/* Tseng Labs  1024x768 16 colors NI */
#define	VT_SIGMAH	10	/* Sigma VGA/H  800x600 16 colors */
#define VT_PVGA1A	11	/* Paradise PVGA1A 800x600 16 colors */
#define VT_V7VRAM6	12	/* Video 7 VRAM	640x480 16 colors */
#define VT_V7VRAM7	13	/* Video 7 VRAM	720x540 16 colors */
#define VT_V7VRAM8	14	/* Video 7 VRAM	800x600 16 colors */
#define VT_V7VRAM1_2	15	/* Video 7 VRAM 1024x768  2 colors */
#define VT_V7VRAM1_4	16	/* Video 7 VRAM 1024x768  4 colors */
#define VT_V7VRAM1_16	17	/* Video 7 VRAM 1024x768 16 colors */
#define VT_GENEGA_6	18	/* Genoa EGA    640x480 16 colors */
#define VT_GENEGA_8	19	/* Genoa EGA    800x600 16 colors */
#define VT_ORVGA8	20	/* Orchid VGA   800x600 16 colors */
#define VT_GVGA8_6	21	/* Genoa VGA    800x600 16 colors */
#define VT_DELL7	22	/* Dell VGA (Video 7)    720x540 16 colors */
#define VT_DELL8	23	/* Dell VGA (Video 7)    800x600 16 colors */
#define	VT_CIRRUS7	24	/* Cirrus chip	720x540 16 colors */
#define	VT_CIRRUS8	25	/* Cirrus chip	800x600 16 colors */
#define	VT_VGAWON	26	/* ATI VGA Wonder 800x600 16 colors */
#define VT_HP16		27	/* Sigma VGA/HP-16 800x600 16 colors */
#define	VT_ORVGAf8	28	/* Orchid VGA 800x600 fixed freq monitor */
#define	VT_ORVGAf1	29	/* Orchid VGA 1024x768 fixed freq monitor */
#define	VT_QVGA8_5	30	/* Quadram VGA SPECTRA 800X560 16 colors */
#define	VT_QVGA8_6	31	/* Quadram VGA SPECTRA 800X600 16 colors */
#define	VT_1024i_7	32	/* Video 7 1024i  720x540 16 colors */
#define	VT_1024i_8	33	/* Video 7 1024i  800x600 16 colors */
#define	VT_1024i_1	34	/* Video 7 1024i  1024x768 16 colors */
#define	VT_1024i_1a	35	/* Video 7 1024i  alt 1024x768 16 colors */
#define	VT_ORVGA800	36	/* Orchid Designer 800 VGA 800x600 */
#define	VT_PVGA1024_8	37	/* Paradise VGA 1024 800x600 */
#define	VT_PVGA1024_1	38	/* Paradise VGA 1024 1024x768 */
#define VT_V7VRAM8a	39	/* Video 7 VRAM	alt 800x600 16 colors */
#define VT_ET40008	40	/* Tseng Labs ET4000 800x600 16 colors */
#define VT_ET40008a	41	/* Tseng Labs ET4000 alt 800x600 16 colors */
#define VT_ATI4800	42	/* ATI VGA Wonder V4 800x600 16 colors */
#define VT_T89_8	43	/* Trident VGA 8900, 800x600 16 colors */
#define VT_T89_8a	44	/* Trident VGA 8900, alt 800x600 16 colors */
#define VT_T89_1	45	/* Trident VGA 8900, 1024x768 16 colors */
#define VT_T89_1ni	46	/* Trident VGA 8900, 1024x768 16 colors N/I */
#define VT_LEGEND_8	47	/* Sigma VGA Legend, 800x600 16 colors */
#define VT_LEGEND_8a	48	/* Sigma VGA Legend, alt 800x600 16 colors */
#define VT_ATIPLUS_8	49	/* ATI VGA Wonder+ 800x600 16 colors */
#define VT_MICRO4_8	50	/* Microlabs ET4000 800x600 16 colors */
#define VT_MICRO4_8a	51	/* Microlabs ET4000 alt 800x600 16 colors */
#define VT_VDC600	52	/* AT&T VDC600 800x600 16 colors */


/* 
 * NOTICE:  Insert new types here.  Entries from here to the end are dummy
 * entries that really use other register initialization table entries but
 * need their own unique type for other reasons.  This saves a bunch of 
 * space in the reg_init table.
 */
#define	VT_EGAPAN_6	100	/* EGA panning, 640x800 */
#define	VT_EGAPAN_8	101	/* EGA panning, 800x600 */
#define	VT_EGAPAN_1	102	/* EGA panning, 1024x480 */
#define	VT_VGAPAN_6	103	/* VGA panning, 640x800 */
#define	VT_VGAPAN_8	104	/* VGA panning, 800x600 */
#define	VT_VGAPAN_1	105	/* VGA panning, 1024x480 */
#define	VT_PEGAPAN_6	106	/* PEGA panning, 640x800 */
#define	VT_PEGAPAN_8	107	/* PEGA panning, 800x600 */
#define	VT_PEGAPAN_1	108	/* PEGA panning, 1024x480 */
#define	VT_GEGAPAN_6	109	/* GEGA panning, 640x800 */
#define	VT_GEGAPAN_8	110	/* GEGA panning, 800x600 */
#define	VT_GEGAPAN_1	111	/* GEGA panning, 1024x480 */
#define VT_V7FW6	112	/* Video 7 FastWrite 640x480 16 colors */
#define VT_V7FW7	113	/* Video 7 FastWrite 720x540 16 colors */
#define VT_V7FW8	114	/* Video 7 FastWrite 800x600 16 colors */
#define VT_V7FW1_2	115	/* Video 7 FastWrite 1024x768  2 colors */
#define VT_V7FW1_4	116	/* Video 7 FastWrite 1024x768  4 colors */
#define VT_ATI5800	117	/* ATI VGA Wonder V5 800x600 16 colors */
#define VT_V7FW8a	118	/* Video 7 FastWrite 800x600 (alt) 16 colors */


/* 
 * constants for various PC displays.
 */
#define	PC_HERC_ADR		0xB000

#define SLBYTES 80	/* number of bytes in a scanline for EGA/VGA */
#define PLANES	4	/* number of planes on an EGA/VGA */
#define COLORS	16	/* number of colors on an EGA/VGA */

/*
 * Hercules defines
 */
#define HERC_6845	0x3b4	/* address of 6845 */
#define HERC_MODE	0x3b8	/* display mode control port */
#define HERC_STAT	0x3ba	/* display status port */
#define HERC_CFG	0x3bf	/* configuration switch */

#define	NHERC_6845	16	/* number of 6845 registers */

/*
 * EGA/VGA defines
 */
#define VGA_SEQ		0x3c4	/* sequencer address register */
#define VGA_GRAPH	0x3ce	/* graphics address register */
#define	VGA_ATTR	0x3c0	/* attribute address/data register */
#define GR_SR		0	/* graphics controller set/reset reg */
#define GR_ENAB_SR	1	/* graphics controller enable set/reset reg */
#define GR_FUNC		3	/* graphics controller function select */
#define BITMASK		8	/* graphics controller bitmap mask reg */

#define VGA_COPY	0x0000	/* Data unmodified */
#define VGA_AND		0x0800	/* Data AND'ed with latches */
#define VGA_OR		0x1000	/* Data OR'ed with latches */
#define VGA_XOR		0x1800	/* Data XOR'ed with latches */

/*
 * Stuff that's been removed from kd.h in 2.0
 */
#ifdef	SEQ_RESET
#undef	SEQ_RESET
#endif
#define	SEQ_RESET	0x01	/* sychronously reset sequencer */

#ifdef	VGA_BASE
#undef	VGA_BASE
#endif
#define	VGA_BASE	0xa0000	/* location of enhanced display memory */

/*
 * Define MCOUNT to be nothing so we can successfully build without profiling
 */
#define	MCOUNT
