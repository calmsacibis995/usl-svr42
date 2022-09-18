/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*      Copyright (c) 1991 Intel Corporation    */
/*      All Rights Reserved     */

/*      INTEL CORPORATION PROPRIETARY INFORMATION       */

/*      This software is supplied to AT & T under the terms of a license   */
/*      agreement with Intel Corporation and may not be copied nor         */
/*      disclosed except in accordance with the terms of that agreement.   */

#ident	"@(#)uts-x86at:io/asy/asy.h	1.11"
#ident	"$Header: $"

/*
 * Defines for ioctl calls (VP/ix)
 */

#define AIOC			('A'<<8)
#define AIOCINTTYPE		(AIOC|60)	/* set interrupt type */
#define AIOCDOSMODE		(AIOC|61)	/* set DOS mode */
#define AIOCNONDOSMODE	(AIOC|62)	/* reset DOS mode */
#define AIOCSERIALOUT	(AIOC|63)	/* serial device data write */
#define AIOCSERIALIN	(AIOC|64)	/* serial device data read */
#define AIOCSETSS		(AIOC|65)	/* set start/stop chars */
#define AIOCINFO		(AIOC|66)	/* tell usr what device we are */

/* Ioctl alternate names used by VP/ix */
#define VPC_SERIAL_DOS		AIOCDOSMODE	
#define VPC_SERIAL_NONDOS	AIOCNONDOSMODE
#define VPC_SERIAL_INFO		AIOCINFO
#define VPC_SERIAL_OUT		AIOCSERIALOUT
#define VPC_SERIAL_IN		AIOCSERIALIN

/* Defines for MERGE ioctl */
#define	COMPPIIOCTL		(AIOC|67)	/* Do com_ppiioctl() */

