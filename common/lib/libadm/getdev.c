/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/getdev.c	1.4.9.2"
#ident  "$Header: getdev.c 1.2 91/06/25 $"
/*LINTLIBRARY*/

/*
 *  getdev.c
 *
 *  Contents:
 *	getdev()	List devices that match certain criteria.
 */
/*
 * Header files referenced:
 *	<sys/types.h>	System Data Types
 *	<stdio.h>	Standard I/O Definitions
 *	<errno.h>	Error handling
 *	<stdlib.h>	Storage allocation functions
 *	<fcntl.h>	File controlling
 *	<ctype.h>	Character types
 *	<string.h>	String handling
 *	<devmgmt.h>	Global device-management def'ns
 *	"devtab.h"	Local device-management dev'ns
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<string.h>
#include	<devmgmt.h>
#include	"devtab.h"

/*
 * Local definitions
 *	NULL		Nil address
 *	TRUE		Boolean TRUE
 *	FALSE		Boolean FALSE
 */

#ifndef	NULL
#define	NULL			0
#endif

#ifndef	TRUE
#define	TRUE			('t')
#endif

#ifndef	FALSE
#define	FALSE			0
#endif


/*
 *  Comparison values.  These values are placed in the struct srch
 *  structure by buildsearchlist() and are used to compare values
 *  in matches().
 *	EQUAL		Attribute must equal this value
 *	NOTEQUAL	Attribute must not equal this value
 *	EXISTS		Attribute must exist
 *	NOEXISTS	Attribute must not exist
 *	IGNORE		Ignore this entry
 *	ENDLIST		This entry ends the list
 */

#define	EQUAL			1
#define	NOTEQUAL		2
#define	EXISTS			3
#define	NOEXISTS		4
#define	IGNORE			5
#define	ENDLIST			0


#define LOGNAMEMAX		37 	/* 32 + > + rwx + \n */

/*
 *  Structure definitions:
 * 	deviceent	Defines a device that matches criteria
 *	srch		Describes a criteria
 */

struct deviceent {
	struct deviceent       *next;	/* Pointer to next item in the list */
	char		       *name;	/* Presentation name of the device */
};

struct srch {
	char   *name;			/* Name of field to compare */
	char   *cmp;			/* Value to compare against */
	int	fcn;			/* Type of comparison (see above) */
};
/*
 * Local functions referenced
 *	oktoaddtolist()		Determines if device can be added to the
 *				list by examining the devices list and
 *				the options governing the search
 *	initdevicelist()	Initializes the linked list of devices
 *				to be included in the list-to-return
 *	freedevicelist()	Frees the resources allocated to the linked
 *				list of devices
 *	addtodevicelist()	Adds an entry to the linked list of devices
 *
 *	buildsearchlist()	Builds a list of struct srch structures from
 *				the criteria strings
 *	freesearchlist()	Frees the resources allocated to the list of
 *				struct srch structures
 *	buildreturnlist()	Builds the list of devices to return from the
 *				linked list of devices we've accumulated
 *	makealiaslist()		Builds a list of aliases from the list of
 *				devices presented by the caller
 *	freealiaslist()		Frees the resources allocated to the list of
 *				devices aliases
 *	getnextmatch()		Get the next device that matches the search
 *				criteria
 *	matchallcriteria()	See if the device attributes match all of the
 *				search criteria
 *	matchanycriteria()	See if the device attributes match any of the
 *				search criteria
 *	matches()		See if the criteria and attribute match
 *
 *  	secattrmatches()	Checks criteria that involves secure attributes
 *
 */

static	char	       *oktoaddtolist();
static	void		initdevicelist();
static	void		freedevicelist();
static	int		addtodevicelist();
static	struct srch    *buildsearchlist();
static	void 		freesearchlist();
static	char	      **buildreturnlist();
static	char	      **makealiaslist();
static	void		freealiaslist();
static	char	       *getnextmatch();
static	int		matchallcriteria();
static	int		matchanycriteria();
static	int		matches();
static	int		secattrmatches();


/*
 * Global Data
 */

/*
 * Static Data
 *	devicelisthead	The first item (dummy) in the linked list of devices
 *			we're building
 *	devicelist	Structure describing the linked list of devices
 */

static	struct deviceent	devicelisthead;
static	struct {
	struct deviceent       *head;
	int			count;
} devicelist = {&devicelisthead, 0};

/*
 *  char **getdev(devices, criteria, options)
 *	char  **devices
 *	char  **criteria
 *	int	options
 *
 *	This function builds a list of devices that match criteria,
 *	governed by the device list.
 *
 *  Arguments:
 *	devices		The list of devices to select from or the list of
 *			devices to exclude, depending on the value of
 *			"options"
 *	criteria	The list of criteria governing the device selection
 *			Of the form <attr><op><val>
 *	options		Options controlling the device selection.  May require
 *			that a device meet all of the criteria (default is
 *			any one of the criteria), or may require that the
 *			devices in the list of devices be excluded from the
 *			generated list (default is to select only those
 * 			devices in the list)
 *
 *  Returns:  char **
 *	The address of the first item in the list of devices that meet
 *	the selection criteria
 */

