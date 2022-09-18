/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_DLPI_WD_H	/* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_WD_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/dlpi_wd.h	1.3"
#ident  "$Header: $"

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

/*
 *  STREAMS structures
 */
#define	DL_NAME			"wd"
#define	DLdevflag		wddevflag
#define	DLrminfo		wdrminfo
#define	DLwminfo		wdwminfo
#define	DLrinit			wdrinit
#define	DLwinit			wdwinit

/*
 *  Functions
 */
#define DLopen			wdopen
#define	DLclose			wdclose
#define DLrput			wdrput
#define	DLwput			wdwput
#define	DLioctl			wdioctl
#define	DLinfo			wdinfo
#define	DLloopback		wdloopback
#define	DLmk_ud_ind		wdmk_ud_ind
#define	DLxmit_packet	wdxmit_packet
#define	DLinfo_req		wdinfo_req
#define	DLcmds			wdcmds
#define	DLprint_eaddr	wdprint_eaddr
#define	DLbind_req		wdbind_req
#define	DLrsrv			wdrsrv
#define	DLunbind_req	wdunbind_req
#define	DLunitdata_req	wdunitdata_req
#define	DLerror_ack		wderror_ack
#define	DLuderror_ind	wduderror_ind
#define	DLpromisc_off	wdpromisc_off
#define	DLpromisc_on	wdpromisc_on
#define	DLset_eaddr		wdset_eaddr
#define	DLadd_multicast	wdadd_multicast
#define	DLdel_multicast	wddel_multicast
#define DLget_multicast wdget_multicast
#define	DLdisable		wddisable
#define	DLenable		wdenable
#define	DLreset			wdreset
#define	DLis_multicast	wdis_multicast
#define	DLrecv 		wdrecv
#define	DLproc_llc	wdproc_llc
#define	DLform_80223	wdproc_80223
#define DLis_us		wdis_us
#define DLis_broadcast	wdis_broadcast
#define DLis_validsnap	wdis_validsnap
#define DLmk_test_con	wdmk_test_con
#define DLinsert_sap	wdinsert_sap
#define DLsubsbind_req	wdsubsbind_req
#define DLtest_req	wdtest_req
#define DLremove_sap	wdremove_sap
#define DLis_equalsnap	wdis_equalsnap
#define DLform_snap	wdform_snap
#define	DLmk_snap_ind	wdmk_snap_ind

#define DLbdspecioctl	wdbdspecioctl
#define DLbdspecclose	wdbdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	wdboards
#define DLconfig	wdconfig
#define DLsaps		wdsaps
#define DLstrlog	wdstrlog
#define DLifstats	wdifstats
#define	DLinetstats	wdinetstats
#define	DLid_string	wdid_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header include 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC	(DL_MAX_PACKET - 3)
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		24576	/* 32 x 3 x 256 */
#define	DL_LOWATER		6144	/* 8 x 3 x 256 */

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		60	/* 64 bytes - 4 byte CRC */

#endif	/* _IO_DLPI_ETHER_DLPI_WD_H */
