/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:xterm_ioctl.h	1.8"
#endif
/*
 xterm_ioctl.h (C hdr file)
	Acc: 601052336 Tue Jan 17 09:58:56 1989
	Mod: 601054120 Tue Jan 17 10:28:40 1989
	Sta: 601054120 Tue Jan 17 10:28:40 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

#define	M_B40x25	0x00
#define	M_C40x25	0x01
#define	M_B80x25	0x02
#define	M_C80x25	0x03
#define	M_BG320		0x04
#define	M_CG320		0x05
#define	M_BG640		0x06
#define	M_EGAMONO80x25	0x07
#define	M_CG320_D	0x0D
#define	M_CG640_E	0x0E
#define	M_EGAMONOAPA	0x0F
#define	M_CG640x350	0x10
#define	M_ENHMONOAPA2	0x11
#define	M_ENH_CG640	0x12
#define	M_ENH_B40x25	0x13
#define	M_ENH_C40x25	0x14
#define	M_ENH_B80x25	0x15
#define	M_ENH_C80x25	0x16
#define	M_ENH_B80x43	0x70
#define	M_ENH_C80x43	0x71
#define	M_MCA_MODE	0xFF


#define KEYMAP_SIZE     2572
#define KBENTRY_SIZE    4
#define STRMAP_SIZE	512
#define FKEY_SIZE	34
#define NUMBER_OF_FK	35
#define SCRNMAP_SIZE	256

#define	doinput()		(bcnt-- > 0 ? *bptr++ : in_put())

typedef struct {
	unsigned char map[8];
	unsigned char spcl;
	unsigned char flgs;
} Key_t;

typedef struct {
	short n_keys;
	Key_t keys[257];
} Keymap_t;

typedef struct {
	short	ndif;
	Keymap_t	orig;
	Keymap_t	active;
} Trans_table;

typedef struct {
	unchar	kb_table;
	unchar	kb_index;
	short	kb_value;
} Kbentry;

#define KEYDEF_SIZE	30
typedef struct {
	unsigned short	keynum;
	char		*keydef[KEYDEF_SIZE];
	char		flen;
} Fkeyarg;

extern	Trans_table	*KBTrans_table;
extern  char		*FKTrans_table[NUMBER_OF_FK];
extern	unsigned char	*SCRTrans_table;
extern	Boolean		KBTranslation;

#define	IOCTL_GOOD_RC	'\000'
#define	IOCTL_BAD_RC	'\001'

#define	REPLY_TO_IOCTL(buffer, size, ioctl_name) \
	if (write(screen->respond, buffer, size) != size) { \
		Panic("ioctl_reply: error replying to ioctl %s\n", ioctl_name); \
	}

#define IOCTL_READ(ioctl_buf, count) \
	{	\
		int	ind;	\
		for(ind=0; ind < count; ind++)	\
			ioctl_buf[ind] = doinput();	\
	}

#define RETURN_BUF_HEADER(ioctl_buf, ioctl_char, return_code) \
	{ \
		ioctl_buf[0] = '\033'; \
		ioctl_buf[1] = '@'; \
		ioctl_buf[2] = '3'; \
		ioctl_buf[3] = ioctl_char; \
		ioctl_buf[4] = return_code; \
	}
/*
	This macro may be used to return an integer value.
	It ASSUMES that ioctl_char is the character representing the 
	ioctl you wish to reply to. Furthermore, this macro is only
	good for returning values from 0 - 255 (inclusive).
*/
#define RETURN_VALUE(ioctl_buf, ioctl_char, return_code, return_value) \
	{ \
		RETURN_BUF_HEADER(ioctl_buf, ioctl_char, return_code); \
		ioctl_buf[5] = return_value; \
		ioctl_buf[6] = '\000'; \
		ioctl_buf[7] = '\000'; \
		ioctl_buf[8] = '\000'; \
	}


#define REMAP_OUTPUT_STRING(str, len) \
	{ \
	     register int LEN = len; \
	     register unsigned char c; \
    	     while (--LEN >= 0) { \
		    c = str[LEN]; \
 	            str[LEN] = SCRTrans_table[c]; \
	     } \
	}
