/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/devdealloc.c	1.3.6.2"
#ident	"$Header: devdealloc.c 1.3 91/06/25 $"

/*
 * Header Files :
 */
#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<devmgmt.h>
#include	<sys/time.h>
#include	<mac.h>
#include	<acl.h>
#include	"devtab.h"



/*
 * Local Definitions
 */
static struct devstat	sysbuf = { DEV_SYSTEM, 0, 0, 0, 0, 0};


/*
 * int devdealloc(device)
 * 	const char	*device;
 *
 * This library functions allows privileged processes to set security
 * attributes of the specified <device>, back to "system configuration".
 *
 * The system configuration is as follows:
 *	range=		hilevel=lolevel=0
 *	state=		DEV_PRIVATE (DEV_PUBLIC if driver flag=INITPUB)
 *	mode=		DEV_STATIC
 *	release_flag=	DEV_SYSTEM
 * This is accomplished by invoking devstat() system call with the
 * the release_flag set to DEV_SYSTEM.
 *
 *  RETURN VALUE: int
 *
 *	0	if the function succeeds. 
 *	-1	if the function fails.
 *	   <errno> is set to one of the following values:
 *	   EACCES if access to Device Database is denied.
 *	   ENODEV if <device> is not defined in the Device Database
 *	   EAGAIN if the Device Database is in Use, and cannot be locked.
 *	   ENOPKG if enhanced security package is not installed.
 */
int 
devdealloc(device)
 	char		*device;
{
	char		*s_alias; 
	int		err, perm, dev_type, rtn;

	/* check if enhanced security package is running */
	if (!mac_running()) {
	    /* errno set by lvlproc() in mac_running() */
	    return(FAILURE);
	}

	if ((rtn=ddb_check()) < 0) {
	    if (errno==0) {
		/* none of the DDB files present */
		errno = ENOENT;
	    }
	    return(FAILURE);
	} else if (rtn == 0) {
	    /* magic-nos in DDB files do not match */
	    errno = EACCES;
	    return(FAILURE);
	}

	/* lock the Device Database */
	if (lock_ddb()==FAILURE) {
	    /* Device Database in use */
	    return(FAILURE);
	}
	if (s_alias=devattr(device, DDB_SECDEV)) {
	    if (strcmp(device,s_alias)==0) { 
		dev_type = DEV_SECDEV;      /* secure  alias */
	    } else {
		/* device <> s_alias */
		if (valid_path(device))
		    dev_type = DEV_DSF;      /* pathname of dsf */
		else
		    dev_type = DEV_ALIAS;    /* logical alias */
	    }
	    /* devresetdsfs() also un-does(back-tracks) upon failure    */
	    if((err=devresetdsfs(device,dev_type,(level_t)NULL,
					(struct dev_daca *)NULL)) < 0) {
		err = errno;	/* save errno */
	        unlock_ddb();
		errno = err;
	        return(FAILURE);
	    }
	    unlock_ddb();
	} else {
	    /* DDB inaccessible or <device> not found in DDB */
	    return(FAILURE);
	}
	return(SUCCESS);
}

/*
 * int devresetdsfs(device, dev_type, devlvl, dacbuf)
 * 	char			*device;
 * 	int			dev_type;
 *	level_t			devlvl;
 * 	struct dev_daca		*dacbuf;
 *
 * This functions resets(deallocates) the security attributes on the specified 
 * <device>, to system configuration (state=DEV_SYSTEM). <device> could be
 * either an absolute pathname to a character or block device special file(dsf),
 * or a <device alias> name.
 * 
 * If specified <device> is a dsf(dev_type = DEV_DSF), then only that 
 * dsf is used to set security attributes. If <device> is a 
 * logical alias(dev_type = DEV_ALIAS), then all dsfs mapped to that 
 * alias, as defined in the DDB are used. If device is a 
 * secure device alias (dev_type = DEV_SECDEV), then
 * all dsfs mapped to all aliases, that define their secdev attribute
 * equal to the specified secure device alias are used.
 * 
 * It uses the level in <devlvl>, to set the level on the deallocated
 * device(device special file). If <devlvl> is NULL, then the level
 * on the device(dsf) is unchanged.
 *
 * It uses the information in <dacbuf> to set the DAC ownership and 
 * permissions on the device special files deallocated. If <dacbuf> is
 * a NULL pointer, then the DAC permissions are unchanged.
 */
