/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/sdi.cf/Space.c	1.14"
#ident	"$Header: $"

/*
 * sdi/space.c
 */

#include "config.h"
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/vtoc.h>	/* needed for dcd.h */
#include <sys/dcd.h>
#include <sys/wd7000.h>
#include <sys/aha.h>
#include <sys/dpt_sdi.h>
#include <sys/mcis.h>

/* XXX for now */
#define	SDI_RTABSZ	8
#define	SDI_HBASWSZ	8

int sdi_major = SDI__CMAJOR_0;

struct sdi_edt edtpool[NSDIDEV];	/* pool of edt entries */
struct sdi_edt edt_hash[EDT_HASH_LEN];	/* edt hash table */
struct owner	owner_pool[NOWNER];	/* owner pool */

long   sdi_started = 0;

struct hba_cfg HBA_tbl[SDI_HBASWSZ];
int	sdi_hbaswsz = SDI_HBASWSZ;

void	(*sdi_rinits[SDI_RTABSZ])();
int	sdi_rtabsz = SDI_RTABSZ;

/*
 * Number of luns to INQUIRY for ID_TAPE devices.
 *	WARNING: a 0 setting causes NO tapes to be claimed.
 *	1-8 number of luns to INQUIRY, starting with lun 0.
 *	1 (default)
 */
int sdi_tape_luns = 1;
