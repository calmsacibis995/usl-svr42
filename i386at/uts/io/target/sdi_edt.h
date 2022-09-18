/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_SDI_EDT_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_EDT_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/target/sdi_edt.h	1.3"
#ident	"$Header: $"

/* 		SDI Equipped Device Table information		*/

/* driver ioctl commands */

#define BIOC		('B'<<8)	/* For BUS ioctl() commands 	*/
#define	B_GETTYPE	(BIOC|1)	/* Get bus and driver name 	*/
#define	B_GETDEV	(BIOC|2)	/* Get device for pass through 	*/
#define B_REDT		(BIOC|4)	/* Read Extended EDT 		*/
#define B_HA_CNT	(BIOC|5)	/* get # of HA boards configured*/
#define B_GET_SUBDEVS	(BIOC|6)	/* Get # of subdevices per LU	*/

#define	SDI_DEV(x)	((((x)->ha_slot) << 3) | ((x)->tc_id))

#define HAMINOR(c,t,l)	((c << 5) | (t << 2) | l)

#define SCSI_SUBDEVS	16	/* Number of subdevices per minor 	*/

#define MAX_HAS		8		/* The max HA's in a system.	*/


#define	NAME_LEN	10
#define INQ_LEN		25		/* inquiry data length		*/
#define MAX_TCS		8
#define MAX_LUS		8

struct bus_type {
	char	bus_name[NAME_LEN];	/* Name of the driver's bus */
	char	drv_name[NAME_LEN];	/* Driver prefix */
};


/* This structure is stored in the space file by target	*/
/* drivers  on a per TC type which that driver supports.*/

struct tc_data
{
	unsigned char	tc_inquiry[INQ_LEN];	/* TC inquiry data	*/
	unsigned char	max_lus;	/* max LUs supported by TC	*/
};


/* This structure is used by target drivers to determine	*/
/* how many controllers are configured in the system		*/

struct tc_edt
{
	unsigned char	ha_slot;
	unsigned char	tc_id;
	unsigned char	n_lus;
	unsigned char	lu_id[MAX_LUS];
};


/* These defines are used to extract the HA occurence and the type (single ended
 * or differential) out of the "ha_slot" of the edt structure.
 */

/* SCSI Version Flags */
#define SINGLE_ENDED    1
#define DIFFERENTIAL    2

#define BUS_TYPE(x)	((((x)>>7) & 0x1) ? DIFFERENTIAL : SINGLE_ENDED)
#define BUS_OCCUR(x)	((x) & 0x7)

struct scsi_edt		/* SCSI bus equipped device table. One per HA */
{
	short	c_maj;			/* Target drv. character major number */
	short	b_maj;			/* Target drv. block major number     */
	unsigned char	pdtype;		/* Target controller SCSI device type */
	unsigned char	tc_equip;	/* one if TC is equipped	      */
	unsigned char	ha_slot;	/* Host Adaptor controller slot number*/
	unsigned char	n_lus;		/* number of equipped LUS on TC	      */
	unsigned char	lu_id[MAX_LUS];	/* one if LU is equipped      */
	char		drv_name[NAME_LEN];	/* target driver ASCII name   */
	unsigned char	tc_inquiry[INQ_LEN];	/* TC vendor and product name */
};

struct drv_majors {
	major_t b_maj;		/* Block major number		*/
	major_t c_maj;		/* Character major number	*/
};

#endif /* _IO_TARGET_SDI_EDT_H */
