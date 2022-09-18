/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/f3.h	1.1"
/*
 * @(#)f3.h 1.5 89/05/03
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

/*
 *  f3.h
 */

/*
 *      F3 operator address 
 */
#define f3_ADDADDX               0
#define f3_SUBADDX               1
#define f3_MULADDX               2
#define f3_DIVADDX               3
#define f3_SQRTADDX              4
#define f3_NEGADDX               5
#define f3_ABSADDX               6
#define f3_MINADDX               7
#define f3_MAXADDX               8

#define f3_CEILADDX              9
#define f3_FLOORADDX            10
#define f3_TRUNCADDX            11
#define f3_ROUNDADDX            12

#define f3_SINADDX              13
#define f3_COSADDX              14
#define f3_TANADDX              15
#define f3_ASINADDX             16
#define f3_ACOSADDX             17
#define f3_ATAN2ADDX            18

#define f3_GTADDX               19
#define f3_GEADDX               20
#define f3_EQADDX               21
#define f3_NEADDX               22
#define f3_LTADDX               23
#define f3_LEADDX               24

#define f3_IFADDX               25
#define f3_IFELSEADDX           26
#define f3_FOREVERADDX          27
#define f3_BREAKADDX            28
#define f3_RETURNADDX           29

#define f3_DUPADDX              30
#define f3_CONSTANTADDX         31

#define f3_ASSIGNADDX           32
#define f3_VALUEOFADDX          33

#define f3_ARRAYADDX            36
#define f3_INDEXADDX            37
#define f3_CATADDX              38
#define f3_SUBARRAYADDX         39

#define f3_TRANSFORMADDX        40
#define f3_PIXRATIOADDX         41

#define f3_POPADDX              42
#define f3_SIZEOFADDX           43
#define f3_STACKDEPTHADDX       44
#define f3_ROLLADDX             45
#define f3_BOXADDX              46
#define	f3_CALLSYMADDX          47
#define	f3_EXCHANGEADDX         48
#define	f3_SLOPEADDX            49
#define	f3_INVERTADDX           50

#define f3_NOOPADDX             63



/*
 *  A hard limit on the reserved area for predefined procedures.
 */
#define f3_RESERVEDAREALIMIT    64


/*
 *  Constant area size
 */
#define f3_MAXSTACKSIZE         100
#define f3_HASHTBLSIZE          256

/*
 *  Constants for the curve types.
 */
#define f3_NOTACURVE            -1
#define f3_LINETOKEN            0x0
#define f3_CONICTOKEN           0x1
#define f3_BEZIERTOKEN          0x2



