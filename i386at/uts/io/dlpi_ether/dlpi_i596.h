/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _IO_DLPI_ETHER_DLPI_I596_H    /* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_I596_H    /* wrapper symbol for kernel use */

#ident	"@(#)uts-x86at:io/dlpi_ether/dlpi_i596.h	1.5"
#ident  "$Header: $"

/*
 *  Device dependent symbol names.
 */
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
#include <io/dlpi_ether/i596.h>

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
#include <sys/i596.h>

#endif	/* _KERNEL_HEADERS */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"i596"
#define	DLdevflag		i596devflag
#define	DLrminfo		i596rminfo
#define	DLwminfo		i596wminfo
#define	DLrinit			i596rinit
#define	DLwinit			i596winit

/*
 *  Functions
 */
#define DLopen			i596open
#define	DLclose			i596close
#define DLrput			i596rput
#define	DLwput			i596wput
#define	DLioctl			i596ioctl
#define	DLinfo			i596info
#define	DLloopback		i596loopback
#define	DLmk_ud_ind		i596mk_ud_ind
#define	DLxmit_packet	i596xmit_packet
#define	DLinfo_req		i596info_req
#define	DLcmds			i596cmds
#define	DLprint_eaddr	i596print_eaddr
#define	DLbind_req		i596bind_req
#define	DLrsrv			i596rsrv
#define	DLunbind_req	i596unbind_req
#define	DLunitdata_req	i596unitdata_req
#define	DLerror_ack		i596error_ack
#define	DLuderror_ind	i596uderror_ind
#define	DLpromisc_off	i596promisc_off
#define	DLpromisc_on	i596promisc_on
#define	DLset_eaddr		i596set_eaddr
#define	DLadd_multicast	i596add_multicast
#define	DLdel_multicast	i596del_multicast
#define	DLget_multicast	i596get_multicast
#define	DLdisable		i596disable
#define	DLenable		i596enable
#define	DLreset			i596reset
#define	DLis_multicast	i596is_multicast

#define DLrecv			i596recv
#define DLproc_llc		i596proc_llc
#define	DLform_80223	i596form_80223
#define DLis_us			i596is_us
#define DLis_broadcast	i596is_broadcast
#define DLis_validsnap	i596is_validsnap
#define DLis_equalsnap	i596is_equalsnap
#define DLform_snap		i596form_snap
#define DLmk_test_con	i596mk_test_con
#define DLinsert_sap	i596insert_sap
#define DLsubsbind_req	i596subsbind_req
#define DLtest_req		i596test_req
#define DLremove_sap	i596remove_sap

#define DLbdspecioctl	i596bdspecioctl
#define DLbdspecclose	i596bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	i596boards
#define DLconfig	i596config
#define DLsaps		i596saps
#define DLstrlog	i596strlog
#define DLifstats	i596ifstats
#define	DLinetstats	i596inetstats
#define	DLid_string	i596id_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header includes 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC	(DL_MAX_PACKET - 3) 
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER			4096
#define	DL_LOWATER			256

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#define TBD_BUF_SIZ 1520
#define RBD_BUF_SIZ 1520

#define CSW_BIT		0x40	/* 596's CSW_BIT */
#define MODE_32BIT	0x02	/* 32-Bit Segmented Mode */
#define FLEX_MODE	0x08	/* Flexible Mode */

#pragma pack(1)
/* only pertinent elements are provided */
struct	ls_config {
	uchar_t		info_length;
	uchar_t		sys_info[20];
	uchar_t		lan_eaddr[6];
	ushort_t	lan_t_on;
	ushort_t	lan_t_off;
	uchar_t		lan_fifo_threshold;
	uchar_t		lan_irq;
	uchar_t		lan_enabled;
	ushort_t	lan_reserved[7];
	uchar_t		scsi_info[17];
};

typedef struct EISA_function_record_info {
	uchar_t				board_info[35];
	char				type_subt[80];
	struct ls_config 	lan_scsi_config;
	uchar_t				extra_stuff[140];
	uchar_t				reserved[30];
} Einfo_t;
#pragma pack()

#ifndef lint

/* Inline asm routines for Ethernet address acquisition logic. */
asm	void i596read_EISA_records(virt_INT15, slot, funct, info_ptr, ret_code_ptr)
{
%mem		virt_INT15, slot, funct, info_ptr, ret_code_ptr;
	push	%edi
	push	%esi
	movl	virt_INT15, %edi	/* INT15 entry point */
	movb	slot, %cl			/* cl = slot number */
	movb	funct, %ch			/* ch = function number */
	movl	info_ptr, %esi		/* esi = destination for info */
	movb	$0x81, %al			/* read function info */
	movb	$0xd8, %ah
	pushf
	push	%cs
	call	*%edi				/* ah has return code */
	movl	ret_code_ptr, %esi
	movb	%ah, (%esi)
	pop		%esi
	pop		%edi
}

#endif	/* ifdef lint */

#endif	/* _IO_DLPI_ETHER_DLPI_I596_H */