char  **
getdev(devices, criteria, options)
	char  **devices;		/* List of devices to constrain */
	char  **criteria;		/* List of selection criteria */
	int	options;		/* Options governing the search */
{
	/* Automatic data */
	FILE		*fp;		/* DDB_TAB file pointer         */
	char	      **aliases;	/* List of constraining devices */
	char	      **returnlist;	/* List of ptrs to aliases to return */
	struct srch    *searchlist;	/* Pointer to searching criteria */
	char	       *entry;		/* Pointer to alias in record */
	int		errflag;	/* FLAG:  TRUE if error */
	char		buf[80];	/* Stores DDB_TAB's first line. This 
					 * value is ignored */

	/*
	 *  Initializations
	 */

	/*  Make sure the exclude/include list is all aliases */
	aliases = makealiaslist(devices);
	if (devices && !aliases) return((char **) NULL);

	/*  Build the search list  */
	if (criteria) {
	    if (!(searchlist = buildsearchlist(criteria, options)))
		return((char **) NULL);
	} else searchlist = (struct srch *) NULL;

	/*  Initialize searching  */
	initdevicelist();

	/*
	 * Open DDB_TAB to get each device(alias) defined in the
	 * Device Database, and all its attributes
	 */
	if ((fp=fopen(DDB_TAB, "r"))==(FILE *)NULL) {
	    return((char **)NULL);
	}

	/* skip magic number */
	if (fgets(buf, 80, fp) == (char *)NULL) {
	    return((char **)NULL);
	}

	/*
	 *  Keep on going until we get no more matches
	 */
	errflag = FALSE;
	while (!errflag && (entry = getnextmatch(fp, searchlist, options))) {
	    if (entry = oktoaddtolist(entry, devices, aliases, options)) {
		errflag = addtodevicelist(entry);
	    }
	}


	/*
	 *  Clean up:
	 *    -	Free the entry space we've allocated.
	 *    -	Close the device table.
	 *    - Build the list to return to the caller.
	 *    - Free the accumulate device space (but not the strings!)
	 *    - Free the alias list
	 *    - Return the built list to the caller.
	 */

	returnlist = buildreturnlist();
	freedevicelist();
	freealiaslist(aliases);
	fclose(fp);
	return(returnlist);
}

/*
 *  char *oktoaddtolist(devalias, devices, aliases, options)
 *	char   *devalias
 *	char  **devices
 *	char  **aliases
 *	int	options
 *
 *	This function determines the device "devalias" can be
 *	added to the list of devices we're accumulating.  If so,
 *	it returns the device name (not the alias).
 *
 *  Arguments:
 *	devalias	The device alias that may or may not belong in the
 *			list we're building.
 *	devices		The devices specified by the caller
 *	aliases		The aliases of the devices specified by the caller
 *			(1-1 correspondence with "devices")
 *	options		Options controlling the search
 */

static	char *
oktoaddtolist(devalias, devices, aliases, options)
	char   *devalias;	/* Alias to check against list */
	char  **devices;	/* List of devices to check against */
	char  **aliases;	/* List of alias of those devices */
	int	options;	/* Options governing search */
{
	/* Automatic data */
	char   *rtnval;		/* Value to return */
	int	found;		/* Flag:  TRUE if found */

	/* If there's a constraint list, is this device in it? */
	if (devices && aliases) {

	    /* Set "found" to TRUE if the device is in the list */
	    found = FALSE;
	    while (!found && *aliases) {
		if (strcmp(devalias, *aliases) == 0) found = TRUE;
		else {
		    devices++;
		    aliases++;
		}
	    }

	    /* Set value to return */
	    if (found) 
		rtnval = (options&DTAB_EXCLUDEFLAG) ? (char *) NULL : *devices;
	    else
		rtnval = (options&DTAB_EXCLUDEFLAG) ? devalias : (char *) NULL;

	} else 
		rtnval = devalias;  /* No constraint list */

	return(rtnval);
}

/*
 *  void initdevicelist()
 *
 *	This function initializes the list of accumulated devices.
 *
 *  Arguments:  None
 *
 *  Returns:  Void.
 *
 *  Notes:
 */

static	void
initdevicelist()
{
	/* Make the list a null list */
	(devicelist.head)->next = (struct deviceent *) NULL;
	devicelist.count = 0;
}

/*
 *  void freedevicelist()
 *
 *	This function frees the resources allocated to the linked list of
 *	devices we've been accumulating.
 *
 *  Arguments:  none
 *
 *  Returns:  void
 */

