/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386at/ktool/idtools/mdep.c	1.10"
#ident	"$Header:"

/*
 * Machine-specific routines for ID/TP.
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"

struct entry_def *edef_intr;


/* check if vector, I/O addresses, and/or device memory addresses overlap */

int
mdep_check(sdp, drv)
	struct sdev	*sdp;
	driver_t	*drv;
{
        register ctlr_t *ctlr2;
	register ivec_t *ivec;

        /* check itype */
	if (sdp->itype < SITYP || sdp->itype > EITYP) {
		sprintf(errbuf, RITYP, sdp->itype, SITYP, EITYP);
		error(1);
		return(0);
	}

	if (sdp->itype == 0) {
		if (sdp->vector != 0 || sdp->ipl != 0) {
			sprintf(errbuf,
"Interrupt vector or ipl specified even though itype is 0");
			if (!INSTRING(drv->mdev.mflags, COMPAT)) {
				error(1);
				return(0);
			} else {
				warning(1);
				return(1);
			}
		}
	} else {

		/* check ipl value - must be 1 to 8, inclusive */
		if (sdp->ipl < SIPL || sdp->ipl > EIPL) {
			sprintf(errbuf, RIPL, sdp->ipl, SIPL, EIPL);
			error(1);
			return(0);
		}

                /* check range of IVN */
                if (sdp->vector < SIVN || sdp->vector > EIVN) {
                        sprintf(errbuf, RIVN, sdp->vector, SIVN, EIVN);
                        error(1);
                        return(0);
                }

		/* check for inconsistent use of vector */
		ivec = &ivec_info[sdp->vector];
		if (ivec->itype == 0) {
			ivec->itype = sdp->itype;
			ivec->ipl = sdp->ipl;
		} else if (ivec->itype != sdp->itype ||
			   ivec->ipl != sdp->ipl) {
			sprintf(errbuf, VECDIFF, ivec->itype, ivec->ipl);
			error(1);
			return(0);
		} else {
			switch (sdp->itype) {
			case 1:	/* no sharing */
				sprintf(errbuf, CVEC, sdp->name,
					ivec->ctlrs->sdev.name);
				error(1);
				return(0);
			case 2: /* sharing within a driver only */
				for (ctlr2 = ivec->ctlrs; ctlr2 != NULL;) {
					if (ctlr2->driver != drv) {
						sprintf(errbuf, CVEC, sdp->name,
							ctlr2->sdev.name);
						error(1);
						return(0);
					}
					ctlr2 = ctlr2->vec_link;
				}
				break;
			}
		}
        }

        /* check I/O address */
	if (sdp->sioa > sdp->eioa) {
		/* out of order */
		sprintf(errbuf, OIOA, sdp->sioa, sdp->eioa);
		error(1);
		return(0);
	}
        if (sdp->eioa != 0) {
                /* check range of I/O addresses */
                if (sdp->sioa < SIOA || sdp->eioa > EIOA) {
                        sprintf(errbuf, RIOA, sdp->sioa, sdp->eioa, SIOA, EIOA);
                        error(1);
                        return(0);
                }
        }

        /* check controller memory address */
	if (sdp->scma > sdp->ecma) {
		/* out of order */
		sprintf(errbuf, OCMA, sdp->scma, sdp->ecma);
		error(1);
		return(0);
	}
        if (sdp->ecma != 0) {
                /* check range of device memory address */
                if (sdp->scma < SCMA) {
                        sprintf(errbuf, RCMA, sdp->scma, SCMA);
                        error(1);
                        return(0);
                }
        }

        /* check DMA channel */
        if (sdp->dmachan < -1 || sdp->dmachan > DMASIZ) {
                sprintf(errbuf, RDMA, sdp->dmachan, DMASIZ);
                error(1);
                return(0);
        }

	for (ctlr2 = ctlr_info; ctlr2 != NULL; ctlr2 = ctlr2->next) {
                /* check I/O address conflicts */
                if (sdp->eioa != 0 && ctlr2->sdev.eioa != 0) {
                        if (OVERLAP(sdp->sioa, sdp->eioa,
				    ctlr2->sdev.sioa, ctlr2->sdev.eioa) &&
			    !(INSTRING(drv->mdev.mflags, IOOVLOK) &&
			      INSTRING(ctlr2->driver->mdev.mflags, IOOVLOK))) {
				sprintf(errbuf, CIOA, ctlr2->sdev.name,
						sdp->name);
				error(1);
				return(0);
                        }
		}

                /* check device memory address conflicts */
                if (sdp->ecma != 0 && ctlr2->sdev.ecma != 0) {
                        if (OVERLAP(sdp->scma, sdp->ecma,
				    ctlr2->sdev.scma, ctlr2->sdev.ecma)) {
                                sprintf(errbuf, CCMA, ctlr2->sdev.name,
						sdp->name);
                                error(1);
                                return(0);
                        }
		}

                /* check DMA channel conflicts */
                if (sdp->dmachan != -1) {
                        if (sdp->dmachan == ctlr2->sdev.dmachan &&
			    !INSTRING(drv->mdev.mflags, DMASHR) &&
			    !INSTRING(ctlr2->driver->mdev.mflags, DMASHR)) {
                                sprintf(errbuf, CDMA, ctlr2->sdev.name,
						sdp->name);
                                error(1);
                                return(0);
                        }
		}
        }

        return(1);
}


