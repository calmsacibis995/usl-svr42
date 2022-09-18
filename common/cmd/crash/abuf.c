/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/abuf.c	1.4.3.6"
#ident	"$Header: abuf.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function: abuf.
 */

#include <a.out.h>
#include <stdio.h>
#include "crash.h"
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stat.h>
#include <sys/privilege.h>
#include <sys/resource.h>
#include <sys/auditrec.h>
#include <sys/utsname.h>
#include <sys/stropts.h>
#include <sys/sysmacros.h>

#define BSZ  1		/* byte size field */
#define SSZ  2		/* short size field */
#define LSZ  4		/* long size field */

static struct syment *Abuf=NULL;	/* namelist symbol pointer */
static struct syment *Alog=NULL;	/* namelist symbol pointer */
static struct syment *Actl=NULL;	/* namelist symbol pointer */
static struct syment *Amac=NULL;	/* namelist symbol pointer */
static struct syment *Anbuf=NULL;	/* namelist symbol pointer */
static struct syment *Utsnm=NULL;	/* namelist symbol pointer */

static abufctl_t adtbuf;		/* internal audit buffer structure */
static alogctl_t adtlog;		/* internal audit log structure */
static actlctl_t adtctl;		/* internal audit control structure */

static int type = LSZ;			/* default abuf type */
static char mode = 'x';			/* default abuf mode */
extern long vtop();
extern long lseek();
static acbufs_t *bufs;
static void prbinary();			/* write audit buffer in binary format*/
static void bytehead();			/* write byte order and header record info */
static int file_redir = 0;		/* default no file redirection */


/*
 * get data in audit buffer 
 */
int
getabuf()
{

	long curbuf, nxtbuf;
	int b, c, data, flagcnt = 0;
	int nbuf, count = 0;
	int inusecnt = 0;

	if (!Abuf && !(Abuf = symsrch("adt_bufctl")))
		error("audit buffer structure not found in symbol table\n");
	readmem(Abuf->n_value, 1,-1,
		(char *)&adtbuf,sizeof(adtbuf),"audit buffer control structure");
	if (adtbuf.a_vhigh==0) 
		fprintf(fp,"abuf: audit buffer mechanism bypassed\n");

	if (!Anbuf && !(Anbuf = symsrch("adt_nbuf")))
		error("number of audit buffers not found\n");
	readmem(Anbuf->n_value, 1,-1,
		(char *)&nbuf,sizeof(int),"number of audit buffers");

	if ((bufs=(acbufs_t *)malloc(sizeof(acbufs_t)*nbuf))== NULL)
		error("abuf: unable to allocate space for audit buffer control substructure\n");

	readmem((long)adtbuf.a_cp, 1,-1,
		(acbufs_t *)bufs,sizeof(acbufs_t)*nbuf,"audit buffer substructure");

	optind = 1;
	while((c = getopt(argcnt,args,"bcdoxw:")) !=EOF) {
		switch(c) {
			/* character format */
                        case 'c' :      mode = 'c';
                                        type = BSZ;
                                        flagcnt++;
                                        break;
			/* decimal format */
                        case 'd' :      mode = 'd';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* octal format */
                        case 'o' :      mode = 'o';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* hexadecimal format */
                        case 'x' :      mode = 'x';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* binary format */
			case 'b' :      mode = 'b';
                                        flagcnt++;
                                        break;
			/* file redirection */
			case 'w' :      file_redir =1;
					if (mode != 'b')
						redirect();
                                        break;
                        default  :      longjmp(syn,0);
                                        break;
		}
	}
	if (flagcnt > 1) 
	   error("only one mode may be specified: b, c, d, o, or x.\n");


	if (mode == 'b') {
		if (file_redir) {
			bytehead();
			curbuf = adtbuf.a_curbuf;
			if (bufs[curbuf].b_flag & ADTNOBUF) {
				data = adtbuf.a_asize;
				prbinary(adtbuf.a_ap,data);
			}else {
				data = bufs[adtbuf.a_curbuf].b_inuse;
				while(data && count<nbuf) {
					count++;
					prbinary(adtbuf.a_bp+(curbuf*adtbuf.a_bsize),data);
					nxtbuf=((curbuf == nbuf-1) ? 0 : (curbuf+1));
					curbuf=nxtbuf;
					data=bufs[curbuf].b_inuse;
				}
			}
		}else
			error("unable to display in binary format\n");
	}else {
		fprintf(fp,"\tAUDIT BUFFER CONTROL STRUCTURE\n");
		fprintf(fp,"\ta_vhigh  = %d\n",adtbuf.a_vhigh);
		fprintf(fp,"\ta_bsize  = %d\n",adtbuf.a_bsize);
		fprintf(fp,"\ta_curbuf = %d\n",adtbuf.a_curbuf);
		fprintf(fp,"\ta_wptr   = %x\n",adtbuf.a_wptr);
		fprintf(fp,"\ta_bp     = %x\n",adtbuf.a_bp);
		fprintf(fp,"\ta_cp     = %x\n",adtbuf.a_cp);
		fprintf(fp,"\ta_ap     = %x\n",adtbuf.a_ap);
		fprintf(fp,"\ta_asize  = %d\n",adtbuf.a_asize);
		fprintf(fp,"\n\tNUMBER OF AUDIT BUFFERS = %d\n\n",nbuf);
		fprintf(fp,"\tBUFFER\tADDR\t\tINUSE\tFLAGS\n");
		for (b=0; b<nbuf; b++) {
			fprintf(fp,"\t%d\t%x\t%d\t0x%x\n", 
				b,adtbuf.a_bp+(b*adtbuf.a_bsize),
				bufs[b].b_inuse,bufs[b].b_flag);
			inusecnt+=bufs[b].b_inuse;
		}
		(void)fprintf(fp,"\n\n\tAudit Buffer Size: %d bytes\n",adtbuf.a_bsize);
		(void)fprintf(fp,"\tAmount of Data: %d bytes\n",inusecnt);

		curbuf = adtbuf.a_curbuf;
		if (bufs[curbuf].b_flag & ADTNOBUF) {
			data = adtbuf.a_asize;
			prabuf(adtbuf.a_ap,data);
		}else {
			data = bufs[adtbuf.a_curbuf].b_inuse;
			while(data && count<nbuf) {
				count++;
				prabuf(adtbuf.a_bp+(curbuf*adtbuf.a_bsize),data);
				nxtbuf=((curbuf == nbuf-1) ? 0 : (curbuf+1));
				curbuf=nxtbuf;
				data=bufs[curbuf].b_inuse;
			}
		}
	}
	return(0);
}

