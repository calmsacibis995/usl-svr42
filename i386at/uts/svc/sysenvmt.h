/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _SVC_SYSENVMT_H	/* wrapper symbol for kernel use */
#define _SVC_SYSENVMT_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:svc/sysenvmt.h	1.4"
#ident	"$Header: $"

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

#define MX_FTP		5	/* max font table entries */

/*
 *      Map structure for detected ROM BIOS found at 0xc8000 thru 0xE0000
 */

struct rom_map {
	paddr_t         start;
	unchar          size;   /* ROM size in 512 byte units */
	unchar          flag;
	unchar          mfgID;
	unchar          type;
};

#define MX_ROM_MAP	4	/* max rom map entries */

/*
 *	System Descriptor Table
 */
#pragma pack(1)

struct	sys_desc {
	short	len;		/* Table length in bytes */
	unchar	sd_model;
	unchar	sd_submodel;
	unchar	sd_BIOSrev;
	unchar	sd_feature1;
	unchar	sd_feature2;
	unchar	sd_feature3;
	unchar	sd_feature4;
	unchar	sd_feature5;
};
#pragma pack()

/*
 *      Machine definitions for machine field
 */

#define MPC_UNKNOWN	0
#define MPC_AT386	1	/* Generic AT 386 */
#define MPC_COMPAQ	2	/* Compaq */
#define MPC_ATT		4	/* AT&T 6386 WGS */
#define MPC_M380	6	/* Olivetti M380 Series */
#define MPC_DELL	7	/* Dell 386 machines except model 325 */
#define MPC_D325	8	/* Dell 386 model 325 */
#define MPC_ALR		9	/* Advanced Logic Research */
#define MPC_ZDS		10	/* Zenith Data Systems */
#define MPC_TOSHIBA	11	/* Toshoba Personal Computer */
#define MPC_NECpwrmate	12	/* NEC Powermate */
#define MPC_INTEL30X	13	/* Intel 300 series(recent); 301,302,303 */
#define MPC_I386PSA	14	/* Older(at least BIOS wise) 300 series */
#define MPC_MC386	0x81	/* Generic Micro Channel Machine */
#define MPC_PS2		0x82	/* IBM PS/2 */
#define MPC_APRICOT	0x83	/* Apricot Personal Computer */
#define MPC_OL_Px00	0x84	/* Olivetti Micro-channel P500 & P800 */

/*
 *	Machine model definitions. Us with machine type definitions above.
 */

/* OLIVETTI */

#define OL_M380		0x45	/* M380 including XP1 and XP3 */
#define OL_XP5		0xC5	/* M380 - XP5, tower with display */
#define OL_XP479	0x50	/* M380 - XP4/7/9 */
#define OL_P500		0x61	/* Micro-channel P500 */
#define OL_P800		0x62	/* Micro-channel P800 */

/* Compaq */
#define CPQ_SYSPRO	0x45	/* system Pro				*/

/*
 *      Definitions for adapter, Display Controller Type
 */

#define MADT_UNKNOWN	0
#define	MADT_CGA80	2 	/* color graphics 80 column */
#define	MADT_MONO	3 	/* monochrome/text */
#define	MADT_COMPAQHR	4 	/* compaq hi res mono */

#define	MADT_EGA	0x10	/* regular EGA adapter */
#define	MADT_VEGA	0x11	/* Video 7 vega delux EGA adapter */

#define	MADT_VGA	0x20	/* regular VGA adapter */
#define	MADT_COMPAQVGA	0x21 	/* compaq vga adapter */
#define	MADT_V7VRAM	0x21 	/* Video 7 vram vga adapter */
#define	MADT_V7_E	0x22 	/* Video 7 vga adapter with extensions support*/

#define	MADT_Z449	0x80 	/* Zenith Z449 card */
#define	MADT_T5100	0x81 	/* Toshiba T5100 */

/*
 *      Definitions for int_mon and ext_mon
 */

#define MONT_UNKN        0x00    /* None or unknown */
#define MONT_CPQDM       0x01    /* Compaq dual-mode */
#define MONT_RGB         0x02    /* 5153 style RGB Monitor */
#define MONT_COL         0x03    /* Color Monitor */
#define MONT_PLAS        0x04    /* 640x400 Plasma Display */
#define MONT_MULTS       0x05    /* Variable frequency, Multi-sync style */


/*
 *      Definitions for fp_kind are in sys/fp.h
 */

/*
 *      Definitions for weitek_kind are in sys/weitek.h
 */

#define B_MAXDPE	2	/* Max entries for drive parameters */

/*
 *	Hard disk parameter table. Contains info from CMOS and system
 * 	RAM following BIOS initalization.
 */

