/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/devattr.c	1.2.10.2"
#ident	"$Header: devattr.c 1.3 91/06/25 $"

/*LINTLIBRARY*/

/*
 *  devattr.c
 *
 *  Contents:
 *	devattr()	Get the value of a attribute for a specific device
 */

/*
 *  Header files needed
 *	<sys/types.h>		System Data Types
 *	<stdio.h>		Standard I/O Definitions
 *	<errno.h>		Error-value definitions
 *	<string.h>		String function and constant definitions
 *	<stdlib.h>		Storage allocation functions
 *	<devmgmt.h>		Device table definitions available to the world
 *	"devtab.h"		Local device table definitions
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
#include	<devmgmt.h>
#include	"devtab.h"


/*
 *  Externals referenced
 */

extern	int		get_devrec();
extern	char		*getattrval();
/*
 *  Local constant definitions
 */


/*
 *  Local static data
 *	devrec		Last dev_record read from Device Database.
 *			This contains all attributes defined for device.
 */
static dev_record	Devrec;


/*
 *  char *devattr(device, attr)
 *
 *	This function searches the DDB_TAB file, looking for the device
 *	specified by <device>.  If it finds a record corresponding to that
 *	device (see below for a definition of that correspondence), it
 *	extracts the value of the field <attr> from that record, if any.
 *	It returns a pointer to that value, or (char *) NULL if none.
 *
 *	This function retains the device attrs in-core, in the static 
 *	structure- <Devrec>, so that it does not have to access disk if
 *	the same device is quiried again. However, if some other device
 *	is queried in the subsequent call to devattr(), it will access
 *	the DDB files on disk to get that device's attribute-values.
 *
 *  Arguments:
 *	device		Pointer to the character-string that describes the
 *			device whose record is to be looked for
 *	attr		The device's attribute to be looked for
 *
 *  Returns:  char *
 *	A pointer to the character-string containing the value of the
 *	attribute <attr> for the device <device>, or (char *) NULL if none
 *	was found.  If the function returns (char *) NULL and the error was
 *	detected by this function, it sets "errno" to indicate the problem.
 *
 *  "errno" Values:
 *	EACCES		Permissions deny reading access of the Device Database
 *			files
 *	ENOENT		The specified Device Database files could not be found
 *	ENODEV		Device not found in the Device Database
 *	EINVAL		The device does not have that attribute defined
 *	ENOMEM		No memory available
 */

char *
devattr(device, attribute)
	char   *device;		/* The device ) we're to look for */
	char   *attribute;	/* The attribute to extract */
{
	static int		first=TRUE;	/* flag=TRUE; first time */

	/* Automatic data */
	char	 		*val;		/* Value of attribute */
	char	 		*rtnval;	/* Value to return */


	/* clear any previous errors */
	ddb_errset(0);

	if (__tabversion__ == 0) {

		__tabversion__ = gettabversion();

		if (__tabversion__ == FAILURE) {
			return((char *)NULL);
		}
	}
	/* function invoked first time? */
	if (first) {
	    /* get all the attrs for the specified device *
	     * from the Device Database files.            */
	    if (get_devrec(device, &Devrec) == FAILURE) {
		/* set errno to appropriate error code */
		errno = ddb_errget();
		return((char *) NULL);
	    }
	    first = FALSE;
	} else {
	    /* Is Devrec buffer full ? */
	    if (Devrec.tab) {
		/* Yes, then does it contain attrs of SAME device? */
		if (strcmp(Devrec.tab->alias,device)!=0) {
		    /* NO, different device, free memory alloc'd for Devrec */
		    free_devrec(&Devrec);
		    /* Get the record for the new device */
		    if (get_devrec(device, &Devrec) == FAILURE) {
			/* set errno to appropriate error code */
			errno = ddb_errget();
			return((char *) NULL);
		    }
		}
	    } else {
		/* Devrec empty, get record for new device */
		if (get_devrec(device, &Devrec) == FAILURE) {
		    /* set errno to appropriate error code */
		    errno = ddb_errget();
		    return((char *) NULL);
		}
	    }
	}

	/* extract attr value from dev_record of specified <device> */
	if (val=getattrval(attribute, &Devrec)) {
	    if (rtnval = (char *)malloc(strlen(val)+1))
		strcpy(rtnval, val);
	    else errno = ENOMEM;
	} else {
	    rtnval = (char *)NULL;
	    errno = EINVAL;
	}

	/* Fini */
	return(rtnval);
}
