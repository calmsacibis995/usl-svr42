/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/listdev.c	1.2.10.2"
#ident  "$Header: listdev.c 1.2 91/06/25 $"
/*LINTLIBRARY*/

/*
 * listdev.c
 *
 *  Contains:
 *	listdev()	List attributes defined for a device
 */

/*
 *  Header files needed:
 *	<sys/types.h>	System Data Types
 *	<stdio.h>	Standard I/O functions
 *	<string.h>	Standard string definitions
 *	<stdlib.h>	Storage allocation functions
 *	<devmgmt.h>	Device management definitions
 *	"devtab.h"	Local device table definitions
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<devmgmt.h>
#include	"devtab.h"


/*
 *  Externals Referenced:
 *	get_devrec()	Gets all attributes(values) defined for <device>.
 */

extern	int	get_devrec();
extern  int	errno;
/*
 *  Local Definitions:
 */
#define QUIT_LIST() { \
	freeattrlist();\
	return((char **)NULL);\
	}
/*
 *  Local, Static data, functions:
 */
typedef struct attrent {
	struct attrent	*next;	/* ptr to next attr name */
	char		*name;	/* attribute name        */
} attr_ent;

static struct {
	attr_ent	*head;	/* head of attr name list */
	int		count;	/* no. of attrs in list   */
} attrlist = { (struct attrent *)NULL, 0 };

static void insert_attr();
static void freeattrlist();

/*
 * char **listdev(device)
 *	char   *device;
 *
 *	Generate an alphabetized list of attribute names of the
 *	attributes defined for the device <device>.
 *
 *  Arguments:
 *	device		Device who's attributes are to be listed
 *
 *  Returns: char **
 *	List of attribute names of the attributes defined for this
 *	device.  (Never empty since all devices have the "alias"
 *	attribute defined.)
 */