static	void
freedevicelist()
{
	/* Automatic data */
	struct deviceent       *pdevice;	/* Pointer to current entry */
	char		       *freeblk;	/* Pointer space to free */

	/* List has a dummy head node */
	pdevice = (devicelist.head)->next;
	while (pdevice) {
	    freeblk = (char *) pdevice;
	    pdevice = pdevice->next;
	    free(freeblk);
	}
}

/*
 *  int addtodevicelist(deventry)
 *	char   *deventry
 *
 * 	This function adds the device <deventry> to the list of devices already
 *	accumulated.  It will not add the device if that device already exists
 *	in the list.  The function returns 0 if successful, -1 if not with
 *	"errno" set (by functions called) to indicate the error.
 *
 *  Arguments:
 *	deventry		char *
 *				The name of the device to add to the list of
 *				accumulated devices
 *
 *  Returns:
 *	0	If successful
 *	-1	If failed.  "errno" will be set to a value that indicates the
 *		error.
 *
 *  Notes:
 *    -	The memory allocation scheme has the potential to fragment the memory
 *	in the malloc heap.  We're allocating space for a local structure,
 *	which will be freed by getdev(), then allocating space for the device
 *	name, which will be freed (maybe) by the application using getdev().
 *	Not worrying about this at the moment.
 */

static	int
addtodevicelist(deventry)
	char   *deventry;
{
	/* Automatic data */
	struct deviceent       *p;	/* Pointer to current device */
	struct deviceent       *q;	/* Pointer to next device */
	struct deviceent       *new;	/* Pointer to the alloc'd new node */
	char		       *str;	/* Pointer to alloc'd space for name */
	int			rtncd;	/* Value to return to the caller */
	int			cmpcd;	/* strcmp() value, comparing names */
	int			done;	/* Loop control, TRUE if done */


	/* Initializations */
	rtncd = FALSE;


	/*
	 * Find the place in the found device list devicelist where this
	 * device is to reside
	 */

	p = devicelist.head;
	done = FALSE;
	while (!done) {
	    if (!(q = p->next)) done = TRUE;
	    else if ((cmpcd = strcmp(deventry, q->name)) <= 0) done = TRUE;
	    else p = q;
	}

	/*
	 *  If the device is not already in the list, insert it in the list
	 */

	if (!q || (cmpcd != 0)) {

	    /* Alloc space for the new node */
	    if (new = (struct deviceent *) malloc(sizeof(struct deviceent))) {

		/* Alloc space for the device character string */
		if (str = (char *) malloc((unsigned) strlen(deventry)+1)) {

		    /*
		     * Insert an entry in the found device list containing
		     * this device name
		     */
		    new->next = q;
		    p->next = new;
		    new->name = strcpy(str, deventry);
		    devicelist.count++;
		}

		/* Couldn't alloc space for the device name.  Error. */
		else rtncd = TRUE;
	    }

	    /* Couldn't alloc space for new node in the found list.  Error. */
	    else rtncd = TRUE;

	}

	/* Return an value indicating success or failure */
	return(rtncd);
}

/*
 *  struct srch *buildsearchlist(criteria)
 *	char  **criteria
 *
 *	This function builds a list of search criteria structures from the
 *	criteria strings in the list of criteria whose first argument is
 *	specified by "criteria".
 *
 *  Arguments:
 *	criteria	The address of the first item in a list of
 *			character-strings specifying search criteria
 *
 *  Returns: struct srch *
 *	The address of the structure in the list of structures describing the
 *	search criteria.
 *
 *  Notes:
 *    -	The <options> argument isn't currently being used.  It is still
 *	among the arguments to this function for historical (hysterical?)
 *	reasons.  Since someday the options may affect this function,
 *	the argument is being kept around.
 *    -	The only "regular expression" currently supported by the
 *	kywd:exp and kywd!:exp forms is exp=*.  This function assumes
 *	that kywd:exp means "if kywd exist" and that kywd!:exp means
 *	"if kywd doesn't exist".
 */

