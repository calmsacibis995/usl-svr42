/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:ioctl.c	1.16"
#endif
/*
 ioctl.c (C source file)
	Acc: 601052291 Tue Jan 17 09:58:11 1989
	Mod: 601054062 Tue Jan 17 10:27:42 1989
	Sta: 601054062 Tue Jan 17 10:27:42 1989
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

#include "ptyx.h"
#include "xterm.h"
#include "xterm_ioctl.h"
#include "error.h"
#include "sys/errno.h"
#define  XK_MISCELLANY
#include <X11/keysymdef.h>

extern int bcnt;
extern char *bptr;

/* pio_keymap puts a new keyboard mapping table.  It receives   */
/* the table from consem.					*/

int pio_keymap (screen, ioctl_char)
register TScreen *screen;
char ioctl_char;
{
    register int i, j, k;
    register Key_t *orig, *active;
    char buf[KEYMAP_SIZE];

    /* if translation table doesn't already exist, allocate one */
    /* and read the standard translation table into the 'orig'  */
    /* half of the table					*/

    if (KBTrans_table == (Trans_table *) NULL)
    {
	_alloc_KBTrans();
	_read_KBTrans(buf);
	KBTrans_table->ndif = 0;
	store_Keymap (buf, &KBTrans_table->orig);
    }

    /* read the content of the new table into bufer  */

    IOCTL_READ (buf, KEYMAP_SIZE);
    orig = KBTrans_table->orig.keys;
    active = KBTrans_table->active.keys;
    store_Keymap (buf, &KBTrans_table->active);

    /* compare contents of the original and active translation */
    /* tables and count number of differences.  This is done   */
    /* for optimization: if at any time the active table will  */
    /* become equal to the original we'll stop using the       */
    /* translation table				       */

    k = (KBTrans_table->orig.n_keys > KBTrans_table->active.n_keys) ?
         KBTrans_table->orig.n_keys : KBTrans_table->active.n_keys;

    for (i=0; i<k; i++)
    {    if (orig->spcl != active->spcl || orig->flgs != active->flgs)
         {
	     KBTrans_table->ndif++;
	     continue;
	 }
	 for (j=0; j<8; j++)
	      if (orig->map[j] != active->map[j])
		  break;
	 if (j != 8)
	     KBTrans_table->ndif++;
    }

    if (KBTrans_table->ndif == 0)
	KBTranslation = FALSE;
    else
	KBTranslation = TRUE;

    /* return to consem, indicating success		*/

    RETURN_BUF_HEADER(buf, ioctl_char, IOCTL_GOOD_RC);
    REPLY_TO_IOCTL(buf, 5, "PIO_KEYMAP");
}
    


/* gio_keymap returns to consem keyboard mapping table.  if no */
/* table currently exists, this function will read standart    */
/* mapping table and return its content			       */

int gio_keymap (screen, ioctl_char)
register TScreen *screen;
char ioctl_char;
{
    register int i, j, k;
    char ioctl_buf[KEYMAP_SIZE+5];

    RETURN_BUF_HEADER (ioctl_buf, ioctl_char, IOCTL_GOOD_RC);

    /* if translation table is not in memory, read it from file */
    if (KBTrans_table == (Trans_table *) NULL)
    {
	_read_KBTrans(&ioctl_buf[5]);
    }
    else
    {
	Key_t *keys = KBTrans_table->active.keys;

        ioctl_buf[5] = (char) (KBTrans_table->active.n_keys / 256);
        ioctl_buf[6] = (char) (KBTrans_table->active.n_keys % 256);
        for (i=0, j=7; i<257; i++)
        {    
	    for (k=0; k<8; k++)
	          ioctl_buf[j++] = keys->map[k];
	    ioctl_buf[j++] = keys->spcl;
	    ioctl_buf[j++] = keys->flgs;
        }
    }

    /* send keyboard translation map to consem	*/

    REPLY_TO_IOCTL (ioctl_buf, KEYMAP_SIZE + 5, "GIO_KEYMAP");
}


/* kdgkbent gets an entry from the keyboard translation table */

