/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:net/lockmgr/klm.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>

int udp_no = UDP_CMAJOR_0;
int clone_no = CLN_CMAJOR_0;

/*
 * time to wait for lack of resources on server
 */
int backoff_timeout = 30;

/*
 * first attempt if klm port# known 
 */
int first_retry = 1;

/*
 * attempts after new port number obtained
 */
int normal_retry = 1;

/*
 * normal timeout
 */
int normal_timeout = 8;

/*
 * attempts after working reply recieved
 */
int working_retry = 3;

/*
 * timeout of rpc call to lockmgr
 * after working reply recieved
 */
int working_timeout = 3;