static 	struct srch *
buildsearchlist(criteria)
	char	      **criteria;	/* Criteria from caller */
{
	/*  Automatic data  */
	struct srch    *rtnbuf;		/* Value to return */
	struct srch    *psrch;		/* Running pointer */
	char	       *str;		/* Ptr to malloc()ed string space */
	char	       *p;		/* Temp pointer to char */
	int		noerror;	/* TRUE if all's well */
	int		n;		/* Temp counter */
	char	      **pp;		/* Running ptr to (char *) */


	/*  Initializations  */
	rtnbuf = (struct srch *) NULL;		/* Nothing to return yet */
	noerror = TRUE;				/* No errors (yet) */

	/* If we were given any criteria ... */
	if (criteria) {

	    /* Count the number of criteria in the list */
	    for (n = 1, pp = criteria ; *pp++ ; n++) ;

	    /* Allocate space for structures describing the criteria */
	    if (rtnbuf = (struct srch *) malloc(n*sizeof(struct srch))) {

		/* Build structures describing the criteria */
		pp = criteria;
		psrch = rtnbuf;
		while (noerror && *pp) {

		    /* Keep list sane for cleanup if necessary */
		    psrch->fcn = ENDLIST;

		    /* Alloc space for strings referenced by the structure */
		    if (str = (char *) malloc((unsigned) strlen(*pp)+1)) {

			/* Extract field name, function, and compare string */
			(void) strcpy(str, *pp);

			/* If criteria contains an equal sign ('=') ... */
			if (p = strchr(str+1, '=')) {
			    if (*(p-1) == '!') {
				*(p-1) = '\0';
				psrch->fcn = NOTEQUAL;
			    }
			    else {
				*p = '\0';
				psrch->fcn = EQUAL;
			    }
			    psrch->cmp = p+1;
			    psrch->name = str;
			    psrch++;
			}

			/* If criteria contains a colon (':') ... */
			else if (p = strchr(str+1, ':')) {
			    if (*(p-1) == '!') {
				*(p-1) = '\0';
				psrch->fcn = NOEXISTS;
			    }
			    else {
				*p = '\0';
				psrch->fcn = EXISTS;
			    }
			    psrch->cmp = p+1;
			    psrch->name = str;
			    psrch++;
			}
		    }
		    else {
			/* Unable to malloc() string space.  Clean up */
			freesearchlist(rtnbuf);
			noerror = FALSE;
		    }
		    /* Next criteria */
		    pp++;
		}
		/* Terminate list */
		if (noerror) psrch->fcn = ENDLIST;
	    }
	    else {
		/* Unable to malloc() list space */
		noerror = FALSE;
	    }
	}

	/* Return a pointer to allocated space (if any) */
	return(rtnbuf);
}

/*
 *  void freesearchlist(list)
 *	struct srch  *list
 *
 *	This function frees the resources allocated to the searchlist <list>.
 *
 *  Arguments:
 *	list		The list whose resources are to be released.
 *
 *  Returns:  void
 */

static	void
freesearchlist(list)
	struct srch	       *list;
{
	/* Automatic data */
	struct srch	       *psrch;		/* Running ptr to structs */


	/* Free all of the string space allocated for the structure elememts */
	for (psrch = list ; psrch->fcn != ENDLIST ; psrch++) {
	    free(psrch->name);
	}

	/* Free the list space */
	free(list);
}

/*
 *  char **buildreturnlist()
 *
 *	This function builds a list of addresses of character-strings
 *	to be returned from the linked-list of devices we've been
 *	building.  It returns a pointer to the first item in that list.
 *
 *  Arguments:  none
 *
 *  Returns:  char **
 *	The address of the first item in the return list
 */

static	char **
buildreturnlist()
{
	/* Automatic data */
	char		      **list;
	char		      **q;
	struct deviceent       *p;


	/*
	 * Allocate space for the return list,
	 * with space for the terminating node
	 */

	if (list = (char **) malloc((devicelist.count+1)*sizeof(char *))) {

	    /*
	     * Walk the list of accumulated devices, putting pointers to
	     * device names in the list to return
	     */

	    q = list;
	    for (p = devicelist.head->next ; p ; p = p->next) *q++ = p->name;

	    /* End the list with a null-pointer */
	    *q = (char *) NULL;
	}


	/* Return a pointer to the list we've built */
	return(list);
}

/*
 *  char **makealiaslist(devices)
 *	char  **devices		List of aliases
 *
 *	Builds a list of aliases of the devices in the "devices"
 *	list.  This list will be terminated by (char *) NULL and
 *	will have the same number of elements as "devices".  If
 *	a device couldn't be found, that alias will be "".  There
 *	will be a one-to-one correspondence of devices to aliases
 *	in the device list "devices" and the generated list.
 *
 *  Arguments:
 *	devices		The list of devices to derive aliases from
 *
 *  Returns:  char **
 *	The address of the list of addresses of aliases.  The list
 *	and aliases will be allocated using the malloc() function.
 */