char **
listdev(device) 
	char   *device;		/* Device to describe */
{
	/*  Automatic data  */

	dev_record	       devrec;		/* dev_record struct   */
	attr_val	       *atval;		/* Ptr to attr val pair */
	attr_ent	       *ent, *curr;	/* attr name entry      */
	char		      **list;		/* Ptr to alloc'd list */
	int			noerror;	/* FLAG, TRUE if :-) */
	int			i;		/* Temp counter */
				

	/*  If <device> defined, get all the device attributes */
	if (get_devrec(device, &devrec) == FAILURE) {
	    errno = ddb_errget();
	    return((char **)NULL);
	}

	/*  
	 *  Accumulate all the attributes defined for the device
	 *  in an attribute list <attrlist>.
	 */

	/* attributes defined in DDB_TAB 
	 * To allow 4.1 cmds to work with 4.0 device.tab format
	 * the values expected to be found in device.tab 
	 * are the following:
	 * alias:cdevice:bdevice:pathname:secdev:oam attrs
	 */
	
	/*  Alias, is always defined defined  */
	if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
	    ent->name = DDB_ALIAS;
	    ent->next = (attr_ent *)NULL;
	    attrlist.head=ent;
	    attrlist.count = 1;
	} else QUIT_LIST();

	curr = ent;

	/*  secdev, if defined  */
	if (devrec.tab->secdev){		
	    if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		ent->name = DDB_SECDEV;
		ent->next = (attr_ent *)NULL;
		/* link new entry & increment count */
		curr->next=ent;
		curr = ent;
		attrlist.count++;
	    } else QUIT_LIST();
	}

	/*  cdevice, if defined  */
	if (devrec.tab->cdevice) {		
	    if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		ent->name = DDB_CDEVICE;
		ent->next = (attr_ent *)NULL;
		/* link new entry & increment count */
		curr->next=ent;
		curr = ent;
		attrlist.count++;
	    } else QUIT_LIST();
	}

	/*  bdevice, if defined  */
	if (devrec.tab->bdevice) {		
	    if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		ent->name = DDB_BDEVICE;
		ent->next = (attr_ent *)NULL;
		/* link new entry & increment count */
		curr->next=ent;
		curr = ent;
		attrlist.count++;
	    } else QUIT_LIST();
	}

	if (devrec.tab->pathname) {		/*  Pathname, if defined  */
	    if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		ent->name = DDB_PATHNAME;
		ent->next = (attr_ent *)NULL;
		/* link new entry & increment count */
		curr->next=ent;
		curr = ent;
		attrlist.count++;
	    } else QUIT_LIST();
	}

	/*  Other attributes, if any  */
	if (atval = devrec.tab->attrlist) do {
	    if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		ent->name = atval->attr;
		ent->next = (attr_ent *)NULL;
		/* link new entry & increment count */
		curr->next=ent;
		curr = ent;
		attrlist.count++;
	    } else QUIT_LIST();
	} while (atval = atval->next);
	
	/* count attributes defined in DDB_DSFMAP(dsf attrs) */
	if (devrec.dsf) {

	    if (devrec.dsf->cdevlist) {	/*  Char spcls, if defined  */
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_CDEVLIST;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.dsf->bdevlist) {	/*  Blk spcls, if defined  */
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_BDEVLIST;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	}

	/* count attributes defined in DDB_SEC(sec attrs) */
	/* Security attrs can be defined only when *
	 * Enhanced Security package is installed. */
	if (devrec.sec) {
	    /* sec. alias has been purposely ommitted, *
	     * since it is an internal representaion of*
	     * the <secdev> attribute.                 */
	    if (devrec.sec->range) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_RANGE;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->state) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_STATE;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->mode) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_MODE;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->startup) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_STARTUP;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->st_level) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_ST_LEVEL;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->st_owner) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_ST_OWNER;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->st_group) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_ST_GROUP;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
	 	} else QUIT_LIST();
	    }
	    if (devrec.sec->st_other) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_ST_OTHER;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	   }
	   if (devrec.sec->ual_enable) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_UAL_ENABLE;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->users) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_USERS;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	    }
	    if (devrec.sec->other) {
		if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
		    ent->name = DDB_OTHER;
		    ent->next = (attr_ent *)NULL;
		    /* link new entry & increment count */
		    curr->next=ent;
		    curr = ent;
		    attrlist.count++;
		} else QUIT_LIST();
	     }
	}

	/* allocate memory for pointer list */
	list = (char **)NULL;
	if (list=(char **)malloc((attrlist.count+2)*sizeof(char *))) {
	    /* initialize 1st attr to NULL, for sorting purposes */
	    list[0] = (char *)NULL;
	    ent = attrlist.head;	
	    /* copy attrs from attrlist into output list */
	    for (i=1 ; i<=attrlist.count ; i++) {
		if (list[i]=(char *)malloc(strlen(ent->name)+1)) {
		    strcpy(list[i], ent->name);
		    /* insert attr in sorted order in list */
		    insert_attr(i, list[i], list);
		    /* get next attribute */
		    ent = ent->next;
		} else QUIT_LIST();
	    }
	    list[i] = (char *)NULL;	/* end list with NULL */
	    list++;			/* skip 1st NULL entry*/
	} else QUIT_LIST();

	/* free memory for devrec & attrlist */
	free_devrec(&devrec);
	freeattrlist();

	/* Fini */
	return(list);
}


/*
 *  void insert_attr(n, attr, list)
 *	int        n;
 *	char  *attr;
 *	char  **list;
 *
 *	This function inserts the specified attribute, <attr>, into
 *	a sorted <list> of attribute names.
 *	It uses the standard Insertion Sort algorithm.
 *
 *  Arguments:
 *	n	item number being inserted
 *	attr	attribute string 
 *	list	array of pointers to attributes
 *
 *  Returns: void 
 *
 */

static void
insert_attr(n, attr, list)
int	n;
char	*attr;
char	**list;
{
	char		*key;		/* entry to be inserted */
	int		i, cmp;

	key = attr;
	i = n-1;

	while (i > 0) {
	/* compare key with each attribute in <list> */
	if (strcmp(list[i], key) > 0) {
		/* if array element > key, switch pointers to items */
		list[i+1] = list[i];
		i = i-1;
	} else {
		/* correct position of key found, break out */
		break;
	}
	}
	/* insert key */
	list[i+1] = key;
	return;
}

/*
 * static void freeattrlist()
 *	Frees memory allocated for attr-list.
 */
static
void freeattrlist()
{
	attr_ent	*next, *curr;

	if (curr=attrlist.head) do {
	next = curr->next;
	free(curr);
	} while(curr=next);
}
