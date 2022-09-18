/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uname:i386/cmd/uname/uname.c	1.29.2.8"
#ident "$Header: uname.c 1.4 91/07/23 $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
/***************************************************************************
 * Command: uname
 * Inheritable Privileges: P_SYSOPS,P_DACREAD,P_DACWRITE
 *       Fixed Privileges: None
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<signal.h>
#include	<stdlib.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<sys/utsname.h>
#include	<sys/systeminfo.h>
#include        <string.h>
#include        <sys/types.h>
#include        <sys/fcntl.h>
#include        <sys/stat.h>
#include        <sys/sysi86.h>



struct utsname  unstr, *un;
	/*  Enhanced Application Compatibility  */
struct scoutsname scostr, *sco;		/* -X option for SCO-specific info  */
	/*  End Enhanced Application Compatibility  */

extern void exit();
extern int uname();
extern char *getenv();
extern int optind;
extern char *optarg;

/*
 * Procedure:     main
 *
 * Restrictions:
 *                setlocale: none
 *                pfmt: none
 *                fopen: none
 *                strerror: none
 *                sysi86(2): none
*/

	/*  Enhanced Application Compatibility 
	 *  If SCOMPAT is set:
	       if value is rel:ver
		   release will be set to rel and verseion will be set to ver
	       else release=3.2 and version=2
	 *  End Enhanced Application Compatibility  */
