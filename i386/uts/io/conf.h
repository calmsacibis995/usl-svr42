/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_CONF_H	/* wrapper symbol for kernel use */
#define _IO_CONF_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/conf.h	1.5"
#ident	"$Header: $"

/*
 * Declaration of block device switch. Each entry (row) is
 * the only link between the main unix code and the driver.
 * The initialization of the device switches is in the file conf.c.
 */
struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_print)();
	int	(*d_size)();
	char	*d_name;
	struct iobuf	*d_tab;
	int	*d_flag;
};

extern struct bdevsw bdevsw[];		/* switch table for block devices */
extern struct bdevsw shadowbsw[];

/*
 * Declaration of character device switch.  Each entry (row) is
 * the only link between the main unix code and the driver.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_mmap)();
	int	(*d_segmap)();
	int	(*d_poll)();
	struct tty *d_ttys;
	struct streamtab *d_str;
	char	*d_name;
	int	*d_flag;
};

extern struct cdevsw cdevsw[];		/* switch table for character devices */
extern struct cdevsw shadowcsw[];

/*
 * And the console co routine.	This is declared as
 * a configuration parameter so that it can be changed
 * to match /dev/console.
 */
struct	conssw {
    int (*co)();
    int co_dev;
    int (*ci)();
    int co_mode;
};

extern struct conssw conssw;

/*
 * Device flags.
 *
 * Bit 0 to bit 15 are reserved for kernel.
 * Bit 16 to bit 31 are reserved for different machines.
 */
#define D_NEW		0x00	/* new-style driver */
#define	D_OLD		0x01	/* old-style driver */
#define D_DMA		0x02	/* driver does DMA  */
/*
 * Added for UFS.
 */
#define D_SEEKNEG       0x04    /* negative seek offsets are OK */
#define D_TAPE          0x08    /* magtape device (no bdwrite when cooked) */

#define ROOTFS_NAMESZ	7	/* Maximum length of root fstype name */
/*
 * Added for pre-4.0 drivers backward compatibility.
 */
#define D_NOBRKUP	0x10	/* no breakup needed for new drivers */
/*
 * Security additions, for drivers requiring a special MAC access policy.
 */
#define D_INITPUB	0x20	/* device is public in system setting */
#define D_NOSPECMACDATA	0x40	/* no MAC access check for data transfer */ 
				/* and no inode access time change */ 
#define D_RDWEQ		0x80	/* destructive reads, read equal, write eq */
#define SECMASK		0xE0	/* mask for security flags */

#define	FMNAMESZ	8	/* maximum length of a STREAMS module name */

/*
 * Declaration of STREAMS module switch.
 */
struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;
	int	*f_flag;	/* same as device flag */
};
extern struct fmodsw fmodsw[];	/* STREAMS module switch table */

/*
 * Total number of types of block devices, character devices, STREAMS modules
 * in their respective tables.
 */
extern int	bdevcnt;
extern int	cdevcnt;
extern int	fmodcnt;

/*
 * Line discipline switch.
 */
struct linesw {
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_input)();
	int	(*l_output)();
	int	(*l_mdmint)();
};
extern struct linesw linesw[];	/* switch table for line discipline */

extern int	linecnt;	/* total number of types of line disciplines */

#endif	/* _IO_CONF_H */