int 
devresetdsfs(device, dev_type, devlvl, dacbuf)
	char			*device;
	int			dev_type;
	level_t			devlvl;
	struct dev_daca		*dacbuf;
{
	FILE			*fp;
	char			dsf[MAXDSFLEN];
	int			dsf_type;
	int			err;
	int			dtype;		/* dsf type */
	char			alias[DDB_MAXALIAS],  /* alias name */
				salias[DDB_MAXALIAS]; /* secure dev alias  */

	char 			buf[80]; /* Where file's first line is stored.
					  * Value id not used. */

	struct acl		aclbuf[NACLBASE] = {{USER_OBJ, 0, 0},
						    {GROUP_OBJ,0, 0},
						    {CLASS_OBJ,0, 0},
						    {OTHER_OBJ,0, 0}};


	err = 0;
	if (dev_type == DEV_DSF) {
	    if (devstat(device, DEV_SET, &sysbuf)<0) {
		return(FAILURE);
	    }
	    if(devlvl) {

		/* set level */
		if (lvlfile(device,MAC_SET,&devlvl)<0) {
		    switch(errno) {
			    case (EPERM):
			    case (EACCES):
		    		ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
				break;
			    default:
		    		ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
				break;
		   }
		   return(FAILURE);
		}

	    }
	    if (dacbuf) {

		/* set DAC ownership */
		if (chown(device,dacbuf->uid,dacbuf->gid)<0)
		    return(FAILURE);

		/* set DAC permissions */
		if (chmod(device,dacbuf->mode)<0)
    		    return(FAILURE);

		/* if startup attributes are defined
		 * clear up any existing acl's 
		 */
    		aclbuf[0].a_perm = (dacbuf->mode>>6) & 7; /* USER_OBJ */
    		aclbuf[1].a_perm = (dacbuf->mode>>3) & 7; /* GROUP_OBJ */
    		aclbuf[2].a_perm = aclbuf[1].a_perm;	/* CLASS_OBJ */
    		aclbuf[3].a_perm = dacbuf->mode & 7;	/* OTHER_OBJ */
    		if (acl(device,ACL_SET,NACLBASE,aclbuf)<0) {
			err = errno;	/* save errno */
			switch(err=errno) {
			case(ENOSYS):
				getdsfmap(device, &dtype, alias, salias);
	    			ddb_errmsg(SEV_ERROR,4,E_NOACL,device,salias);
	    			break;
			case(EPERM):
	    			ddb_errmsg(SEV_ERROR,4,E_PERM, device);
	    			break;
			default:
	    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
	    		break;
			}
       			return(FAILURE);
	    	}
	   }
	} else {
	    /* set sec. attrs. on all dsfs mapped to device */
	    if ((fp=fopen(DDB_DSFMAP, "r"))==(FILE *)NULL) {
		return(FAILURE);
	    }

	    /* skip magic no */
	    if (fgets(buf,80,fp) == (char *)NULL)     {
		fclose(fp);
		return(FAILURE);
	    }

	    while (getnextdsf(fp,device,dev_type,dsf,&dsf_type)==SUCCESS) {

		/* set security attrs */ 
		if (devstat(dsf, DEV_SET, &sysbuf)<0) {
		    err = errno;
		}
		if(devlvl) {
		    /* set level */
		    if (lvlfile(dsf,MAC_SET,&devlvl)<0)
			return(FAILURE);
		}
		if (dacbuf) {
		    /* set DAC ownership */
		    if (chown(dsf,dacbuf->uid,dacbuf->gid)<0) {
			err = errno;
		    }
		    /* set DAC permissions */
		    if (chmod(dsf,dacbuf->mode)<0) {
			err = errno;
		    }

	    	    aclbuf[0].a_perm = (dacbuf->mode>>6) & 7;	/* USER_OBJ  */
	    	    aclbuf[1].a_perm = (dacbuf->mode>>3) & 7;	/* GROUP_OBJ */
	    	    aclbuf[2].a_perm = aclbuf[1].a_perm;	/* CLASS_OBJ */
	    	    aclbuf[3].a_perm = dacbuf->mode & 7;	/* OTHER_OBJ */
	    	    if (acl(dsf,ACL_SET,NACLBASE,aclbuf)<0) {
			err = errno;	/* save errno */
			switch(err=errno) {
			case(ENOSYS):
		    		ddb_errmsg(SEV_ERROR,4,E_NOACL,dsf,device);
		    		break;
			case(EPERM):
		    		ddb_errmsg(SEV_ERROR,4,E_PERM, device);
		    		break;
			default:
		    		ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    	break;
			}
	       		return(FAILURE);
	    	    }
	        }
	    }	/* end while */
	    fclose(fp);
	}
	if (err) {
	    errno = err;	/* restore errno */
	    return(FAILURE);
	} else {
	    return(SUCCESS);
	}
}