static	char **
makealiaslist(devices)
	char  **devices;
{
	/*  Automatic data  */
	char	      **pp;		/* Running ptr to (char *) */
	char	      **qq;		/* Running ptr to (char *) */
	char          **aliases;	/* List being returned */
	char	       *alias;		/* Alias of current device */
	int		olderrno;	/* Value of errno on entry */
	int		noerror;	/* Flag, TRUE if all's well */
	int		n;		/* Count of entries in "devices" */


	noerror = TRUE;
	olderrno = errno;
	if (devices) {

	    /* Get the number of entries in the constaint list */
	    for (n = 1 , pp = devices ; *pp ; pp++) n++;

	    /* Get space for the alias list */
	    if (aliases = (char **) malloc(n*sizeof(char *))) {

		/* Build the alias list */
		qq = aliases;
		for (pp = devices ; noerror && *pp ; pp++) {

		    /* Get the device's alias and put it in the list */
		    if (alias = devattr(*pp, DDB_ALIAS)) *qq++ = alias;
		    else {
			errno = olderrno;
			if (alias = (char *) malloc(strlen("")+1))
			    *qq++ = strcpy(alias, "");
			else {
			    /* No space for a null string?  Yeech... */
			    for (qq = aliases ; *qq ; qq++) free(*qq);
			    free((char *) aliases);
			    aliases = (char **) NULL;
			    noerror = FALSE;
			}
		    }
		}
		if (noerror) *qq = (char *) NULL;

	    } else noerror = FALSE;  /* malloc() failed */

	} else aliases = (char **) NULL;  /* No constraint list */

	/* Return ptr to generated list or NULL if none or error */
	return(aliases);
}

/*
 *  void freealiaslist(aliaslist)
 *	char  **aliaslist;
 *
 *	Free the space allocated to the aliaslist.  It frees the space
 *	allocated to the character-strings referenced by the list then
 *	it frees the list.
 *
 *  Arguments:
 *	aliaslist	The address of the first item in the list of
 *			aliases that is to be freed
 *
 *  Returns:  void
 */

static	void
freealiaslist(aliaslist)
	char  **aliaslist;		/* Ptr to new device list */
{
	/* Automatic Data */
	char   **pp;			/* Running pointer */

	/* If there's a list ... */
	if (aliaslist) {

	    /* For each entry in the old list, free the entry */
	    for (pp = aliaslist ; *pp ; pp++) free(*pp);

	    /* Free the list */
	    free(aliaslist);
	}
}

/*
 *  char *getnextmatch(fp, criteria, options)
 *	FILE			*fp
 *	struct srch	       *criteria
 *	int			options
 *
 *  	Gets the next device in the Device Database(DDB_TAB file <fp>)
 *	that matches the criteria. Returns the alias of that device.
 *
 *  Arguments:
 *	fp		File pointer of DDB_TAB file
 *	criteria	The linked list of criteria to use to match a device
 *	options		Options modifying the criteria (only one that's really
 *			important is the DTAB_ANDCRITERIA flag)
 *
 *  Returns:  char *
 *	A pointer to a malloc()ed string containing the alias of the next
 *	device that matches the criteria, or (char *) NULL if none.
 */

static	char   *
getnextmatch(fp, criteria, options)
	FILE		*fp;
	struct srch	*criteria;
	int		options;
{
	/* Automatic data */
	char			device[DDB_MAXALIAS]; /* device alias in DDB*/
	dev_record		devrec;		/* device record from DDB */
	char		       *alias;		/* Alias of device found  */
	int			notdone;	/* Flag, done yet?        */
	int			noerror;	/* Flag, had an error yet? */


	/*
	 *  Initializations:
	 *    -	No alias yet
	 *    - Not finished yet
	 *    -	Make sure there are criteria we're to use
	 */

	alias = (char *) NULL;
	notdone = TRUE;
	noerror = TRUE;

	/*  If we're to "and" the criteria...  */
	if (options & DTAB_ANDCRITERIA) {

	    /*
	     *  Search the Device Database until we've got a record that matches
	     *  all of the criteria or we run out of records
	     */

	    while (notdone && (getnextdev(fp, device)==SUCCESS)) {
		if (get_devrec(device,&devrec)==SUCCESS) {
		    if (!criteria || matchallcriteria(&devrec, criteria)) {
			if (alias=(char *)malloc((unsigned)strlen(devrec.tab->alias)+1))
			    (void) strcpy(alias,devrec.tab->alias);
			else noerror = FALSE;
			notdone = FALSE;
		    }
		    free_devrec(&devrec);
		}
	    }
	}
	else {

	    /*
	     *  Search the Device Database until we've got a record that matches
	     *  any of the criteria or we run out of records
	     */

	    while (notdone && (getnextdev(fp,device)==SUCCESS)) {
		if (get_devrec(device,&devrec)==SUCCESS) {
		    if (!criteria || matchanycriteria(&devrec, criteria)) {
			if (alias =(char *)malloc((unsigned)strlen(devrec.tab->alias)+1))
			    (void) strcpy(alias, devrec.tab->alias);
			else noerror = FALSE;
			notdone = FALSE;
		    }
		}
		free_devrec(&devrec);
	    }
	}


	/* Return pointer to extracted alias (or NULL if none) */
	if ((alias == (char *) NULL) && noerror) errno = ENOENT;
	return(alias);
}

