/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/i386/i_87fp.h	1.3"

/* 64-bit significand, 16-bit exponent */
typedef unsigned short fp_tempreal[5];

/* Save state for both the extended word 80287 and 80387 */
struct sdbfpstate_t {
	unsigned int fp_control;
	unsigned int fp_status;
	unsigned int fp_tag;
	unsigned int fp_ip;
	unsigned short fp_cs;
	unsigned short fp_ds;
	unsigned long fp_data_addr;
	unsigned long fp_unknown;
	fp_tempreal fp_stack[8];
	short new_fp_status;
	short fp_padding; /* for even longword alignment */
};

int		fpregvals[16];	/* floating point register values */
sdbfpstate_t	sdbfpstate;
extern int	emetbit;	/* EM & ET bit in CR0 (Control Regsietr 0).
				 * ET bit (bit 4):
				 *	indicates whether coprocessor 80387 does
				 *	present in the system.
				 * EM bit (bit 2):
				 *	indicates whether the coprocessor functions
				 *	to be emulated.
				 */

#define	EMET_BITS	0x14
#define	EM_BIT		0x4

#define	EM_ONLY		((emetbit & EMET_BITS) == EM_BIT)
#define	STACK_TOP	(sdbfpstate.fp_status >> 11 & 0x7)
#define	TAG_OF_(n)	(sdbfpstate.fp_tag >> ((n) * 2) & 0x3)
