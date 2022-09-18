/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/wd7000.cf/Space.c	1.4"
#ident	"$Header: $"


#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/immu.h>
#include <sys/had.h>

#include "config.h"

struct scsi_edt sc_edt[MAX_TCS * WD__CNTLS];
union cdb_item wd_cdb_pool[MAX_LUS * MAX_TCS * WD__CNTLS];
int wd_cdbsz = (MAX_LUS * MAX_TCS * WD__CNTLS);
long wd_hacnt = 0;

struct wd_addr wd_ad[WD__CNTLS];

int wd_dbgsize = 11;
char wd_Debug[11] = { 0, 0,0,0,0,0,0,0,0,0,0};

char wd_Board[11] = { 1, 1,1,1,0,0,0,0,0,0,0};

struct	hba_idata	wd_idata[WD__CNTLS]	= {
#ifdef	WD__0
	{ 1, "(wd7000,1st) WD7000 SCSI HBA",
	  7, WD__0_SIOA, WD__0_CHAN, WD__0_VECT, WD__0, 0 }
#endif
#ifdef	WD__1
	,
	{ 1, "(wd7000,2nd) WD7000 SCSI HBA",
	  7, WD__1_SIOA, WD__1_CHAN, WD__1_VECT, WD__1, 0 }
#endif
#ifdef	WD__2
	,
	{ 1, "(wd7000,3rd) WD7000 SCSI HBA",
	  7, WD__2_SIOA, WD__2_CHAN, WD__2_VECT, WD__2, 0 }
#endif
#ifdef	WD__3
	,
	{ 1, "(wd7000,4th) WD7000 SCSI HBA",
	  7, WD__3_SIOA, WD__3_CHAN, WD__3_VECT, WD__3, 0 }
#endif
};

int	wd__cntls	= WD__CNTLS;