/*
 * int matchallcriteria(devrec, criteria)
 *
 *	This function examines the record contained in "devrec" and
 *	determines if that record meets all of the criteria specified by
 *	"criteria".
 *
 * Arguments:
 *	dev_record	*devrec		The Device Database entry to examine.
 *	struct srch	*criteria	The criteria to match.
 *
 * Returns:	int
 *	Returns TRUE if the record matches criteria, FALSE otherwise.
 */

static	int
matchallcriteria(devrec, criteria)
	dev_record	       *devrec;		/* Entry to check */
	struct srch	       *criteria;	/* Criteria governing match */
{
	/* Automatic data */
	struct srch    *p;		/* Pointer to current criteria */
	int		matched;	/*TRUE if record matches all criteria */
	char		*value;		/* DDB's attr value string */
	int 		atype;		/* attribute's type */


	/* Test only if there's criteria to test against */
	if (criteria && (criteria->fcn != ENDLIST)) {

	    matched = TRUE;
	    for (p = criteria ; matched && (p->fcn != ENDLIST) ; p++) {

		/* Don't compare against this criteria 
		 * if it's function is  "IGNORE" */
		if (p->fcn != IGNORE) {

		    /* check if attr-value meets requested criteria */
		    if (value=(char *)getattrval(p->name,devrec)) {

			/* If security attribute, secattrmatches()
			 * determines if there is a match or not 
			 * after translating external representation
			 * to their internal one */

			if (getattrtype(p->name,&atype) == TYPE_SEC) {
				matched =
				   secattrmatches(value,p->cmp,atype,p->fcn);
			} else {
				matched =
				   matches(value,p->cmp,p->fcn);
			}

		    } else {

			/* attr not defined in devrec */
			if (p->fcn != NOEXISTS) {
			    /* fails to meet criteria-EQUAL,NOTEQUAL,EXISTS */
			    matched = FALSE;
			}
		    } 

		}  /* Search function is not "IGNORE" */

	    }  /* for loop, checking each criteria */

	}

	/* Return a value indicating if the record matches all criteria */
	return(matched);
}

/*
 * int matchanycriteria(devrec, criteria)
 *
 *	This function examines the record contained in "devrec" and
 *	determines if that record meets any of the criteria specified by
 *	"criteria".
 *
 * Arguments:
 *	dev_record	*devrec		The Device Database entry to examine.
 *	struct srch      *criteria	The criteria to match.
 *
 * Returns:	int
 *	Returns TRUE if the record matches criteria, FALSE otherwise.
 */

static	int
matchanycriteria(devrec, criteria)
	dev_record	       *devrec;		/* Entry to check */
	struct srch	       *criteria;	/* Criteria governing match */
{
	/* Automatic data */
	struct srch    *p;		/* Pointer to current criteria */
	int		matched;	/* FLAG: TRUE if record matched */
	char		*value;		/* attr value string            */
	int		atype;		/* attribute type */


	/* Test only if there's criteria to test against */
	if (criteria && (criteria->fcn != ENDLIST)) {

	    matched = FALSE;
	    for (p = criteria ; !matched && (p->fcn != ENDLIST) ; p++) {

		/* Don't compare against this criteria if it's function is
		 * "IGNORE" */
		if (p->fcn != IGNORE) {
		    /* check if attr-value meets requested criteria */
		    if (value=(char *)getattrval(p->name,devrec)) {

			/* If security attribute, secattrmatches()
			 * determines if there is a match or not 
			 * after translating external representation
			 * to their internal one */

			if (getattrtype(p->name,&atype) == TYPE_SEC) {
				matched =
				   secattrmatches(value,p->cmp,atype,p->fcn);
			} else {
				matched =
				   matches(value,p->cmp,p->fcn);
			}

		    } else {
			/* attr not defined in devrec */
			if (p->fcn == NOEXISTS) {
			    /* meets criteria- NOEXISTS */
			    matched = TRUE;
			}
		    }
		}  /* Search function is not "IGNORE" */

	    }  /* for loop, checking each criteria */

	} else matched = TRUE;  /* No criteria specified, it's a match */


	/* Return a value indicating if the record matches all criteria */
	return(matched);
}

/*
 *  int matches(value, compare, function)
 *	char   *value
 *	char   *compare
 *	int	function
 *
 *	This function sees if the operation <function> is satisfied by
 *	comparing the value <value> with <compare>.  It returns TRUE
 *	if so, FALSE otherwise.
 *
 *  Arguments:
 *	value		Value to compare
 *	compare		Value to compare against
 *	function	Function to be satisfied
 *
 *  Returns:  int
 *	TRUE if the function is satisfied, FALSE otherwise
 */

