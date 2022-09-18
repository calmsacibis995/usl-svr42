/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:sgtty.h	1.3.3.1"

#ifndef _SGTTY_H
#define _SGTTY_H

/*
 * Modes
 */
#define	HUPCL	01
#ifndef _SYS_IOCTL_H
#define	XTABS	02
#define	LCASE	04
#define	ECHO	010
#define	CRMOD	020
#define	RAW	040
#define	ODDP	0100
#define	EVENP	0200
#define ANYP	0300
#define	NLDELAY	001400
#define	TBDELAY	002000
#define	CRDELAY	030000
#define	VTDELAY	040000
#define BSDELAY 0100000
#define ALLDELAY 0177400

/*
 * Delay algorithms
 */
#define	CR0	0
#define	CR1	010000
#define	CR2	020000
#define	CR3	030000
#define	NL0	0
#define	NL1	000400
#define	NL2	001000
#define	NL3	001400
#define	TAB0	0
#define	TAB1	002000
#endif /* _SYS_IOCTL_H */
#define	NOAL	004000
#ifndef _SYS_IOCTL_H
#define	FF0	0
#define	FF1	040000
#define	BS0	0
#define	BS1	0100000
#endif /* _SYS_IOCTL_H */

#ifndef _SYS_TTOLD_H

/*
 * Structure for stty and gtty system calls.
 */
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	int	sg_flags;		/* mode flags */
};

/* BSD local special chars. Structure for TIOCSLTC/TIOCGLTC */
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

/*
 * Speeds
 */
#ifdef m88k
#define	B0	0
#define	B50	000000200000
#define	B75	000000400000
#define	B110	000000600000
#define	B134	000001000000
#define	B150	000001200000
#define	B200	000001400000
#define	B300	000001600000
#define	B600	000002000000
#define	B1200	000002200000
#define	B1800	000002400000
#define	B2400	000002600000
#define	B4800	000003000000
#define	B9600	000003200000
#define	B19200	000003400000
#define EXTA	000003400000
#define	B38400	000003600000
#define EXTB	000003600000
#else
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define EXTA	14
#define EXTB	15
#endif

/*
 *	ioctl arguments
 */
#define FIOCLEX		(('f'<<8)|1)
#define FIONCLEX	(('f'<<8)|2)
#define	TIOCHPCL	(('t'<<8)|2)
#define	TIOCGETP	(('t'<<8)|8)
#define	TIOCSETP	(('t'<<8)|9)
#define	TIOCEXCL	(('t'<<8)|13)
#define	TIOCNXCL	(('t'<<8)|14)
#endif /*_SYS_TTOLD_H */

#endif 	/* _SGTTY_H */
