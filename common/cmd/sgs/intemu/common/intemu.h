/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)intemu:common/intemu.h	1.1"
/*
* common/intemu.h -- interfaces to large, fixed size, two's complement
*			integer computation package
*
* Except for comparison, all the computation functions modify their
* (first) number argument.  Note that the functions that create numbers
* do not return pointers to necessarily modifiable objects.  All copying
* is the responsibility of the caller.
*/

#ifndef NUMSIZE
#   define NUMSIZE	64	/* default size */
#endif

#ifdef __STDC__
#   include <stddef.h>
#   include <limits.h>
#else
#   ifndef size_t
#	define size_t unsigned
#   endif
#   ifndef CHAR_BIT
#	define CHAR_BIT	8	/* good guess */
#   endif
#endif

#define _nBIT_(type)	(sizeof(type) * CHAR_BIT)
#define _aLEN_(type)	((NUMSIZE - 1 + _nBIT_(type)) / _nBIT_(type))

typedef union _intnum_t	/* external version of a number */
{
	unsigned char	uc[_aLEN_(unsigned char)];
	unsigned short	us[_aLEN_(unsigned short)];
	unsigned int	ui[_aLEN_(unsigned int)];
	unsigned long	ul[_aLEN_(unsigned long)];
} IntNum;

typedef struct _numsize_t	/* size information about a number */
{
	unsigned int	sz_nsbit;	/* min. bits needed if signed */
	unsigned int	sz_nubit;	/* min. bits needed if unsigned */
	unsigned int	sz_minbit;	/* minimum of in_n{s,u}bit */
} NumSize;

#undef _nBIT_
#undef _aLEN_

#ifdef __STDC__

		/*
		* General utility functions.
		* num_init is passed pointers to two functions:
		* an internal error function with printf-like arguments
		* and a realloc-like allocation/deallocation function.
		*/
void	num_init(void (*)(const char *, ...),	/* initialize package */
		void *(*)(void *, size_t));
void	num_free(IntNum *);			/* deallocate number */
void	num_size(const IntNum *, NumSize *);	/* set NumSize */

		/*
		* Number creation functions.
		* If a nonnull pointer is passed as the first argument,
		* the result is placed in the object to which it points.
		* If a null pointer is passed, a pointer to a long-lived,
		* possibly shared object will be returned.  If a shared
		* number matches the requested value, the return will
		* always be a pointer to such.
		* If the last argument to num_fromstr is not null, the
		* pointed to character pointer will be set to nonnull
		* only when an invalid string is processed.  It points
		* to a null-terminated string that explains the error.
		* If the last argument is null, num_fromstr produces its
		* best shot at a reasonable value from its input.
		*/
IntNum	*num_fromslong(IntNum *, long);
IntNum	*num_fromulong(IntNum *, unsigned long);
IntNum	*num_fromstr(IntNum *, const char *, size_t, char **);

		/*
		* Functions that produce printable versions
		* of a number.  The resulting null-terminated
		* string is reused by each subsequent call to
		* the same function.
		*/
char	*num_tohex(const IntNum *);
char	*num_tosdec(const IntNum *);
char	*num_toudec(const IntNum *);
char	*num_tooct(const IntNum *);
char	*num_tobin(const IntNum *);

		/*
		* Output functions.
		* A nonzero return indicates that the number
		* did not entirely fit in the target object.
		*/
int	num_tosint(const IntNum *, int *);
int	num_touint(const IntNum *, unsigned int *);
int	num_toslong(const IntNum *, long *);
int	num_toulong(const IntNum *, unsigned long *);
void	num_tosbendian(const IntNum *, unsigned char *, size_t);
void	num_toubendian(const IntNum *, unsigned char *, size_t);
void	num_toslendian(const IntNum *, unsigned char *, size_t);
void	num_toulendian(const IntNum *, unsigned char *, size_t);

		/*
		* Unary computation functions.
		* A nonzero return indicates an overflow.
		*/
void	num_complement(IntNum *);	/* num = ~num */
int	num_negate(IntNum *);		/* num = -num */
void	num_not(IntNum *);		/* num = !num */
int	num_snarrow(IntNum *, unsigned); /* num = TRUNC(num) (signed) */
int	num_unarrow(IntNum *, unsigned); /* num = TRUNC(num) (unsigned) */

		/*
		* Binary computation functions.
		* A nonzero return indicates an overflow,
		* except for the comparison functions.
		* Those functions with signed and unsigned versions
		* are respectively named "num_s..." and "num_u...".
		*/
int	num_smultiply(IntNum *, const IntNum *);	/* dst *= src */
int	num_umultiply(IntNum *, const IntNum *);		/* unsigned */
int	num_sdivide(IntNum *, const IntNum *);		/* dst /= src */
int	num_udivide(IntNum *, const IntNum *);			/* unsigned */
int	num_sremainder(IntNum *, const IntNum *);	/* dst %= src */
int	num_uremainder(IntNum *, const IntNum *);		/* unsigned */
int	num_sdivrem(IntNum *, IntNum *, const IntNum *, const IntNum *);
int	num_udivrem(IntNum *, IntNum *, const IntNum *, const IntNum *);

void	num_gcd(IntNum *, const IntNum *);	/* greatest common divisor */
int	num_lcm(IntNum *, const IntNum *);	/* least common multiple */

int	num_sadd(IntNum *, const IntNum *);		/* dst += src */
int	num_uadd(IntNum *, const IntNum *);			/* unsigned */
int	num_subtract(IntNum *, const IntNum *);		/* dst -= src */

int	num_llshift(IntNum *, int);		/* dst <<= int (logical) */
int	num_alshift(IntNum *, int);		/* dst <<= int (arithmetic) */
int	num_lrshift(IntNum *, int);		/* dst >>= int (logical) */
int	num_arshift(IntNum *, int);		/* dst >>= int (arithmetic) */
void	num_lrotate(IntNum *, int);		/* dst <<<= int */
void	num_rrotate(IntNum *, int);		/* dst >>>= int */

int	num_scompare(const IntNum *, const IntNum *);	/* <0, 0, >0 */
int	num_ucompare(const IntNum *, const IntNum *);		/* unsigned */

void	num_and(IntNum *, const IntNum *);	/* dst &= src */
void	num_xor(IntNum *, const IntNum *);	/* dst ^= src */
void	num_or(IntNum *, const IntNum *);	/* dst |= src */

#else	/* !__STDC__ */

void	num_init(), num_free(), num_info();
IntNum	*num_fromstr(), *num_fromslong(), *num_fromulong();
char	*num_tohex(), *num_tosdec(), *num_toudec(), *num_tooct(), *num_tobin();
void	num_tosbendian(), num_toubendian(), num_toslendian(), num_toulendian();
int	num_toslong(), num_toulong();
void	num_complement(), num_not();
int	num_negate(), num_snarrow(), num_unarrow();
int	num_smultiply(), num_umultiply(), num_sdivide(), num_udivide();
int	num_sremainder(), num_uremainder(), num_sdivrem(), num_udivrem();
void	num_gcd();
int	num_lcm();
int	num_sadd(), num_uadd(), num_subtract();
int	num_llshift(), num_alshift(), num_lrshift(), num_arshift();
void	num_lrotate(), num_rrotate();
int	num_scompare(), num_ucompare();
void	num_and(), num_xor(), num_or();

#endif	/* __STDC__ */