static	int
matches(value, compare, function)
	char   *value;
	char   *compare;
	int	function;
{
	/*  Automatic data  */
	int	rtn;		/* Value to return */


	if (value == NULL)
		value = "";

	/* Do case depending on the function */
	switch(function) {

	/* attr=val */
	case EQUAL:
	    rtn = (strcmp(value, compare) == 0);
	    break;

	/* attr!=val */
	case NOTEQUAL:
	    rtn = (strcmp(value, compare) != 0);
	    break;

	/* attr:* */
	case EXISTS:
	    rtn = TRUE;
	    break;

	/* attr!:* */
	case NOEXISTS:
	    rtn = FALSE;
	    break;

	/* Shouldn't get here... */
	default:
	    rtn = FALSE;
	    break;
	}

	/* Return a value indicating if the match was made */
	return(rtn);
}

/*
 *  static int secattrmatches(value, compare, fieldno, function)
 *	char   *value
 *	char   *compare
 *	int    	fieldno
 *	int	function
 *
 *	This function handles criteria that consists of security
 * 	attributes. If the attribute is stored in a different representation
 *	than what the user sees, the function translates the criteria's
 *	attribute into the internal represetation and then compares it
 *	with the value found in the DDB. 
 * 	It returns TRUE if the function is satisfied; 
 *	otherwise, it returns FALSE.
 *
 *  Arguments:
 *	value		Value to compare
 *	compare		Value to compare against
 *	fieldno		Value returned from getattrtype representing
 *			the type of security attribute
 *	function	Function to be satisfied
 *
 *  Returns:  int
 *	TRUE if the function is satisfied, FALSE otherwise
 */

