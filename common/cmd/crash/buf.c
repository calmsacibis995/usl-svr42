/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)crash:common/cmd/crash/buf.c	1.13.1.3"
/*
 * This file contains code for the crash functions: bufhdr, buffer, od.
 */

#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5inode.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/ino.h>
#include <sys/buf.h>
#include "crash.h"

#define BSZ  1		/* byte size */
#define SSZ  2		/* short size */
#define LSZ  4		/* long size */

#define DATE_FMT	"%a %b %e %H:%M:%S %Y\n"
/*
 * 	%a	abbreviated weekday name
 *	%b	abbreviated month name
 *	%e	day of month
 *	%H	hour
 *	%M	minute
 *	%S	second
 *	%Y	year
 */
#define SBUFSIZE	2048
#define INOPB		SBUFSIZE/sizeof(struct dinode)


static struct syment *Hbuf;	/* namelist symbol pointer */
static struct syment *Nbuf;	/* namelist symbol pointer */
static struct syment *Pageio;
static char bformat = 'x';	/* buffer format */
static int type = LSZ;		/* od type */
static char mode = 'x';		/* od mode */
char buffer[SBUFSIZE];		/* buffer buffer */
static  char time_buf[50];	/* holds date and time string */
struct	buf bbuf;		/* used by buffer for bufhdr */


int
getbufhdr()
{
	int full = 0;
	int phys = 1;
	long addr = -1;
	int nbuf;
	struct hbuf hbuf;
	int c;
	int i;
	struct buf *bufp,bufdata;
	char *heading = "BUF      MAJ/MIN    BLOCK    ADDRESS \n\tFOR      BCK      AVF      AVB      FLAGS\n";

	if(!Hbuf)
		if(!(Hbuf = symsrch("hbuf")))
			error("hbuf not found in symbol table\n");

	if(!Nbuf)
		if(!(Nbuf = symsrch("nbuf")))
			error("nbuf not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 0;
					break;
			default  :	longjmp(syn,0);
		}
	}
	readmem(Nbuf->n_value,1,-1,&nbuf,sizeof(nbuf),"nbuf");
	fprintf(fp,"NUMBER OF bio BUFFER HEADERS ALLOCATED= %d\n",nbuf);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) 
		do {
			if((bufp = (struct buf *)strcon(args[optind],'h')) == (struct buf *)-1)
				continue;
			readmem(bufp,phys,-1,&bufdata,sizeof(bufdata),"buf data");
			prbufhdr(full,bufp,bufdata,heading);
		}while(args[++optind]);
	else {
		for(i = 0; i < vbuf.v_hbuf; i++ ) {
		long hbuf_addr = Hbuf->n_value + i * sizeof(hbuf);
		readmem(hbuf_addr,1,-1,&hbuf,sizeof(hbuf),"hbuf");
		for( bufp = hbuf.b_forw; (long)bufp != hbuf_addr; bufp = bufdata.b_forw ) {
			readmem(bufp,1,-1,&bufdata,sizeof(bufdata),"buf data");
			prbufhdr(full,bufp,bufdata,heading);
		}
		}
		fprintf(fp,"pageio_out BUFFERS\n");
		if(!(Pageio = symsrch("pageio_out")))
			return;

		readmem(Pageio->n_value,1,-1,&hbuf,sizeof(hbuf),"pageio_out head");
		for( bufp = hbuf.b_forw; (long)bufp != Pageio->n_value; bufp = bufdata.b_forw ) {
			readmem(bufp,1,-1,&bufdata,sizeof(bufdata),"pageio_out buf data");
			prbufhdr(full,bufp,bufdata,heading);
		}
	}


}


