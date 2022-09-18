/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamhdrs:common/head/devmgmt.h	1.12.9.2"
#ident  "$Header: devmgmt.h 1.4 91/06/21 $"

#ifndef _DEVMGMT_H
#define _DEVMGMT_H

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
/*
 * devmgmt.h
 *
 * Contents:
 *    -	Device Management definitions,
 *    -	getvol() definitions
 */

/*
 * Device management definitions
 *	- Default pathnames (relative to installation point)
 * 	- Environment variable namess
 * 	- Standard field names in the device table
 *	- Device attribute names & valid values in Device Database
 *	- Flags
 *	- Miscellaneous definitions
 */

/*
 * Default pathnames (relative to the package installation
 * point) to the files used by Device Management:
 *
 *	DTAB_PATH	Device table
 *	DGRP_PATH	Device group table
 *	DVLK_PATH	Device reservation table
 */

/*
 * Device Database files (DDB files):
 *      - DDB_TAB = DTAB_PATH   OAM attributes of devices
 *      - DDB_DSFMAP            DSF mapping to devices
 *      - DDB_SEC               Security attributes of devices 
 */

#define DTAB_PATH       "/etc/device.tab"
#define DDB_TAB         DTAB_PATH
#define	DGRP_PATH			"/etc/dgroup.tab"
#define	DVLK_PATH			"/etc/devlkfile"
#define DDB_DSFMAP      "/etc/security/ddb/ddb_dsfmap"
#define DDB_SEC         "/etc/security/ddb/ddb_sec"

/*
 * Old (saved copy) of Device Database files 
 *      - ODDB_TAB 		OAM attributes of devices
 *      - ODDB_DSFMAP            DSF mapping to devices
 *      - ODDB_SEC               Security attributes of devices 
 */

#define ODDB_TAB	 "/etc/Odevice.tab"
#define ODDB_DSFMAP      "/etc/security/ddb/Oddb_dsfmap"
#define ODDB_SEC         "/etc/security/ddb/Oddb_sec"

/*
 * Other Device Management files:
 *      - DGRP_PATH		Device group table
 *      - DVLK_PATH		Device reservation table
 */

#define	DGRP_PATH	"/etc/dgroup.tab"
#define	DVLK_PATH	"/etc/devlkfile"

/*
 * Names of environment variables
 *
 *	OAM_DEVTAB	Name of variable that defines the pathname to
 *			the device-table file
 *	OAM_DGROUP	Name of variable that defines the pathname to
 *			the device-group table file
 *	OAM_DEVLKTAB	Name of variable that defines the pathname to
 *			the device-reservation table file
 */

#define	OAM_DEVTAB			"OAM_DEVTAB"
#define	OAM_DGROUP			"OAM_DGROUP"
#define	OAM_DEVLKTAB			"OAM_DEVLKTAB"

/*
 * Standard device attribute names in Device Database(DDB)
 */
/* Security attributes of devices */
#define DDB_ALIAS		"alias"
#define DDB_RANGE		"range"
#define DDB_STATE		"state"
#define DDB_MODE		"mode"
#define DDB_STARTUP		"startup"
#define DDB_ST_LEVEL		"startup_level"
#define DDB_ST_OWNER		"startup_owner"
#define DDB_ST_GROUP		"startup_group"
#define DDB_ST_OTHER		"startup_other"
#define DDB_UAL_ENABLE		"ual_enable"
#define DDB_USERS		"users"
#define DDB_OTHER		"other"

/* valid values for security attrs */
/* STATE */
#define DDB_PUBLIC		"public"
#define DDB_PRIVATE		"private"
#define DDB_PUB_PRIV		"pub_priv"

/* MODE  */
#define DDB_STATIC		"static"
#define DDB_DYNAMIC		"dynamic"

/* device special file attributes of devices */
#define DDB_CDEVICE	        "cdevice"
#define DDB_BDEVICE	        "bdevice"
#define DDB_CDEVLIST		"cdevlist"
#define DDB_BDEVLIST		"bdevlist"

/* OA&M attributes of devices */
#define DTAB_ALIAS		DDB_ALIAS
#define DDB_SECDEV		"secdev"
#define DDB_PATHNAME		"pathname"
/* any other OA&M attrs defined here */

/*
 * Standard field names in the device table
 */

#define	DTAB_CDEVICE			"cdevice"
#define	DTAB_BDEVICE			"bdevice"
#define	DTAB_PATHNAME			"pathname"

/*
 * Device types: defines the type of device being allocated.
 *	DEV_DSF		specified device special file(dsf) allocated
 *	DEV_ALIAS	all dsfs mapped to logical device alias allocated
 *	DEV_SECDEV	all dsfs mapped to secure device alias allocated
 */

#define DEV_DSF		1
#define DEV_ALIAS	2
#define DEV_SECDEV	3

/*
 * dev_daca struct defines the DAC attributes of device(device special file)
 */
struct dev_daca {
	uid_t		uid;		/* owner of dsf */
	gid_t		gid;		/* group of dsf */
	mode_t		mode;		/* DAC permissions */
};

/* 
 * Flags:
 *	For getdev() and getdgrp():
 *		DTAB_ANDCRITERIA	Devices must meet all criteria
 *					instead of any of the criteria
 *		DTAB_EXCLUDEFLAG	The list of devices or device groups
 *					is the list that is to be excluded,
 *					not those to select from.
 *		DTAB_LISTALL		List all device groups, even those that
 *					have no valid members (getdgrp() only).
 */

#define	DTAB_ANDCRITERIA		0x01
#define	DTAB_EXCLUDEFLAG		0x02
#define	DTAB_LISTALL			0x04

/*
 * Miscellaneous Definitions
 *
 *	DTAB_MXALIASLN	Maximum alias length
 *	DDB_MAXALIAS	Maximum alias length
 *	DDB_MAGICLEN	length of magic number in DDB files
 */

#define DDB_MAXALIAS		64 + 1 /* +1 for '\0' to end the string  */
#define DDB_MAGICLEN			35
#define MAGICTOK			"MAGIC%NO"
#define DTAB_MXALIASLN			DDB_MAXALIAS

/*
 * Device Management Structure definitions
 *	reservdev	Reserved device description
 */

/*
 * struct reservdev
 *
 *	Structure describes a reserved device.
 */

struct reservdev{
	char   *devname;	/* Alias of the reserved device */
	pid_t	key;		/* Key used to reserve the device */
};

/*
 * Device Management Functions:
 *
 *	devattr()	Returns a device's attribute
 *	devreserv()	Reserves a device
 *	devfree()	Frees a reserved device
 *	reservdev()	Return list of reserved devices
 *	getdev()	Get devices that match criteria
 *	getdgrp()	Get device-groups containing devices 
 *			that match criteria
 *	listdev()	List attributes defined for a device
 *	listdgrp()	List members of a device-group
 */

	char		       *devattr();
	int			devfree();
	char		      **devreserv();
	char		      **getdev();
	char		      **getdgrp();
	char		      **listdev();
	char		      **listdgrp();
	struct reservdev      **reservdev();
/*
 * getvol() definitions
 */

#define	DM_BATCH	0x0001
#define DM_ELABEL	0x0002
#define DM_FORMAT	0x0004
#define DM_FORMFS	0x0008
#define DM_WLABEL	0x0010
#define DM_OLABEL	0x0020

int			getvol();	
#endif	/* _DEVMGMT_H */