struct hdparam { 		/* hard disk parameters */
	ushort	hdp_ncyl;	/* # cylinders (0 = no disk) */
	unchar	hdp_nhead;	/* # heads */
	unchar	hdp_nsect;	/* # sectors per track */
	ushort	hdp_precomp;	/* write precomp cyl */
	ushort	hdp_lz;		/* landing zone */
	ushort	hdp_type;	/* drive type from CMOS,
				/* lower byte = CMOS(0x10) 4bits
				/* upper byte = CMOS(0x19,20) */
};

/*
 *	Floppy disk parameter table. Contains info from CMOS and system
 * 	RAM following BIOS initalization.
 */

struct b_fdparam { 		/* floppy disk parameters */
	unchar	fdp_type;	/* Drive type from CMOS byte 0x10 */
	unchar	fdp_ncyl;	/* # cylinders/tracks (0 = no disk) */
				/* max track #, zero based */
	unchar	fdp_nsect;	/* # sectors per track */
	unchar	fdp_secsiz;	/* sector size, encoded per tech ref */
	unchar	fdp_RWgap;	/* Read/Write data gap length */
	unchar	fdp_FORMgap;	/* Format gap length */
	unchar	fdp_steprate;	/* Step rate, head settle time */
	unchar	fdp_xferate;	/* Data transfer rate */
};

/*
 *      Definitions for machenv.flags, environment indicators
 */

#define MEM_CACHE	0x01	/* This machine has cache memory */
#define CPQ_CACHE       0x02    /* Uses Compaq style cache workaround to
				   support dual port memory on I/O boards,
				   no caching above 0x80000000 */
#define MEM_SHADOW	0x04	/* machine support shadow ram	*/
#define MC_BUS		0x10	/* Machine has micro-channel bus */
#define AT_BUS		0x20	/* Machine has AT bus */
#define IS_SX           0x100   /* Processor is SX chip */
#define IS_486          0x200   /* Processor is 80486 chip */

struct sysenvmt {
	unsigned int  machflags;	/* environment flags */

	unsigned char fp_kind;    /* math co-processor 80x87 type */
	unsigned char weitek_kind;

	unsigned char machine;  /* distinguish machine manufacturer */
	unsigned char c_speed;  /* Clock speed in MHz, =20 for 20MHz */
	unsigned char m_model;	/* model number */
	unsigned char m_revno;	/* revision number */
	unsigned char equip;    /* CMOS equipment byte */
	unsigned char numhd;    /* Number of HD drives from 0040:0075 */

	ushort	CMOSmembase;	/* Base memory from CMOS(0x15,16) in KB */
	ushort	CMOSmemext;	/* Extended memory from CMOS(0x17,18) in KB */
	ushort	CMOSmem_hi;	/* Memory over 1MB from CMOS(0x30,31) in KB */
	ushort	base_mem;	/* Base memory size from 0040:0048 */
	ushort	sys_mem;	/* system memory size from int15 ah=0x88 */

	ushort	COMM_base[4];	/* COMM port base address from 0040:0000-0006 */
	ushort	LPT_base[4];	/* LPT port base address from 0040:0008-000E */

	unsigned char adapter;  /* graphics adapter type */
	unsigned char gmodes;   /* supported modes */
	unsigned char int_mon;  /* internal monitor type */
	unsigned char ext_mon;  /* external monitor type */
	unsigned short m_delay; /* required mode switch delay time in MS */
	paddr_t  font_ptr[MX_FTP]; /* ega font pointers */

	struct hdparam hdparamt[B_MAXDPE];    /* hard disk parameters */
	struct b_fdparam fdparamt[B_MAXDPE];    /* floppy disk parameters */

	struct	rom_map	rom_map[MX_ROM_MAP];	/* Found rom bios */

	struct	sys_desc sys_desc;

	union machine_dependent {
		char	reserve[40];
		struct compaq {
			unsigned char act_disp;	/* active display 	*/
			unsigned char ctl_mode;	/* control mode 	*/
			unsigned char reserved;	/* 0 			*/
			unsigned char modes;	/* supported modes 	*/
			unsigned char int_mon;	/* internal monitor type */
			unsigned char ext_mon;	/* external monitor type */
		} compaq;
		struct	intel {
			unsigned char	set_reg;	/* setup register */
		} intel;
		struct olivetti {
			unsigned char	swa13_16;	/* switch settings */
			char		*cons_dra;	/* console data reg */
		} olivetti;
	} md;
};

extern struct sysenvmt *sysenvmtp;

#endif	/* _SVC_SYSENVMT_H */
