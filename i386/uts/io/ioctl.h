/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_IOCTL_H	/* wrapper symbol for kernel use */
#define _IO_IOCTL_H	/* subject to change without notice */
#define _SYS_IOCTL_H
#ident	"@(#)uts-x86:io/ioctl.h	1.5"
#ident	"$Header: $"

/*
 *      There are some inherent problems in having a single file
 *      ioctl.h, with both System V and BSD flags. Introducing
 * 	BSD flags into this file creates compilation problems
 *	with flags such as ECHO, NL1 etc., if termio.h and ioctl.h
 *      are included by the same file. Since these two files can
 * 	be only included by System V applications, /usr/include/sys/ioctl.h
 *      will be System V mode and all the BSD flags will be turned off
 *      using #ifdef BSD_COMP. This file will also exist in 
 *	/usr/ucbinclude/sys/ioctl.h for BSD applications but without the
 *      BSD flags turned off. System V appliactions can use ioctl.h without
 *      any changes, System V applications requiring BSD flags should
 *      -D BSD_COMP when compiling (and be warned about the common
 *      flags between System V and BSD) and BSD applications should
 * 	use /usr/ucbinclude/sys/ioctl.h.
 *     
 */

#define	IOCTYPE	0xff00

#define	LIOC	('l'<<8)
#define	LIOCGETP	(LIOC|1)
#define	LIOCSETP	(LIOC|2)
#define	LIOCGETS	(LIOC|5)
#define	LIOCSETS	(LIOC|6)

#define	DIOC	('d'<<8)
#define	DIOCGETC	(DIOC|1)
#define	DIOCGETB	(DIOC|2)
#define	DIOCSETE	(DIOC|3)

/* BSD related defines */

#ifdef BSD_COMP

#ifdef _KERNEL_HEADERS

#ifndef _IO_TTYCHARS_H
#include <io/ttychars.h>	/* SVR4.0COMPAT */
#endif
#ifndef _IO_TTYDEV_H
#include <io/ttydev.h>		/* SVR4.0COMPAT */
#endif
#ifndef _IO_TTOLD_H
#include <io/ttold.h>		/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/ttychars.h>	/* SVR4.0COMPAT */
#include <sys/ttydev.h>		/* SVR4.0COMPAT */
#include <sys/ttold.h>		/* SVR4.0COMPAT */

#else

#include <sys/ttychars.h>	/* SVR4.0COMPAT */
#include <sys/ttydev.h>		/* SVR4.0COMPAT */
#include <sys/ttold.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define	TANDEM		O_TANDEM
#define	CBREAK		O_CBREAK
#ifndef _SGTTY_H
#define	LCASE		O_LCASE
#define	ECHO		O_ECHO
#define	CRMOD		O_CRMOD
#define	RAW		O_RAW
#define	ODDP		O_ODDP
#define	EVENP		O_EVENP
#define	ANYP		O_ANYP
#define	NLDELAY		O_NLDELAY
#define		NL0		O_NL0
#define		NL1		O_NL1
#define		NL2		O_NL2
#define		NL3		O_NL3
#define	TBDELAY		O_TBDELAY
#define		TAB0		O_TAB0
#define		TAB1		O_TAB1
#define		TAB2		O_TAB2
#define	XTABS		O_XTABS
#define	CRDELAY		O_CRDELAY
#define		CR0		O_CR0
#define		CR1		O_CR1
#define		CR2		O_CR2
#define		CR3		O_CR3
#define	VTDELAY		O_VTDELAY
#define		FF0		O_FF0
#define		FF1		O_FF1
#define	BSDELAY		O_BSDELAY
#define		BS0		O_BS0
#define		BS1		O_BS1
#define 	ALLDELAY	O_ALLDELAY
#endif /* _SGTTY_H */
#define	CRTBS		O_CRTBS
#define	PRTERA		O_PRTERA
#define	CRTERA		O_CRTERA
#define	TILDE		O_TILDE
#define	MDMBUF		O_MDMBUF
#define	LITOUT		O_LITOUT
#define	TOSTOP		O_TOSTOP
#define	FLUSHO		O_FLUSHO
#define	NOHANG		O_NOHANG
#define	L001000		O_L001000
#define	CRTKIL		O_CRTKIL
#define	PASS8		O_PASS8
#define	CTLECH		O_CTLECH
#define	PENDIN		O_PENDIN
#define	DECCTQ		O_DECCTQ
#define	NOFLSH		O_NOFLSH

#ifdef _KERNEL_HEADERS

#ifndef _FS_FILIO_H
#include <fs/filio.h>			/* COMPATIBILITY */
#endif
#ifndef _NET_TRANSPORT_SOCKIO_H
#include <net/transport/sockio.h>	/* COMPATIBILITY */
#endif

#elif defined(_KERNEL)

#include <sys/filio.h>			/* COMPATIBILITY */
#include <sys/sockio.h>			/* COMPATIBILITY */

#else

#include <sys/filio.h>			/* COMPATIBILITY */
#include <sys/sockio.h>			/* COMPATIBILITY */

#endif /* _KERNEL_HEADERS */

#endif /* BSD_COMP */

/*
**	Union for use by all device handler ioctl routines.
*/
union ioctl_arg {
	struct termio	*stparg;	/* ptr to termio struct */
	struct Generic	*sparg;		/* ptr to generic struct */
	char		*cparg;		/* ptr to character */
	char		carg;		/* character */
	int		*iparg;		/* ptr to integer */
	int		iarg;		/* integer */
	long            *lparg;         /* ptr to long */
	long            larg;           /* long */
};

/*
 * Commands needed for XENIX ioctl() compatibility
 */

#define	TIOC	('T'<<8)
#define	TCFLSH	(TIOC|7)

#endif	/* _IO_IOCTL_H */