/* Machine-dependent controller initialization. */

void
mdep_ctlr_init(ctlr)
	register ctlr_t *ctlr;
{
        register ctlr_t *ctlr2;

	ctlr->vec_link = NULL;
	if (ctlr->sdev.itype) {
		register ivec_t *ivec = &ivec_info[ctlr->sdev.vector];
		/*
		 * Add this controller to the list for the vector,
		 * but only if this driver isn't already in the list.
		 */
		for (ctlr2 = ivec->ctlrs; ctlr2; ctlr2 = ctlr2->vec_link) {
			if (ctlr2->driver == ctlr->driver)
				break;
		}
		if (ctlr2 == NULL) {
			ctlr->vec_link = ivec->ctlrs;
			ivec->ctlrs = ctlr;
		}

		/*
		 * For each device configured for an interrupt, creat an implied
		 * "intr" entry-point, even if one wasn't given explicitly.
		 * This is a special case needed for backward compatibility,
		 * since the "intr" entry-point was not explicitly specified in
		 * the version 0 Master file, even though others were.
		 */
		if (INSTRING(ctlr->driver->mdev.mflags, COMPAT) &&
		    !drv_has_entry(ctlr->driver, edef_intr)) {
			if (lookup_entry(edef_intr->suffix,
					 &ctlr->driver->mdev.entries, 0) == -1) {
				sprintf(errbuf, TABMEM, "entry-point list");
				fatal(0);
			}
		}
	}
}


/* Print out interrupt vector information. */

void
mdep_prvec(fp)
        register FILE *fp;
{
        register ctlr_t *ctlr;
	register driver_t *drv;
	ulong level_intr_mask = 0;	/* level-sensitive mask to handle
					 * level-sensitive interrupts. (For
					 * EISA bus architectures.)
					 */
        int i, nintr = 0;

        fprintf(fp, "/* Table of Interrupt Vectors */\n\n");
        fprintf(fp, "extern void intnull();\n");
        fprintf(fp, "extern void clock();\n");

	for (ctlr = ctlr_info; ctlr != NULL; ctlr = ctlr->next) {
		if (ctlr->sdev.itype == 0 || ctlr->driver->loadable)
			continue;

		drv = ctlr->driver;

		if (!drv->intr_decl) {
			fprintf(fp, "extern void %sintr();\n",
				drv->mdev.prefix);
			drv->intr_decl = 1;
		}
        }

        for (i = 1; i < 256; i++) {
		if ((ctlr = ivec_info[i].ctlrs) == NULL)
			continue;
		ivec_info[i].used = 0;
		do {
			if (!ctlr->driver->loadable)
				ivec_info[i].used++;
			ctlr = ctlr->vec_link;
		} while (ctlr != NULL);
		if (ivec_info[i].used <= 1)
			continue;
		fprintf(fp, "void shrint%d() {\n", i);
		for(ctlr = ivec_info[i].ctlrs; ctlr != NULL; 
			ctlr = ctlr->vec_link) {
			if (!ctlr->driver->loadable)
				fprintf(fp, "\t%sintr(%d);\n",
					ctlr->driver->mdev.prefix, i);
		}
		fprintf(fp, "}\n" );
        }

	fprintf(fp, "\nvoid (*ivect[])() = {\n");

        /* the clock always goes first */
        fprintf(fp, "\tclock\t\t/* 0\t*/");

	for (i = 1; i < 256; i++) {
		fprintf(fp, ",\n\t");
		if (ivec_info[i].used) {
			if (ivec_info[i].used > 1)
				fprintf(fp, "shrint%d", i);
			else
				for(ctlr = ivec_info[i].ctlrs; ctlr != NULL; 
					ctlr = ctlr->vec_link)
					if (!ctlr->driver->loadable) {
						fprintf(fp, "%sintr",
						ctlr->driver->mdev.prefix);
						break;
					}
                        if (nintr < i)
				nintr = i;
                } else
                        fprintf(fp, "intnull");
		fprintf(fp, "\t\t/* %d\t*/", i);
        }
        fprintf(fp, "\n};\n");

        fprintf(fp, "int nintr = %d;\n", (nintr+1));

	/* handle level sensitive interrupt sharing (type = 4).
	 * Need to build a bit mask to mark the vector numbers
	 * that are level-sensitive. The assumption here is that
	 * there's a max of 16 hw interrupts. The data type used
	 * is a long int, though, to handle further expansion.
	 */

	for (i = SIVN; i <= EIVN; i++) {
		if (ivec_info[i].itype == 4 && ivec_info[i].used)
			level_intr_mask |= (1 << i);
	}

        /* Indices in this table correspond to indices in the ivect table. */
        fprintf(fp,"\n/* Table of ipl values for interrupt handlers. */\n");
        fprintf(fp,"\nunsigned char intpri[] = {\n");

        /* do the clock's IPL first */
	ivec_info[CLK_IVN].ipl = CLK_IPL;
	ivec_info[CLK_IVN].used++;

        for (i = 0; i < 256; i++)
		fprintf(fp, "\t%d\t/* %d\t*/,\n", ivec_info[i].used ? ivec_info[i].ipl : 0, i);

        fprintf(fp, "};\n");

	/* print level sensitive mask */
	fprintf(fp, "unsigned long level_intr_mask = 0x%x;\n", level_intr_mask);

}