kdgkbent(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    /* ioctl_buf must be KEYMAP_SIZE, because it may be used as   */
    /* argument to _read_KBTrans() via _kb_setup()		  */

    char table, ind, ioctl_buf[KEYMAP_SIZE];

    /* set up keyboard translation table, read input from consem  */
    /* and extract table and ind information from that input      */
    /* if table and ind information is valid (rc=1), put appropr. */
    /* information into buffer.  else leave buffer as is          */

    if (_kb_setup (ioctl_buf, &table, &ind))
    {
    	ioctl_buf[5] = ioctl_buf[0];
    	ioctl_buf[6] = ioctl_buf[1];
    	ioctl_buf[7] = 0;
    	RETURN_BUF_HEADER (ioctl_buf, ioctl_char, IOCTL_GOOD_RC);

    	/* if KBTranslation is TRUE, take information from the active */
    	/* part of the table, else take it from the orig. part 	  */

    	ioctl_buf[8] = (KBTranslation) ?
			KBTrans_table->active.keys[ind].map[table] :
			KBTrans_table->orig.keys[ind].map[table];
    }
    else
    	RETURN_BUF_HEADER (ioctl_buf, ioctl_char, IOCTL_BAD_RC);

    /* send information back to consem				  */

    REPLY_TO_IOCTL(ioctl_buf, 9, "KDGKBENT");
}


/* kdskbent: set an entry in keyboard translation table	*/

kdskbent(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    /* ioctl_buf must be KEYMAP_SIZE, because it may be used as   */
    /* argument to _read_KBTrans() via _kb_setup()		  */

    char table, ind, ioctl_buf[KEYMAP_SIZE];

    /* set up keyboard translation table, read input from consem  */
    /* and extract table and ind information from that input      */
    /* if table and ind information is valid (rc=1), store new    */
    /* value into table.  else leave the table alone		  */

    if (_kb_setup (ioctl_buf, &table, &ind))
    {
        KBTrans_table->active.keys[ind].map[table] = ioctl_buf[3];
        RETURN_BUF_HEADER (ioctl_buf, ioctl_char, IOCTL_GOOD_RC); 
    }
    else
        RETURN_BUF_HEADER (ioctl_buf, ioctl_char, IOCTL_BAD_RC); 

    /* we don't care what is in the last 4 positions of the buffer */
    /* since consem is not going to use it any way		   */

    REPLY_TO_IOCTL(ioctl_buf, 5, "KDSKBENT");
}



_kb_setup(buf, table, indx)
char *buf, *table, *indx;
{
    /* if the table is not already in memory, it will allocate    */
    /* storage for it and then read in default keyboard mapping   */

    if (KBTrans_table == (Trans_table *) NULL)
    {
	_alloc_KBTrans();
	_read_KBTrans(buf);
	KBTrans_table->ndif = 0;
	store_Keymap (buf, &KBTrans_table->orig);
    }

    /* read input from consem into bufer			  */

    IOCTL_READ(buf, KBENTRY_SIZE);

    /* extract table and value information from the bufer        */
    /* table value is adjusted: we have only one translation table*/
    /* KBTrans_table, which has 8 entries per key.  KD[GS]KBENT   */
    /* uses only 4 entries per key (no cntr modifier)		  */

    if (buf[0] > 3 || buf[1] > 257)
        return (0);
    else
    {
    	*table = (buf[0] == 0) ? 0 : 2 * buf[0] - 1;
    	*indx = buf[1];
	return (1);
    }
}



_alloc_KBTrans ()
{
     if ((KBTrans_table = (Trans_table *) calloc ((unsigned) 1,
             sizeof (Trans_table))) == (Trans_table *) NULL)
          SysError (ERROR_IOALLOC);
}


/* reads default keyboard translation table from the file	*/

_read_KBTrans(buf)
char *buf;
{
    register int std_file, cntr;

    if ((std_file = open ("/usr/X/lib/xterm.stdkbmap", O_RDONLY)) == -1)
         SysError (ERROR_IOOPEN);

    if ((cntr = read (std_file, buf, KEYMAP_SIZE)) != KEYMAP_SIZE)
         SysError (ERROR_IOREAD);

    close (std_file);
}


store_Keymap (buf, table)
char  *buf;
Keymap_t *table;
{
    register int i, j, k;
    Key_t    *keys = table->keys;

    /* the first 2 bytes contain short */

    table->n_keys = buf[0] * 256 + buf[1];

    for (i=0, j=2; i<257; i++, keys++)
    {    
	for (k=0; k<8; k++)
	      keys->map[k] = buf[j++];
	keys->spcl = buf[j++];
	keys->flgs = buf[j++];
    }
}


/* get string mapping table from kernel.  this table is an array */
/* of 512 bytes, containing null terminating strings that re-    */
/* define function keys.					 */