/* print buffer headers */
int
prbufhdr(full,bufaddr,bhbuf,heading)
int full;
long bufaddr;
struct buf bhbuf;
char *heading;
{
	register int b_flags;
	int procslot,forw,back,avf,avb,fforw,fback;

	if(full)
		fprintf(fp,"%s",heading);
	fprintf(fp,"%8x",bufaddr);
	if( bhbuf.b_dev != (short)0xffff ) {
		fprintf(fp," %4u,%-5u %-8x %8x\n",
			getemajor(expdev(bhbuf.b_dev)),
			geteminor(expdev(bhbuf.b_dev)),
			bhbuf.b_blkno,
			bhbuf.b_un.b_addr);
	} else {
		fprintf(fp," %4u,%-5u %-8x %8x\n",
			getemajor(bhbuf.b_edev)&L_MAXMAJ,
			geteminor(bhbuf.b_edev),
			bhbuf.b_blkno,
			bhbuf.b_un.b_addr);
	}
	fprintf(fp,"\t%8x",bhbuf.b_forw);
	fprintf(fp," %8x",bhbuf.b_back);
	if(!(bhbuf.b_flags & B_BUSY)) {
		fprintf(fp," %8x",bhbuf.av_forw);
		fprintf(fp," %8x",bhbuf.av_back);
	}
	else fprintf(fp," -         -       ");
	b_flags = bhbuf.b_flags;
	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		b_flags == B_WRITE ? " write" : "",
		b_flags & B_READ ? " read" : "",
		b_flags & B_DONE ? " done" : "",
		b_flags & B_ERROR ? " error" : "",
		b_flags & B_BUSY ? " busy" : "",
		b_flags & B_PHYS ? " phys" : "",
		b_flags & B_MAP ? " map" : "",
		b_flags & B_WANTED ? " wanted" : "",
		b_flags & B_AGE ? " age" : "",
		b_flags & B_ASYNC ? " async" : "",
		b_flags & B_DELWRI ? " delwri" : "",
		b_flags & B_OPEN ? " open" : "",
		b_flags & B_STALE ? " stale" : "",
		b_flags & B_VERIFY ? " verify" : "",
		b_flags & B_FORMAT ? " format" : "");
	if(full) {
		fprintf(fp,"\tBCNT ERR RESI  START  PROC  RELTIME VP\n");
		fprintf(fp,"\t%4d %3d %4d %8x",
			bhbuf.b_bcount,
			bhbuf.b_error,
			bhbuf.b_resid,
			bhbuf.b_start);
		procslot = proc_to_slot((long)bhbuf.b_proc);
		if (procslot == -1)
			fprintf(fp,"  - ");
		else
			fprintf(fp," %4d",procslot);
		fprintf(fp," %8x",bhbuf.b_reltime);
		fprintf(fp," %8x\n",bhbuf.b_vp);
		fprintf(fp,"\n");
	}
}

