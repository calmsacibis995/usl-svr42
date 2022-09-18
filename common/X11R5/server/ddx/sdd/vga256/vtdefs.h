/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/vtdefs.h	1.2"

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

#define STB_6x4_256	0	/* STB 256 color 640x480 */
#define TLI_6x4_256	1	/* Generic Tseng Labs 256 color 640x480 */
#define STB_8x6_256	2	/* STB 256 color 800x600 */
#define TLI_1x7_256	3	/* STB 256 color 1024x768 */
#define TLI_400_256	4	/* STB 256 color 640x400 */
#define	VGA_256		5	/* VGA in 320x200 */
#define ORV256_8	6	/* Orchid 256 color 800x600 */
#define ORD256_8	7	/* Orchid Designer 800 256 color 800x600 */
#define ORF256_8	8	/* Orchid 256 color 800x600 fixed freq */
#define GENOA256_8	9	/* Genoa VGA 256 color 800x600 */
#define V7256_400	10	/* Video 7 VRAM VGA 256 color 640x400 */
#define V7256_6		11	/* Video 7 VRAM VGA 256 color 640x480 */
#define V7256_7		12	/* Video 7 VRAM VGA 256 color 720x540 */
#define V7256_8		13	/* Video 7 VRAM VGA 256 color 800x600 */
#define V7256_8a	14	/* Video 7 VRAM VGA 256 color 800x600 Nanao */
#define ATI256_6	15	/* ATI VGA Wonder 256 color 640x480 */
#define ATI256_8	16	/* ATI VGA Wonder 256 color 800x600 */
#define TEC256_8	17	/* Techmar VGA 256 color 800x600 */
#define P256_400	18	/* Paradise VGA 256 color 640x400  */
#define P256_6		19	/* Paradise VGA 256 color 640x480  */
#define V71024i256_400	20	/* Video 7 1024i VGA 256 color 640x400  */
#define V71024i256_6	21	/* Video 7 1024i VGA 256 color 640x480  */
#define ET4000256_6	22	/* Tseng Labs ET4000 256 colors 640x480 */
#define ET4000256_8	23	/* Tseng Labs ET4000 256 colors 800x600 */
#define ET4000256_8a	24	/* ET4000 256 colors 800x600 alternate */
#define ET4000256_1	25	/* Tseng Labs ET4000 256 colors 1024x768 */
#define ET4000256_1ni	26	/* ET4000 256 colors 1024x768 non-interlaced*/
#define ORII256_1	27	/* Orchid Prodesigner II 256 colors 1024x768 */
#define STB256PLUS_1	28	/* STB Extra/EM 16+ 256 colors 1024x768 */
#define ATI2564_8	29	/* ATI VGA Wonder V4 256 color 800x600 */
#define T89256_6	30	/* Trident 8900 256 colors 640x480 */
#define T89256_8	31	/* Trident 8900 256 colors 800x600 */
#define T89256_8a	32	/* Trident 8900 256 colors 800x600 alternate */
#define T89256_1	33	/* Trident 8900 256 colors 1024x768 */
#define T89256_1ni	34	/* Trident 8900 1024x768 non-interlaced*/
#define T88256_400	35	/* Trident 8800 256 colors 640x400 */
#define T88256_6	36	/* Trident 8800 256 colors 640x480 */
#define LEGEND256_6	37	/* Sigma VGA Legend 256 colors 640x480 */
#define LEGEND256_8	38	/* Sigma VGA Legend 256 colors 800x600 */
#define LEGEND256_8a	39	/* Sigma VGA Legend 256 colors 800x600 alt */
#define LEGEND256_1	40	/* Sigma VGA Legend 256 colors 1024x768 */
#define LEGEND256_1ni	41	/* Sigma VGA Legend 1024x768 non-interlaced*/
#define ATIPLUS256_400	42	/* ATI VGA Wonder+ 640x400 256 colors */
#define ATIPLUS256_6	43	/* ATI VGA Wonder+ 640x480 256 colors */
#define ATIPLUS256_8	44	/* ATI VGA Wonder+ 800x600 256 colors */
#define MICRO3256_8	45	/* Microlabs ET3000 800x600 256 colors */
#define MICRO4256_8	46	/* Microlabs ET4000 800x600 256 colors */
#define MICRO4256_8a	47	/* Microlabs ET4000 alt 800x600 256 colors */


/*
 * NOTICE:  Insert new types here.  Entries from here to the end are 
 * dummy entries that really use other register initialization table
 * entries but need the own unique type for other reasons.  This saves
 * a bunch of space in the reg_init table.
 */
#define P256_1024_400	100	/* Paradise VGA 1024 256 color 640x400  */
#define P256_1024_6	101	/* Paradise VGA 1024 256 color 640x480  */
#define ATI2564_6	102	/* ATI VGA Wonder V4 256 color 640x480 */
#define ATI2565_6	103	/* ATI VGA Wonder V5 256 color 640x480 */
#define ATI2565_8	104	/* ATI VGA Wonder V5 256 color 800x600 */
#define FW256_400	105	/* Video 7 FASTWRITE VGA 256 color 640x400 */
#define FW256_6		106	/* Video 7 FASTWRITE VGA 256 color 640x480 */
#define FW256_7		107	/* Video 7 FASTWRITE VGA 256 color 720x540 */
#define FW256_8		108	/* Video 7 FASTWRITE VGA 256 color 800x600 */
#define FW256_8a	109	/* Video 7 FASTWRITE VGA 800x600 Nanao */
#define EDGE256		110	/* ATI VGA EDGE 256 color 640x400 */

#define SLBYTES 2048	/* max number of bytes in a scanline for V256 */


/*
 * Stuff that's been removed from kd.h in 2.0
 */
#ifdef	SEQ_RESET
#undef	SEQ_RESET
#endif
#define	SEQ_RESET	0x01	/* sychronously reset sequencer */

/*
 *  Temporary change. Sequencer timings changed 1/5  for processor access.
 *
#ifdef	SEQ_RUN
#undef	SEQ_RUN
#endif
#define	SEQ_RUN	0x02	 
* sychronously reset sequencer */

/*
 *  End of change.
 */

#ifdef	V256_BASE
#undef	V256_BASE
#endif
#define	V256_BASE	0xa0000	/* location of enhanced display memory */

/*
 * Define MCOUNT to be nothing so we can successfully build without profiling
 */
#define	MCOUNT