gio_strmap(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    register int i, len, ind = 5;
    char     buf[STRMAP_SIZE+5];

    RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_GOOD_RC);

    for (i=0; i<NUMBER_OF_FK && ind<STRMAP_SIZE+5; i++) 
    {
	 if (FKTrans_table[i] == NULL)
	     buf[ind++] = '\0';
         else
	 {
	     len = strlen (FKTrans_table[i]);
	     if ((ind + len) > STRMAP_SIZE)
		  len = STRMAP_SIZE - ind;
	     strncpy (&buf[ind], FKTrans_table[i], len);
	     ind += len;
	     if (ind < STRMAP_SIZE)
	         buf[ind++] = '\0';
	 }
    }

    for (; ind < STRMAP_SIZE; ind++)
	 buf[ind] = '\0';

    /* send information back to consem				  */

    REPLY_TO_IOCTL(buf, STRMAP_SIZE + 5, "GIO_STRMAP")
}



/* put string mapping table into kernel.  this table is an array */
/* of 512 bytes, containing null terminating strings that re-    */
/* define function keys.					 */

pio_strmap(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    register int i, len, ind = 0;
    char     buf[STRMAP_SIZE];
    KeySym keysym;

    /* read input from consem into bufer			  */

    IOCTL_READ(buf, STRMAP_SIZE);

    /* go through the bufer, extract strings, and put them into  */
    /* appropriate entries in the FKTrans_table.  We may have to  */
    /* free the storage used for the old entries.		  */

    for (i=0; i<NUMBER_OF_FK && ind<STRMAP_SIZE; i++) 
    {
	 if (FKTrans_table[i] != NULL)
	     free (FKTrans_table[i]);
	 len = strlen (&buf[ind]);
	 if (len > 0)
	 {
	     if ((FKTrans_table[i] = strdup (&buf[ind])) == NULL)
		  SysError (ERROR_IOALLOC1);
	 }
	 else
	     FKTrans_table[i] = NULL;
	 ind += len + 1;

	 /* rebind the function keys to the new definitions	*/

	 XRebindKeysym (screen->display, keysym, 
		        (KeySym *) NULL, 0,
			(Char *) FKTrans_table[i], len);
    }

    /* return 1 to consem, indicating success		*/

    RETURN_BUF_HEADER(buf, ioctl_char, IOCTL_GOOD_RC);
    REPLY_TO_IOCTL(buf, 5, "PIO_STRMAP");
}

/* obntain the current definition of a function key */

getfkey(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    register int i;
    register unsigned long keynum = 0;
    char buf[FKEY_SIZE+5];

    /* read the input from consem		        */

    IOCTL_READ((&buf[5]), FKEY_SIZE);

    /* extract the key FK number (it is in the first 2 bytes)   */

    keynum = buf[6]*256 + buf[5];

    if (keynum > NUMBER_OF_FK)
    {
	RETURN_VALUE (buf, ioctl_char, IOCTL_BAD_RC, EINVAL);
	
	/* fill ther rest of the buffer with nulls	*/

	for (i=(FKEY_SIZE+4); i>=9; i--)
	     buf[i] = '\0';
    }
    else
    {
	char *string = FKTrans_table[keynum];
	int  len = strlen (string);

	RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_GOOD_RC);

	if (len > KEYDEF_SIZE)
	    len = KEYDEF_SIZE;

	strncpy (&buf[7], string, len);
	if (len < KEYDEF_SIZE)
	    buf[7+len] = '\0';
	buf[FKEY_SIZE+3] = (char) len;
    }

    REPLY_TO_IOCTL(buf, FKEY_SIZE+5, "GETFKEY");
}


/* set the definition of a function key */

setfkey(screen, ioctl_char)
register TScreen *screen;
char     ioctl_char;
{
    register unsigned short keynum;
    char buf[FKEY_SIZE];

    /* read the input from consem		        */

    IOCTL_READ (buf, FKEY_SIZE);

    /* extract the key FK number (it is in the first 2 bytes)   */

    keynum = buf[1]*256 + buf[0];

    if (keynum > NUMBER_OF_FK)
    {
        RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_BAD_RC);
    }
    else
    {
	int len = buf [FKEY_SIZE - 2];
	XRebindKeysym (screen->display, (KeySym) (XK_F1 + keynum),
		       (KeySym *) 0, 0,
			(Char *) &buf[2], len);

	/* put the string into table	*/

	if (FKTrans_table[keynum] != NULL)
	    free (FKTrans_table[keynum]);
	if (len > 0)
	{
	    if ((FKTrans_table[keynum] = strdup (&buf[2])) == NULL)
	         SysError (ERROR_IOALLOC1);
	}
	else
	    FKTrans_table[keynum] = NULL;

        RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_GOOD_RC);
    }

    /* return to consem */

    REPLY_TO_IOCTL(buf, 5, "SETFKEY");
}


