/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_CONSEM_CONSEM_H	/* wrapper symbol for kernel use */
#define _IO_CONSEM_CONSEM_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/consem/consem.h	1.2"
#ident	"$Header: $"

/*
* Console Emulator. A Pushable Streams Module to provide 386
* UNIX and XENIX console operations in an X Window System Environment
*/

#define CS_OPEN	1	/* This structure in Use */
#define WAITING	2	/* Waiting for a response from user level */
#define CO_REQ	4	/* Pending COPYOUT acknowledge */

#define CSEM_O	01	/* COPYOUT data to application issuing ioctl */
#define CSEM_I	02	/* COPYIN data from application issuing ioctl */
#define CSEM_B	03	/* COPYIN and COPYOUT data ioctl */
#define CSEM_R	04	/* Return "rval" to application issuing ioctl */
#define CSEM_N	010	/* Return Only Sucess; no COPY or "rval" */

struct csem{
	int state;		/* State Flag */
	int indx;		/* index into escape table below (csem_esc_t)*/
	int to_id;		/* Pending Timeout identifier */
	mblk_t *c_mblk;		/* Message Block Pointer */
	queue_t *c_q;		/* Queue Pointer */
	struct iocblk *iocp;	/* ioctl Pointer */
	int ioc_cmd;		/* Saved ioctl cmd value */
	ushort ioctl_uid;	/* Saved ioctl user id */
	ushort ioctl_gid;	/* Saved ioctl user group id */
	uint ioc_count;		/* Saved ioctl data transfer */
	uint ioc_id;		/* Streams ioctl ioctl operation identifer */
};

/* Output Escape sequence to xterm is: <ESC>, @, 2, esc_at	*/
/* Input Escape sequence from xterm is: <ESC>, @, 3, esc_at	*/

struct csem_esc_t{
		char esc_at;	/* Single char to follow escape preamble */
		char *name;	/* ioctl ascii name */
		int   ioctl;	/* ioctl value */
		short type;	/* type (0=only return sucess or failure) */
		short b_in;	/* bytes in */
		short b_out;	/* bytes out */
};

#endif	/* _IO_CONSEM_CONSEM_H */
