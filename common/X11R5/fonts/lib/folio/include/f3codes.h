/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/f3codes.h	1.1"
/*
 * @(#)f3codes.h 1.2 89/03/10
 *
 */
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


/*  	BYTE ENCODING

    	All F3 tokens have an escaped form consisting of an escape byte followed
    	by data bytes. Generally speaking, the escape byte denotes the type of
    	data and the data bytes define a particular instance. Byte values 101 to
    	127 are used as escapes (not all of them are assigned, though). Because
    	some types of tokens are deemed to occur very frequently, byte values 0
    	to 100 and 128 to 255 are used to encode some particular instances of these
    	tokens in just one byte:

    	000 - 100   Very Small Integers
    	128 - 191   Short Address References; these values have the form <10aaaaaa>;
    	    	    the <a> field describes a global address in the range 0-55 (a<=55)
    	    	    or a local address displacement (with respect to the local stack
    	    	    pointer) in the range 0-7 (a-55).
    	192 - 255   Short Quoted Addresses; these values have the form <11aaaaaa>;
    	    	    the <a> field encodes global or local addresses in the same way
    	    	    as before.
 */
#define	f3_BYTE_IE_OR_SA(b) 	((b)&0x80)
#define	f3_IE	    	    	0
#define	f3_SA	    	    	0x80

#define f3_MAX_SADDX            56
#define f3_MAX_SLADDX           7
#define f3_SAR_ESC              0x80
#define f3_SQA_ESC              0xC0

#define	f3_VSINT_MAX	    	100
#define	f3_BYTE_AR_OR_QA(b)    	((b)&0x40)
#define	f3_AR	    	    	0
#define	f3_QA	    	    	0x40
#define	f3_BYTE_ADDX_BITS(b)	((b)&0x3F)
#define	f3_ADDX_IS_GLOBAL(a)	((a)<=55)
#define	f3_ADDX_IS_LOCAL(a)    	((a)>55)
#define	f3_ADDX_LOCAL_DISP(a)	((a)-56)


/*************************************  ESCAPED TOKENS  ***************************/

    	    	    	    	    /*  NUMBERS  **********************************/
    	    	    	    	    /*                                            */
#define	f3_BINT_ESC    	    	101 /*	BYTE INTEGER	    <b>	    	          */
    	    	    	    	    /*	value = b,  	0 <= b <= 255             */
#define	f3_WINT_ESC	    	102 /*	WORD INTEGER   	    <bb>   	          */
    	    	    	    	    /*	value = bb,	-2^15 <= bbb < 2^15       */
#define	f3_SREAL_ESC   	    	103 /*  SMALL REAL  	    <bb>    	          */
    	    	    	    	    /*	value = bb/1024, -32 <= v < 32	          */
#define	f3_FREAL_ESC   	    	104 /*	FRACT REAL   	    <bbbb>                */
    	    	    	    	    /*	value = bbbb/2^16, -2^15 <= v < 2^15	  */
#define	f3_MAXNUM	0x00007FFF
#define	f3_MINNUM	0xFFFF8000
#define	f3_MAXSREALFR	(0x00007FFF<<6)
#define	f3_MINSREALFR	(-f3_MAXSREALFR-1)


    	    	    	    	    /*  ARRAYS OF NUMBERS  ************************/
    	    	    	    	    /*                                            */
    	    	    	    	    /*  All immediate arrays begin with a 2 byte  */
    	    	    	    	    /*  <size>, (size < 2^15); then <size> array  */
    	    	    	    	    /*  elements follow, each one encoded as a    */
    	    	    	    	    /*	single number.	    	    	          */
    	    	    	    	    /*                                            */
#define	f3_BIARRAY_ESC 	    	105 /*	BINT ARRAY	 	<b>               */
#define	f3_WIARRAY_ESC	    	106 /*	WINT ARRAY	    	<bb>	          */
#define	f3_SRARRAY_ESC   	107 /*	SREAL ARRAY	    	<bb>	          */
#define	f3_FRARRAY_ESC		108 /*	FREAL ARRAY	    	<bbbb>            */

    	    	    	    	    /*  ADDRESS REFERENCES & QUOTED ADDRESSES  ****/
    	    	    	    	    /*	    	    	    	    	          */
#define	f3_GAR_ESC	    	109 /*	GLOBAL ADDRESS REFERENCE    <b>	          */
#define	f3_LAR_ESC	    	110 /*	LOCAL  ADDRESS REFERENCE    <b>	          */
#define	f3_GQA_ESC	    	111 /*	GLOBAL QUOTED ADDRESS	    <b>	          */
#define	f3_LQA_ESC	    	112 /*	LOCAL  QUOTED ADDRESS	    <b>	          */

    	    	    	    	    /*  OPERATOR DEFINITIONS  *********************/
    	    	    	    	    /*	    	    	    	    	          */
    	    	    	    	    /*  The byte immediately following the BEGIN  */
    	    	    	    	    /*	token is the number of local variables of */
    	    	    	    	    /*  the $preamble, $symbol or $proc operator  */
    	    	    	    	    /*  (0<=b<=127), or to denote a simple block  */
    	    	    	    	    /*	(b==128)    	    	    	          */
#define	f3_BEGINOP    	    	113 /*	BEGIN OPERATOR DEFINITION   	          */
#define	f3_ENDOP    	    	114 /*	END OPERATOR DEFINITION   	          */
#define	f3_SIMPLEBLOCK          0x80
#define	f3_BYTE_SIMPLEBLOCK(b)	((b)==0x80)


