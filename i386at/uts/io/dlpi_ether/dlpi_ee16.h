/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_DLPI_ETHER_DLPI_EE16_H	/* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_EE16_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/dlpi_ee16.h	1.10"
#ident	"$Header: $"

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef	_KERNEL_HEADERS

#include <util/types.h>
#include <io/stream.h>
#include <util/param.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <io/stropts.h>
#include <io/strstat.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/tcpip/strioc.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/tcpip/if.h>
#include <net/dlpi.h>
#include <mem/immu.h>
#include <io/ddi.h>
#include <svc/systm.h>
#ifndef lint
#include <net/tcpip/byteorder.h>
#endif
#include <io/rtc/rtc.h>
#include <util/cmn_err.h>
#include <io/dlpi_ether/ee16.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <net/strioc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <sys/dlpi.h>
#include <sys/immu.h>
#include <sys/ddi.h>
#include <sys/systm.h>
#ifndef	lint
#include <sys/byteorder.h>
#endif
#include <sys/rtc.h>
#include <sys/cmn_err.h>
#include <sys/ee16.h>

#endif	/* _KERNEL_HEADERS */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"ee16"
#define	DLdevflag		ee16devflag
#define	DLrminfo		ee16rminfo
#define	DLwminfo		ee16wminfo
#define	DLrinit			ee16rinit
#define	DLwinit			ee16winit

/*
 *  Functions
 */
#define DLopen			ee16open
#define	DLclose			ee16close
#define DLrput			ee16rput
#define	DLwput			ee16wput
#define	DLioctl			ee16ioctl
#define	DLinfo			ee16info
#define	DLloopback		ee16loopback
#define	DLmk_ud_ind		ee16mk_ud_ind
#define	DLxmit_packet	ee16xmit_packet
#define	DLinfo_req		ee16info_req
#define	DLcmds			ee16cmds
#define	DLprint_eaddr	ee16print_eaddr
#define	DLbind_req		ee16bind_req
#define	DLrsrv			ee16rsrv
#define	DLunbind_req	ee16unbind_req
#define	DLunitdata_req	ee16unitdata_req
#define	DLerror_ack		ee16error_ack
#define	DLuderror_ind	ee16uderror_ind
#define	DLpromisc_off	ee16promisc_off
#define	DLpromisc_on	ee16promisc_on
#define	DLset_eaddr		ee16set_eaddr
#define	DLadd_multicast	ee16add_multicast
#define	DLdel_multicast	ee16del_multicast
#define	DLget_multicast	ee16get_multicast
#define	DLdisable		ee16disable
#define	DLenable		ee16enable
#define	DLreset			ee16reset
#define	DLis_multicast	ee16is_multicast
#define DLrecv		ee16recv
#define DLproc_llc	ee16proc_llc
#define	DLform_80223	ee16form_80223
#define DLis_us		ee16is_us
#define DLis_broadcast	ee16is_broadcast
#define DLis_validsnap	ee16is_validsnap
#define DLis_equalsnap	ee16is_equalsnap
#define DLform_snap	ee16form_snap
#define DLmk_test_con	ee16mk_test_con
#define DLinsert_sap	ee16insert_sap
#define DLsubsbind_req	ee16subsbind_req
#define DLtest_req	ee16test_req
#define DLremove_sap	ee16remove_sap

#define DLbdspecioctl	ee16bdspecioctl
#define DLbdspecclose	ee16bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	ee16boards
#define DLconfig	ee16config
#define DLsaps		ee16saps
#define DLstrlog	ee16strlog
#define DLifstats	ee16ifstats
#define	DLinetstats	ee16inetstats
#define	DLid_string	ee16id_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header includes 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC      	(DL_MAX_PACKET - 3) 
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		4096
#define	DL_LOWATER		256

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#define TBD_BUF_SIZ 1520
#define RBD_BUF_SIZ 1520

#define	BYTE	0
#define WORD	1

#define read_buffer(size, location, bio, dest) \
			if (inb(bio + AUTOID)) ;	       \
			outw(bio + RDPTR, location);	   \
			if (size == BYTE)				   \
				dest = inb(bio + DXREG);	   \
			else dest = inw(bio + DXREG)

#define write_buffer(size, location, value, bio) \
			if (inb(bio + AUTOID)) ;	        \
			outw(bio + WRPTR, location); 	    \
			if (size == BYTE)				    \
				outb(bio + DXREG, value);		\
			else outw(bio + DXREG, value)

/* Inline asm routines for block IO moves */
#ifndef lint
#ifndef C_PIO

asm	void bcopy_from_buffer(dest, src, count, base_io)
{
%mem	dest, src, count, base_io;lab	r_1;
	movl	count, %ecx
	movl	dest, %eax
	movl	base_io, %edx
	push	%edi
	movl	%eax, %edi		/* %edi = destination for read */

	push	%edx			/* avoids race condition */
	addl	$AUTOID, %edx
	inb		(%dx)
	pop		%edx

	push	%edx
	movl	src, %eax  		/* (base_io + RDPTR) <- location */
	addl	$RDPTR, %edx	
	outw	(%dx)
	pop		%edx
	addl	$DXREG, %edx	
	testw	$1, %cx		/* bit 0 set? */
	jz		r_1
	inb		(%dx)
	stosb
	dec		%ecx
r_1:
	shrl	$1, %ecx		/* cx <- cx / 2 */
	rep		
	insw
	pop	%edi
}

asm	void bcopy_to_buffer(src, dest, count, base_io)
{
%mem	src, dest, count, base_io;lab t_1;
	movl	count, %ecx
	movl	src, %eax
	movl	base_io, %edx
	push	%esi
	movl	%eax, %esi		    /* %esi = source for write */

	push	%edx				/* avoids race condition */
	addl	$AUTOID, %edx
	inb		(%dx)
	pop		%edx

	push	%edx
	addl	$WRPTR, %edx
	movl	dest, %eax
	outw    (%dx)
	pop     %edx
	addl	$DXREG, %edx
	testw	$1, %cx		/* bit 0 set? */
	jz		t_1
	lodsb
	outb	(%dx)			
	dec		%ecx
t_1:
	shrl	$1, %ecx		/* cx <- cx / 2 */
	rep		
	outsw
	pop		%esi
}

#endif	/* ifdef C_PIO */
#endif	/* ifdef lint */


#endif	/* _IO_DLPI_ETHER_DLPI_EE16_H */