/* get arguments for buffer function */
int
getbuffer()
{
	int phys = 0;
	int fflag = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;


	optind = 1;
	while((c = getopt(argcnt,args,"bcdrxiopw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'b' :	bformat = 'b';
					fflag++;
					break;
			case 'c' :	bformat = 'c';
					fflag++;
					break;
			case 'd' :	bformat = 'd';
					fflag++;
					break;
			case 'x' :	bformat = 'x';
					fflag++;
					break;
			case 'i' :	bformat = 'i';
					fflag++;
					break;
			case 'o' :	bformat = 'o';
					fflag++;
					break;
			default  :	longjmp(syn,0);
					break;
		}
	}
	if(fflag > 1)
		longjmp(syn,0);
	if(args[optind]) {
		getargs(vbuf.v_buf,&arg1,&arg2);
		if(arg1 != -1) {
			addr = arg1;
			prbuffer(phys,addr);
		}
	}
	else longjmp(syn,0);
}


/* print buffer */
int
prbuffer(phys,addr)
int phys;
long addr;
{

	readmem(addr,!phys,-1,
		(char *)&bbuf,sizeof bbuf,"buffer");
	readmem((long)bbuf.b_un.b_addr,1,-1,(char *)buffer,
		sizeof buffer,"buffer");
	fprintf(fp,"BUFFER %x:  \n", addr);
	switch(bformat) {
		case 'b' :	prbalpha(bbuf.b_bufsize);
				break;
		case 'c' :	prbalpha(bbuf.b_bufsize);
				break;
		case 'd' :	prbnum(bbuf.b_bufsize);
				break;
		case 'x' :	prbnum(bbuf.b_bufsize);
				break;
		case 'i' :	prbinode(bbuf.b_bufsize);
				break;
		case 'o' :	prbnum(bbuf.b_bufsize);
				break;
		default  :	error("unknown format\n");
				break;
	}
}

/* print buffer in numerical format */
int
prbnum(size)
int size;
{
	int *ip,i;

	/*
	for(i = 0, ip=(int *)buffer; ip !=(int *)&buffer[SBUFSIZE]; i++, ip++) {
	*/
	for(i = 0, ip=(int *)buffer; i < size; i++, ip++) {
		if(i % 4 == 0)
			fprintf(fp,"\n%5.5x:\t", i*4);
		fprintf(fp,bformat == 'o'? " %11.11o" :
			bformat == 'd'? " %10.10u" : " %8.8x", *ip);
	}
	fprintf(fp,"\n");
}


/* print buffer in character format */
int
prbalpha(size)
int size;
{
	char *cp;
	int i;

	for(i=0, cp = buffer; i < size; i++, cp++) {
		if(i % (bformat == 'c' ? 16 : 8) == 0)
			fprintf(fp,"\n%5.5x:\t", i);
		if(bformat == 'c') putch(*cp);
		else fprintf(fp," %4.4o", *cp & 0377);
	}
	fprintf(fp,"\n");
}


/* print buffer in inode format */
int
prbinode(size)
int size;
{
	struct	dinode	*dip;
	long	_3to4();
	int	i,j;

	for(i=1,dip = (struct dinode *)buffer;i <
		 size; i++, dip++) {
	fprintf(fp,"\ni#: %ld  md: ", (bbuf.b_blkno - 2) *
		INOPB + i);
		switch(dip->di_mode & IFMT) {
		case IFCHR: fprintf(fp,"c"); break;
		case IFBLK: fprintf(fp,"b"); break;
		case IFDIR: fprintf(fp,"d"); break;
		case IFREG: fprintf(fp,"f"); break;
		case IFIFO: fprintf(fp,"p"); break;
		default:    fprintf(fp,"-"); break;
		}
		fprintf(fp,"\n%s%s%s%3x",
			dip->di_mode & ISUID ? "u" : "-",
			dip->di_mode & ISGID ? "g" : "-",
			dip->di_mode & ISVTX ? "t" : "-",
			dip->di_mode & 0777);
		fprintf(fp,"  ln: %u  uid: %u  gid: %u  sz: %ld",
			dip->di_nlink, dip->di_uid,
			dip->di_gid, dip->di_size);
		if((dip->di_mode & IFMT) == IFCHR ||
			(dip->di_mode & IFMT) == IFBLK ||
			(dip->di_mode & IFMT) == IFIFO)
			fprintf(fp,"\nmaj: %d  min: %1.1o\n",
				dip->di_addr[0] & 0377,
				dip->di_addr[1] & 0377);
		else
			for(j = 0; j < NADDR; j++) {
				if(j % 7 == 0)
					fprintf(fp,"\n");
				fprintf(fp,"a%d: %ld  ", j,
					_3to4(&dip->di_addr[3 * j]));
			}
	
		cftime(time_buf, DATE_FMT, &dip->di_atime); 
		fprintf(fp,"\nat: %s", time_buf);
		cftime(time_buf, DATE_FMT, &dip->di_mtime); 
		fprintf(fp,"mt: %s", time_buf);
		cftime(time_buf, DATE_FMT, &dip->di_ctime); 
		fprintf(fp,"ct: %s", time_buf);
	}
	fprintf(fp,"\n");
}



/* covert 3 byte disk block address to 4 byte address */
long
_3to4(ptr)
register  char  *ptr;
{
	long retval;
	register  char  *vptr;

	vptr = (char *)&retval;
	*vptr++ = 0;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	return(retval);
}


/* get arguments for od function */
int
getod()
{
	int phys = 0;
	int count = 1;
	int proc = Procslot;
	long addr = -1;
	int c;
	struct syment *sp;
	int typeflag = 0;
	int modeflag = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"tlxcbdohapw:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			case 'p' :	phys = 1;
					break;
			case 'c' :	mode = 'c';
					if (!typeflag)
						type = BSZ;
					modeflag++;
					break;
			case 'a' :	mode = 'a';
					if (!typeflag)
						type = BSZ;
					modeflag++;
					break;
			case 'x' :	mode = 'x';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'd' :	mode = 'd';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'o' :	mode = 'o';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'h' :	mode = 'h';
					type = LSZ;
					typeflag++;
					modeflag++;
					break;
			case 'b' :	type = BSZ;
					typeflag++;
					break;
			case 't' :	type = SSZ;
					typeflag++;
					break;
			case 'l' :	type = LSZ;
					typeflag++;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(typeflag > 1) 
		error("only one type may be specified:  b, t, or l\n");	
	if(modeflag > 1) 
		error("only one mode may be specified:  a, c, o, d, or x\n");	
	if(args[optind]) {
		if(*args[optind] == '(') 
			addr = eval(++args[optind]);
		else if(sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if((addr = strcon(args[optind],'h')) == -1)
			error("%s not found in symbol table\n",args[optind]);
		if(args[++optind]) 
			if((count = strcon(args[optind],'d')) == -1)
				error("\n");
		prod(addr,count,phys,proc);
	}
	else longjmp(syn,0);
}




/* print dump */
int
prod(addr,count,phys,proc)
long addr;
int count,phys,proc;
{
	int i,j;
	char ch;
	unsigned short shnum;
	long lnum;
	long value;
	char *format;
	int precision;
	char hexchar[16];
	char *cp;
	int nbytes;
	unsigned long raddr;

#ifdef ALIGN_ADDR
	switch(type) {
		case LSZ :  	if(addr & 0x3) {	/* word alignment */
		    			fprintf(fp,"warning: word alignment performed\n");
		    			addr &= ~0x3;
			 	}
		case SSZ :  	if(addr & 0x1) {	/* word alignment */
		    			fprintf(fp,"warning: word alignment performed\n");
		    			addr &= ~0x1;
			 	}
	}
#endif
	if(mode == 'h') {
		cp = hexchar;
		nbytes = 0;
	}
	for(i = 0; i < count; i++) {
		switch(type) {
			case BSZ :  
				readmem(addr,!phys,proc,&ch,
					sizeof(ch),"buffer");
			        value = ch & 0377;
				    break;
			case SSZ : 
				readmem(addr,!phys,proc,&shnum,
					sizeof(shnum),"buffer");
				    value = shnum;
				    break;	
			case LSZ :
				readmem(addr,!phys,proc,&lnum,
					sizeof(lnum),"buffer");
				    value = lnum;
				    break;
		}
		if(((mode == 'c') && ((i % 16) == 0)) ||
			((mode != 'a') && (mode != 'c') && (i % 4 == 0))) {
				if(i != 0) {
					if(mode == 'h') {
						fprintf(fp,"   ");
						for(j = 0; j < nbytes; j++) {
							if(hexchar[j] < 040 ||
							hexchar[j] > 0176)
								fprintf(fp,".");
							else fprintf(fp,"%c",
								hexchar[j]);
						}
						cp = hexchar;
						nbytes = 0;
					}
					fprintf(fp,"\n");
				}
				fprintf(fp,"%8.8x:  ", addr);
			}
		switch(mode) {
			case 'a' :  switch(type) {
					case BSZ :  putc(ch,fp);
						    break;
					case SSZ :  putc((char)shnum,fp);
						    break;
					case LSZ :  putc((char)lnum,fp);
						    break;
				    }
				    break;
			case 'c' :  switch(type) {
					case BSZ :  putch(ch);
						    break;
					case SSZ :  putch((char)shnum);
						    break;
					case LSZ :  putch((char)lnum);
						    break;
				    }
				    break;
			case 'o' :  format = "%.*o   ";
				    switch(type) {
					case BSZ :  precision = 3;
						    break;
					case SSZ :  precision = 6;
						    break;
					case LSZ :  precision = 11;
						    break;
			   		}
			 	    fprintf(fp,format,precision,value);
			 	    break;
			case 'd' :  format = "%.*d   ";
				    switch(type) {
					case BSZ :  precision = 3;
						    break;
					case SSZ :  precision = 5;
						    break;
					case LSZ :  precision = 10;
						    break;
				    }
			 	    fprintf(fp,format,precision,value);
			   	    break;
			case 'x' :  format = "%.*x   ";
				    switch(type) {
					case BSZ :  precision = 2;
						    break;
					case SSZ :  precision = 4;
						    break;
					case LSZ :  precision = 8;
						    break;
				    }
			 	    fprintf(fp,format,precision,value);
				    break;
			case 'h' :  fprintf(fp,"%.*x   ",8,value);
				    *((long *)cp) = value;
				    cp +=4;
				    nbytes += 4;
				    break;
		}
		addr += type;
	}
	if(mode == 'h') {
		if(i % 4 != 0)  
			for(j = 0; (j+(i%4)) < 4; j++)
				fprintf(fp,"           ");
		fprintf(fp,"   ");
		for(j = 0; j < nbytes; j++) 
			if(hexchar[j] < 040 || hexchar[j] > 0176)
				fprintf(fp,".");
			else fprintf(fp,"%c",hexchar[j]);
	}
	fprintf(fp,"\n");
}