void
mdep_drvpostproc()
{
}


void
mdep_prdrvconf(fp, drv, caps)
	FILE	*fp;
	register driver_t *drv;
	char	*caps;
{
	register ctlr_t *ctlr;
	int	dmachan, itype;
	int	same_dmachan, same_itype;

	/* These used to be per-driver, but are now per-controller;
	   for compatibility, if all controllers have the same value,
	   define it here. */
	ctlr = drv->ctlrs;
	dmachan = ctlr->sdev.dmachan;
	itype = ctlr->sdev.itype;
	same_dmachan = same_itype = 1;
	while ((ctlr = ctlr->drv_link) != NULL) {
		if (ctlr->sdev.dmachan != dmachan)
			same_dmachan = 0;
		if (ctlr->sdev.itype != itype)
			same_itype = 0;
	}
	if (same_dmachan)
		fprintf(fp, "#define\t%s_CHAN\t%hd\n", caps, dmachan);
	if (same_itype)
		fprintf(fp, "#define\t%s_TYPE\t%hd\n", caps, itype);
}


void
mdep_prctlrconf(fp, ctlr, caps)
	FILE	*fp;
	register ctlr_t *ctlr;
	char	*caps;
{
	fprintf(fp, "#define\t%s_%hd_VECT\t%hd\n",
		caps, ctlr->num, ctlr->sdev.vector);
	fprintf(fp, "#define\t%s_%hd_SIOA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.sioa);
	fprintf(fp, "#define\t%s_%hd_EIOA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.eioa);
	fprintf(fp, "#define\t%s_%hd_SCMA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.scma);
	fprintf(fp, "#define\t%s_%hd_ECMA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.ecma);
	fprintf(fp, "#define\t%s_%hd_CHAN\t%ld\n",
		caps, ctlr->num, ctlr->sdev.dmachan);
	fprintf(fp, "#define\t%s_%hd_TYPE\t%ld\n",
		caps, ctlr->num, ctlr->sdev.itype);
	fprintf(fp, "#define\t%s_%hd_IPL\t%ld\n",
		caps, ctlr->num, ctlr->sdev.ipl);
}


/*ARGSUSED*/
void
mdep_devsw_decl(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_bdevsw(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_cdevsw(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_prconf(fp)
	FILE	*fp;
{
}

mdep_printr(fp, drv, caps)
FILE *fp;
driver_t *drv;
char *caps;
{
	ctlr_t *ctlr;
	int i;

	for (ctlr = drv->ctlrs, i = 0; ctlr != NULL; ctlr = ctlr->drv_link, i++)
		fprintf(fp, "\t{ %s_%d_VECT, %s_%d_IPL, %s_%d_TYPE },\n",
			caps, i, caps, i, caps, i);
	fprintf(fp, "\t{ -1, 0, 0 }\n");
}