static	int
secattrmatches(value, compare, fieldno, function)
char   *value;		/* device's attribute's value */
char   *compare;	/* criteria's attribute's value */
int	fieldno;	/* security attribute */
int	function;
{
	int	rtn = TRUE;		/* Value to return */
	char 	*valstr1, *valstr2;
	char 	*critstr1, *critstr2;
	char 	*criteria;
	char 	*p,*q;

	level_t lid_d,	/* LID found in the DDB's entry */
		lid_c;  /* LID from the criteria */
	uid_t	c_uid;
	gid_t	gid;

	char    group[LOGNAMEMAX], user[LOGNAMEMAX], vuser[LOGNAMEMAX];

	int	ccnt,	/* count for users in criteria */
		vcnt;	/* count for users in value */

	if (value == NULL)
		value = "";

	criteria = (char *)malloc(strlen(compare) + 1);
	if (!criteria) {
		return(FALSE);
	}

	/* Make a local copy of the criteria. getfield() affects
	 * the string, and using a local copy prevents any problems
	 */
	strcpy(criteria, compare);

	switch(fieldno) {
	case (1):	/* range */

		/* If criteria is :* (EXISTS) or !:* (NOEXISTS), 
		 * no need to compare any values */
		if (function == EXISTS) {
			break;
		} else if (function == NOEXISTS && value) {
			rtn = FALSE;
			break;
		}

		/* First check hilevel of value and criteria. 
		 * If they match, and the criteria function is
		 * not NOTEQUAL, then convert lolevel of value 
		 * and criteria. If the hilevels don't match, 
		 * return according to the criteria function */

		/* Translate the attribute's value string to LID's */
		valstr1 = (char *) getfield(value,"-",&valstr2);
		lid_d = (level_t) strtol(valstr1, (char **)0, 0);

		/* convert criteria's hilevel string to LID */
		critstr1 = (char *) getfield(criteria,"-",&critstr2);
		if (lvlin(critstr1, &lid_c) < 0) {
			rtn = FALSE;
			break;
		}

		/* compare the hilevel LID's */
		if (lvlequal(&lid_c, &lid_d) > 0) {

			/* The hilevel of the device's entry and the 
			 * criteria are equal. If criteria 
			 * was != (NOTEQUAL), return FALSE
			 */
			if (function == NOTEQUAL) {
				rtn = FALSE;
				break;
			}

			/* Translate the lolevels */

			/* Translate attribute's value string to LID's */
			lid_d = (level_t) strtol(valstr2, (char **)0, 0);
	
			/* Translate criteria's string to LID */
			if (lvlin(critstr2, &lid_c) < 0)
				rtn = FALSE;

			if (lvlequal(&lid_c, &lid_d) > 0) {
				rtn = TRUE;
			} else
				rtn = FALSE;
	
		} else {
			/* hilevels don't match */
			if (function == NOTEQUAL)
				rtn = TRUE;
			else
				rtn = FALSE;
		}
		break;

	case (2):       /* state */
	case (3):       /* mode */
	case (4):       /* startup */

		rtn = matches(value, compare, function);
		break;

	case (5): 	/* startup level */

		/* If criteria is :* (EXISTS) or !:* (NOEXISTS), 
		 * no need to compare any values 
		 */
		if (function == EXISTS) {
			break;
		} else if (function == NOEXISTS && value) {
			rtn = FALSE;
			break;
		}

		/* Translate the device's level's string to LID's */
		lid_d = (level_t) strtol(value, (char **)0, 0);

		/* convert criteria's string to LID */
		if (lvlin(criteria, &lid_c) < 0) {
			rtn = FALSE;
			break;
		}

		if (lvlequal(&lid_c, &lid_d) > 0) {

			/* The levels are equal. If criteria 
			 * was != (NOTEQUAL) set rtn to FALSE */
			if (function == NOTEQUAL)
				rtn = FALSE;
		} else {
			if (function == EQUAL)
				rtn = FALSE;
		}
		break;

	case (6):	/* startup owner */

		/* If criteria is :* (EXISTS) or !:* (NOEXISTS), 
		 * no need to compare values */
		if (function == EXISTS) {
			break;
		} else if (function == NOEXISTS && value) {
			rtn = FALSE;
			break;
		}

		/* get the user name or uid */
		critstr1 = (char *) getfield(criteria,">",&critstr2);
		if (parse_uid(critstr1, &c_uid) != SUCCESS) {
			rtn = FALSE;
			break;
		}

		sprintf(user,"%u",c_uid);
		strcat(user, ">");
		strcat(user, critstr2);
		rtn = matches(value,user,function);

		break;

	case (7):	/* startup group */

		/* If criteria is :* (EXISTS) or !:* (NOEXISTS), 
		 * no need to compare any values */
		if (function == EXISTS) {
			break;
		} else if (function == NOEXISTS && value) {
			rtn = FALSE;
			break;
		}

		/* get the group specified in criteria */
		critstr1 = (char *) getfield(criteria,">",&critstr2);
		if (parse_gid(critstr1, &gid) != SUCCESS) {
			rtn = FALSE;
			break;
		}

		sprintf(group,"%u",gid);
		strcat(group, ">");
		strcat(user, critstr2);
		rtn = matches(value,group,function);
		break;

	case (8):	/* startup other */
	case (9):	/* ual_enable */

		rtn = matches(value, compare, function);
		break;

	case (10):	/* users */

		/* If criteria is :* (EXISTS) or !:* (NOEXISTS), 
		 * no need to compare any values */
		if (function == EXISTS) {
			break;
		} else if (function == NOEXISTS && value) {
			rtn = FALSE;
			break;
		}

		/* get count of number of users in criteria */
		ccnt = getlistcnt(compare);
		if (!ccnt) {
			rtn = FALSE;
			break;
		}

		/* get count of number of users in value  */
		vcnt = getlistcnt(value);

		if (ccnt == vcnt) {
			if (function == NOTEQUAL) {
				rtn = FALSE;
				break;
			}
		} else  {
			if (function == EQUAL) {
				rtn = FALSE;
				break;
			} else 
				break;
		}

		/* Number of users in criteria and value are the
		 * same and the function must be EQUAL
		 * From this point on, in order to return true
		 * all the users and the associated permissions
		 * must be the same in both value and criteria.
		 * If at any point, the uid or the permissions
		 * dont' match, return FALSE.
		 */

		p = value;
		q = criteria;

		while (*p != '\0' && vcnt-- > 1) {

			/* get uid and permissions from value */
			valstr1 = (char *) getfield(p,">",&p);
			valstr2 = (char *) getfield(p,",",&p);

			strcpy(vuser,valstr1);
			strcat(vuser,">");
			strcat(vuser,valstr2);

			/* get user name or uid and permissions from criteria */
			critstr1 = getfield(q,">", &q);
			critstr2 = getfield(q,",", &q);
			if (parse_uid(critstr1, &c_uid) != SUCCESS) {
				rtn = FALSE;
				break;
			}

			sprintf(user,"%u",c_uid);
			strcat(user, ">");
			strcat(user, critstr2);

			if (strcmp(vuser,user) != 0) {
				rtn = FALSE;
				break;
			}
		}
		if (rtn == FALSE)
			break;

		if (*p == '\0' || *q == '\0') {
			rtn = FALSE;
			break;
		}

		/* first or last uid/user in value */
		valstr1 = (char *) getfield(p,">",&p);
		strcpy(vuser,valstr1);
		strcat(vuser,">");
		strcat(vuser,p);
		
		/* first or last uid/user in criteria */
		critstr1 = getfield(q,">", &q);
		if (parse_uid(critstr1, &c_uid) != SUCCESS) {
			rtn = FALSE;
			break;
		}

		sprintf(user,"%u",c_uid);
		strcat(user, ">");
		strcat(user, q);

		if (strcmp(vuser,user) != 0)
			rtn = FALSE;
		break;
	
	case (11):	/* other */

		rtn = matches(value, compare, function);
		break;

	default:
		rtn = FALSE;
		break;

	} /* end of switch */

	free(criteria);
	return(rtn);
}
