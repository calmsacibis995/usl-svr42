/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libspeedo:speedo/spfile.c	1.2"
/* $XConsortium: spfile.c,v 1.4 91/07/15 18:16:46 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *
 * $NCDId: @(#)spfile.c,v 4.6 1991/06/12 13:17:24 lemke Exp $
 *
 * Author: Dave Lemke, Network Computing Devices Inc
 *
 */

#include	<stdio.h>
#include	"fontfilest.h"

#include	"spint.h"

SpeedoFontPtr cur_spf = (SpeedoFontPtr) 0;

#ifdef EXTRAFONTS
#include	"ncdkeys.h"
#endif

#include	"keys.h"

#ifdef EXTRAFONTS
static ufix8 skey[] =
{
    SKEY0,
    SKEY1,
    SKEY2,
    SKEY3,
    SKEY4,
    SKEY5,
    SKEY6,
    SKEY7,
    SKEY8
};				/* Sample Font decryption key */

static ufix8 rkey[] =
{
    RKEY0,
    RKEY1,
    RKEY2,
    RKEY3,
    RKEY4,
    RKEY5,
    RKEY6,
    RKEY7,
    RKEY8
};				/* Retail Font decryption key */

#endif				/* EXTRAFONTS */

static ufix8 mkey[] =
{
    KEY0,
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY5,
    KEY6,
    KEY7,
    KEY8
};				/* Font decryption key */


static      fix15
read_2b(ptr)
    ufix8      *ptr;
{
    fix15       tmp;

    tmp = *ptr++;
    tmp = (tmp << 8) + *ptr;
    return tmp;
}

static      fix31
read_4b(ptr)
    ufix8      *ptr;
{
    fix31       tmp;

    tmp = *ptr++;
    tmp = (tmp << 8) + *ptr++;
    tmp = (tmp << 8) + *ptr++;
    tmp = (tmp << 8) + *ptr;
    return tmp;
}

/*
 * loads the specified char's data
 */
buff_t     *
sp_load_char_data(file_offset, num, cb_offset)
    fix31       file_offset;
    fix15       num;
    fix15       cb_offset;
{
    SpeedoMasterFontPtr master = cur_spf->master;

    if (fseek(master->fp, (long) file_offset, (int) 0)) {
	SpeedoErr("can't seek to char\n");
    }
    if ((num + cb_offset) > master->mincharsize) {
	SpeedoErr("char buf overflow\n");
    }
    if (fread((master->c_buffer + cb_offset), sizeof(ufix8), num,
	      master->fp) != num) {
	SpeedoErr("can't get char data\n");
    }
    master->char_data.org = (ufix8 *) master->c_buffer + cb_offset;
    master->char_data.no_bytes = num;

    return &master->char_data;
}