main(argc, argv)
char **argv;
int argc;
{
        char *nodename;
        int Sflg=0;
        char *optstring="asnrpvmS:X";

        int sflg=0, nflg=0, rflg=0, vflg=0, pflg=0, mflg=0, errflg=0, Xflg=0, optlet;
	char procbuf[256], *eac_rel, *eac_ver;
	char dflt_eac_rel[]="3.2";
	char dflt_eac_ver[]="2";

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:uname");

	umask(~(S_IRWXU|S_IRGRP|S_IROTH) & S_IAMB);
        un = &unstr;
        uname(un);
	eac_ver = eac_rel = getenv("SCOMPAT"); 
	if (eac_rel) {
		eac_ver=strpbrk(eac_rel,  ":"); /* eac_ver points to : */
		if ( eac_ver) {
		   strcpy(eac_ver,++eac_ver);
		   *(eac_rel + (strlen(eac_rel) - strlen(eac_ver) -1)) = '\0';
		}
		   /* pattern must be release:version */
		if (*(eac_rel) == '\0' || *(eac_ver) == '\0') {
		   eac_rel=dflt_eac_rel;
		   eac_ver=dflt_eac_ver;
		}
	}

        while((optlet=getopt(argc, argv, optstring)) != EOF)
                switch(optlet) {
	/*  Enhanced Application Compatibility  */
		case 'X':
			Xflg++;
			break;
	/*  End Enhanced Application Compatibility  */
                case 'a':
                        sflg++; nflg++; rflg++; vflg++; mflg++; pflg++;
                        break;
                case 's':
                        sflg++;
                        break;
                case 'n':
                        nflg++;
                        break;
                case 'r':
                        rflg++;
                        break;
                case 'v':
                        vflg++;
                        break;
                case 'm':
                        mflg++;
                        break;
		case 'p':
			pflg++;
			break;
                case 'S':
                        Sflg++;
                        nodename = optarg;
			Createrc2file(nodename);
                        break;
                case '?':
                        errflg++;
                }

        if(errflg || (optind != argc))
                usage(errflg);

        if((Sflg > 1) || 
           (Sflg && (sflg || nflg || rflg || vflg || mflg || pflg || Xflg))) {
                usage(errflg);
        }

        /* If we're changing the system name */
        if(Sflg) {
		FILE *file;
		char curname[SYS_NMLN];
		int len = strlen(nodename);
		int curlen, i;
		
                /*
                 * The size of the node name must be less than SYS_NMLN.
                 */
                if(len > SYS_NMLN - 1) {
                        (void) pfmt(stderr, MM_ERROR,
                        	":730:Name must be <= %d letters\n",SYS_NMLN-1);
                        exit(1);
                }

		/*
		 * NOTE:
		 * The name of the system is stored in a file for use
		 * when booting because the non-volatile RAM on the
		 * porting base will not allow storage of the full
		 * internet standard nodename.
		 * If sufficient non-volatile RAM is available on
		 * the hardware, however, storing the name there would
		 * be preferable to storing it in a file.
		 */

		/* 
		 * Only modify the file if the name requested is
		 * different than the name currently stored.
		 * This will mainly be useful at boot time
		 * when 'uname -S' is called with the name stored 
		 * in the file as an argument, to change the
		 * name of the machine from the default to the
		 * stored name.  In this case only the string
		 * in the global utsname structure must be changed.
		 */

		if ((file = fopen("/etc/nodename", "r")) != NULL) {
			curlen = fread(curname, sizeof(char), SYS_NMLN, file);
			for (i = 0; i < curlen; i++) {
				if (curname[i] == '\n') {
					curname[i] = '\0';
					break;
				}
			}
			if (i == curlen) {
				curname[curlen] = '\0';
			}
			fclose(file);
		} else {
			curname[0] = '\0';
		}

		if (strcmp(curname, nodename) != 0) {
			if ((file = fopen("/etc/nodename", "w")) == NULL) {
				(void) pfmt(stderr, MM_ERROR,
					":148:Cannot create %s: %s\n",
					"/etc/nodename", strerror(errno));
				exit(1);
			} 

			if (fprintf(file, "%s\n", nodename) < 0) {
				(void) pfmt(stderr, MM_ERROR,
					":333:Write error in %s: %s\n",
					"/etc/nodename", strerror(errno));
				exit(1);
			}
			fclose(file);
		}		
		
                /* replace name in kernel data section */
                sysi86(SETNAME, nodename, 0);
                exit(0);
        }
                                                    /* "uname -s" is the default */
        if( !(sflg || nflg || rflg || vflg || mflg || pflg || Xflg))
                sflg++;
        if(sflg)
                (void) fprintf(stdout, "%.*s", sizeof(un->sysname), un->sysname);
        if(nflg) {
                if(sflg) (void) putchar(' ');
                (void) fprintf(stdout, "%.*s", sizeof(un->nodename), un->nodename);
        }
        if(rflg) {
                if(sflg || nflg) (void) putchar(' ');
		if( eac_rel )
			(void) fprintf(stdout,"%.*s",strlen(eac_rel), eac_rel);
                else (void) fprintf(stdout, "%.*s", sizeof(un->release), un->release);
        }
        if(vflg) {
                if(sflg || nflg || rflg) (void) putchar(' ');
		if( eac_ver )
			(void) fprintf(stdout,"%.*s",strlen(eac_ver), eac_ver);
                else (void) fprintf(stdout, "%.*s", sizeof(un->version), un->version);
        }
        if(mflg) {
                if(sflg || nflg || rflg || vflg) (void) putchar(' ');
                (void) fprintf(stdout, "%.*s", sizeof(un->machine), un->machine);
        }
	if (pflg) {
		if (sysinfo(SI_ARCHITECTURE, procbuf, sizeof(procbuf)) == -1) {
			pfmt(stderr, MM_ERROR, ":731:Sysinfo failed: %s\n",
				strerror(errno));
			exit(1);
		}
                if(sflg || nflg || rflg || vflg || mflg) (void) putchar(' ');
		(void) fprintf(stdout, "%.*s", strlen(procbuf), procbuf);
	}
	/*  Enhanced Application Compatibility  */
	signal((int)SIGSYS, SIG_IGN);
	if (Xflg) {		
		sco = &scostr;
		if (scoinfo(sco, sizeof(struct scoutsname)) == -1) {
		   perror("uname -X");
		   usage();
		}
		(void) putchar('\n');
		(void) fprintf(stdout,"System = %s\n",sco->sysname);
		(void) fprintf(stdout,"Node = %s\n",sco->nodename);
		if( eac_rel )
		     (void) fprintf(stdout, "Release = %s\n", eac_rel);
		else (void) fprintf(stdout,"Release = %s\n",sco->release);
		(void) fprintf(stdout,"KernelID = %s\n",sco->kernelid);
		(void) fprintf(stdout,"Machine = %s\n",sco->machine);
		(void) fprintf(stdout,"BusType = %s\n",sco->bustype);
		(void) fprintf(stdout,"Serial = %s\n",sco->sysserial);
		(void) fprintf(stdout,"Users = %s\n",sco->numuser);
		(void) fprintf(stdout,"OEM# = %d\n",sco->sysoem);
		(void) fprintf(stdout,"Origin# = %d\n",sco->sysorigin);
		(void) fprintf(stdout,"NumCPU = %d\n",sco->numcpu);
	}
	/*  End Enhanced Application Compatibility  */
        (void) putchar('\n');
        exit(0);
}


/*  The X option is not advertised. hence, it is missing from the 
    usage message	*/

usage(err)
int err;
{
	if (!err)
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
        (void) pfmt(stderr, MM_ACTION,
        	":732:Usage:\n\tuname [-snrvmap]\n\tuname [-S system name]\n");
        exit(1);
}



Createrc2file(nodename)
char *nodename;
{
	FILE *fp;
	
        /*
         * The size of the node name must be less than SYS_NMLN.
         */
	if (strlen(nodename) > (size_t) SYS_NMLN - 1  ) {
                (void) pfmt(stderr, MM_ERROR,
                      	":730:Name must be <= %d letters\n",SYS_NMLN-1);
               	exit(1);
	}

	if ((fp = fopen("/etc/rc2.d/S11uname", "w")) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":148:Cannot create %s: %s\n",
			"/etc/rc2.d/S11uname", strerror(errno));
       		exit(1);
	} 

	fprintf(fp, "uname -S %s", nodename);
}
