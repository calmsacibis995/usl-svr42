/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386at/ktool/idtools/devconf.h	1.4"
#ident	"$Header:"

/* Device-driver configuration information for ID tools. */


typedef struct per_driver driver_t;
typedef struct per_ctlr ctlr_t;
typedef struct per_vector ivec_t;


/* Per-driver table;  corresponds one-to-one to mdevice */
extern struct per_driver {
	struct mdev	mdev;		/* config info from mdevice */
	short		n_ctlr;		/* number of controllers configured */
	short		intr_decl;	/* an intr routine has been declared */
	ctlr_t		*ctlrs;		/* list of controllers configured */
	short		tot_units;	/* total units for all controllers */
	unsigned short	vars;	/* "exported" variables present in the driver */
	driver_t	*bdev_link;	/* link for block device list */
	driver_t	*cdev_link;	/* link for character device list */
	driver_t	*next;		/* next driver in driver_info list */
					/* sorted by mdev.order */
	short		loadable;	/* dynamic loadable module or not */
} *driver_info;


/* Block and character device lists */
extern driver_t *bdevices, *cdevices;


/* Per-controller table; corresponds one-to-one to configured sdevice lines */
extern struct per_ctlr {
	struct sdev	sdev;		/* config info from sdevice */
	driver_t	*driver;	/* link to this ctlr's driver */
	short		num;		/* ctlr # per driver (0 to n_ctlr-1) */
	ctlr_t		*drv_link;	/* link to other ctlrs w/same driver */
	ctlr_t		*vec_link;	/* link to other ctlrs sharing the
						same interrupt vector */
	ctlr_t		*next;		/* next ctlr in ctlr_info list */
} *ctlr_info;


/* Per-vector table */
extern struct per_vector {
	short		itype;		/* interrupt type for this vector */
	short		ipl;		/* interrupt priority level */
	ctlr_t		*ctlrs;		/* list of devices using this vector */
	short		used;		/* number of static configured modules
					   using this vector */
} ivec_info[256];


int getdevconf();
driver_t *mfind();
ctlr_t *sfind();
