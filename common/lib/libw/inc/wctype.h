/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:inc/wctype.h	1.1.2.4"
#ident  "$Header: wctype.h 1.2 91/06/26 $"

/*	definitions for international function		*/

#ifndef _WCHAR_T
#define _WCHAR_T
typedef long    wchar_t;
#endif

extern	unsigned _iswctype();
extern	wchar_t _trwctype();
wchar_t	_ctmp_;

/*	bit definition for character class	*/

#define	_E1	0x00000100	/* phonogram (international use) */
#define	_E2	0x00000200	/* ideogram (international use) */
#define	_E3	0x00000400	/* English (international use) */
#define	_E4	0x00000800	/* number (international use) */
#define	_E5	0x00001000	/* special (international use) */
#define	_E6	0x00002000	/* other characters (international use) */
#define	_E7	0x00004000	/* reserved (international use) */
#define	_E8	0x00008000	/* reserved (international use) */

#define	_E9	0x00010000
#define	_E10	0x00020000
#define	_E11	0x00040000
#define	_E12	0x00080000
#define	_E13	0x00100000
#define	_E14	0x00200000
#define	_E15	0x00400000
#define	_E16	0x00800000
#define	_E17	0x01000000
#define	_E18	0x02000000
#define	_E19	0x04000000
#define	_E20	0x08000000
#define	_E21	0x10000000
#define	_E22	0x20000000
#define	_E23	0x40000000
#define	_E24	0x80000000

/*	data structure for supplementary code set
		for character class and conversion	*/

struct	_wctype {
	long	tmin;		/* minimum code for wctype */
	long	tmax;		/* maximum code for wctype */
	unsigned char  *index;	/* class index */
	unsigned int   *type;	/* class type */
	long	cmin;		/* minimum code for conversion */
	long	cmax;		/* maximum code for conversion */
	long	*code;		/* conversion code */
};

/*	character classification functions	*/

#define	iswalpha(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_U|_L)	\
				 	    : isalpha(_ctmp_))
#define	iswupper(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_U)	\
					    : isupper(_ctmp_))
#define	iswlower(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_L)	\
					    : islower(_ctmp_))
#define	iswdigit(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_N)	\
					    : isdigit(_ctmp_))
#define	iswxdigit(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_X)	\
					    : isxdigit(_ctmp_))
#define	iswalnum(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_U|_L|_N) \
					    : isalnum(_ctmp_))
#define	iswspace(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_S)	\
					    : isspace(_ctmp_))
#define	iswpunct(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_P)	\
					    : ispunct(_ctmp_))
#define	iswprint(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_P|_U|_L|_N|_B|_E1|_E2|_E5|_E6)\
					    : isprint(_ctmp_))
#define	iswgraph(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_P|_U|_L|_N|_E1|_E2|_E5|_E6)\
					    : isgraph(_ctmp_))
#define	iswcntrl(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_C)	\
					    : iscntrl(_ctmp_))
#define	iswascii(c)	isascii(c)
#define	isphonogram(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_E1) : 0)
#define	isideogram(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_E2) : 0)
#define	isenglish(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_E3) : 0)
#define	isnumber(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_E4) : 0)
#define	isspecial(c)	((_ctmp_=(c)) > 255 ? _iswctype(_ctmp_,_E5) : 0)
#define	towupper(c)	((_ctmp_=(c)) > 255 ? _trwctype(_ctmp_,_L)	\
					    : toupper(_ctmp_))
#define	towlower(c)	((_ctmp_=(c)) > 255 ? _trwctype(_ctmp_,_U)	\
					    : tolower(_ctmp_))

#define H_EUCMASK	0x8080 		/* mask bit   for 2 bytes code */
#define H_P00		0 		/* code set 0 for 2 bytes code */
#define H_P11		0x8080 		/* code set 1 for 2 bytes code */
#define H_P01		0x0080 		/* code set 2 for 2 bytes code */
#define H_P10		0x8000		/* code set 3 for 2 bytes code */
#define EUCMASK		0x30000000 	/* mask bit   for 4 bytes code */
#define P00		0 		/* code set 0 for 4 bytes code */
#define P11		0x30000000 	/* code set 1 for 4 bytes code */
#define P01		0x10000000 	/* code set 2 for 4 bytes code */
#define P10		0x20000000	/* code set 3 for 4 bytes code */

#define	iscodeset0(c)	isascii(c)
#ifndef _WCHAR16
#define	iscodeset1(c)	(((c) & EUCMASK) == P11)
#define	iscodeset2(c)	(((c) & EUCMASK) == P01)
#define	iscodeset3(c)	(((c) & EUCMASK) == P10)
#else
#define	iscodeset1(c)	(((c) & H_EUCMASK) == H_P11)
#define	iscodeset2(c)	((((c) & H_EUCMASK) == H_P01) && (!iscntrl(c)))
#define	iscodeset3(c)	(((c) & H_EUCMASK) == H_P10)
#endif
