/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODDRV_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODDRV_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/mod/moddrv.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _IO_CONF_H
#include <io/conf.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/conf.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Private data for device drivers.
 */

struct	mod_drv_data	{
	/*
	 * Block device info.
	 */
	struct	bdevsw		drv_bdevsw;	/* bdevsw[] table image */
	int	bmajor_0;			/* the first block major number */
	int	bmajors;			/* number of block majors */
	/*
	 * Character device info.
	 */
	struct	cdevsw		drv_cdevsw;	/* cdevsw[] table image */
	int	cmajor_0;			/* the first character major number */
	int	cmajors;			/* the number of character majors */
};


struct	mod_drvintr	{
	struct	intr_info	*drv_intrinfo;	/* points to an array of structures */
	void	(*ihndler)();			/* the drivers interrupt handler */
};

struct	intr_info	{
	int	ivect_no;	/* the interrupt vector */
	int	int_pri;	/* the interrupt priority */
	int	itype;		/* type of interrupt */
};

#endif	/* _UTIL_MOD_MODDRV_H */
