/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)spell:hashlook.c	1.8.1.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/spell/hashlook.c,v 1.1 91/02/28 20:10:09 ccs Exp $"
#include <stdio.h>
#include "hash.h"
#include "huff.h"

unsigned *table;
int index[NI];

#define B (BYTE*sizeof(unsigned))
#define L (BYTE*sizeof(long)-1)
#define MASK (~(1L<<L))

#define fetch(wp,bp) ((wp[0]<<(B-bp))|(wp[1]>>bp))

hashlook(s)
char *s;
{
	long h;
	long t;
	register bp;
	register unsigned *wp;
	int i;
	long sum;
	unsigned *tp;

	h = hash(s);
	t = h>>(HASHWIDTH-INDEXWIDTH);
	wp = &table[index[t]];
	tp = &table[index[t+1]];
	bp = B;
	sum = (long)t<<(HASHWIDTH-INDEXWIDTH);
	for(;;) {
		{/*	this block is equivalent to
			 bp -= decode((fetch(wp,bp)>>1)&MASK, &t);*/
			long y;
			long v;

			if (bp == 0)
				y = 0;
			else
				y = wp[0] << (B - bp);
			if (bp < 32)
				y |= (wp[1] >> bp);
			y = (y >> 1) & MASK;

			if(y < cs) {
				t = y >> (L+1-w);
				bp -= w-1;
			}
			else {
				for(bp-=w,v=v0; y>=qcs; y=(y<<1)&MASK,v+=n)
					bp -= 1;
				t = v + (y>>(L-w));
			}
		}
		while(bp<=0) {
			bp += B;
			wp++;
		}
		if(wp>=tp&&(wp>tp||bp<B))
			return(0);
		sum += t;
		if(sum<h)
			continue;
		return(sum==h);
	}
}


prime(argc,argv)
char **argv;
{
	register FILE *f;
	register fd;
	extern char *malloc();
	if(argc <= 1)
		return(0);

	if((f = fopen(argv[1], "ri")) == NULL)
		return(0);
	if(rhuff(f)==0
	|| fread((char*)index, sizeof(*index),  NI, f) != NI
	|| (table = (unsigned*)malloc(index[NI-1]*sizeof(*table))) == 0
	|| fread((char*)table, sizeof(*table), index[NI-1], f)
	   != index[NI-1])
		return(0);
	fclose(f);

	hashinit();
	return(1);
}