/*
 * Print or display contents of the audit buffer.
 */
static int
prabuf(addr,size)
long addr;
int size;
{
	int i;
	char ch;
	long lnum;
	long value;
	char *format;
	int precision;

	for (i=0; i<size; i++) {
		switch(type) {
			case BSZ :  readmem(addr,1,0,&ch,
					sizeof(ch),"audit buffer");
			            value = ch & 0377;
				    break;
			case LSZ :
				    readmem(addr,1,0,(char *)&lnum,
					sizeof(lnum),"audit buffer");
				    value = lnum;
				    break;
		}
		if (((mode == 'c') && ((i % 16) == 0)) 
		 || ((mode != 'c') && ((i % 4)  == 0))) {
				if (i!=0) 
					(void)fprintf(fp,"\n");
				(void)fprintf(fp,"\t%8.8x:  ", addr);
			}
		switch(mode) {
			case 'c' :  switch(type) {
					case BSZ :  putch(ch);
						    break;
					case LSZ :  putch((char)lnum);
						    break;
				    }
				    break;
			case 'o' :  format = "%.*o   ";
				    switch(type) {
					case BSZ :  precision = 3;
						    break;
					case LSZ :  precision = 11;
						    break;
			   		}
			 	    (void)fprintf(fp,format,precision,value);
			 	    break;
			case 'd' :  format = "%.*d   ";
				   switch(type) {
					case BSZ :  precision = 3;
						    break;
					case LSZ :  precision = 10;
						    break;
				    }
			 	    (void)fprintf(fp,format,precision,value);
			   	    break;
			case 'x' :  format = "%.*x   ";
				    switch(type) {
					case BSZ :  precision = 2;
						    break;
					case LSZ :  precision = 8;
						    break;
				    }
			 	    (void)fprintf(fp,format,precision,value);
				    break;
		}
		addr += type;
	}
	(void)fprintf(fp,"\n");
}


/*
 * create a file that contains the contents
 * of the audit buffer in binary format.
 */
static void
prbinary(addr,size)
long addr;
int size;
{
	char *buf;
	int fd;

	if ((fd=open(optarg,O_WRONLY|O_APPEND|O_CREAT,0600)) > 0) {
		if ((buf=(char * )malloc(sizeof(int) * size))== NULL)
			error("abuf: unable to allocate space for reading audit buffer\n");
		readmem((long)addr,1,-1,(char *)buf,size," audit_buffer");
		if ((write(fd,buf,size)) == -1) {
			close(fd);
			error("error writing buffer to file\n");
		}
	}else
		error("error opening file\n");
	close(fd);
}

