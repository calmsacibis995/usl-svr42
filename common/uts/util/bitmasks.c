/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/bitmasks.c	1.1.3.2"
#ident	"$Header: $"

/*
 * Bit mask tables for general use.
 */

int setmask[] = {	/* table of bit masks with first N bits set */
	0x0,
	0x1,
	0x3,
	0x7,
	0xf,
	0x1f,
	0x3f,
	0x7f,
	0xff,
	0x1ff,
	0x3ff,
	0x7ff,
	0xfff,
	0x1fff,
	0x3fff,
	0x7fff,
	0xffff,
	0x1ffff,
	0x3ffff,
	0x7ffff,
	0xfffff,
	0x1fffff,
	0x3fffff,
	0x7fffff,
	0xffffff,
	0x1ffffff,
	0x3ffffff,
	0x7ffffff,
	0xfffffff,
	0x1fffffff,
	0x3fffffff,
	0x7fffffff,
	0xffffffff,
};

int sbittab[] = {	/* table of bit masks with Nth bit set */
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80,
	0x100,
	0x200,
	0x400,
	0x800,
	0x1000,
	0x2000,
	0x4000,
	0x8000,
	0x10000,
	0x20000,
	0x40000,
	0x80000,
	0x100000,
	0x200000,
	0x400000,
	0x800000,
	0x1000000,
	0x2000000,
	0x4000000,
	0x8000000,
	0x10000000,
	0x20000000,
	0x40000000,
	0x80000000,
};

int cbittab[] = {	/* table of bit masks with Nth bit clear */
	0xfffffffe,
	0xfffffffd,
	0xfffffffb,
	0xfffffff7,
	0xffffffef,
	0xffffffdf,
	0xffffffbf,
	0xffffff7f,
	0xfffffeff,
	0xfffffdff,
	0xfffffbff,
	0xfffff7ff,
	0xffffefff,
	0xffffdfff,
	0xffffbfff,
	0xffff7fff,
	0xfffeffff,
	0xfffdffff,
	0xfffbffff,
	0xfff7ffff,
	0xffefffff,
	0xffdfffff,
	0xffbfffff,
	0xff7fffff,
	0xfeffffff,
	0xfdffffff,
	0xfbffffff,
	0xf7ffffff,
	0xefffffff,
	0xdfffffff,
	0xbfffffff,
	0x7fffffff,
};