/* get screen mapping table from the kernel	*/

int gio_scrnmap (screen, ioctl_char)
register TScreen *screen;
char ioctl_char;
{
    register int i, j;
    char buf[SCRNMAP_SIZE+5];

    if (SCRTrans_table != NULL)
	for (i=0, j=5; i<SCRNMAP_SIZE; i++, j++)
	     buf[j] = SCRTrans_table[i];

    /* if there is no translation, the mapping is 1 to 1 */

    else
	for (i=0, j=5; i<SCRNMAP_SIZE; i++, j++)
	     buf[j] = i;

    RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_GOOD_RC);
    REPLY_TO_IOCTL(buf, SCRNMAP_SIZE+5, "GIO_SCRNMAP");

}


int pio_scrnmap (screen, ioctl_char)
register TScreen *screen;
char ioctl_char;
{
    register int i;
    char buf[SCRNMAP_SIZE];

    /* read the input from consem		        */

    IOCTL_READ (buf, SCRNMAP_SIZE);

    /* if there is no table, allocate one		*/

    if (SCRTrans_table == NULL)
        if ((SCRTrans_table = (unsigned char *) calloc
				((unsigned) 1, SCRNMAP_SIZE)) == NULL)
             SysError (ERROR_IOALLOC);

    /* put screen mapping information into table	*/

    for (i=0; i<SCRNMAP_SIZE; i++)
	 SCRTrans_table[i] = buf[i];

    /* if the result is 1 to 1 mapping, get rid of the table	*/

    for (i=0; i<SCRNMAP_SIZE; i++)
	 if (SCRTrans_table[i] != i)
	     break;

    if (i == SCRNMAP_SIZE)
    {
	free (SCRTrans_table);
	SCRTrans_table = NULL;
    }

    RETURN_BUF_HEADER (buf, ioctl_char, IOCTL_GOOD_RC);
    REPLY_TO_IOCTL(buf, 5, "PIO_SCRNMAP");
}



static _XCheckForMod(event, ModifierMask, modifier)
     unsigned int ModifierMask;
     unsigned int modifier;
     XKeyEvent *event;
{
     register i, j;
     XModifierKeymap *m = event->display->modifiermap;
     
     j = (ffs(ModifierMask) - 1) * m-> max_keypermod;
     
     for (i = j; i < j + m->max_keypermod; i++)
        if (XKeycodeToKeysym(event->display, m->modifiermap[i], 0) == modifier) 
           return 1;
     
     return 0;
}



int i386LookupSymbol (event, buf)
     XKeyEvent *event;
     char *buf;
{
     register int keyval = event->keycode - 8;	/* i386 server specific */
     Key_t *keys = &(KBTrans_table->active.keys[keyval]);
     register unsigned char flgs = keys->flgs;
     register int ind = 0;

     if (event->state & ShiftMask)	ind |= 01;
     if (event->state & ControlMask)	ind |= 02;
     if (event->state & Mod1Mask)	ind |= 04;	/* Alt */

     if (ind == 3)      ind = 4;
     else if (ind == 4) ind = 3;

	if (ind <= 1) {
	    if (((event->state & LockMask) && (flgs == 'C' || flgs == 'B')) ||
		((((event-> state & Mod2Mask) &&
		  _XCheckForMod(event, Mod2Mask, XK_Num_Lock)) ||
		((event-> state & Mod1Mask) &&
		  _XCheckForMod(event, Mod1Mask, XK_Num_Lock)) ||
		((event-> state & Mod3Mask) &&
 		  _XCheckForMod(event, Mod3Mask, XK_Num_Lock)) ||
		((event-> state & Mod4Mask) &&
		  _XCheckForMod(event, Mod4Mask, XK_Num_Lock)) ||
		((event-> state & Mod5Mask) &&
		  _XCheckForMod(event, Mod5Mask, XK_Num_Lock))) &&
		   (flgs == 'N' || flgs == 'B')))
		    buf[0] = keys->map[(ind == 0 ? 1 : 0)];
	    else
		    buf[0] = keys->map[ind];
	}
	else
	    buf[0] = keys->map[ind];
}
