/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/i386/Reg1.h	1.4"

//
// NAME
//	Reg1.h
//
// ABSTRACT
//	Register names and indices (machine dependent)
//
// DESCRIPTION
//	Each machine has its own set of register names.  This header file
//	defines the names for the i386, and 80387 co-processor registers.
//  It is included by "Reg.h"
//

#ifndef REG1_H
#define REG1_H

//
//	General Purpose registers 
//	(enumerated according to the Intel reference manual pg. 17-5)
//
#define REG_EAX  0
#define REG_ECX  1
#define REG_EDX  2
#define REG_EBX  3
#define REG_ESI  6
#define REG_EDI  7
//
//	Stack register
//
#define REG_ESP  4
#define REG_EBP  5
//
//	Instruction Pointer register
//
#define REG_EIP     8
//
//	Flags Register
//
#define REG_EFLAGS  9
#define REG_TRAPNO  10
//
//	80387 Floating Point Registers
//	These registers are 80 bit registers
//
#define FP_REG	11
#define FP_ST0	11	// identical to FP_REG
#define FP_ST1	12
#define FP_ST2	13
#define FP_ST3	14
#define FP_ST4	15
#define FP_ST5	16
#define FP_ST6	17
#define FP_ST7	18
//
//	80387 Floating point special registers
//	FP_CW	Floating Point Control Register
//	FP_SW	Floating Point Status Register
//	FP_TW	Floating Point Tag Register
//	FP_IP	Floating PointInstruction Pointer
//	FP_DP	Floating Point Data Pointer
//
#define FP_CTL	19
#define FP_SW	19	// identical to FP_CTL
#define FP_CW	20
#define FP_TW	21
#define FP_IP	22
#define FP_DP	23

#define REG_END	24
// 
//  synonyms
//
#define REG_PC REG_EIP
#define REG_FP REG_EBP
#define REG_AP REG_EBP
//
// FP_STACK index in regs[]
//
#define FP_INDEX	FP_REG
#define FP_STACK	REG_END

#endif  /*REG1_H */
