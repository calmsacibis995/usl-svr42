/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_DLPI_IMX586_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_DLPI_IMX586_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/dlpi_imx586.h	1.8"
#ident  "$Header: $"

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
#include <svc/systm.h>
#include <io/rtc/rtc.h>
#include <util/cmn_err.h>
#include <net/tcpip/byteorder.h>
#include <io/dlpi_ether/imx586.h>

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
#include <sys/systm.h>
#include <sys/rtc.h>
#include <sys/cmn_err.h>
#include <sys/byteorder.h>
#include <sys/imx586.h>

#endif	/* _KERNEL_HEADERS */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"imx586"
#define	DLdevflag		imx586devflag
#define	DLrminfo		imx586rminfo
#define	DLwminfo		imx586wminfo
#define	DLrinit			imx586rinit
#define	DLwinit			imx586winit

/*
 *  Functions
 */
#define DLopen			imx586open
#define	DLclose			imx586close
#define DLrput			imx586rput
#define	DLwput			imx586wput
#define	DLioctl			imx586ioctl
#define	DLinfo			imx586info
#define	DLloopback		imx586loopback
#define	DLmk_ud_ind		imx586mk_ud_ind
#define	DLxmit_packet	imx586xmit_packet
#define	DLinfo_req		imx586info_req
#define	DLcmds			imx586cmds
#define	DLprint_eaddr	imx586print_eaddr
#define	DLbind_req		imx586bind_req
#define	DLrsrv			imx586rsrv
#define	DLunbind_req	imx586unbind_req
#define	DLunitdata_req	imx586unitdata_req
#define	DLerror_ack		imx586error_ack
#define	DLuderror_ind	imx586uderror_ind
#define	DLpromisc_off	imx586promisc_off
#define	DLpromisc_on	imx586promisc_on
#define	DLset_eaddr		imx586set_eaddr
#define	DLadd_multicast	imx586add_multicast
#define	DLdel_multicast	imx586del_multicast
#define	DLget_multicast	imx586get_multicast
#define	DLdisable		imx586disable
#define	DLenable		imx586enable
#define	DLreset			imx586reset
#define	DLis_multicast	imx586is_multicast
#define DLrecv		imx586recv
#define DLproc_llc	imx586proc_llc
#define	DLform_80223	imx586form_80223
#define DLis_us		imx586is_us
#define DLis_broadcast	imx586is_broadcast
#define DLis_validsnap	imx586is_validsnap
#define DLmk_test_con	imx586mk_test_con
#define DLinsert_sap	imx586insert_sap
#define DLsubsbind_req	imx586subsbind_req
#define DLtest_req	imx586test_req
#define DLremove_sap	imx586remove_sap
#define DLis_equalsnap	imx586is_equalsnap
#define DLform_snap	imx586form_snap

#define DLbdspecioctl	imx586bdspecioctl
#define DLbdspecclose	imx586bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	imx586boards
#define DLconfig	imx586config
#define DLsaps		imx586saps
#define DLstrlog	imx586strlog
#define DLifstats	imx586ifstats
#define	DLinetstats	imx586inetstats
#define	DLid_string	imx586id_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header include 5 additional bytes in addition to the
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

#endif	/* _IO_DLPI_ETHER_DLPI_IMX586_H */
