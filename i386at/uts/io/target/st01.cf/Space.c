/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/st01.cf/Space.c	1.3"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include "config.h"


struct dev_spec *st01_dev_spec[] = {
        0
};

struct dev_cfg ST01_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, 0xffff, 0xff, 0xff, ID_TAPE, 0, ""   },
};

int ST01_dev_cfg_size = sizeof(ST01_dev_cfg)/sizeof(struct dev_cfg);

int	St01_cmajor = ST01_CMAJOR_0;	/* Character major number	*/

int	St01_jobs = 20;		/* Allocation per LU device	*/

int	St01_reserve = 1;	/* Flag for reserving tape on open */