static void
bytehead()
{
 	char		*ap,*wap,*byordbuf;
	int		fd,size,mac;
 	idrec_t		*id;
	trailrec_t	*t;
	struct utsname	utsname;
	struct stat	statbuf;
	char		sp[]={' '};
	int		ts,ss,rs;

	ss=ts=rs=0;
	/* check if the file exists */
	if (stat(optarg, &statbuf) == -1) {
		if ((fd=open(optarg,O_WRONLY|O_CREAT,0600)) > 0) {
			/* write out machine byte ordering info */
			if ((byordbuf=(char *)calloc(sizeof(char),ADT_BYORDLEN))==NULL) {
				close(fd);
				error("abuf: unable to allocate space for audit buffer byte ordering string\n");
			}
			strcpy(byordbuf,ADT_BYORD);
			if ((write(fd,byordbuf,sizeof(char)*ADT_BYORDLEN)) != (sizeof(char) * ADT_BYORDLEN)) {
				close(fd);
				error("error writing buffer to file \"%s\"\n",optarg);
			}
		        size = sizeof(idrec_t) + sizeof(trailrec_t) + sizeof(struct utsname);
			if ((ap = (char *)malloc(size)) == NULL) {
				close(fd);
				error("abuf: unable to malloc space for header record\n");
			}

			(void)memset(ap,'\0',size);
		
			/* write out audit log header record info */
			wap=ap;
			id=(idrec_t *)wap;
			id->cmn.c_rtype = id->cmn.c_event = A_FILEID;
		
			if (!Alog && !(Alog = symsrch("adt_logctl")))
				error("audit log structure not found in symbol table\n");
			readmem(Alog->n_value, 1,-1,
				(char *)&adtlog,sizeof(adtlog),"audit log control structure");
			id->cmn.c_seqnm=adtlog.a_seqnum;
			strncpy(id->spec.i_mmp, adtlog.a_mmp, ADT_DATESZ);
			strncpy(id->spec.i_ddp, adtlog.a_ddp, ADT_DATESZ);
		
			id->cmn.c_pid = 0;
			id->cmn.c_time = 0;
			id->cmn.c_status = 0;
		
			if (!Amac && !(Amac = symsrch("mac_installed")))
				error("MAC installed flag not found in symbol table\n");
			readmem(Amac->n_value, 1,-1,
				(char *)&mac,sizeof(int),"MAC installed flag");
			id->spec.i_mac = mac;
		
			if (!Actl && !(Actl = symsrch("adt_ctl")))
				error("audit control structure not found in symbol table\n");
			readmem(Actl->n_value, 1,-1,
				(char *)&adtctl,sizeof(adtctl),"audit control structure");
			strncpy(id->spec.i_aver, adtctl.a_version, ADT_VERLEN);
			wap+=sizeof(idrec_t);
		
			if (!Utsnm && !(Utsnm = symsrch("utsname")))
				error("utsname not found in symbol table\n");
			readmem(Utsnm->n_value, 1,-1,
				(char *)&utsname,sizeof(utsname),"utsname structure");
			ss=strlen(utsname.sysname);
			strcpy(wap,utsname.sysname);
			wap+=ss;
			*wap=*sp;
			wap++;
			ts+=ss+1;

			ss=strlen(utsname.nodename);
			strcpy(wap,utsname.nodename);
			wap+=ss;
			*wap=*sp;
			wap++;
			ts+=ss+1;

			ss=strlen(utsname.release);
			strcpy(wap,utsname.release);
			wap+=ss;
			*wap=*sp;
			wap++;
			ts+=ss+1;

			ss=strlen(utsname.version);
			strcpy(wap,utsname.version);
			wap+=ss;
			*wap=*sp;
			wap++;
			ts+=ss+1;

			ss=strlen(utsname.machine);
			strcpy(wap,utsname.machine);
			wap+=ss;
			*wap='\0';
			wap++;
			ts+=ss+1;

			rs=ROUND2WORD(ts);
			t=(trailrec_t*)(ap + rs + sizeof(idrec_t));
			t->t_rtype = t->t_event = A_FILEID;
			id->cmn.c_size = t->t_size = (sizeof(idrec_t)+rs+sizeof(trailrec_t));	
			if ((write(fd,ap,id->cmn.c_size)) != id->cmn.c_size) {
				close(fd);
				error("error writing buffer to file \"%s\"\n",optarg);
			}
		}else 
			error("error opening file \"%s\"\n",optarg);
	}else 
              		error("abuf: file \"%s\" already exists, try another!\n",optarg);
}