int
open_master(filename, master)
    char       *filename;
    SpeedoMasterFontPtr *master;
{
    SpeedoMasterFontPtr spmf;
    ufix8       tmp[16];
    ufix16      cust_no;
    FILE       *fp;
    ufix32      minbufsize;
    ufix16      mincharsize;
    ufix8      *f_buffer;
    ufix8      *c_buffer;
    int         ret;
    ufix8      *key;

    spmf = (SpeedoMasterFontPtr) xalloc(sizeof(SpeedoMasterFontRec));
    if (!spmf)
	return AllocError;
    bzero(spmf, sizeof(SpeedoMasterFontRec));

    /* open font */
    spmf->fname = (char *) xalloc(strlen(filename) + 1);
#ifdef DEBUG
	fprintf(stderr,"mastersize=%d\n", sizeof(SpeedoMasterFontRec));
	fprintf(stderr,"filename=%s\n", filename);
#endif
    fp = fopen(filename, "r");
    if (!fp) {
	ret = BadFontName;
	goto cleanup;
    }
    strcpy(spmf->fname, filename);
    spmf->fp = fp;
    spmf->state |= MasterFileOpen;

    if (fread(tmp, sizeof(ufix8), 16, fp) != 16) {
	ret = BadFontName;
	goto cleanup;
    }
    minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
#ifdef DEBUG
fprintf(stderr,"minbufsize=%d\n",minbufsize);
#endif
    f_buffer = (ufix8 *) xalloc(minbufsize);
    if (!f_buffer) {
	ret = AllocError;
	goto cleanup;
    }
    spmf->f_buffer = f_buffer;

    fseek(fp, (ufix32) 0, 0);

    /* read in the font */
    if (fread(f_buffer, sizeof(ufix8), (ufix16) minbufsize, fp) != minbufsize) {
	ret = BadFontName;
	goto cleanup;
    }
    spmf->copyright = (char *) (f_buffer + FH_CPYRT);
    spmf->mincharsize = mincharsize = read_2b(f_buffer + FH_CBFSZ);

    c_buffer = (ufix8 *) xalloc(mincharsize);
#ifdef DEBUG
fprintf(stderr,"mincharsize alloc=%d\n",mincharsize);
#endif
    if (!c_buffer) {
	ret = AllocError;
	goto cleanup;
    }
    spmf->c_buffer = c_buffer;

    spmf->font.org = spmf->f_buffer;
    spmf->font.no_bytes = minbufsize;

    cust_no = sp_get_cust_no(spmf->font);

    /* XXX add custom encryption stuff here */

#ifdef EXTRAFONTS
    if (cust_no == SCUS0) {
	key = skey;
    } else if (cust_no == RCUS0) {
	key = rkey;
    } else
#endif

    if (cust_no == CUS0) {
	key = mkey;
    } else {
	SpeedoErr("Non - standard encryption for \"%s\"\n", filename);
	ret = BadFontName;
	goto cleanup;
    }
    spmf->key = key;
    sp_set_key(key);

    spmf->first_char_id = read_2b(f_buffer + FH_FCHRF);
    spmf->num_chars = read_2b(f_buffer + FH_NCHRL);


    spmf->enc = bics_map;
    spmf->enc_size = bics_map_size;

#ifdef EXTRAFONTS
    {				/* choose the proper encoding */
	char       *f;

	f = rindex(filename, '/');
	if (f) {
	    f++;
	    if (strncmp(f, "bx113", 5) == 0) {
		spmf->enc = adobe_map;
		spmf->enc_size = adobe_map_size;
	    }
	}
    }
#endif

    /* XXX slam back to ISO Latin1 */
    spmf->first_char_id = spmf->enc[0];
    /* size of extents array */
    spmf->max_id = spmf->enc[(spmf->enc_size - 1) * 2];
    spmf->num_chars = spmf->enc_size;
#ifdef DEBUG
	fprintf(stderr,"spmf->num_chars=%d max_id=%d\n",spmf->num_chars,spmf->max_id);
#endif
    *master = spmf;

    return Successful;

cleanup:
    *master = (SpeedoMasterFontPtr) 0;
    close_master_font(spmf);
    return ret;
}

close_master_font(spmf)
    SpeedoMasterFontPtr spmf;
{
    if (!spmf)
	return;
    if (spmf->state & MasterFileOpen)
	fclose(spmf->fp);
#ifdef DEBUG
fprintf(stderr,"close file: %s\n",spmf->fname);
#endif
    xfree(spmf->fname);
    xfree(spmf->f_buffer);
    xfree(spmf->c_buffer);
    xfree(spmf);
}

void
close_master_file(spmf)
    SpeedoMasterFontPtr spmf;
{
    (void) fclose(spmf->fp);
    spmf->state &= ~MasterFileOpen;
}


/*
 * reset the encryption key, and make sure the file is opened
 */
void
sp_reset_master(spmf)
    SpeedoMasterFontPtr spmf;
{
    sp_set_key(spmf->key);
#ifdef DEBUG
fprintf(stderr,"reset  Master %d\n",spmf->fname);
#endif
    if (!(spmf->state & MasterFileOpen)) {
	spmf->fp = fopen(spmf->fname, "r");
	/* XXX -- what to do if we can't open the file? */
	spmf->state |= MasterFileOpen;
    }
}
