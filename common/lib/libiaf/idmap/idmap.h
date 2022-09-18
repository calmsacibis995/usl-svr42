/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/idmap/idmap.h	1.2.1.2"
#ident  "$Header: idmap.h 1.2 91/06/25 $"

#define	MODULE_ID	"UX:idmap"
#define	MAPDIR		"/etc/idmap"
#define	ATTRMAP		"attrmap"
#define	LOGFILE		"/var/adm/log/idmap.log"
#define	OLDLOGFILE	"/var/adm/log/idmap.log.old"
#define	IDATA		"idata"
#define	UIDATA		"uidata"
#define	DOTMAP		".map"
#define	DOTTMP		".tmp"
#define	IDATA_MODE	((mode_t) 0660)
#define	ATTR_MODE	IDATA_MODE
#define	UIDATA_MODE	IDATA_MODE
#define	SECURE_MODE	((mode_t) 0000)
#define	LOGFILE_MODE	((mode_t) 0660)
#define	DIR_MODE	((mode_t) 0770)
#define	IDMAP_LOGIN	"sys"
#define	IDMAP_GROUP	"sys"

#define	MAXLOGLEN	100 * 512
#define	MAXFILE		64
#define	MAXLINE		512
#define	MAXFIELDS	10	/* this cannot be more than 10 */

/* name field information structure */
typedef struct {
	char type;
	char *value;
} FIELD;
