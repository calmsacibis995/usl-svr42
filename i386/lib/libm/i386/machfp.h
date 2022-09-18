/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:i386/machfp.h	1.1"

#if _IEEE

/* byte order with low order bits at lowest address */
/* double precision */
typedef  union {
	struct {
		unsigned  lo	:32;
		unsigned  hi	:20;
		unsigned  exp	:11;
		unsigned  sign	:1;
	} fparts;
	struct {
		unsigned  lo	:32;
		unsigned  hi	:19;
		unsigned  qnan_bit	:1;
		unsigned  exp	:11;
		unsigned  sign	:1;
	} nparts;
	struct {
		unsigned  lo	:32;
		unsigned  hi	:32;
	} fwords;
	double	d;
} _dval;

/* single precision */
typedef  union {
	struct {
		unsigned fract	:23;
		unsigned exp	:8;
		unsigned sign	:1;
	} fparts;
	struct {
		unsigned fract	:22;
		unsigned qnan_bit	:1;
		unsigned exp	:8;
		unsigned sign	:1;
	} nparts;
	unsigned long	fword;
	float	f;
} _fval;

/* break up mantissa of float into 15 bit chunks */
/* 386 byte order */

union	rdval {
	double	d;
	struct {
		unsigned int	p4 : 15;
		unsigned int	p3 : 15;
		unsigned int	p2b : 2;
		unsigned int	p2a : 13;
		unsigned int	p1 : 7;
		unsigned int 	exp : 11; 
		unsigned int	sgn : 1;
	} dp;
} ;

union	rfval {
	float	d;
	struct {
		unsigned int	p2 : 15;
		unsigned int	p1 : 8;
		unsigned int 	exp : 8; 
		unsigned int	sgn : 1;
	} dp;
} ;

#endif	/* _IEEE */
