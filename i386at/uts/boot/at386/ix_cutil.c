/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/ix_cutil.c	1.2"
#ident	"$Header: $"

#include "util/types.h"
#include "util/param.h"
#include "io/cram/cram.h"
#include "util/inline.h"
#include "svc/bootinfo.h"

#include "boot/boot.h"
#include "boot/bootlink.h"
#include "boot/s51kconf.h"

extern	paddr_t bt_resv_base;
extern	short 	fd_adapt_lev;
extern 	ushort	bootdriv;
extern	short	bps;		/* bytes per sector */
extern	short	spt;		/* disk sectors per track */
extern	short	spc;		/* disk sectors per cylinder */
extern 	struct 	bootenv bootenv;

unsigned char
CMOSread(loc)
int loc;
{
	outb(CMOS_ADDR, loc);	/* request cmos address */
	return(inb(CMOS_DATA));	/* return contents      */
}

/*
 *  Copy s2 to s1, return s1
 */

char *
strcpy(s1, s2)
register char *s1, *s2;
{
	char *os1 = s1;

	while ((*s1++ = *s2++) != '\0')
		;
	return (os1);
}

char *
memset(dest, c, cnt)
register char *dest;
register unsigned char c;
register size_t cnt;
{
	char *odest = dest;

	while ( cnt-- > 0)
		*dest++ = c;
	return (odest);
}

memcmp(ref, cmp, cnt)
register char *ref, *cmp;
register size_t	cnt;
{
	while ((*ref++ == *cmp++) && (--cnt > 0)) ;
	return((int)(*--ref & 0xff)-(int)(*--cmp & 0xff));
}

char *
strcat(dest, sorc)
register char *dest, *sorc;
{
	char	*od = dest;

	while (*dest++ != '\0') ;
	dest--;
	while ((*dest++ = *sorc++) != '\0') ;
	return(od);
}

unsigned char
getchar()
{
	struct int_pb	ic;

	ic.intval = 0x16;
	for (ic.ax = 0; ic.ax == 0; )

		doint(&ic);

	return(ic.ax & 0xFF);
}

void
goany()
{
	char	c;
	printf("Press ENTER to continue");
	c = getchar();
	putchar('\r');
	putchar('\n');
}
#ifdef BOOT_DEBUG2
extern	int	dread_cnt;
#endif

disk(startsect, destination, num_sects)
int startsect;
paddr_t destination;
short num_sects;
{
	register int move;
	register short cyl;
	short	head;
	short	sector;
	short	retry = 0;
	short	sec_xfer;
	struct 	int_pb ic;

#ifdef BOOT_DEBUG2
	dread_cnt++;
#endif

#ifdef BOOT_DEBUG2
	printf("disk: sect %x, to %x, %x sectors \n",startsect,
			destination, num_sects);
#endif

	sec_xfer = num_sects;
/*	if floppy read and error exceeds adaptive level,
 *	convert to single sector read.
 */
	if (!(bootdriv) && (fd_adapt_lev > FD_ADAPT_LEV))
		sec_xfer = 1;

/*	read until all requests are satisfied				*/
	ic.intval = 0x13;
	for(;;) {
		cyl = startsect / spc;
		move = startsect - (cyl * spc);
		head = move / spt;
		sector = move - (head * spt) + 1; /* convert to one based */
		ic.es = destination >> 4;
		ic.bx = destination & 0xF;
		ic.ax = 0x200 + sec_xfer;
		ic.dx = (head << 8) + bootdriv;
		ic.cx = (cyl & 0xFF) << 8;
		ic.cx |= (((cyl >> 2) & 0xC0) | (sector & 0x3F));

/*		if disk read error, retry or abort			*/
		if ((doint(&ic)) && (((ic.ax >> 8) & 0xFF) != ECC_COR_ERR)) {
			if (retry++ > RD_RETRY) {
/*				if we are using track read from floppy,
 *				then revert back to sector read
 */
				if (!(bootdriv) && sec_xfer != 1) {
					printf("disk: BIOS does not support track read; convert to single sector read\n");
					sec_xfer = 1;
					retry = 0;
					fd_adapt_lev++;
				} else {
					printf("disk: read error\n"); 
					bootabort();
				}
			} 
		} else {
/*			if all transferred, then stop			*/
			if (!(num_sects -= sec_xfer))
				return;
			startsect += sec_xfer;
			destination += (sec_xfer * bps);
		}
	}
}


shomem(used,idm,cnt,bmp)
char *idm;
int  cnt, used;
struct bootmem *bmp;
{
	int i;

	printf("%s %d\n",idm,cnt);
	for (i = 0; i < cnt; i++, bmp++) {
		printf("%d %x %x %x",i,bmp->base,bmp->extent,bmp->flags);
		if (used)
			printf(" %x\n", bootenv.sectaddr[i]);
		else
			printf("\n");
	}
	goany();
}


#ifdef BOOT_DEBUG
#define DH -16

extern	printn();

bdump(dptr,cnt)
char *dptr;
int   cnt;
{
	int	lctr = 20;
	long	*cdwptr;
	char	c;
	int	i;

	dptr = (char *)((unsigned long)dptr & (unsigned long)0xfffffff0);
	cdwptr = (long *)dptr;
	printf("Dumping %x for %d bytes\n",dptr,cnt);
	for ( ;cnt > 0; cnt-=16 ) {
		printn( (long)cdwptr, DH);
		for (i=0; i<4; i++) {
			putchar(' ');
			putchar(' ');
			printn(*cdwptr++, DH);
		}
		putchar(' ');
		putchar(' ');
		for (i=0; i<16;i++) {
			c = *dptr++;
			if (c < 0x21 || c > 0x7e)
				putchar('.');
			else
				putchar(c);
		}
		putchar('\r');
		putchar('\n');
		if (--lctr == 0) {
			goany();
			lctr = 20;
		}
	}
	goany();
}
#endif


/*
 *	Allocate ram from the top of the base memory
 */
bt_malloc(cnt)
int	cnt;
{
	if (!bootenv.bf_resv_base)
		bootenv.bf_resv_base = (1024 * MEM_BASE());

	bootenv.bf_resv_base -= (cnt+16);
	return(bootenv.bf_resv_base);

}

/*
 *	testing memory at two different locations to determine
 *	whether the physical lines are wrapped around
 */
memwrap(memsrt, memoff)
ulong	memsrt;
ulong	memoff;
{
	ulong	*ta;
	ulong	*ta_wrap;
	ulong	save_val;
	int	mystatus = 1;	/* assume memory is wrap around		*/

	ta = (ulong *)memsrt;
	ta_wrap = (ulong *)(memsrt-memoff);
	*ta = MEMTEST1;
	if ((*ta == MEMTEST1) && (*ta != *ta_wrap)) {
		*ta = MEMTEST2;
		if ((*ta == MEMTEST2) && (*ta != *ta_wrap))
			mystatus = 0;
	}

#ifdef BOOT_DEBUG
	printf("memwrap: ta= 0x%x ta_wrap= 0x%x memory %s\n", ta, ta_wrap, (mystatus? "WRAP": "NO WRAP"));
#endif

	return(mystatus);
}
