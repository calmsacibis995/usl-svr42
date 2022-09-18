/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cpio:i386/cmd/cpio/cpio.c	1.30.38.48"
#ident  "$Header: cpio.c 1.4 91/08/28 $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 
/***************************************************************************
 * Command: cpio
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACWRITE,P_DACREAD, P_FSYSRANGE,
 *                         P_FILESYS,P_COMPAT,P_OWNER,P_MULTIDIR,P_SETPLEVEL,
 *			   P_SETFLEVEL
 *       Fixed Privileges: None
 *
 *
 * cpio: copy file archives in and out
 *
 *******************************************************************/ 

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/secsys.h>
#include <sys/statvfs.h>
#include <sys/fs/vx_ioctl.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <ctype.h>
#include <archives.h>
#include <locale.h>
#include "cpio.h"
#include <priv.h>
#include <acl.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/param.h>
#include <libgen.h>
#include <termios.h>
#include <sys/mman.h>


#define CPIO_PATH	"/usr/bin/cpio"
#define	NENTRIES	128	/* initial number of entries for ACL buf */
#define	cpioMAJOR(x)	(int)(((unsigned) x >> 8) & 0x7F)
#define	cpioMINOR(x)	(int)(x & 0xFF)

extern
int	atoi(),
	gmatch(),
	getopt(),
	g_init(),
	g_read(),
	g_write();

extern
char	*mktemp();

extern
long	lseek(),
	ulimit();

extern
mode_t	umask();

extern
void	exit(),
	free(),
	*malloc(),
	*memalign(),
	*realloc();

static
struct Lnk *add_lnk();

static
int	bfill(),
	chg_lvl_proc(),
	chgreel(),
	ckname(),
	creat_hdr(),
	creat_lnk(),
	creat_mld(),
	creat_spec(),
	creat_tmp(),
	gethdr(),
	getname(),
	getpriv(),
	matched(),
	missdir(),
	nondigit(),
	openout(),
	read_hdr();


static
long	cksum(),
	mklong();

static
void	bflush(),
	chg_lvl_file(),
	ckopts(),
	data_in(),
	data_out(),
	data_pass(),
	file_in(),
	file_out(),
	file_pass(),
	flush_lnks(),
	flush_tty(),
	getpats(),
	ioerror(),
	mkshort(),
#ifdef __STDC__
	msg(...),
#else
	msg(),
#endif
	reclaim(),
	restore_lvl_proc(),
	rstbuf(),
	rstfiles(),
	scan4trail(),
	setpasswd(),
	setup(),
	set_tym(),
	sigint(),
	swap(),
	transferdac(),
	usage(),
	verbose(),
	write_hdr(),
	write_trail(),
	/* Enhanced Application Compatibility Support     */
	truncate_sco();  /* needed for [-T] option */
	/* End Enhanced Application Compatibility Support */

static 
struct passwd	*Curpw_p,	/* Current password entry for -t option */
		*Rpw_p,		/* Password entry for -R option */
		*dpasswd;

static
struct group	*Curgr_p,	/* Current group entry for -t option */
		*dgroup;

/* Data structure for buffered I/O. */

static
struct buf_info {
	char	*b_base_p,	/* Pointer to base of buffer */
		*b_out_p,	/* Position to take bytes from buffer at */
		*b_in_p,	/* Position to put bytes into buffer at */
		*b_end_p;	/* Pointer to end of buffer */
	long	b_cnt,		/* Count of unprocessed bytes */
		b_size;		/* Size of buffer in bytes */
} Buffr;

static struct cpioinfo TmpSt;

/* Generic header format */

static
struct gen_hdr {
	ulong	g_magic,	/* Magic number field */
		g_ino;		/* Inode number of file */
	mode_t	g_mode;		/* Mode of file */
	uid_t	g_uid;		/* Uid of file */
	gid_t	g_gid;		/* Gid of file */
	ulong	g_nlink;	/* Number of links */
	time_t	g_mtime;	/* Modification time */
	long	g_filesz;	/* Length of file */
	ulong	g_dev,		/* File system of file */
		g_rdev,		/* Major/minor numbers of special files */
		g_namesz,	/* Length of filename */
		g_cksum;	/* Checksum of file */
	char	g_gname[32],
		g_uname[32],
		g_version[2],
		g_tmagic[6],
		g_typeflag;
	char	*g_tname,
		*g_prefix,
		*g_nam_p;	/* Filename */
} Gen, *G_p;

/* Data structure for handling multiply-linked files */
static
char	prebuf[155],
	nambuf[100],
	fullnam[256];


static
struct Lnk {
	short	L_cnt,		/* Number of links encountered */
		L_data;		/* Data has been encountered if 1 */
	struct gen_hdr	L_gen;	/* gen_hdr information for this file */
	struct Lnk	*L_nxt_p,	/* Next file in list */
			*L_bck_p,	/* Previous file in list */
			*L_lnk_p;	/* Next link for this file */
} Lnk_hd;

static
struct hdr_cpio	Hdr;

static
struct stat	ArchSt,	/* stat(2) information of the archive */
		SrcSt,	/* stat(2) information of source file */
		DesSt;	/* stat(2) of destination file */

/*
 * bin_mag: Used to validate a binary magic number,
 * by combining two bytes into an unsigned short.
 */

static
union bin_mag{
	unsigned char b_byte[2];
	ushort b_half;
} Binmag;

static
union tblock *Thdr_p;	/* TAR header pointer */

/*
 * swpbuf: Used in swap() to swap bytes within a halfword,
 * halfwords within a word, or to reverse the order of the 
 * bytes within a word.  Also used in mklong() and mkshort().
 */

static
union swpbuf {
	unsigned char	s_byte[4];
	ushort	s_half[2];
	ulong	s_word;
} *Swp_p;

static
char	Adir,			/* Flags object as a directory */
	Aspec,			/* Flags object as a special file */
	Time[50],		/* Array to hold date and time */
	*Ttyname = "/dev/tty",  /* Default terminal interface device */
	T_lname[100],		/* Array to hold links name for tar */
	*Buf_p,			/* Buffer for file system I/O */
	*Empty,			/* Empty block for TARTYP headers */
	*Full_p,		/* Pointer to full pathname */
	*Efil_p,		/* -E pattern file string */
	*Eom_p = ":32:Change to part %d and press RETURN key. [q] ",
	*Fullnam_p,		/* Full pathname */
	*Hdr_p,			/* -H header type string */
	*IOfil_p,		/* -I/-O input/output archive string */
	*Lnkend_p,		/* Pointer to end of Lnknam_p */
	*Lnknam_p,		/* Buffer for linking files with -p option */
	*Nam_p,			/* Array to hold filename */
	*Own_p,			/* New owner login id string */
	*Renam_p,		/* Buffer for renaming files */
	*Symlnk_p,		/* Buffer for holding symbolic link name */
	*Over_p,		/* Holds temporary filename when overwriting */
	**Pat_pp = 0,		/* Pattern strings */
	noeraseflag;		/* Flags header got flushed, can't be erased */

static
int	Append = 0,	/* Flag set while searching to end of archive */
	Archive,	/* File descriptor of the archive */
	Bufsize = BUFSZ,	/* Default block size */
	Buf_error = 0,	/* I/O error occured during buffer fill */
	Device,		/* Device type being accessed (used with libgenIO) */
	Dir_mode,	/* Mode of missing directories that we must create */
	Error_cnt = 0,	/* Cumulative count of I/O errors */
	Exists_flag,	/* Flag for if the directory already exists */
	Finished = 1,	/* Indicates that a file transfer has completed */
	Hdrsz = ASCSZ,	/* Fixed length portion of the header */
	Hdr_type,		/* Flag to indicate type of header selected */
	Ifile,		/* File des. of file being archived */
	Ofile,		/* File des. of file being extracted from archive */
	Oldstat = 0,	/* Create an old style -c hdr (small dev's)	*/
	Onecopy = 0,	/* Flags old vs. new link handling */
      	Orig_umask,	/* Original umask of process */
	Pad_val = 0,	/* Indicates the number of bytes to pad (if any) */
	Target_mld = 0,	/* Flag for if target dir is an mld */
	Volcnt = 1,	/* Number of archive volumes processed */
	Verbcnt = 0,	/* Count of number of dots '.' output */
	Vflag = 0,	/* Verbose flag (-V) */
	vflag = 0,	/* Verbose flag (-v) */
	Eomflag = 0,
	Dflag = 0;

static
mode_t	Def_mode = 0777;	/* Default file/directory protection modes */

static
gid_t	Lastgid = -1;	/* Used with -t & -v to record current gid */

static
uid_t	Lastuid = -1;	/* Used with -t & -v to record current uid */

static
int	extent_op = OCe_WARN;	/* Used with -e as type of propagation */

struct	vx_extcpio {
	int	magic;
	struct	vx_ext extinfo;
} vx_extcpio;

static
long	Args,		/* Mask of selected options */
	Blocks,		/* Number of full blocks transferred */
	Max_filesz,	/* Maximum file size from ulimit(2) */
	Max_namesz = APATH,	/* Maximum size of pathnames/filenames */
	SBlocks,	/* Cumulative char count for short reads */
	Pagesize;	/* Machine page size */

static
FILE	*Ef_p,			/* File pointer of pattern input file */
	*Err_p = stderr,	/* File pointer for error reporting */
	*Out_p = stdout,	/* File pointer for non-archive output */
	*Rtty_p,		/* Input file pointer for interactive rename */
	*Wtty_p,		/* Output file pointer for interactive rename */
	*Tty_p;			/* File pointer for terminal interface device */

static
ushort	Ftype = S_IFMT;		/* File type mask */

static
uid_t	Uid,			/* Uid of invoker */
	priv_id;		/* Super-user id if there is one, else -1 */

static int lpm = 1,	 	/* default file base mechanism installed */
	   privileged,		/* flag indicating if process is privileged */
	   aclpkg = 1,	 	/* ACL security package toggle */
	   macpkg = 1,	 	/* MAC security package toggle */
	   mldpriv = 1,  	/* indicate whether the invoking user has the
		            	   P_MULTIDIR privilege to create an MLD */
	   lvlpriv = 1;  	/* indicate whether the invoking user has the
		            	   P_SETPLEVEL privilege to change the level of 
		            	   the process and whether the process level is
			    	   successfully obtained */

static lid_t Proc_lvl = 0, 	/* level of process */
	     File_lvl;		/* level of file */

/* Enhanced Application Compatibility Support */
#define MAX_NAMELEN	14	/* max length of file name (-T option) */
static unsigned Mediasize = 0; 	/* default: ignore media size; (-K option) */	
static unsigned Mediaused = 0; 	/* accumulated media used, Kbytes */
/* End Enhanced Application Compatibility Support */

static const char
	mutex[] = "-%c and -%c are mutually exclusive.",
	mutexid[] = ":33",
	badaccess[] = "Cannot access \"%s\"",
	badaccessid[] = ":34",
	badaccarch[] = "Cannot access the archive",
	badaccarchid[] = ":35",
	badcreate[] = "Cannot create \"%s\"",
	badcreateid[] = ":36",
	badcreatdir[] = "Cannot create directory for \"%s\"",
	badcreatdirid[] = ":37",
	badfollow[] = "Cannot follow \"%s\"",
	badfollowid[] = ":38",
	badread[] = "Read error in \"%s\"",
	badreadid[] = ":39",
	badreadsym[] = "Cannot read symbolic link \"%s\"",
	badreadsymid[] = ":40",
	badreadtty[] = "Cannot read \"%s\"",
	badreadttyid[] = ":1097",
	badreminc[] = "Cannot remove incomplete \"%s\"",
	badremincid[] = ":42",
	badremtmp[] = "Cannot remove temp file \"%s\"",
	badremtmpid[] = ":43",
	badremincdir[] = "Cannot remove incomplete \"%s\"; directory not empty",
	badremincdirid[] = ":1100",
	badremtmpdir[] = "Cannot remove temp directory \"%s\"; directory not empty",
	badremtmpdirid[] = ":1101",
	badwrite[] = "Write error in \"%s\"",
	badwriteid[] = ":44",
	badinit[] = "Error during initialization",
	badinitid[] = ":45",
	sameage[] = "Existing \"%s\" same age or newer",
	sameageid[] = ":46",
	badcase[] = "Impossible case.",
	badcaseid[] = ":47",
	badhdr[] = "Impossible header type.",
	badhdrid[] = ":48",
	nomem[] = "Out of memory",
	nomemid[] = ":49",
	badappend[] = "Cannot append to this archive",
	badappendid[] = ":50",
	badchmod[] = "chmod() failed on \"%s\"",
	badchmodid[] = ":51",
	badchown[] = "chown() failed on \"%s\"",
	badchownid[] = ":52",
	badpasswd[] = "Cannot get passwd information for %s",
	badpasswdid[] = ":53",
	badgroup[] = "Cannot get group information for %s",
	badgroupid[] = ":54",
	badorig[] = "Cannot recover original \"%s\"",
	badorigid[] = ":55";

/*
 *Procedure:     main
 *
 * Restrictions:
 *               acl(2):		none
 *               lvlproc(2):	none
 *               setlocale:	none
 */

/*
 * main: Call setup() to process options and perform initializations,
 * and then select either copy in (-i), copy out (-o), or pass (-p) action.
 */

main(argc, argv)
char **argv;
int argc;
{
	int gotname = 0, doarch = 0;
	lid_t tmp_lid;


	/* Determine whether id base or file base mechanism is installed. */
	if ((priv_id = secsys(ES_PRVID, 0)) >= 0)
		lpm = 0;	/* id base mechanism is installed */

	/* Determine if process is privileged.  If so, *
	 * the privileged flag will be set.            */
	Uid=getuid();
	if ((privileged = getpriv()) == -1)
		exit(1);

	/* Save original umask of process. */
	Orig_umask = (int)umask(0);
	(void) umask(Orig_umask);

	/* determine whether the ACL security package is installed */
	if ((acl("/", ACL_CNT, 0, 0) == -1) && (errno == ENOPKG))
		aclpkg = 0;

	/* determine whether the MAC security package is installed */
	if ((lvlproc(MAC_GET, &tmp_lid) != 0) && (errno == ENOPKG))
		macpkg = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:cpio");

	setup(argc, argv);
	if (signal(SIGINT, sigint) == SIG_IGN)
		(void)signal(SIGINT, SIG_IGN);
	switch (Args & (OCi | OCo | OCp)) {
	case OCi: /* COPY IN */
		/*
		 * If no -k, cpio detects header type. If -k 
		 * but no -c or -H, cpio detects header type.
		 */
		if ((!(Args & OCk)) || ((Args & OCk) && (!(Args & OCc) && !(Args & OCH))))
			Hdr_type = NONE;

		while (gethdr()) {
			/* Enhanced Application Compatibility Support */
			if ((Args & OCT) && (Gen.g_namesz > MAX_NAMELEN+1)) 
				truncate_sco(Gen.g_nam_p);
			/* End Enhanced Application Compatibility Support */
			file_in();
		}
		/* Do not count "extra" "read-ahead" buffered data */
		if (Buffr.b_cnt > Bufsize)
			Blocks -= (Buffr.b_cnt / Bufsize);
		break;
	case OCo: /* COPY OUT */
		if (Args & OCA)
			scan4trail();
		while ((gotname = getname()) != 0) {
			if (gotname == 1) {
				file_out();
				doarch++;
			}
		}
		if (doarch > 0)
			write_trail();
		break;
	case OCp: /* PASS */
		while ((gotname = getname()) != 0)
			if (gotname == 1)
				file_pass();
		break;
	default:
		msg(EXT, ":56", "Impossible action.");
	}
	Blocks = (Blocks * Bufsize + SBlocks + 0x1FF) >> 9;
	msg(EPOST, ":57", "%ld blocks", Blocks);
	if (Error_cnt == 1)
		msg(EPOST, ":58", "1 error");
	else if (Error_cnt > 1)
		msg(EPOST, ":771", "%d errors", Error_cnt);
	exit(Error_cnt);
}


/*
 * Procedure:	  getpriv
 *
 * Restrictions:  filepriv: 	none
 *		  procpriv:	none
 *
 */

/*
 * Find out if the process is privileged (return 1) or
 * not (return 0).  If the process' working privilege set
 * is at least what the privileges on the cpio binary are,
 * then the process is privileged.  Special case:  if the
 * privileges are equal, but both 0, this is NOT privileged.
 */

static
int 
getpriv()
{
	priv_t procmask[NPRIVS];	/* process' max priv mask */
	priv_t filemask[NPRIVS];	/* file's priv mask */
	priv_t procbuff[NPRIVS * 2];	/* process' max & working privs */
	priv_t filebuff[NPRIVS];	/* file's fixed & inheritable privs */
	int i, j;
	int procprivs;			/* # of process privs (max & working) */
	int fileprivs;			/* # of file privs for cpio binary */
	int workprivs = 0;		/* # of process privs in working set only */

	/* Check if lpm is installed.  If not, check *
	 * if Uid is privileged.		     */		

	if (!lpm) {
		if (Uid == priv_id)
			return(1);
	}

	/* Check the process' privileges against cpio's privileges *
	 * to determine if the process is "privileged".  	   */

	/* Initialize procmask and filemask to all 0's. */

	for (i = 0; i < NPRIVS; i++) {
		procmask[i] = 0;
		filemask[i] = 0;
	}
		
	/* Determine what privileges are associated with *
	 * process.  This includes both max and working. */

	if ((procprivs = procpriv(GETPRV, procbuff, NPRIVS * 2)) == -1) {
		(void)pfmt(stderr, MM_ERROR, 
			":825:Cannot retrieve process privileges");
		return(-1);
	}

	/* Set corresponding procmask bits when we find *
	 * what working privileges are set.             */

	for (i = 0; i < procprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if (procbuff[i] == pm_work(j)) {
				procmask[j] = 1;
				workprivs++;
				break;
			}
		}
	}

	/* If lpm is not installed (and we already know that  *
	 * Uid is not privileged), check if the number of     *
	 * privileges in the process' working set is equal to *
	 * the total number of privileges.  If not, the       *
	 * process is not "privileged".			      */

	if (!lpm) {
		if (workprivs == NPRIVS)
			return(1);
		else
			return(0);
	}

	/* Determine what privileges are associated with *
	 * cpio.  This includes inheritable and fixed.   */

	if ((fileprivs = filepriv(CPIO_PATH, GETPRV, filebuff, NPRIVS)) == -1) {
		(void)pfmt(stderr, MM_ERROR, 
			":890:Cannot retrieve file privileges");
		return(-1);
	}

	/* If the number of working privileges and the number of file *
	 * privileges are both 0, we can stop here; the process       *
	 * is not privileged.					      */
	if (workprivs == 0 && fileprivs == 0)
		return(0);

	/* Set corresponding filemask bits when we find   *
	 * what fixed and inheritable privileges are set. */

	for (i = 0; i < fileprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if ((filebuff[i] == pm_fixed(j)) || 
			    (filebuff[i] == pm_inher(j))) {
				filemask[j] = 1;
				break;
			}
		}
	}

	/* Determine if everything that is set in the filemask *
	 * is set in the procmask (i.e. if the process has at  *
	 * least the privileges that cpio has).		       */

	for (i = 0; i < NPRIVS; i++) {
		if (procmask[i] != filemask[i]) {
			if (filemask[i] == 1) {
				/* This means that cpio has a privilege *
				 * that process does not, so process is *
				 * not "privileged."                    */
				return(0);
			}
		}
	}

	/* If it gets here, then process has at least *
	 * the privileges that cpio has, so process   *
	 * is "privileged".			      */
			
	return(1);
}


/*
 * Procedure:     add_lnk
 *
 * Restrictions:  none
 */

/*
 * add_lnk: Add a linked file's header to the linked file data structure.
 * Either adding it to the end of an existing sub-list or starting
 * a new sub-list.  Each sub-list saves the links to a given file.
 */

static
struct Lnk *
add_lnk(l_p)
register struct Lnk **l_p;
{
	register struct Lnk *t1l_p, *t2l_p, *t3l_p;

	t2l_p = Lnk_hd.L_nxt_p;
	while (t2l_p != &Lnk_hd) {
		if (t2l_p->L_gen.g_ino == G_p->g_ino && t2l_p->L_gen.g_dev == G_p->g_dev)
			break; /* found */
		t2l_p = t2l_p->L_nxt_p;
	}
	if (t2l_p == &Lnk_hd)
		t2l_p = (struct Lnk *)NULL;
	t1l_p = (struct Lnk *)malloc(sizeof(struct Lnk));
	if (t1l_p == (struct Lnk *)NULL)
		msg(EXT, nomemid, nomem);
	t1l_p->L_lnk_p = (struct Lnk *)NULL;
	t1l_p->L_gen = *G_p; /* structure copy */
	t1l_p->L_gen.g_nam_p = (char *)malloc((unsigned int)G_p->g_namesz);
	if (t1l_p->L_gen.g_nam_p == (char *)NULL)
		msg(EXT, nomemid, nomem);
	(void)strcpy(t1l_p->L_gen.g_nam_p, G_p->g_nam_p);
	if (t2l_p == (struct Lnk *)NULL) { /* start new sub-list */
		t1l_p->L_nxt_p = &Lnk_hd;
		t1l_p->L_bck_p = Lnk_hd.L_bck_p;
		Lnk_hd.L_bck_p = t1l_p->L_bck_p->L_nxt_p = t1l_p;
		t1l_p->L_lnk_p = (struct Lnk *)NULL;
		t1l_p->L_cnt = 1;
		t1l_p->L_data = Onecopy ? 0 : 1;
		t2l_p = t1l_p;
	} else { /* add to existing sub-list */
		t2l_p->L_cnt++;
		t3l_p = t2l_p;
		while (t3l_p->L_lnk_p != (struct Lnk *)NULL) {
			t3l_p->L_gen.g_filesz = G_p->g_filesz;
			t3l_p = t3l_p->L_lnk_p;
		}
		t3l_p->L_gen.g_filesz = G_p->g_filesz;
		t3l_p->L_lnk_p = t1l_p;
	}
	*l_p = t2l_p;
	return(t1l_p);
}


/*
 * Procedure:     bfill
 *
 * Restrictions:
 *                g_read	none
 */

/*
 * bfill: Read req_cnt bytes (out of filelen bytes) from the I/O buffer,
 * moving them to rd_buf_p.  When there are no bytes left in the I/O buffer,
 * Fillbuf is set and the I/O buffer is filled.  The variable dist is the 
 * distance to lseek if an I/O error is encountered with the -k option set
 * (converted to a multiple of Bufsize).
 */

static
int
bfill()
{
 	register int i = 0, rv;
	static int eof = 0;

	if (!Dflag) {
	while ((Buffr.b_end_p - Buffr.b_in_p) >= Bufsize) {
		errno = 0;
		if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0) {
			if (errno == ENOSPC)
                                if (((Buffr.b_end_p - Buffr.b_in_p) >= Bufsize)                                 && (Eomflag == 0)) {
                    			Eomflag = 1;
                                        return(1);
                                }
                        if (Eomflag) {
                                (void)chgreel(INPUT);
                                continue;
			} else if (Args & OCk) {
				if (i++ > MX_SEEKS)
					msg(EXT, ":60", "Cannot recover.");
				if (lseek(Archive, Bufsize, SEEK_REL) < 0)
					msg(EXTN, ":61", "lseek() failed");
				Error_cnt++;
				Buf_error++;
				rv = 0;
				continue;
			} else
				ioerror(INPUT);
		} /* (rv = g_read(Device, Archive ... */
		Buffr.b_in_p += rv;
		Buffr.b_cnt += (long)rv;
		if (rv == Bufsize)
			Blocks++;
		else if (!rv) {
			if (!eof) {
				eof = 1;
				break;
			}
			return(-1);
		} else
			SBlocks += rv;
	} /* (Buffr.b_end_p - Buffr.b_in_p) <= Bufsize */
	}
	else {
		errno = 0;
		if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0) {
			return(-1);
                } /* (rv = g_read(Device, Archive ... */
                Buffr.b_in_p += rv;
                Buffr.b_cnt += (long)rv;
                if (rv == Bufsize)
                        Blocks++;
                else if (!rv) {
                        if (!eof) {
                                eof = 1;
				return(rv);
                        }
                        return(-1);
                } else
                        SBlocks += rv;
	}
	return(rv);
}


/*
 * Procedure:     bflush
 *
 * Restrictions:
 *                g_write: none
 */

/*
 * bflush: Move wr_cnt bytes from data_p into the I/O buffer.  When the
 * I/O buffer is full, Flushbuf is set and the buffer is written out.
 */

static
void
bflush()
{
	register int rv;

	while (Buffr.b_cnt >= Bufsize) {
		errno = 0;

		/* Enhanced Application Compatibility Support		*/

		if ((Mediasize) && 
			(Mediaused + (Bufsize >> 10)  > Mediasize)) {
			rv = chgreel(OUTPUT);
			Mediaused = 0;
		}

		/* End Enhanced Application Compatibility Support   	*/

		if ((rv = g_write(Device, Archive, Buffr.b_out_p, Bufsize)) < 0) {
			if (errno == ENOSPC && !Dflag)
				rv = chgreel(OUTPUT);
			else
				ioerror(OUTPUT);
		}
		else if (rv == 0)
			/* If write returns 0, something is wrong, maybe
		 	 * a floppy problem, so bail out now.
		  	 */
			ioerror(OUTPUT);

		/* Enhanced Application Compatibility Support   	*/
		if (Mediasize)
			Mediaused += (rv + 0x3ff) >> 10;  /* round up to 1K units */
		/* End Enhanced Application Compatibility Support   	*/

		Buffr.b_out_p += rv;
		Buffr.b_cnt -= (long)rv;
		if (rv == Bufsize)
			Blocks++;
		else if (rv > 0)
			SBlocks += rv;
	}
	rstbuf();
}

/*
 * Procedure:     chgreel
 *
 * Restrictions:
 *                fopen:	none
 *                fgets:	none
 *                open(2):	none
 *                g_init:	none
 *                g_write:	none
 */

/*
 * flush_tty: Flush tty input stream.  If user accidentally
 * bounced on <RETURN> key on responding to end-of-medium prompt,
 * this will throw out those extra characters. Otherwise,
 * those characters would be taken as the response to next
 * end-of-medium prompt, and not allow time to change medium.
 */
static
void
flush_tty(tty_p)
	FILE *tty_p;
{
	register int fd;

	fd = fileno(tty_p);
	if (isatty(fd)) {
		tcflush(fd, TCIFLUSH);
		fflush(tty_p);
	}
}

/*
 * chgreel: Determine if end-of-medium has been reached.  If it has,
 * close the current medium and prompt the user for the next medium.
 */

static
int
chgreel(dir)
register int dir;
{
	register int lastchar, tryagain, rv;
	int tmpdev;
	char str[APATH];

	if (Tty_p == (FILE *)NULL) {
		/*
	 	* For -G, this should not happen.
		* Ttyname should have already been
	 	* opened in setup().
	 	*/
		Tty_p = fopen(Ttyname, "r+");
		if (Tty_p != (FILE *)NULL)
			setbuf(Tty_p, NULL);
		else
			msg(EXT, ":65", "Cannot open \"%s\"", Ttyname);
	}


	if (dir) {
		(void)pfmt(Tty_p, MM_NOSTD, ":1071:\007End of medium on output.\n");
		(void)fflush(Tty_p);
	}
	else {
		(void)pfmt(Tty_p, MM_NOSTD, ":1072:\007End of medium on input.\n");
		(void)fflush(Tty_p);
	}
	(void)close(Archive);
	Volcnt++;
	for (;;) {
		do {  /* while tryagain */
			tryagain = 0;
			if (IOfil_p) {
				flush_tty(Tty_p);
				if (Args & OCM) {
					/* If user specified message,
					 * must use MM_NOGET so it
					 * won't try to retrieve it 
					 * from the message catalog.
					 */
					(void)pfmt(Tty_p, MM_NOGET|MM_NOSTD, Eom_p, Volcnt);
					(void)fflush(Tty_p);
				}
				else {
					(void)pfmt(Tty_p, MM_NOSTD, Eom_p, Volcnt);
					(void)fflush(Tty_p);
				}
				if (fgets(str, sizeof(str), Tty_p) == (char *)NULL)
					msg(EXT, badreadttyid, badreadtty);
				/*
				 * When doing input and output on a pseudo-tty
				 * (-G option), the file position must be reset
				 * after a read. 
				 */
				rewind(Tty_p);

				switch (*str) {
				case '\n':
					(void)strcpy(str, IOfil_p);
					break;
				case 'q':
					exit(Error_cnt);
				default:
					sleep(1);
					tryagain = 1;
					continue;
				}
			} else {
				flush_tty(Tty_p);
				(void)pfmt(Tty_p, MM_NOSTD, ":1070:To continue, type device/file name when ready.\n");
				(void)fflush(Tty_p);
				if (fgets(str, sizeof(str), Tty_p) == (char *)NULL)
					msg(EXT, badreadttyid, badreadtty, Ttyname);
				lastchar = strlen(str) - 1;
				if (*(str + lastchar) == '\n') /* remove '\n' */
					*(str + lastchar) = '\0';
				if (!*str) { /* user hit RETURN */
					tryagain = 1;
					continue;
				}
			}
			if ((Archive = open(str, dir)) < 0) {
				msg(ERRN, ":65", "Cannot open \"%s\"", str);
				sleep(1);
				tryagain = 1;
				/* If trying again, don't count error. */
				Error_cnt--;
			}
		} while (tryagain);

		(void)g_init(&tmpdev, &Archive);
		if (tmpdev != Device)
			msg(EXT, ":66", "Cannot change media types in mid-stream.");
		if (dir == INPUT)
			break;
		else { /* dir == OUTPUT */
			errno = 0;
			if ((rv = g_write(Device, Archive, Buffr.b_out_p, Bufsize)) == Bufsize)
				break;
			else {
				msg(ERR, ":67", "Cannot write on this medium, try another.");
				(void)close(Archive);
			}
		}
	} /* ;; */
	Eomflag = 0;
	return(rv);
}

/*
 * Procedure:     ckname
 *
 * Restrictions:
 *                pfmt		none
 *                fflush	none
 *                fgets		none
 */

/*
 * ckname: Check filenames against user specified patterns,
 * and/or ask the user for new name when -r is used.
 */

static
int
ckname()
{
	register int lastchar;

	if (G_p->g_namesz > Max_namesz) {
		msg(ERR, ":68", "Name exceeds maximum length - skipped.");
		return(F_SKIP);
	}
	if (Pat_pp && !matched())
		return(F_SKIP);
	if ((Args & OCr) && !Adir) { /* rename interactively */
		(void)pfmt(Wtty_p, MM_NOSTD, ":69:Rename \"%s\"? ", G_p->g_nam_p);
		(void)fflush(Wtty_p);
		if (fgets(Renam_p, Max_namesz, Rtty_p) == (char *)NULL)
			msg(EXT, badreadttyid, badreadtty, Ttyname);
		if (feof(Rtty_p))
			exit(Error_cnt);
		lastchar = strlen(Renam_p) - 1;
		if (*(Renam_p + lastchar) == '\n') /* remove trailing '\n' */
			*(Renam_p + lastchar) = '\0';
		if (*Renam_p == '\0') {
			msg(POST, ":70", "%s Skipped.", G_p->g_nam_p);
			*G_p->g_nam_p = '\0';
			return(F_SKIP);
		} else if (strcmp(Renam_p, ".")) {
			G_p->g_nam_p = Renam_p;
		}
	}
	VERBOSE((Args & OCt), G_p->g_nam_p);
	if (Args & OCt)
		return(F_SKIP);
	return(F_EXTR);
}

/*
 * Procedure:     ckopts
 *
 * Restrictions:
 *                fopen:		none
 *                open(2):	none
 *                getpwnam:	none
 */

/*
 * ckopts: Check the validity of all command line options.
 */

static
void
ckopts(mask)
register long mask;
{
	register int oflag;
	register char *t_p;
	register long errmsk;
	char inval_opt;

	/* Check for invalid options with -i. */
	if (mask & OCi) {
		errmsk = mask & INV_MSK4i;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCa)
				inval_opt = 'a';
			else if (mask & OCo)
				inval_opt = 'o';
			else if (mask & OCp)
				inval_opt = 'p';
			else if (mask & OCA)
				inval_opt = 'A';
			else if (mask & OCL)
				inval_opt = 'L';
			else if (mask & OCO)
				inval_opt = 'O';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCK)
				inval_opt = 'K';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'i');
		}
	}

	/* Check for invalid options with -o. */
	else if (mask & OCo) {
		errmsk = mask & INV_MSK4o;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCi)
				inval_opt = 'i';
			else if (mask & OCk)
				inval_opt = 'k';
			else if (mask & OCm)
				inval_opt = 'm';
			else if (mask & OCp)
				inval_opt = 'p';
			else if (mask & OCr)
				inval_opt = 'r';
			else if (mask & OCt)
				inval_opt = 't';
			else if (mask & OCE)
				inval_opt = 'E';
			else if (mask & OCI)
				inval_opt = 'I';
			else if (mask & OCR)
				inval_opt = 'R';
			else if (mask & OC6)
				inval_opt = '6';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCT)
				inval_opt = 'T';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'o');
		}
	}

	/* Check for invalid options with -p. */
	else if (mask & OCp) {
		errmsk = mask & INV_MSK4p;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCf)
				inval_opt = 'f';
			else if (mask & OCi)
				inval_opt = 'i';
			else if (mask & OCk)
				inval_opt = 'k';
			else if (mask & OCo)
				inval_opt = 'o';
			else if (mask & OCr)
				inval_opt = 'r';
			else if (mask & OCt)
				inval_opt = 't';
			else if (mask & OCA)
				inval_opt = 'A';
			else if (mask & OCE)
				inval_opt = 'E';
			else if (mask & OCG)
				inval_opt = 'G';
			else if (mask & OCH)
				inval_opt = 'H';
			else if (mask & OCI)
				inval_opt = 'I';
			else if (mask & OCO)
				inval_opt = 'O';
			else if (mask & OC6)
				inval_opt = '6';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCK)
				inval_opt = 'K';
			else if (mask & OCT)
				inval_opt = 'T';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'p');
		}
	}

	/* None of -i, -o, or -p were specified. */
	else
		msg(USAGE, ":71", "One of -i, -o or -p must be specified.");

	/* Check for mutually exclusive options. */
	/* This bunch are the mutually exclusive header formats. */
	if ((mask & OCc) && (mask & OCH) && (strcmp("tar", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("TAR", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("ustar", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("USTAR", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("odc", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OC6))
		msg(USAGE, mutexid, mutex, 'c', '6');
	if ((mask & OCH) && (mask & OC6))
		msg(USAGE, mutexid, mutex, 'H', '6');

	/* These are various other mutually exclusive options. */
	if ((mask & OCB) && (mask & OCC))
		msg(USAGE, mutexid, mutex, 'B', 'C');

	if ((mask & OCM) && !((mask & OCI) || (mask & OCO)))
		msg(USAGE, ":72", "-M not meaningful without -O or -I.");
	if ((mask & OCA) && !(mask & OCO))
		msg(USAGE, ":73", "-A requires the -O option.");

	if (Bufsize <= 0) {
		msg(ERR, ":74", "Illegal size given for -C option.");
		exit(Error_cnt);
	}

	if (mask & OCH) {
		t_p = Hdr_p;
		while (*t_p != NULL) {
			if (isupper(*t_p))
				*t_p = 'a' + (*t_p - 'A');
			t_p++;
		}
		if (!strcmp("odc", Hdr_p)) {
			Hdr_type = CHR;
			Max_namesz = CPATH;
			Onecopy = 0;
			Oldstat = 1;
		} 
		else if (!strcmp("crc", Hdr_p)) {
			Hdr_type = CRC;
			Max_namesz = APATH;
			Onecopy = 1;
		} 
		else if (!strcmp("tar", Hdr_p)) {
			if(Args & OCo) {
				Hdr_type = USTAR;
				Max_namesz = HNAMLEN - 1;
			} 
			else {
				Hdr_type = TAR;
				Max_namesz = TNAMLEN - 1;
			}
			Onecopy = 0;
		} 
		else if (!strcmp("ustar", Hdr_p)) {
			Hdr_type = USTAR;
			Max_namesz = HNAMLEN - 1;
			Onecopy = 0;
		} 
		else {
			msg(ERR, ":75", "Invalid header \"%s\" specified", Hdr_p);
			exit(Error_cnt);
		}
	}

	if (mask & OCr) {
		if (Rtty_p == (FILE *)NULL) {
			Rtty_p = fopen(Ttyname, "r");
			setbuf(Rtty_p, NULL);
		}
		if (Wtty_p == (FILE *)NULL) {
			Wtty_p = fopen(Ttyname, "w");
			setbuf(Wtty_p, NULL);
		}
		if (Rtty_p == (FILE *)NULL || Wtty_p == (FILE *)NULL) {
			msg(ERR, ":65", "Cannot open \"%s\"", Ttyname);
			exit(Error_cnt);
		}
	}

	if ((mask & OCE) && (Ef_p = fopen(Efil_p, "r")) == (FILE *)NULL) {
		msg(ERR, ":77", "Cannot open \"%s\" to read patterns", Efil_p);
		exit(Error_cnt);
	}

	if ((mask & OCI) && (Archive = open(IOfil_p, O_RDONLY)) < 0) {
		msg(ERR, ":78", "Cannot open \"%s\" for input", IOfil_p);
		exit(Error_cnt);
	}

	if (mask & OCO) {
		if (mask & OCA) {
			if ((Archive = open(IOfil_p, O_RDWR)) < 0) {
				msg(ERR, ":79", "Cannot open \"%s\" for append", IOfil_p);
				exit(Error_cnt);
			}
		} else {
			oflag = (O_WRONLY | O_CREAT | O_TRUNC);
			if ((Archive = open(IOfil_p, oflag, 0666)) < 0) {
				msg(ERR, ":80", "Cannot open \"%s\" for output", IOfil_p);
				exit(Error_cnt);
			}
		}
	}

	if (mask & OCR) {
		if (!privileged) {
			msg(ERR, ":891", "Must be privileged for %s option", "-R");
			exit(Error_cnt);
		}
		if ((Rpw_p = getpwnam(Own_p)) == (struct passwd *)NULL) {
			msg(ERR, ":82", "Unknown user id: %s", Own_p);
			exit(Error_cnt);
		}
	}

	/* if forced extent attribute save - must be char archive if -o */
	if ((mask & OCo) && extent_op == OCe_FORCE &&
	    Hdr_type != ASC && Hdr_type != CRC) {
		msg(ERR, ":1087", "Attribute preservation requires -c option or -Hcrc option");
		exit(Error_cnt);
	}

	if ((mask & OCo) && !(mask & OCO))
		Out_p = stderr;

	/* Enhanced Application Compatibility Support */

	/* K option */
	if (Mediasize)	{
		if (Bufsize & 0x3ff) {
			if (mask & OCC)
				msg(ERR, ":1074", "Invalid argument \"%d\" for -C; must be multiple of 1K when used with -K", Bufsize);
			else
				msg(ERR, ":1075", "Must use -C to specify a 1K-multiple bufsize when using -K"); 
			exit(Error_cnt);
		
		}
	}

	/* End Enhanced Application Compatibility Support 	*/
}

/*
 * Procedure:     cksum
 *
 * Restrictions:
 *                read(2):	none
 */

/*
 * cksum: Calculate the simple checksum of a file (CRC) or header
 * (TARTYP (TAR and USTAR)).  For -o and the CRC header, the file is opened and 
 * the checksum is calculated.  For -i and the CRC header, the checksum
 * is calculated as each block is transferred from the archive I/O buffer
 * to the file system I/O buffer.  The TARTYP (TAR and USTAR) headers calculate 
 * the simple checksum of the header (with the checksum field of the 
 * header initialized to all spaces (\040).
 */

static
long 
cksum(hdr, byt_cnt)
char hdr;
int byt_cnt;
{
	register char *crc_p, *end_p;
	register int cnt;
	register long checksum = 0L, lcnt, have;

	switch (hdr) {
	case CRC:
		if (Args & OCi) { /* do running checksum */
			end_p = Buffr.b_out_p + byt_cnt;
			for (crc_p = Buffr.b_out_p; crc_p < end_p; crc_p++)
				checksum += (long)*crc_p;
			break;
		}
		/* OCo - do checksum of file */
		lcnt = G_p->g_filesz;
		while (lcnt > 0) {
			have = (lcnt < CPIOBSZ) ? lcnt : CPIOBSZ;
			errno = 0;
			if (read(Ifile, Buf_p, have) != have) {
				msg(ERR, ":83", "Error computing checksum.");
				checksum = -1L;
				break;
			}
			end_p = Buf_p + have;
			for (crc_p = Buf_p; crc_p < end_p; crc_p++)
				checksum += (long)*crc_p;
			lcnt -= have;
		}
		if (lseek(Ifile, 0, SEEK_ABS) < 0)
			msg(ERRN, ":84", "Cannot reset file after checksum");
		break;
	case TARTYP: /* TAR and USTAR */
		crc_p = Thdr_p->tbuf.t_cksum;
		for (cnt = 0; cnt < TCRCLEN; cnt++) {
			*crc_p = '\040';
			crc_p++;
		}
		crc_p = (char *)Thdr_p;
		for (cnt = 0; cnt < TARSZ; cnt++) {
			checksum += (long)*crc_p;
			crc_p++;
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	} /* hdr */
	return(checksum);
}

/*
 * Procedure:     creat_hdr
 *
 * Restrictions:
 *                open:	none
 *                getpwuid:	none
 *                getgrgid:	P_MACREAD
 */

/*
 * creat_hdr: Fill in the generic header structure with the specific
 * header information based on the value of Hdr_type.
 */

static
int
creat_hdr()
{
	register ushort ftype;
	char goodbuf[MAXNAM];
	char junkbuf[NAMSIZ];
	char abuf[PRESIZ];
	char *tmpbuf, *lastslash;
	int split, i, Alink;

	ftype = SrcSt.st_mode & Ftype;
	Adir = (ftype == S_IFDIR);
	Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO || ftype == S_IFNAM);
	Alink = (ftype == S_IFLNK);

	/* We want to check here if the file is really readable --
	   If we wait till later it's to late and cpio will fail */

	if (!Adir && !Aspec && !Alink) {
		if ((Ifile = open(Gen.g_nam_p, O_RDONLY)) < 0) {
			msg(ERR, ":65", "Cannot open \"%s\"", Gen.g_nam_p);
			return(0);
		}
	}
	switch (Hdr_type) {
	case BIN:
		Gen.g_magic = CMN_BIN;
		break;
	case CHR:
		Gen.g_magic = CMN_BIN;
		break;
	case ASC:
		Gen.g_magic = CMN_ASC;
		break;
	case CRC:
		Gen.g_magic = CMN_CRC;
		break;
	case USTAR:
		/* If the length of the fullname is greater than 256,
	   	print out a message and return.
		*/
		for (i = 0; i < MAXNAM; i++)
			goodbuf[i] = '\0';
		for (i = 0; i < NAMSIZ; i++)
			junkbuf[i] = '\0';
		for (i = 0; i < PRESIZ; i++)
			abuf[i] = '\0';
		i = 0;

		if ((split = strlen(Gen.g_nam_p)) > MAXNAM) {
			if (Ifile > 0)
				close(Ifile);
			msg(ERR, ":85", "%s: file name too long", Gen.g_nam_p);
			return(0);
		} else if (split > NAMSIZ) {
			/* The length of the fullname is greater than 100, so
		   	we must split the filename from the path
			*/
			(void)strcpy(&goodbuf[0], Gen.g_nam_p);
			tmpbuf = goodbuf;
			lastslash = strrchr(tmpbuf, '/');
			i = strlen(lastslash++);
			/* If the filename is greater than 100 we can't
		   	archive the file
			*/
			if (i > NAMSIZ) {
				if (Ifile > 0)
					close(Ifile);
				msg(WARN, ":86", "%s: filename is greater than %d",
				lastslash, NAMSIZ);
				return(0);
			}
			(void)strncpy(&junkbuf[0], lastslash, strlen(lastslash));
			/* If the prefix is greater than 155 we can't archive the
		   	file.
			*/
			if ((split - i) > PRESIZ) {
				if (Ifile > 0)
					close(Ifile);
				msg(WARN, ":87", "%s: prefix is greater than %d",
				Gen.g_nam_p, PRESIZ);
				return(0);
			}
			(void)strncpy(&abuf[0], &goodbuf[0], split - i);
			Gen.g_tname = junkbuf;
			Gen.g_prefix = abuf;
		} else {
			Gen.g_tname = Gen.g_nam_p;
		}
		(void)strcpy(Gen.g_tmagic, "ustar");
		(void)strcpy(Gen.g_version, "00");
		dpasswd = getpwuid(SrcSt.st_uid);
		if (dpasswd == (struct passwd *) NULL)
			msg(WARN, badpasswdid, badpasswd, Gen.g_nam_p);
		else
			(void)strncpy(&Gen.g_uname[0], dpasswd->pw_name, 32);
		procprivl (CLRPRV, MACREAD_W, 0);
		dgroup = getgrgid(SrcSt.st_gid);
		procprivl (SETPRV, MACREAD_W, 0);
		if (dgroup == (struct group *) NULL)
			msg(WARN, badgroupid, badgroup, Gen.g_nam_p);
		else
			(void)strncpy(&Gen.g_gname[0], dgroup->gr_name, 32);
		switch(ftype) {
			case S_IFDIR:
				Gen.g_typeflag = DIRTYPE;
				break;
			case S_IFREG:
				if (SrcSt.st_nlink > 1)
					Gen.g_typeflag = LNKTYPE;
				else
					Gen.g_typeflag = REGTYPE;
				break;
			case S_IFLNK:
				Gen.g_typeflag = SYMTYPE;
				break;
			case S_IFBLK:
				Gen.g_typeflag = BLKTYPE;
				break;
			case S_IFCHR:
				Gen.g_typeflag = CHRTYPE;
				break;
			case S_IFIFO:
				Gen.g_typeflag = FIFOTYPE;
				break;
			case S_IFNAM:
				Gen.g_typeflag = NAMTYPE;
				break;
		}
	/* FALLTHROUGH*/
	case TAR:
		T_lname[0] = '\0';
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}

	Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
	Gen.g_uid = SrcSt.st_uid;
	Gen.g_gid = SrcSt.st_gid;
	Gen.g_dev = SrcSt.st_dev;
	Gen.g_ino = SrcSt.st_ino;
	Gen.g_mode = SrcSt.st_mode;
	Gen.g_mtime = SrcSt.st_mtime;
	Gen.g_nlink = SrcSt.st_nlink;
	if (ftype == S_IFREG || ftype == S_IFLNK)
		Gen.g_filesz = SrcSt.st_size;
	else
		Gen.g_filesz = 0L;
	Gen.g_rdev = SrcSt.st_rdev;
	return(1);
}

/*
 * Procedure:     creat_lnk
 *
 * Restrictions:
 *                link(2):	none
 *                unlink(2):	none
 */

/*
 * creat_lnk: Create a link from the existing name1_p to name2_p.
 */

static
int
creat_lnk(name1_p, name2_p)
register char *name1_p, *name2_p;
{
	register int cnt = 0;

	do {
		errno = 0;
		if (!link(name1_p, name2_p)) {
			cnt = 0;
			break;
		} else if (errno == EEXIST) {
			/*
			 * If this next statement is true (i.e. same age or
			 * newer and no -u), no need to keep trying; just
			 * return.
			 */
			if (!(Args & OCu) && G_p->g_mtime <= DesSt.st_mtime) {
				msg(POST, sameageid, sameage, name2_p);
				return(0);
			}
			else if (unlink(name2_p) < 0)
				msg(ERRN, ":88", "Cannot unlink \"%s\"", name2_p);
		}
		cnt++;
	} while ((cnt < 2) && !missdir(name2_p));
	if (!cnt) {
		if (vflag)
			(void)pfmt(Err_p, MM_NOSTD, ":89:%s linked to %s\n", name1_p, name2_p);
		VERBOSE((Vflag|vflag), name2_p);
	} else if (cnt == 1)
		msg(ERRN, badcreatdirid, badcreatdir, name2_p);
	else if (cnt == 2)
		msg(WARNN, ":90", "Cannot link \"%s\" and \"%s\"", name1_p, name2_p);
	return(cnt);
}

/*
 * Procedure:     creat_spec
 *
 * Restrictions:
 *                stat(2):	none
 *                mkdir(2):	none
 *                mknod(2):	none
 */

/*
 * creat_spec:
 */

static
int
creat_spec()
{
	register char *nam_p;
	register int cnt, result, rv = 0;
	char *curdir;
	int lvl_change = 0; /* indicate whether level of process is changed */

	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		nam_p = G_p->g_nam_p;
	result = stat(nam_p, &DesSt);
	if (Adir) {
		curdir = strrchr(nam_p, '.');
		if (curdir != NULL && curdir[1] == NULL)
			return(1);
		else {
			if (!result && (Args & OCd)) {
				/* If here, directory already exists.*/
				Exists_flag = 1;
				rstfiles(U_KEEP);
				return(1);
			}
			if (!result || !(Args & OCd) || !strcmp(nam_p, ".") || !strcmp(nam_p, ".."))
				return(1);
		}
	} else if (!result && creat_tmp(nam_p) < 0)
		return(0);
	cnt = 0;

	do {
		/* change level of process to that of the source file */
		if (macpkg && (Args & OCp) && (Args  & OCm))
			lvl_change = chg_lvl_proc(Nam_p);
		if (Adir) {
			/* create an MLD if source directory is an MLD */
			if (macpkg && (Args & OCp) && (Args  & OCd)) {
				result = creat_mld(Nam_p, nam_p, G_p->g_mode);
			}
			else /* a regular directory is to be created */
				result = 1;

			/* a regular directory is to be created */
			if (result == 1)
				result = mkdir(nam_p, G_p->g_mode);
		}
		else if (Aspec)
			result = mknod(nam_p, (int)G_p->g_mode, (int)G_p->g_rdev);

		/* restore level of process */
		if (lvl_change)
			restore_lvl_proc();

		if (result >= 0) {
			/* transfer ACL of source file to destination file */
			if (Args & OCp)
				transferdac(Nam_p, nam_p, G_p->g_mode);
			cnt = 0;
			break;
		}
		cnt++;
	} while (cnt < 2 && !missdir(nam_p));


	switch (cnt) {
	case 0:
		rv = 1;
		rstfiles(U_OVER);
		break;
	case 1:
		msg(ERRN, badcreatdirid, badcreatdir, nam_p);
		if (*Over_p != '\0')
			rstfiles(U_KEEP);
		break;
	case 2:
		if (Adir)
			msg(ERRN, ":91", "Cannot create directory \"%s\"", nam_p);
		else if (Aspec)
			msg(ERRN, ":92", "mknod() failed for \"%s\"", nam_p);

		if (*Over_p != '\0')
			rstfiles(U_KEEP);
		break;
	default:
		msg(EXT, badcaseid, badcase);
	}
	return(rv);
}

/*
 * Procedure:     creat_tmp
 *
 * Restrictions:
 *                mktemp(2):	none
 *                rename(2):	none
 */

/*
 * creat_tmp:
 */

static
int
creat_tmp(nam_p)
char *nam_p;
{
	register char *t_p;

	if ((Args & OCp) && G_p->g_ino == DesSt.st_ino && G_p->g_dev == DesSt.st_dev) {
		msg(ERR, ":93", "Attempt to pass a file to itself.");
		return(-1);
	}
	if (G_p->g_mtime <= DesSt.st_mtime && !(Args & OCu)) {
		msg(POST, sameageid, sameage, nam_p);
		if (Ifile > 0)
			(void)close(Ifile);
		return(-1);
	}
	if (Aspec) {
		msg(ERR, ":94", "Cannot overwrite \"%s\"", nam_p);
		return(-1);
	}
	(void)strcpy(Over_p, nam_p);
	t_p = Over_p + strlen(Over_p);
	while (t_p != Over_p) {
		if (*(t_p - 1) == '/')
			break;
		t_p--;
	}
	(void)strcpy(t_p, "XXXXXX");
	(void)mktemp(Over_p);
	if (*Over_p == '\0') {
		msg(ERR, ":95", "Cannot get temporary file name.");
		return(-1);
	}
	if (rename(nam_p, Over_p) < 0) {
		msg(ERRN, ":772", "Cannot rename %s", nam_p);
		*Over_p = '\0';
		return(-1);
	}
	return(1);
}

/*
 * Procedure:     data_in
 *
 * Restrictions:
 *                write(2):	none
 */

/*
 * data_in:  If proc_mode == P_PROC, bread() the file's data from the archive 
 * and write(2) it to the open fdes gotten from openout().  If proc_mode ==
 * P_SKIP, or becomes P_SKIP (due to errors etc), bread(2) the file's data
 * and ignore it.  If the user specified any of the "swap" options (b, s or S),
 * and the length of the file is not appropriate for that action, do not
 * perform the "swap", otherwise perform the action on a buffer by buffer basis.
 * If the CRC header was selected, calculate a running checksum as each buffer
 * is processed.
 */

static
void
data_in(proc_mode)
register int proc_mode;
{
	register char *nam_p;
	register int cnt, pad;
	register long filesz, cksumval = 0L;
	register int rv, swapfile = 0;
	int	exterr = 0;
	int	noextend = 0;
	struct	vx_ext extbuf;
	struct	statvfs statvfsbuf;

	nam_p = G_p->g_nam_p;
	if ((G_p->g_mode & Ftype) == S_IFLNK && proc_mode != P_SKIP) {
		proc_mode = P_SKIP;
		VERBOSE((Vflag|vflag), nam_p);
	}
	if (Args & (OCb | OCs | OCS)) { /* verfify that swapping is possible */
		swapfile = 1;
		if (Args & (OCs | OCb) && G_p->g_filesz % 2) {
			msg(ERR, ":98", "Cannot swap bytes of \"%s\", odd number of bytes", nam_p);
			swapfile = 0;
		}
		if (Args & (OCS | OCb) && G_p->g_filesz % 4) {
			msg(ERR, ":99", "Cannot swap halfwords of \"%s\", odd number of halfwords", nam_p);
			swapfile = 0;
		}
	}

	extbuf.ext_size = 0;
	extbuf.a_flags = 0;
	noextend = 0;
	filesz = G_p->g_filesz;
	/* see if need to preserve extent information */
	if (proc_mode != P_SKIP && extent_op != OCe_IGNORE &&
	    vx_extcpio.magic == VX_CPIOMAGIC &&
	    (vx_extcpio.extinfo.ext_size ||
	     vx_extcpio.extinfo.reserve ||
	     vx_extcpio.extinfo.a_flags)) {
		if (fstatvfs(Ofile, &statvfsbuf) ||
		    strcmp(statvfsbuf.f_basetype, "vxfs") ||
		    ((long)vx_extcpio.extinfo.ext_size * 1024L) %
				(long)statvfsbuf.f_frsize ||
		    ((long)vx_extcpio.extinfo.reserve * 1024L) %
				(long)statvfsbuf.f_frsize)
			exterr = 1;
		 vx_extcpio.extinfo.ext_size /=
			(long)statvfsbuf.f_frsize / 1024L;
		 vx_extcpio.extinfo.reserve /=
			(long)statvfsbuf.f_frsize / 1024L;
		if ((vx_extcpio.extinfo.a_flags & VX_NOEXTEND) &&
		     filesz > vx_extcpio.extinfo.reserve) {
			noextend++;
			vx_extcpio.extinfo.a_flags &= ~VX_NOEXTEND;
		}
		if (exterr == 0) {
			if (ioctl(Ofile, VX_SETEXT, &vx_extcpio.extinfo) != 0) {
				exterr = 1;
			}
			extbuf = vx_extcpio.extinfo;
		}
		if (exterr) {
			if (extent_op == OCe_WARN) {
				exterr = 0;
				msg(WARNN, ":1096", "Cannot maintain attributes of %s", nam_p);
			} else {
				msg(ERRN, ":1096", "Cannot maintain attributes of %s", nam_p);
			}
		}
	}
	if (proc_mode != P_SKIP && !fstatvfs(Ofile, &statvfsbuf) &&
	    !strcmp(statvfsbuf.f_basetype, "vxfs") && filesz > MAXBSIZE) {
		extbuf.reserve = (daddr_t)(filesz + statvfsbuf.f_frsize - 1) /
			(long)statvfsbuf.f_frsize;
		extbuf.a_flags |= VX_NORESERVE;
		(void)ioctl(Ofile, VX_SETEXT, &extbuf);
		extbuf.a_flags &= ~VX_NORESERVE;
	}
	while (filesz > 0) {
		cnt = (int)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		FILL(cnt);
		if (proc_mode != P_SKIP && exterr == 0) {
			if (Hdr_type == CRC)
				cksumval += cksum(CRC, cnt);
			if (swapfile)
				swap(Buffr.b_out_p, cnt);
			errno = 0;
			rv = write(Ofile, Buffr.b_out_p, cnt);
			if (rv < cnt) {
				if (rv < 0)
	 				msg(ERRN, badwriteid, badwrite, nam_p);
				else
					msg(EXTN, badwriteid, badwrite, nam_p);
				proc_mode = P_SKIP;
				rstfiles(U_KEEP);
				(void)close(Ofile);
			}
		}
		Buffr.b_out_p += cnt;
		Buffr.b_cnt -= (long)cnt;
		filesz -= (long)cnt;
	} /* filesz */
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		FILL(pad);
		Buffr.b_out_p += pad;
		Buffr.b_cnt -= pad;
	}
	/* 
	 * Do delayed NOEXTEND in cases where isize > reservation.
	 */
	if (proc_mode != P_SKIP && noextend && !exterr) {
		extbuf.a_flags |= VX_NOEXTEND;
		if (ioctl(Ofile, VX_SETEXT, &extbuf)) {
			exterr++;
			if (extent_op == OCe_WARN) {
				msg(WARN, ":712", "%s: ioctl() %s failed: %s", nam_p, "setext", strerror(errno));
			} else {
				msg(ERR, ":712", "%s: ioctl() %s failed: %s", nam_p, "setext", strerror(errno));
			}
		}
	}
	if (proc_mode != P_SKIP) {
		if (Hdr_type == CRC && G_p->g_cksum != cksumval) {
			msg(ERR, ":100", "\"%s\" - checksum error", nam_p);
			rstfiles(U_KEEP);
		} else if (exterr) {
			rstfiles(U_KEEP);
		} else
			rstfiles(U_OVER);
		(void)close(Ofile);
	} 
	else {
		/* For symbolic link, still need to call rstfiles()
	 	* to restore some attributes. 
	 	*/
		if ((G_p->g_mode & Ftype) == S_IFLNK && !(Args & OCt))
			rstfiles(U_OVER);
	}

	VERBOSE((proc_mode != P_SKIP && (Vflag|vflag)), nam_p);
	Finished = 1;
}

/*
 * Procedure:     data_out
 *
 * Restrictions:
 *                readlink(2):	none
 *                read(2):	none
 */

/*
 * data_out:  open the file to be archived, compute the checksum
 * of it's data if the CRC header was specified and write the header.
 * read(2) each block of data and bwrite() it to the archive.  For TARTYP (TAR
 * and USTAR) archives, pad the data with NULLs to the next 512 byte boundary.
 */

static
void
data_out()
{
	register char *nam_p;
	register int cnt, rv, pad;
	register long filesz;
	char *oldBufp;
	struct	statvfs statvfsbuf;
	struct	vx_ext extbuf;

	nam_p = G_p->g_nam_p;
	if (Aspec) {
		write_hdr();
		rstfiles(U_KEEP);
		VERBOSE((Vflag|vflag), nam_p);
		return;
	}
	if ((G_p->g_mode & Ftype) == S_IFLNK && (Hdr_type != USTAR && Hdr_type != TAR)) { /* symbolic link */
		write_hdr();
		FLUSH(G_p->g_filesz);
		errno = 0;
		if (readlink(nam_p, Buffr.b_in_p, G_p->g_filesz) < 0) {
			msg(ERRN, badreadsymid, badreadsym, nam_p);
			return;
		}
		Buffr.b_in_p += G_p->g_filesz;
		Buffr.b_cnt += G_p->g_filesz;
		pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
		if (pad != 0) {
			FLUSH(pad);
			(void)memcpy(Buffr.b_in_p, Empty, pad);
			Buffr.b_in_p += pad;
			Buffr.b_cnt += pad;
		}
		VERBOSE((Vflag|vflag), nam_p);
		return;
	} else if ((G_p->g_mode & Ftype) == S_IFLNK && (Hdr_type == USTAR || Hdr_type == TAR)) {
		if (readlink(nam_p, T_lname, G_p->g_filesz) < 0) {
			msg(ERRN, badreadsymid, badreadsym, nam_p);
			return;
		}
		G_p->g_filesz = 0L;
		write_hdr();
		VERBOSE((Vflag|vflag), nam_p);
		return;
	}

	if (Hdr_type == CRC && (G_p->g_cksum = cksum(CRC, 0)) < 0) {
		msg(POST, ":102", "\"%s\" skipped", nam_p);
		(void)close(Ifile);
		return;
	}

	/* Copy extent information into header */
	memset(&vx_extcpio.extinfo, '\0', sizeof (struct vx_ext));
	if (extent_op != OCe_IGNORE && (G_p->g_mode & Ftype) == S_IFREG) {
		if (fstatvfs(Ifile, &statvfsbuf)) {
			msg(ERRN, ":1095", "fstatvfs() on %s failed", nam_p);
			(void)close(Ifile);
			return;
		}
		if (!strcmp(statvfsbuf.f_basetype, "vxfs")) {
			if (ioctl(Ifile, VX_GETEXT, &extbuf)) {
				msg(ERR, ":712", "%s: ioctl() %s failed: %s", nam_p, "getext", strerror(errno));
				(void)close(Ifile);
				return;
			} else {
				extbuf.reserve *= (daddr_t)statvfsbuf.f_frsize /
					(daddr_t)1024;
				extbuf.ext_size *= 
					(daddr_t)statvfsbuf.f_frsize / 
					(daddr_t)1024;
				vx_extcpio.extinfo = extbuf;
				if ((extbuf.reserve ||
				     extbuf.ext_size ||
				     extbuf.a_flags) &&
				    ((Hdr_type != CRC && Hdr_type != ASC) ||
				     G_p->g_namesz > (EXPNLEN - VX_CPIONEED))) {
					if (extent_op == OCe_FORCE) {
						msg(ERR, ":1096", "Cannot maintain attributes of %s", nam_p);
						(void)close(Ifile);
						return;
					} else {
						msg(WARN, ":1096", "Cannot maintain attributes of %s", nam_p);
					}
				}
			}
		}
	}

	/* This bflush is supposed to keep write_hdr from flushing.  If it 
	 * doesn't, the buffer is too small for the header type, and we 
	 * won't be able to recover if the file turns out to be unreadable. 
	 */

	bflush();	
	oldBufp=Buffr.b_in_p;
	noeraseflag=0;
	write_hdr();
	filesz = G_p->g_filesz;


	if (filesz > 0) {
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		if (Buffr.b_in_p+cnt>=Buffr.b_end_p) 
			cnt = (Buffr.b_end_p - Buffr.b_in_p);
		errno = 0;
		if ((rv = read(Ifile, Buffr.b_in_p, (unsigned)cnt)) < 0) {
			if (noeraseflag) { /* write_hdr flushed */
				msg(EXTN, badreadid, badread, nam_p);
				exit(32);
			} else { /* can recover */
				Buffr.b_in_p=oldBufp;
				msg(POST, ":102", "\"%s\" skipped", nam_p);
			}
		} else {
			Buffr.b_in_p += rv;
			Buffr.b_cnt += (long)rv;
			filesz -= (long)rv;
			FLUSH(0);
		}
	}
	while (filesz > 0) {
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		FLUSH(cnt);
		errno = 0;
		if ((rv = read(Ifile, Buffr.b_in_p, (unsigned)cnt)) < 0) {
			msg(EXTN, badreadid, badread, nam_p);
			break;
		}
		Buffr.b_in_p += rv;
		Buffr.b_cnt += (long)rv;
		filesz -= (long)rv;
	}
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		FLUSH(pad);
		(void)memcpy(Buffr.b_in_p, Empty, pad);
		Buffr.b_in_p += pad;
		Buffr.b_cnt += pad;
	}
	rstfiles(U_KEEP);
	(void)close(Ifile);
	VERBOSE((Vflag|vflag), nam_p);
}

/*
 * Procedure:     data_pass
 *
 * Restrictions:
 *                read(2):	none
 *                write(2):	none
 */

/*
 * data_pass:  If not a special file (Aspec), open(2) the file to be 
 * transferred, read(2) each block of data and write(2) it to the output file
 * Ofile, which was opened in file_pass().
 */

static
void
data_pass()
{
	register int cnt, done = 1;
	register long filesz, orig_filesz;
	struct statvfs istatvfs, ostatvfs;
	struct vx_ext extbuf;
	long extinbytes, resinbytes;
	int exterr = 0;
	int noextend, doext = 0;
	char *addr1, *addr2;

	if (Aspec) {
		rstfiles(U_KEEP);
		(void)close(Ofile);
		VERBOSE((Vflag|vflag), Nam_p);
		return;
	}
	

	/* check if need to preserve extent attributes */
	memset(&extbuf, '\0', sizeof (extbuf));
	noextend = 0;
	filesz = G_p->g_filesz;
	if (fstatvfs(Ifile, &istatvfs) < 0) {
		msg(ERRN, ":1095", "fstatvfs() on %s failed", Nam_p);
		exterr = 1;
	}
	if (exterr == 0 && fstatvfs(Ofile, &ostatvfs) < 0) {
		msg(ERRN, ":1095", "fstatvfs() on %s failed", Fullnam_p);
		exterr = 1;
	}
	if (extent_op != OCe_IGNORE && exterr == 0 &&
	    !strcmp(istatvfs.f_basetype, "vxfs") &&
	    ioctl(Ifile, VX_GETEXT, &extbuf) < 0) {
		msg(ERR, ":712", "%s: ioctl() %s failed: %s", Nam_p, "getext", strerror(errno));
		exterr = 1;
	}
	if (exterr == 0 && 
	    (extbuf.ext_size || extbuf.reserve || extbuf.a_flags)) {
		doext = 1;
		resinbytes = 
		     ((long)extbuf.reserve * (long)istatvfs.f_frsize);
		extinbytes = 
		     ((long)extbuf.ext_size * (long)istatvfs.f_frsize);
	}
	if (doext &&
	    (strcmp(ostatvfs.f_basetype, "vxfs") ||
	     resinbytes % (long)ostatvfs.f_frsize ||
	     extinbytes % (long)ostatvfs.f_frsize)) {
		doext = 0;
		if (extent_op == OCe_WARN) {
			msg(WARN, ":1096", "Cannot maintain attributes of %s", Nam_p);
		} else {
			exterr = 1;
			msg(ERR, ":1096", "Cannot maintain attributes of %s", Nam_p);
		}
	}
	if (doext) {
		extbuf.reserve =
		     (daddr_t)(resinbytes / (long)ostatvfs.f_frsize);
		extbuf.ext_size =
		     (off_t)(extinbytes / (long)ostatvfs.f_frsize);
	}
	if (extbuf.a_flags & VX_NOEXTEND && filesz > extbuf.reserve) {
		noextend++;
		extbuf.a_flags &= ~VX_NOEXTEND;
	}
	if (doext && ioctl(Ofile, VX_SETEXT, &extbuf)) {
		memset(&extbuf, '\0', sizeof (extbuf));
		noextend = 0;
		if (extent_op == OCe_WARN) {
			msg(WARN, ":1096", "Cannot maintain attributes of %s", Nam_p);
		} else {
			exterr = 1;
			msg(ERR, ":1096", "Cannot maintain attributes of %s", Nam_p);
		}
	}
	if (!doext) {
		memset(&extbuf, '\0', sizeof (extbuf));
	}
	if (exterr) {
		rstfiles(U_KEEP);
		(void)close(Ifile);
		(void)close(Ofile);
		return;
	}
	/*
	 * Try to map the input and write it all in one write.
	 * Iff the monolithic write only partially succeeds
	 * is there a problem; total failures fall through to
	 * the old fashioned way; sucesses clear "filesz" to skip 
	 * the old fashioned way.
	 */
	if ((filesz > 0) && ((addr1 = (char *)sbrk(0)) != (char *)-1)) {
		addr1 += Pagesize - 1;
		addr1 = (char *)((long)addr1 & (~(Pagesize - 1)));
		addr2 = mmap(addr1, (size_t)filesz, PROT_READ,
			     MAP_SHARED|MAP_FIXED, Ifile, 0);
		if (addr2 != (char *)-1) {
			/* Save original filesz for munmap() below. */
			orig_filesz = filesz;
			if ((cnt = write(Ofile, addr2, (size_t)filesz)) > 0) {
				if (cnt != filesz) {
					msg(ERRN, badwriteid, badwrite, Fullnam_p);
					done = 0;
				}
				/*
				 * success or unsalvagable failure.
				 */
				filesz = 0;
			}
			(void) munmap(addr2, (size_t)orig_filesz);
			Blocks += ((cnt + (BUFSZ - 1)) / BUFSZ);
		}
	};
	if (filesz > MAXBSIZE && !strcmp(ostatvfs.f_basetype, "vxfs")) {
		extbuf.reserve = (daddr_t)(filesz + ostatvfs.f_frsize - 1) /
			(long)ostatvfs.f_frsize;
		extbuf.a_flags |= VX_NORESERVE;
		(void)ioctl(Ofile, VX_SETEXT, &extbuf);
		extbuf.a_flags &= ~VX_NORESERVE;
	}
	while (filesz > 0) {
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		errno = 0;
		if (read(Ifile, Buf_p, (unsigned)cnt) < 0) {
			msg(ERRN, badreadid, badread, Nam_p);
			done = 0;
			break;
		}
		errno = 0;
		if (write(Ofile, Buf_p, (unsigned)cnt) < 0) {
			msg(ERRN, badwriteid, badwrite, Fullnam_p);
			done = 0;
			break;
		}
		Blocks += ((cnt + (BUFSZ - 1)) / BUFSZ);
		filesz -= (long)cnt;
	}
	if (done)
		rstfiles(U_OVER);
	else
		rstfiles(U_KEEP);
	(void)close(Ifile);
	(void)close(Ofile);

	/* -p option already checked before entry to data_pass() */
	/* transfer ACL of source file to destination file */
	transferdac(Nam_p, Fullnam_p, G_p->g_mode);

	VERBOSE((Vflag|vflag), Fullnam_p);
	Finished = 1;
}

/*
 * Procedure:     file_in
 *
 * Restrictions:  none
 */

/*
 * file_in:  Process an object from the archive.  If a TARTYP (TAR or USTAR)
 * archive and g_nlink == 1, link this file to the file name in t_linkname 
 * and return.  Handle linked files in one of two ways.  If Onecopy == 0, this
 * is an old style (binary or -c) archive, create and extract the data for the first 
 * link found, link all subsequent links to this file and skip their data.
 * If Oncecopy == 1, save links until all have been processed, and then 
 * process the links first to last checking their names against the patterns
 * and/or asking the user to rename them.  The first link that is accepted 
 * for xtraction is created and the data is read from the archive.
 * All subsequent links that are accepted are linked to this file.
 */

static
void
file_in()
{
	register struct Lnk *l_p, *tl_p;
	int lnkem = 0, cleanup = 0;
	int proc_file;
	struct Lnk *ttl_p;

	G_p = &Gen;
	if ((Hdr_type == USTAR || Hdr_type == TAR) && G_p->g_nlink == 1) { /* TAR and USTAR */
		(void)creat_lnk(Thdr_p->tbuf.t_linkname, G_p->g_nam_p);
		return;
	}
	if (Adir) {
		/*
		 * Initialize directory exists flag.  We don't
		 * know yet if the directory already exists.
		 */
		Exists_flag = 0;

		if (ckname() != F_SKIP && creat_spec() > 0)
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
		return;
	}
	if (G_p->g_nlink == 1 || (Hdr_type == TAR || Hdr_type == USTAR)) {
		if (Aspec) {
			if (ckname() != F_SKIP && creat_spec() > 0)
				VERBOSE((Vflag|vflag), G_p->g_nam_p);
		} else {
			if ((ckname() == F_SKIP) || (Ofile = openout()) < 0) {
				data_in(P_SKIP);
			}
			else {
				data_in(P_PROC);
			}
		}
		return;
	}
	tl_p = add_lnk(&ttl_p);
	l_p = ttl_p;
	if (l_p->L_cnt == l_p->L_gen.g_nlink)
		cleanup = 1;
	if (!Onecopy) {
		lnkem = (tl_p != l_p) ? 1 : 0;
		G_p = &tl_p->L_gen;
		if (ckname() == P_SKIP) {
			data_in(P_SKIP);
		}
		else {
			if (!lnkem) {
				if (Aspec) {
					if (creat_spec() > 0)
						VERBOSE((Vflag|vflag), G_p->g_nam_p);
				} else if ((Ofile = openout()) < 0) {
					data_in(P_SKIP);
					reclaim(l_p);
				} else {
					data_in(P_PROC);
				}
			} else {
				(void)strcpy(Lnkend_p, l_p->L_gen.g_nam_p);
				(void)strcpy(Full_p, tl_p->L_gen.g_nam_p);
				(void)creat_lnk(Lnkend_p, Full_p); 
				data_in(P_SKIP);
				l_p->L_lnk_p = (struct Lnk *)NULL;
				free(tl_p->L_gen.g_nam_p);
				free(tl_p);
			}
		}
	} else { /* Onecopy */
		if (tl_p->L_gen.g_filesz)
			cleanup = 1;
		if (!cleanup)
			return; /* don't do anything yet */
		tl_p = l_p;
		while (tl_p != (struct Lnk *)NULL) {
			G_p = &tl_p->L_gen;
			if ((proc_file = ckname()) != F_SKIP) {
				if (l_p->L_data) {
					(void)creat_lnk(l_p->L_gen.g_nam_p, G_p->g_nam_p);
				} else if (Aspec) {
					(void)creat_spec();
					l_p->L_data = 1;
					VERBOSE((Vflag|vflag), G_p->g_nam_p);
				} else if ((Ofile = openout()) < 0) {
					proc_file = F_SKIP;
				} else {
					data_in(P_PROC);
					l_p->L_data = 1;
				}
			} /* (proc_file = ckname()) != F_SKIP */
			tl_p = tl_p->L_lnk_p;
			if (proc_file == F_SKIP && !cleanup) {
				tl_p->L_nxt_p = l_p->L_nxt_p;
				tl_p->L_bck_p = l_p->L_bck_p;
				l_p->L_bck_p->L_nxt_p = tl_p;
				l_p->L_nxt_p->L_bck_p = tl_p;
				free(l_p->L_gen.g_nam_p);
				free(l_p);
			}
		} /* tl_p->L_lnk_p != (struct Lnk *)NULL */
		if (l_p->L_data == 0) {
			data_in(P_SKIP);
		}
	}
	if (cleanup)
		reclaim(l_p);
}

/*
 * Procedure:     file_out
 *
 * Restrictions:  none
 */

/*
 * file_out:  If the current file is not a special file (!Aspec) and it
 * is identical to the archive, skip it (do not archive the archive if it
 * is a regular file).  If creating a TARTYP (TAR or USTAR) archive, the first time
 * a link to a file is encountered, write the header and file out normally.
 * Subsequent links to this file put this file name in their t_linkname field.
 * Otherwise, links are handled in one of two ways, for the old headers
 * (i.e. binary and -c), linked files are written out as they are encountered.
 * For the new headers (ASC and CRC), links are saved up until all the links
 * to each file are found.  For a file with n links, write n - 1 headers with
 * g_filesz set to 0, write the final (nth) header with the correct g_filesz
 * value and write the data for the file to the archive.
 */

static
void
file_out()
{
	register struct Lnk *l_p, *tl_p;
	register int cleanup = 0;
	struct Lnk *ttl_p;

	G_p = &Gen;
	if (!Aspec && IDENT(SrcSt, ArchSt))
		return; /* do not archive the archive if it's a regular file */
	if (Hdr_type == USTAR || Hdr_type == TAR) { /* TAR and USTAR */
		if (Adir) {
			write_hdr();
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
			return;
		}
		if (G_p->g_nlink == 1) {
			data_out();
			return;
		}
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (tl_p == l_p) { /* first link to this file encountered */
			G_p->g_typeflag = REGTYPE;
			data_out();
			return;
		}
		if (Ifile > 0)
			close(Ifile);
		(void)strncpy(T_lname, l_p->L_gen.g_nam_p, l_p->L_gen.g_namesz);
		write_hdr();
		VERBOSE((Vflag|vflag), tl_p->L_gen.g_nam_p);
		free(tl_p->L_gen.g_nam_p);
		free(tl_p);
		return;
	}
	if (Adir) {
		write_hdr();
		VERBOSE((Vflag|vflag), G_p->g_nam_p);
		return;
	}
	if (G_p->g_nlink == 1) {
		data_out();
		return;
	} else {
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (l_p->L_cnt == l_p->L_gen.g_nlink)
			cleanup = 1;
		else if (Onecopy) {
			if (Ifile > 0)
				close(Ifile);
			return; /* don't process data yet */
		}
	}
	if (Onecopy) {
		tl_p = l_p;
		while (tl_p->L_lnk_p != (struct Lnk *)NULL) {
			G_p = &tl_p->L_gen;
			G_p->g_filesz = 0L;
			write_hdr();
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
			tl_p = tl_p->L_lnk_p;
		}
		G_p = &tl_p->L_gen;
	}
	data_out();
	if (cleanup)
		reclaim(l_p);
}

/*
 * Procedure:     file_pass
 *
 * Restrictions:  
 *		  readlink(2):	none
 *		  symlink(2):	none
 *		  lvlfile(2):	none
 */

/*
 * file_pass:  If the -l option is set (link files when possible), and the 
 * source and destination file systems are the same, link the source file 
 * (G_p->g_nam_p) to the destination file (Fullnam) and return.  If not a 
 * linked file, transfer the data.  Otherwise, the first link to a file 
 * encountered is transferred normally and subsequent links are linked to it.
 */

static
void
file_pass()
{
	register struct Lnk *l_p, *tl_p;
	struct Lnk *ttl_p;
	char *save_name;
	int lvl_change = 0; /* indicate whether level of process is changed */
	int result;

	G_p = &Gen;

	if (Adir && !(Args & OCd)) {
		msg(ERR, ":104", "Use -d option to copy \"%s\"", G_p->g_nam_p);
		return;
	}

	save_name = G_p->g_nam_p;
	while (*(G_p->g_nam_p) == '/')
		G_p->g_nam_p++;
	(void)strcpy(Full_p, G_p->g_nam_p);

	if ((Args & OCl) && !Adir && creat_lnk(save_name, Fullnam_p) == 0) {
		close(Ifile);
		return;
	}

	if ((G_p->g_mode & Ftype) == S_IFLNK && !(Args & OCL)) {
		errno = 0;
		if (readlink(save_name, Symlnk_p, G_p->g_filesz) < 0) {
			msg(ERRN, badreadsymid, badreadsym, save_name);
			return;
		}
		errno = 0;
		(void)missdir(Fullnam_p);
		*(Symlnk_p + G_p->g_filesz) = '\0';

		/* change level of process to that of the source file */
		/* -p option already checked before entry to file_pass() */
		if (macpkg && (Args  & OCm))
			lvl_change = chg_lvl_proc(save_name);

		result = symlink(Symlnk_p, Fullnam_p);

		/* restore level of process */
		if (lvl_change)
			restore_lvl_proc();

		if (result < 0) {
			if (errno == EEXIST) {
				if (openout() < 0) {
					return;
				}
			} else {
				msg(ERRN, badcreateid, badcreate, Fullnam_p);
				return;
			}
		}

		rstfiles(U_OVER);
		VERBOSE((Vflag|vflag), Fullnam_p);
		return;
	}


	if (!Adir && G_p->g_nlink > 1) {
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (tl_p == l_p) /* was not found */
			G_p = &tl_p->L_gen;
		else { /* found */
			(void)strcpy(Lnkend_p, l_p->L_gen.g_nam_p);
			(void)strcpy(Full_p, tl_p->L_gen.g_nam_p);
			(void)creat_lnk(Lnknam_p, Fullnam_p);
			l_p->L_lnk_p = (struct Lnk *)NULL;
			free(tl_p->L_gen.g_nam_p);
			free(tl_p);
			if (l_p->L_cnt == G_p->g_nlink) 
				reclaim(l_p);
			if (Ifile > 0)
				close(Ifile);
			return;
		}
	}
	if (Adir || Aspec) {
		/*
		 * Initialize directory exists flag.  We don't
		 * know yet if the directory already exists.
		 */
		Exists_flag = 0;

		if (creat_spec() > 0)
			VERBOSE((Vflag|vflag), Fullnam_p);
	} else if ((Ofile = openout()) > 0)
		data_pass();
}

/*
 * Procedure:     flush_lnks
 *
 * Restrictions:
 *                stat(2): none
 */

/*
 * flush_lnks: With new linked file handling, linked files are not archived
 * until all links have been collected.  When the end of the list of filenames
 * to archive has been reached, all files that did not encounter all their links
 * are written out with actual (encountered) link counts.  A file with n links 
 * (that are archived) will be represented by n headers (one for each link (the
 * first n - 1 have g_filesz set to 0)) followed by the data for the file.
 */

static
void
flush_lnks()
{
	register struct Lnk *l_p, *tl_p;
	long tfsize;

	l_p = Lnk_hd.L_nxt_p;
	while (l_p != &Lnk_hd) {
		(void)strcpy(Gen.g_nam_p, l_p->L_gen.g_nam_p);
		if (stat(Gen.g_nam_p, &SrcSt) == 0) { /* check if file exists */
			tl_p = l_p;
			(void)creat_hdr();
			Gen.g_nlink = l_p->L_cnt; /* "actual" link count */
			tfsize = Gen.g_filesz;
			Gen.g_filesz = 0L;
			G_p = &Gen;
			while (tl_p != (struct Lnk *)NULL) {
				Gen.g_nam_p = tl_p->L_gen.g_nam_p;
				Gen.g_namesz = tl_p->L_gen.g_namesz;
				if (tl_p->L_lnk_p == (struct Lnk *)NULL) {
					Gen.g_filesz = tfsize;
					data_out();
					break;
				}
				write_hdr(); /* archive header only */
				VERBOSE((Vflag|vflag), Gen.g_nam_p);
				tl_p = tl_p->L_lnk_p;
			}
			Gen.g_nam_p = Nam_p;
		} else /* stat(Gen.g_nam_p, &SrcSt) == 0 */
			msg(ERR, ":773", "\"%s\" has disappeared", Gen.g_nam_p);
		tl_p = l_p;
		l_p = l_p->L_nxt_p;
		reclaim(tl_p);
	} /* l_p != &Lnk_hd */
}

/*
 * Procedure:     gethdr
 *
 * Restrictions:  none
 */

/*
 * gethdr: Get a header from the archive, validate it and check for the trailer.
 * Any user specified Hdr_type is ignored (set to NONE in main).  Hdr_type is 
 * set appropriately after a valid header is found.  Unless the -k option is 
 * set a corrupted header causes an exit with an error.  I/O errors during 
 * examination of any part of the header cause gethdr to throw away any current
 * data and start over.  Other errors during examination of any part of the 
 * header cause gethdr to advance one byte and continue the examination.
 */

static
int
gethdr()
{
	register ushort ftype;
	register int hit = NONE, cnt = 0;
	int goodhdr, hsize, offset;
	char *preptr;
	int k = 0;
	int j;
	static int firstpass = 1;
	static int lasthdr = GOOD;

	Gen.g_nam_p = Nam_p;
	do { /* hit == NONE && (Args & OCk) && Buffr.b_cnt > 0 */
		FILL(Hdrsz);
		switch (Hdr_type) {
		case NONE:
		case BIN:
			Binmag.b_byte[0] = Buffr.b_out_p[0];
			Binmag.b_byte[1] = Buffr.b_out_p[1];
			if (Binmag.b_half == CMN_BIN) {
				hit = read_hdr(BIN);
				hsize = HDRSZ + Gen.g_namesz;
				break;
			}
			else if (Binmag.b_half == CMN_BBS) {
				if (Hdr_type == NONE)
					Args |= OCs;
				hit = read_hdr(BIN);
				hsize = HDRSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case CHR:
			if (!strncmp(Buffr.b_out_p, CMS_CHR, CMS_LEN)) {
				hit = read_hdr(CHR);
				hsize = CHRSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case ASC:
			if (!strncmp(Buffr.b_out_p, CMS_ASC, CMS_LEN)) {
				hit = read_hdr(ASC);
				hsize = ASCSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case CRC:
			if (!strncmp(Buffr.b_out_p, CMS_CRC, CMS_LEN)) {
				hit = read_hdr(CRC);
				hsize = ASCSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case USTAR:
			Hdrsz = TARSZ;
			FILL(Hdrsz);
			if ((hit = read_hdr(USTAR)) == NONE) {
				Hdrsz = ASCSZ;
				break;
			}
			hit = USTAR;
			hsize = TARSZ;
			break;
			/*FALLTHROUGH*/
		case TAR:
			Hdrsz = TARSZ;
			FILL(Hdrsz);
			if ((hit = read_hdr(TAR)) == NONE) {
				Hdrsz = ASCSZ;
				break;
			}
			hit = TAR;
			hsize = TARSZ;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		} /* Hdr_type */
		if (hit != NONE) {
			FILL(hsize);
			goodhdr = 1;
			if (Gen.g_filesz < 0L || Gen.g_namesz < 1)
				goodhdr = 0;
			if ((hit == USTAR) || (hit == TAR)) {
				Gen.g_nam_p = &nambuf[0];
				/*
				 * If we get a null and last header was good,
				 * then we're probably at tar trailer.
				 */
				if ((*Gen.g_nam_p == '\0') && (lasthdr == GOOD))
					goodhdr = 1;
				else {
					G_p = &Gen;
					if (G_p->g_cksum != cksum(TARTYP, 0))
						goodhdr = 0;
				}
			} else { /* binary, -c, ASC and CRC */
				if (Gen.g_nlink <= (ulong)0)
					goodhdr = 0;
				if (*(Buffr.b_out_p + hsize - 1) != '\0')
					goodhdr = 0;
			}
			if (!goodhdr) {
				hit = NONE;
				if (!(Args & OCk))
					break;
				/*
				 * If last hdr was bad, we already printed
				 * this msg, so don't print it again for each 
				 * bad byte. But if we've just read good stuff
				 * we want to know that we've hit bad again.
				 */
				if (lasthdr == GOOD) {
					msg(ERR, ":107", "Corrupt header, file(s) may be lost.");
					lasthdr = BAD;
				}
			} else {
				FILL(hsize);
			}
		} /* hit != NONE */
		if (hit == NONE) {
			Buffr.b_out_p++;
			Buffr.b_cnt--;
			if (!(Args & OCk))
				break;
			if (!cnt++)
				msg(ERR, ":108", "Searching for magic number/header.");
		}
	} while (hit == NONE);
	if (hit == NONE) {
		if (Hdr_type == NONE)
			msg(EXT, ":109", "Not a cpio file, bad header.");
		else
			msg(EXT, ":110", "Bad magic number/header.");
	} else if (cnt > 0) {
		msg(EPOST, ":111", "Re-synchronized on magic number/header.");
	}
	if (Hdr_type == NONE)
		Hdr_type = hit;

	if (firstpass) {
		firstpass = 0;
		switch (Hdr_type) {
		case BIN:
			Hdrsz = HDRSZ;
			Max_namesz = CPATH;
			Pad_val = HALFWD;
			Onecopy = 0;
			break;
		case CHR:
			Hdrsz = CHRSZ;
			Max_namesz = CPATH;
			Pad_val = 0;
			Onecopy = 0;
			break;
		case ASC:
		case CRC:
			Hdrsz = ASCSZ;
			Max_namesz = APATH;
			Pad_val = FULLWD;
			Onecopy = 1;
			break;
		case USTAR:
			Hdrsz = TARSZ;
			Max_namesz = HNAMLEN - 1;
			Pad_val = FULLBK;
			Onecopy = 0;
			break;
		case TAR:
			Hdrsz = TARSZ;
			Max_namesz = TNAMLEN - 1;
			Pad_val = FULLBK;
			Onecopy = 0;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		} /* Hdr_type */
	} /* Hdr_type == NONE */
	if ((Hdr_type == USTAR) || (Hdr_type == TAR)) { /* TAR and USTAR */
		Gen.g_namesz = 0;
		if (Gen.g_nam_p[0] == '\0')
			return(0);
		else {
			preptr = &prebuf[0];
			if (*preptr != (char) NULL) {
				k = strlen(&prebuf[0]);
				if (k < PRESIZ) {
					(void)strcpy(&fullnam[0], &prebuf[0]);
					j = 0;
					fullnam[k++] = '/';
					while ((j < NAMSIZ) && (&nambuf[j] != (char) NULL)) {
						fullnam[k] = nambuf[j];
						k++; j++;
					} 
					fullnam[k] = '\0';
				} else if (k >= PRESIZ) {
					k = 0;
					while ((k < PRESIZ) && (prebuf[k] != (char) NULL)) {
						fullnam[k] = prebuf[k];
						k++;
					}
					fullnam[k++] = '/';
					j = 0;
					while ((j < NAMSIZ) && (nambuf[j] != (char) NULL)) {
						fullnam[k] = nambuf[j];
						k++; j++;
					} 
					fullnam[k] = '\0';
				}
				Gen.g_nam_p = &fullnam[0];
			} else
				Gen.g_nam_p = &nambuf[0];
		}
	} else {
		(void)memcpy(Gen.g_nam_p, Buffr.b_out_p + Hdrsz, Gen.g_namesz);
		if (!strcmp(Gen.g_nam_p, "TRAILER!!!"))
			return(0);

		/* extract the extent information from the file name */
		memset(&vx_extcpio, '\0', sizeof(vx_extcpio));
		if (Hdr_type == ASC || Hdr_type == CRC) {
			k = strlen(Gen.g_nam_p) + 1;
			if (k == Gen.g_namesz - VX_CPIONEED) {
				Gen.g_namesz = k;
				sscanf(Gen.g_nam_p + k,
					"%8lx%8lx%8lx%8lx",
					&(vx_extcpio.magic),
					&(vx_extcpio.extinfo.ext_size),
					&(vx_extcpio.extinfo.reserve),
					&(vx_extcpio.extinfo.a_flags));
				if (vx_extcpio.magic != VX_CPIOMAGIC)
					memset(&vx_extcpio, '\0', 
						sizeof(vx_extcpio));
			}
		}
	}
	offset = ((hsize + Pad_val) & ~Pad_val);
	Buffr.b_out_p += offset;
	Buffr.b_cnt -= (long)offset;
	if (Hdr_type == USTAR) {
		if (Gen.g_typeflag == CHRTYPE ||
		    Gen.g_typeflag == BLKTYPE ||
		    Gen.g_typeflag == FIFOTYPE ||
		    Gen.g_typeflag == NAMTYPE) {
			Aspec = 1;
			Adir = 0;  /* might be set from previous file */
		} else if (Gen.g_typeflag == DIRTYPE) {
			Adir = 1;
			Aspec = 0;  /* might be set from previous file */
		} else {
			Aspec = 0;
			Adir = 0;
		}
	} else {
		ftype = Gen.g_mode & Ftype;
		Adir = (ftype == S_IFDIR);
		Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO || ftype == S_IFNAM);
	}
	lasthdr = GOOD;
	return(1);
}

/*
 * Procedure:     getname
 *
 * Restrictions:
 *                lstat(2):	none
 *                stat(2):	none
 */

/*
 * getname: Get file names for inclusion in the archive.  When end of file
 * on the input stream of file names is reached, flush the link buffer out.
 * For each filename, remove leading "./"s and multiple "/"s, and remove
 * any trailing newline "\n".  Finally, verify the existance of the file,
 * and call creat_hdr() to fill in the gen_hdr structure.
 */

static
int
getname()
{
	register int goodfile = 0, lastchar;
	extern int svr32lstat();
	extern int svr32stat();
	char *parent;  /* parent directory of the file */
	char *name;
	struct stat statbuf;

	Gen.g_nam_p = Nam_p;
	while (!goodfile) {
		if (fgets(Gen.g_nam_p, Max_namesz, stdin) == (char *)NULL) {
			if (Onecopy && !(Args &OCp))
				flush_lnks();
			return(0);
		}
		while (*Gen.g_nam_p == '.' && Gen.g_nam_p[1] == '/') {
			Gen.g_nam_p += 2;
			while (*Gen.g_nam_p == '/')
				Gen.g_nam_p++;
		}
		lastchar = strlen(Gen.g_nam_p) - 1;
		if (*(Gen.g_nam_p + lastchar) == '\n')
			*(Gen.g_nam_p + lastchar) = '\0';

		/* 
		 * Check if type "proc" file; if so, SILENTLY skip it.
		 * We've chosen to be silent, since one NEVER wants to
		 * backup proc files, and an ugly error message for each
		 * proc file is of no value to the user. So silently 
		 * skipping /proc files is a feature!
		 * NOTE: We're going to check for type proc on the parent
		 * directory, since conceivably a proc file could get in the
		 * namelist but then disappear and cause problems for stat().
		 * Checking the parent will allow us to easily weed out proc
		 * files, even if corresponding processes disappear on us.
		 */
		parent = dirname(strdup(Gen.g_nam_p));
		if (strcmp(parent, "/") == 0)
			/*
			 * If parent is "/", it could be /proc directory,
			 * so use the filename itself, and not the parent.
			 */
			name = Gen.g_nam_p;
		else
			name = parent;

		if (stat(name, &statbuf) == 0) {
	        	if (strcmp(statbuf.st_fstype, "proc") == 0) {
				/* Skip proc file; get next filename. */
				continue;
        		}
		}
		
		if (Oldstat) {
			if (svr32lstat(Gen.g_nam_p, &TmpSt) == 0) {
				goodfile = 1;
			} else {
				if (errno != EOVERFLOW)
					msg(ERRN, badaccessid, badaccess, Gen.g_nam_p);
				else if (!(Args & OCL) || lstat(Gen.g_nam_p, &SrcSt) < 0 || (SrcSt.st_mode & Ftype) != S_IFLNK)
					msg(ERRN, ":112", "Old format cannot support expanded types on %s", Gen.g_nam_p);
				else {
					goodfile = 1;
					TmpSt.st_mode = SrcSt.st_mode;
				}
			}
			if (goodfile && (TmpSt.st_mode & Ftype) == S_IFLNK && (Args & OCL)) {
				if (svr32stat(Gen.g_nam_p, &TmpSt) < 0) {
					msg(ERRN, badfollowid, badfollow, Gen.g_nam_p);
					goodfile = 0;
				}
			}

			/* svr32stat and svr32lstat will *not* fail with errno
			 * EOVERFLOW in the case that the st_dev field was too
			 * large, it will just set st_dev to NODEV.  But the
			 * only reason cpio saves the st_dev field is to
			 * determine multiple links, so if st_nlink is 1, or
			 * the file is a dir, it doesn't matter.  However, if
			 * st_nlink > 1 and the file is not a dir, then not
			 * knowing the dev number makes it impossible to
			 * determine what it's linked to.  In this case, we
			 * skip the file.
			 */
			if(goodfile && TmpSt.st_dev < 0 && TmpSt.st_nlink > 1 && (TmpSt.st_mode & Ftype) != S_IFDIR) {
				msg(ERR , ":112", "Old format cannot support expanded types on %s", Gen.g_nam_p);
				goodfile = 0;
			} else {
				SrcSt.st_dev = (dev_t)TmpSt.st_dev;
				SrcSt.st_uid = (uid_t)TmpSt.st_uid;
				SrcSt.st_gid = (gid_t)TmpSt.st_gid;
				SrcSt.st_ino = (ino_t)TmpSt.st_ino;
				SrcSt.st_mode = (mode_t)TmpSt.st_mode;
				SrcSt.st_mtime = (ulong)TmpSt.st_modtime;
				SrcSt.st_atime = (ulong)TmpSt.st_actime;
				SrcSt.st_nlink = (nlink_t)TmpSt.st_nlink;
				SrcSt.st_size = (off_t)TmpSt.st_size;
				SrcSt.st_rdev = (dev_t)TmpSt.st_rdev;
			}
		} else {
			if (!lstat(Gen.g_nam_p, &SrcSt)) {
				goodfile = 1;
				if ((SrcSt.st_mode & Ftype) == S_IFLNK && (Args & OCL)) {
					errno = 0;
					if (stat(Gen.g_nam_p, &SrcSt) < 0) {
						msg(ERRN, badfollowid, badfollow, Gen.g_nam_p);
						goodfile = 0;
					}
				}
			} else
				msg(ERRN, badaccessid, badaccess, Gen.g_nam_p);
		}
	}
	if (creat_hdr())
		return(1);
	else return(2);
}

/*
 * Procedure:     getpats
 *
 * Restrictions:
 *                fgets		none
 */

/*
 * getpats: Save any filenames/patterns specified as arguments.
 * Read additional filenames/patterns from the file specified by the
 * user.  The filenames/patterns must occur one per line.
 */

static
void
getpats(largc, largv)
int largc;
register char **largv;
{
	register char **t_pp;
	register int len;
	register unsigned numpat = largc, maxpat = largc + 2;
	
	if ((Pat_pp = (char **)malloc(maxpat * sizeof(char *))) == (char **)NULL)
		msg(EXT, nomemid, nomem);
	t_pp = Pat_pp;
	while (*largv) {
		if ((*t_pp = (char *)malloc((unsigned int)strlen(*largv) + 1)) == (char *)NULL)
			msg(EXT, nomemid, nomem);
		(void)strcpy(*t_pp, *largv);
		t_pp++;
		largv++;
	}
	while (fgets(Nam_p, Max_namesz, Ef_p) != (char *)NULL) {
		if (numpat == maxpat - 1) {
			maxpat += 10;
			if ((Pat_pp = (char **)realloc((char *)Pat_pp, maxpat * sizeof(char *))) == (char **)NULL)
				msg(EXT, nomemid, nomem);
			t_pp = Pat_pp + numpat;
		}
		len = strlen(Nam_p); /* includes the \n */
		*(Nam_p + len - 1) = '\0'; /* remove the \n */
		*t_pp = (char *)malloc((unsigned int)len);
		if(*t_pp == (char *) NULL)
			msg(EXT, nomemid, nomem);
		(void)strcpy(*t_pp, Nam_p);
		t_pp++;
		numpat++;
	}
	*t_pp = (char *)NULL;
}

/*
 * Procedure:     ioerror
 *
 * Restrictions:  none
 */

static
void
ioerror(dir)
register int dir;
{
	register int t_errno;
	register int archtype;	/* archive file type */

	t_errno = errno;
	errno = 0;
	if (fstat(Archive, &ArchSt) < 0)
		msg(EXTN, badaccarchid, badaccarch);
	errno = t_errno;

	archtype = (ArchSt.st_mode & Ftype);
	if ((archtype != S_IFCHR) && (archtype != S_IFBLK)) {
		if (dir) {  /* OUTPUT */
			if (errno == EFBIG)
				msg(EXT, ":113", "ulimit reached for output file.");
			else if (errno == ENOSPC)
				msg(EXT, ":114", "No space left for output file.");
			else
				msg(EXT, ":115", "I/O error - cannot continue");
		}
		else  /* INPUT */
			msg(EXT, ":116", "Unexpected end-of-file encountered.");
	}
	else if (dir)  /* OUTPUT */
		msg(EXT, ":907", "Cannot write to device");
	else  /* INPUT */
		msg(EXT, ":908", "Cannot read from device");
}

/*
 * Procedure:     matched
 *
 * Restrictions:  none
 */

/*
 * matched: Determine if a filename matches the specified pattern(s).  If the
 * pattern is matched (the first return), return 0 if -f was specified, else
 * return 1.  If the pattern is not matched (the second return), return 0 if
 * -f was not specified, else return 1.
 */

static
int
matched()
{
	register char *str_p = G_p->g_nam_p;
	register char **pat_pp = Pat_pp;

	while (*pat_pp) {
		if ((**pat_pp == '!' && !gmatch(str_p, *pat_pp + 1)) || gmatch(str_p, *pat_pp))
			return(!(Args & OCf)); /* matched */
		pat_pp++;
	}
	return(Args & OCf); /* not matched */
}

/*
 * Procedure:     missdir
 *
 * Restrictions:
 *                stat(2):	none
 *                mkdir(2):	none
 */

/*
 * missdir: Create missing directories for files.
 * (Possible future performance enhancement, if missdir is called, we know
 * that at least the very last directory of the path does not exist, therefore,
 * scan the path from the end 
 */

static
int
missdir(nam_p)
register char *nam_p;
{
	register char *c_p;
	register int cnt = 2;
	char str[APATH];
	register char *tmp_p;
	int i;
	int lvl_change = 0; /* indicate whether level of process is changed */

	if (*(c_p = nam_p) == '/') /* skip over 'root slash' */
		c_p++;
	for (; *c_p; ++c_p) {
		if (*c_p == '/') {
			*c_p = '\0';
			if (stat(nam_p, &DesSt) < 0) {
				if (Args & OCd) {

					/* determine source directory */
					if (Args & OCp) {
						tmp_p = Nam_p;
						i = 0;
						while ((*tmp_p) == '/') {
							str[i++] = '/';
							tmp_p++;
						}
						strcpy(str+i, Full_p);
					}
					
					if (macpkg && (Args & OCp)) {

						/* change level of process to
					   	   that of the source dir */
					    	if (Args & OCm)
							lvl_change = chg_lvl_proc(str);

						/* create an MLD if source dir is an MLD */
						cnt = creat_mld(str, nam_p, Def_mode);
					} else /* create a regular dir */
						cnt = 1;

					/* a regular dir is to be created */
					if (cnt == 1) {  
						cnt = mkdir(nam_p, Def_mode);
					}

					/* restore level of process */
					if (lvl_change)
						restore_lvl_proc();

					if (cnt != 0) {
						*c_p = '/';
						return(cnt);
					}
				} else {
					msg(ERR, ":119", "Missing -d option.");
					*c_p = '/';
					return(-1);
				}

			} /* end if (stat(nam_p, &DesSt) < 0) */
			*c_p = '/';
		} /* end if (*c_p == '/') */
	} /* end for */
	if (cnt == 2) /* the file already exists */
		cnt = 0;
	return(cnt);
}

/*
 * Procedure:     mklong
 *
 * Restrictions:  none
 */

/*
 * mklong: Convert two shorts into one long.  For VAX, Interdata ...
 */

static
long
mklong(v)
register short v[];
{
	
	register union swpbuf swp_b;

	swp_b.s_word = 1;
	if (swp_b.s_byte[0]) {
		swp_b.s_half[0] = v[1];
		swp_b.s_half[1] = v[0];
	} else {
		swp_b.s_half[0] = v[0];
		swp_b.s_half[1] = v[1];
	}
	return(swp_b.s_word);
}

/*
 * Procedure:     mkshort
 *
 * Restrictions:  none
 */

/*
 * mkshort: Convert a long into 2 shorts, for VAX, Interdata ...
 */

static
void
mkshort(sval, v)
register short sval[];
register long v;
{
	register union swpbuf *swp_p, swp_b;

	swp_p = (union swpbuf *)sval;
	swp_b.s_word = 1;
	if (swp_b.s_byte[0]) {
		swp_b.s_word = v;
		swp_p->s_half[0] = swp_b.s_half[1];
		swp_p->s_half[1] = swp_b.s_half[0];
	} else {
		swp_b.s_word = v;
		swp_p->s_half[0] = swp_b.s_half[0];
		swp_p->s_half[1] = swp_b.s_half[1];
	}
}

/*
 * Procedure:     msg
 *
 * Restrictions:
 *                fputc		none
 *                fflush	none
 *                pfmt		none
 *                vfprintf	none
 *                gettxt	none
 *                fprintf	none
 */

/*
 * msg: Print either a message (no error) (POST), an error message with or 
 * without the errno (ERRN or ERR), print an error message with or without
 * the errno and exit (EXTN or EXT), or print an error message with the
 * usage message and exit (USAGE).
 */

static
void
/*VARARGS*/
#ifdef __STDC__
msg(...)
#else
msg(va_alist)
va_dcl
#endif
{
	register char *fmt_p, *fmt_pid;
	register int severity;
	register FILE *file_p;
	va_list v_Args;
	int save_errno;

	save_errno = errno;
	if (Vflag && Verbcnt) { /* clear current line of dots */
		(void)fputc('\n', Out_p);
		Verbcnt = 0;
	}
#ifdef __STDC__
	va_start(v_Args,);
#else
	va_start(v_Args);
#endif
	severity = va_arg(v_Args, int);
	if (severity == POST)
		file_p = Out_p;
	else
		if (severity == EPOST || severity == WARN || severity == WARNN)
			file_p = Err_p;
		else {
			file_p = Err_p;
			Error_cnt++;
		}
	fmt_pid = va_arg(v_Args, char *);
	fmt_p = va_arg(v_Args, char *);
	(void)fflush(Out_p);
	(void)fflush(Err_p);

        switch (severity) {
        case EXT:
        case EXTN:
                (void)pfmt(file_p, MM_HALT, NULL);
                break;
        case ERR:
        case ERRN:
	case USAGE:
                (void)pfmt(file_p, MM_ERROR, NULL);
                break;
        case WARN:
        case WARNN:
                (void)pfmt(file_p, MM_WARNING, NULL);
                break;
        case POST:
        case EPOST:
        default:
                break;
        }

	(void)vfprintf(file_p, gettxt(fmt_pid, fmt_p), v_Args);

	if (severity == ERRN || severity == EXTN || severity == WARNN) {
		(void)pfmt(file_p, MM_NOSTD|MM_NOGET, ":  %s\n", strerror(save_errno));
	} else
		(void)fprintf(file_p, "\n");

	(void)fflush(file_p);
	va_end(v_Args);

	if (severity == EXT || severity == EXTN) {
		if (Error_cnt == 1)
			(void)pfmt(file_p, MM_NOSTD, ":121:1 error\n");
		else
			(void)pfmt(file_p, MM_NOSTD, ":122:%d errors\n", Error_cnt);
		exit(Error_cnt);
	}

	if (severity == USAGE)
		usage();
}

/*
 * Procedure:     openout
 *
 * Restrictions:
 *                lstat(2):	none
 *                symlink(2):	none
 *                unlink(2):	none
 *                creat(2):	none
 */

/*
 * openout: Open files for output and set all necessary information.
 * If the u option is set (unconditionally overwrite existing files),
 * and the current file exists, get a temporary file name from mktemp(3C),
 * link the temporary file to the existing file, and remove the existing file.
 * Finally either creat(2), mkdir(2) or mknod(2) as appropriate.
 * 
 */

static
int
openout()
{
	register char *nam_p;
	register int cnt, result;
	int lvl_change = 0; /* indicate whether level of process is changed */

	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		nam_p = G_p->g_nam_p;
	if (Max_filesz < (G_p->g_filesz >> 9)) { /* / 512 */
		msg(ERR, ":123", "Skipping \"%s\": exceeds ulimit by %d bytes",
			nam_p, G_p->g_filesz - (Max_filesz << 9)); /* * 512 */
		return(-1);
	}
	if (!lstat(nam_p, &DesSt) && creat_tmp(nam_p) < 0)
		return(-1);
	cnt = 0;

	do {
		errno = 0;
		if ((G_p->g_mode & Ftype) == S_IFLNK) {
			if ((!(Args & OCp)) && !(Hdr_type == USTAR)) {
			(void)strncpy(Symlnk_p, Buffr.b_out_p, G_p->g_filesz);
			*(Symlnk_p + G_p->g_filesz) = '\0';
			}
			else if ((!(Args & OCp)) && (Hdr_type == USTAR)) {
				(void)strcpy(Symlnk_p, &Thdr_p->tbuf.t_linkname[0]);
			}

			/* change level of process to that of the source file */
			if (macpkg && (Args & OCp) && (Args  & OCm))
				lvl_change = chg_lvl_proc(Nam_p);

			result = symlink(Symlnk_p, nam_p);

			/* restore level of process */
			if (lvl_change)
				restore_lvl_proc();

			if (result >= 0) {
				cnt = 0;
				if (*Over_p != '\0') {
					(void)unlink(Over_p);
					*Over_p = '\0';
				}
				break;
			}
		} else {
			/* change level of process to that of the source file */
			if (macpkg && (Args & OCp) && (Args  & OCm))
				lvl_change = chg_lvl_proc(Nam_p);

			result = creat(nam_p, (int)G_p->g_mode);

			/* restore level of process */
			if (lvl_change)
				restore_lvl_proc();

			if (result >= 0) {
				cnt = 0;
				break;
			}
		}
		cnt++;
	} while (cnt < 2 && !missdir(nam_p));

	switch (cnt) {
	case 0:
		break;
	case 1:
		msg(ERRN, badcreatdirid, badcreatdir, nam_p);
		break;
	case 2:
		msg(ERRN, badcreateid, badcreate, nam_p);
		break;
	default:
		msg(EXT, badcaseid, badcase);
	}
	Finished = 0;
	return(result);
}

/*
 * Procedure:     read_hdr
 *
 * Restrictions:
 *                sscanf	none
 *                makedev	none
 */

/*
 * read_hdr: Transfer headers from the selected format 
 * in the archive I/O buffer to the generic structure.
 */

static
int
read_hdr(hdr)
int hdr;
{
	register int rv = NONE;
	major_t maj, rmaj;
	minor_t min, rmin;
	char tmpnull;

	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz)) {
        	tmpnull = *(Buffr.b_out_p + Hdrsz);
        	*(Buffr.b_out_p + Hdrsz) = '\0';
	}

	switch (hdr) {
	case BIN:
		(void)memcpy(&Hdr, Buffr.b_out_p, HDRSZ);
		if (Hdr.h_magic == (short)CMN_BBS) {
                        swap((char *)&Hdr,HDRSZ);
		}
		Gen.g_magic = Hdr.h_magic;
		Gen.g_mode = Hdr.h_mode;
		Gen.g_uid = Hdr.h_uid;
		Gen.g_gid = Hdr.h_gid;
		Gen.g_nlink = Hdr.h_nlink;
		Gen.g_mtime = mklong(Hdr.h_mtime);
		Gen.g_ino = Hdr.h_ino;
		maj = cpioMAJOR(Hdr.h_dev);
		rmaj = cpioMAJOR(Hdr.h_rdev);
		min = cpioMINOR(Hdr.h_dev);
		rmin = cpioMINOR(Hdr.h_rdev);
		Gen.g_dev = makedev(maj, min);
		Gen.g_rdev = makedev(rmaj,rmin);
		Gen.g_cksum = 0L;
		Gen.g_filesz = mklong(Hdr.h_filesize);
		Gen.g_namesz = Hdr.h_namesize;
		rv = BIN;
		break;
	case CHR:
		if (sscanf(Buffr.b_out_p, "%6lo%6lo%6lo%6lo%6lo%6lo%6lo%6lo%11lo%6o%11lo",
		&Gen.g_magic, &Gen.g_dev, &Gen.g_ino, &Gen.g_mode, &Gen.g_uid, &Gen.g_gid,
		&Gen.g_nlink, &Gen.g_rdev, &Gen.g_mtime, &Gen.g_namesz, &Gen.g_filesz) == CHR_CNT)
			rv = CHR;
			maj = cpioMAJOR(Gen.g_dev);
			rmaj = cpioMAJOR(Gen.g_rdev);
			min = cpioMINOR(Gen.g_dev);
			rmin = cpioMINOR(Gen.g_rdev);
			Gen.g_dev = makedev(maj, min);
			Gen.g_rdev = makedev(rmaj,rmin);
		break;
	case ASC:
	case CRC:
		if (sscanf(Buffr.b_out_p, "%6lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8x%8x%8x%8x%8x%8lx",
		&Gen.g_magic, &Gen.g_ino, &Gen.g_mode, &Gen.g_uid, &Gen.g_gid, &Gen.g_nlink, &Gen.g_mtime,
		&Gen.g_filesz, &maj, &min, &rmaj, &rmin, &Gen.g_namesz, &Gen.g_cksum) == ASC_CNT) {
			Gen.g_dev = makedev(maj, min);
			Gen.g_rdev = makedev(rmaj, rmin);
			rv = hdr;
		}
		break;
	case USTAR: /* TAR and USTAR */
		if (*Buffr.b_out_p == '\0') {
			*Gen.g_nam_p = '\0';
			nambuf[0] = '\0';
			/* 
			 * If Hdr_type is NONE, we have not found
			 * a valid header yet, so return NONE.
			 * If we have found a previous tar header,
			 * then we're probably at the trailer.
			 */
			rv = Hdr_type;
		 } else {
			Thdr_p = (union tblock *)Buffr.b_out_p;
			Gen.g_nam_p[0] = '\0';
			(void)sscanf(Thdr_p->tbuf.t_name, "%100s", &nambuf);
			(void)sscanf(Thdr_p->tbuf.t_mode, "%8lo", &Gen.g_mode);
			(void)sscanf(Thdr_p->tbuf.t_uid, "%8lo", &Gen.g_uid);
			(void)sscanf(Thdr_p->tbuf.t_gid, "%8lo", &Gen.g_gid);
			(void)sscanf(Thdr_p->tbuf.t_size, "%12lo", &Gen.g_filesz);
			(void)sscanf(Thdr_p->tbuf.t_mtime, "%12lo", &Gen.g_mtime);
			(void)sscanf(Thdr_p->tbuf.t_cksum, "%8lo", &Gen.g_cksum);
			if (Thdr_p->tbuf.t_linkname[0] != (char)NULL)
				Gen.g_nlink = 1;
			else
				Gen.g_nlink = 0;
			if (Thdr_p->tbuf.t_typeflag == SYMTYPE)
				Gen.g_nlink = 2;
			Gen.g_typeflag = Thdr_p->tbuf.t_typeflag;
			(void)sscanf(Thdr_p->tbuf.t_magic, "%8lo", &Gen.g_tmagic);
			(void)sscanf(Thdr_p->tbuf.t_version, "%8lo", &Gen.g_version);
			(void)sscanf(Thdr_p->tbuf.t_uname, "%32s", &Gen.g_uname);
			(void)sscanf(Thdr_p->tbuf.t_gname, "%32s", &Gen.g_gname);
			(void)sscanf(Thdr_p->tbuf.t_devmajor, "%8lo", &rmaj);
			(void)sscanf(Thdr_p->tbuf.t_devminor, "%8lo", &rmin);
			(void)sscanf(Thdr_p->tbuf.t_prefix, "%155s", &prebuf);
			Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
			Gen.g_dev = 0;  /* not used for ustar header */
			Gen.g_rdev = makedev(rmaj,rmin);
			rv = USTAR;
		}
		break;
	case TAR:
		if (*Buffr.b_out_p == '\0') {
			*Gen.g_nam_p = '\0';
			/* 
			 * If Hdr_type is NONE, we have not found
			 * a valid header yet, so return NONE.
			 * If we have found a previous tar header,
			 * then we're probably at the trailer.
			 */
			rv = Hdr_type;
		}
		else {
			Thdr_p = (union tblock *)Buffr.b_out_p;
			Gen.g_nam_p[0] = '\0';
			(void)sscanf(Thdr_p->tbuf.t_mode, "%lo", &Gen.g_mode);
			(void)sscanf(Thdr_p->tbuf.t_uid, "%lo", &Gen.g_uid);
			(void)sscanf(Thdr_p->tbuf.t_gid, "%lo", &Gen.g_gid);
			(void)sscanf(Thdr_p->tbuf.t_size, "%lo", &Gen.g_filesz);
			(void)sscanf(Thdr_p->tbuf.t_mtime, "%lo", &Gen.g_mtime);
			(void)sscanf(Thdr_p->tbuf.t_cksum, "%lo", &Gen.g_cksum);
			if (Thdr_p->tbuf.t_typeflag == LNKTYPE)
				Gen.g_nlink = 1;
			else
				Gen.g_nlink = 0;
			(void)sscanf(Thdr_p->tbuf.t_name, "%s", Gen.g_nam_p);
			Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
			rv = TAR;
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz))
		*(Buffr.b_out_p + Hdrsz) = tmpnull;
	return(rv);
}

/*
 * Procedure:     reclaim
 *
 * Restrictions:  none
 */

/*
 * reclaim: Reclaim linked file structure storage.
 */

static
void
reclaim(l_p)
register struct Lnk *l_p;
{
	register struct Lnk *tl_p;
	
	l_p->L_bck_p->L_nxt_p = l_p->L_nxt_p;
	l_p->L_nxt_p->L_bck_p = l_p->L_bck_p;
	while (l_p != (struct Lnk *)NULL) {
		tl_p = l_p->L_lnk_p;
		free(l_p->L_gen.g_nam_p);
		free(l_p);
		l_p = tl_p;
	}
}

/*
 * Procedure:     rstbuf
 *
 * Restrictions:  none
 */

/*
 * rstbuf: Reset the I/O buffer, move incomplete potential headers to
 * the front of the buffer and force bread() to refill the buffer.  The
 * return value from bread() is returned (to identify I/O errors).  On the
 * 3B2, reads must begin on a word boundary, therefore, with the -i option,
 * any remaining bytes in the buffer must be moved to the base of the buffer
 * in such a way that the destination locations of subsequent reads are
 * word aligned.
 */

static
void
rstbuf()
{
 	register int pad;

	if ((Args & OCi) || Append) {
        	if (Buffr.b_out_p != Buffr.b_base_p) {
			pad = ((Buffr.b_cnt + FULLWD) & ~FULLWD);
			Buffr.b_in_p = Buffr.b_base_p + pad;
			pad -= Buffr.b_cnt;
			(void)memcpy(Buffr.b_base_p + pad, Buffr.b_out_p, (int)Buffr.b_cnt);
      			Buffr.b_out_p = Buffr.b_base_p + pad;
		}
		if (bfill() < 0)
			msg(EXT, ":124", "Unexpected end-of-archive encountered.");
	} else { /* OCo */
		(void)memcpy(Buffr.b_base_p, Buffr.b_out_p, (int)Buffr.b_cnt);
		Buffr.b_out_p = Buffr.b_base_p;
		Buffr.b_in_p = Buffr.b_base_p + Buffr.b_cnt;
	}
}

/*
 * Procedure:     setpasswd
 *
 * Restrictions:
 *                getpwnam:	none
 *                getgrnam:	P_MACREAD
 */

static
void
setpasswd(nam)
char *nam;
{
	if ((dpasswd = getpwnam(&Gen.g_uname[0])) == (struct passwd *)NULL) {
		msg(WARN, badpasswdid, badpasswd, &Gen.g_uname[0]);
		msg(WARN, ":125", "%s: owner not changed", nam);
		Gen.g_uid = Uid;  /* uid of invoker (got it in main) */
	} else
		Gen.g_uid = dpasswd->pw_uid;

	/*
	 * restrict the use of P_MACREAD privilege for opening 
	 * /etc/group in getgrnam()
	 */
	procprivl (CLRPRV, MACREAD_W, 0);
	if ((dgroup = getgrnam(&Gen.g_gname[0])) == (struct group *)NULL) {
		msg(WARN, badgroupid, badgroup, &Gen.g_gname[0]);
		msg(WARN, ":126", "%s: group not changed", nam);
		Gen.g_gid = getgid(); /* gid of invoker */
	} else
		Gen.g_gid = dgroup->gr_gid;
	G_p = &Gen;
	procprivl (SETPRV, MACREAD_W, 0);
}

/*
 * Procedure:     rstfiles
 *
 * Restrictions:
 *                unlink(2):	none
 *                link(2):	none
 *                chmod(2):	none
 *                chown(2):	none
 *		  lchown(2):	none
 */

/*
 * rstfiles:  Perform final changes to the file.  If the -u option is set,
 * and overwrite == U_OVER, remove the temporary file, else if overwrite
 * == U_KEEP, unlink the current file, and restore the existing version
 * of the file.  In addition, where appropriate, set the access or modification
 * times, change the owner and change the modes of the file.
 */

static
void
rstfiles(over)
register int over;
{
	register char *inam_p, *onam_p, *nam_p;

	/*
	 * If target MLD, we must change level of process to
	 * what it was when file was created, so that we'll
	 * deflect at the right effective directory while we
	 * restore attributes of the file.
	 */
	if (Target_mld) {
		if (lvlproc(MAC_SET, &File_lvl) == -1) {
			msg(WARNN, ":906", 
			    "Cannot restore attributes of \"%s\"", Fullnam_p);
			return;
		}
	}
			
	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		if (Gen.g_nlink > (ulong)0) 
			nam_p = G_p->g_nam_p;
		else
			nam_p = Gen.g_nam_p;

	if ((Args & OCi) && (privileged) && (Hdr_type == USTAR))
		setpasswd(nam_p);

	if (over == U_KEEP && *Over_p != '\0') {
		msg(POST, ":127", "Restoring existing \"%s\"", nam_p);
		(void)unlink(nam_p);
		if (link(Over_p, nam_p) == -1)
			msg(EXTN, badorigid, badorig, nam_p);
		if (remove(Over_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory. */
				msg(WARN, badremtmpdirid, badremtmpdir, Over_p);
			else
				msg(WARNN, badremtmpid, badremtmp, Over_p);
		}
		*Over_p = '\0';
		return;
	} else if (over == U_OVER && *Over_p != '\0') {
		if (remove(Over_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory. */
				msg(WARN, badremtmpdirid, badremtmpdir, Over_p);
			else
				msg(WARNN, badremtmpid, badremtmp, Over_p);
		}
		*Over_p = '\0';
	}

	if (Args & OCp) {
		inam_p = Nam_p;
		onam_p = Fullnam_p;
			
	} else /* OCi only uses onam_p, OCo only uses inam_p */
		inam_p = onam_p = G_p->g_nam_p;

	/* 
	 * For symbolic link, the only attributes to restore are
	 * owner and group.  (This used to be done in openout().)
	 */
	if (!(Args & OCo) && ((G_p->g_mode & Ftype) == S_IFLNK)) {
		if ((privileged) && (lchown(onam_p, G_p->g_uid, G_p->g_gid) < 0))
			msg(ERRN, badchownid, badchown, nam_p);
		return;
	}

	/*
	 * For the case when a directory already existed (either it
	 * it previously existed, or it was created earlier because of
	 * -depth), we need to transfer acl info (-p only) and change 
	 * the mode.
	 */
	if (!(Args & OCo) && Adir && Exists_flag) {
		if (Args & OCp)
			transferdac(inam_p, onam_p, G_p->g_mode);
		Dir_mode = G_p->g_mode & ~Orig_umask;
		chmod(onam_p, Dir_mode);
		Exists_flag = 0;
	}

	if ((Args & OCm) && !Adir)
		set_tym(onam_p, G_p->g_mtime, G_p->g_mtime);
	if (!(Args & OCo) && (privileged) && (chmod(onam_p, (int)G_p->g_mode) < 0))
		msg(ERRN, badchmodid, badchmod, onam_p);
	if (Args & OCa)
		set_tym(inam_p, SrcSt.st_atime, SrcSt.st_mtime);
	if ((Args & OCR) && (chown(onam_p, Rpw_p->pw_uid, Rpw_p->pw_gid) < 0))
		msg(ERRN, badchownid, badchown, onam_p);
	if ((Args & OCp) && !(Args & OCR) && (privileged)) {
		if ((chown(onam_p, G_p->g_uid, G_p->g_gid)) < 0)
			msg(ERRN, badchownid, badchown, onam_p);
	} else { /* OCi only uses onam_p, OCo only uses inam_p */
		if (!(Args & OCR)) {
			if ((Args & OCi) && (privileged) && (chown(inam_p, G_p->g_uid, G_p->g_gid) < 0))
				msg(ERRN, badchownid, badchown, onam_p);
		}
	}

	/* For target MLD case, set level of file to appropriate level. */
	if (Target_mld) {
		chg_lvl_file(inam_p, onam_p);
		restore_lvl_proc();
	}
}

/*
 * Procedure:     scan4trail
 *
 * Restrictions:
 *                g_read:	none
 */


/*
 * scan4trail: Scan the archive looking for the trailer.
 * When found, back the archive up over the trailer and overwrite 
 * the trailer with the files to be added to the archive.
 */

static
void
scan4trail()
{
	register int rv;
	register long off1, off2;

	Append = 1;
	Hdr_type = NONE;
	G_p = (struct gen_hdr *)NULL;
	while (gethdr()) {
		G_p = &Gen;
		data_in(P_SKIP);
	}
	off1 = Buffr.b_cnt;
	off2 = Bufsize - (Buffr.b_cnt % Bufsize);
	Buffr.b_out_p = Buffr.b_in_p = Buffr.b_base_p;
	Buffr.b_cnt = 0L;
	if (lseek(Archive, -(off1 + off2), SEEK_REL) < 0)
		msg(EXTN, badappendid, badappend);
	if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0)
		msg(EXTN, badappendid, badappend);
	if (lseek(Archive, -rv, SEEK_REL) < 0)
		msg(EXTN, badappendid, badappend);
	Buffr.b_cnt = off2;
	Buffr.b_in_p = Buffr.b_base_p + Buffr.b_cnt;
	Append = 0;
}

/*
 * Procedure:     setup
 *
 * Restrictions:
 *                access(2):	none
 *                g_init:	none
 *                stat(2):	none
 *                ulimit(2):	none
 *		  lvlfile(2):	none
 */

/*
 * setup:  Perform setup and initialization functions.  Parse the options
 * using getopt(3C), call ckopts to check the options and initialize various
 * structures and pointers.  Specifically, for the -i option, save any
 * patterns, for the -o option, check (via stat(2)) the archive, and for
 * the -p option, validate the destination directory.
 */

static
void
setup(largc, largv)
register int largc;
register char **largv;
{
	extern int optind;
	extern char *optarg;
	register char	*opts_p = "abcde:fiklmoprstuvABC:DE:G:H:I:K:LM:O:R:STV6",
			*dupl_p = "Only one occurrence of -%c allowed",
			*dupl_pid = ":128";
	register int option;
	int blk_cnt;
	int orig_mldmode = MLD_QUERY;  /* Original MLD mode of process */
	int result;

	Hdr_type = BIN;
	Efil_p = Hdr_p = Own_p = IOfil_p = NULL;
	memset(&vx_extcpio, '\0', sizeof (vx_extcpio));
	vx_extcpio.magic = VX_CPIOMAGIC;
	while ((option = getopt(largc, largv, opts_p)) != EOF) {
		switch (option) {
		case 'a':	/* reset access time */
			Args |= OCa;
			break;
		case 'b':	/* swap bytes and halfwords */
			Args |= OCb;
			break;
		case 'c':	/* select character header */
			Args |= OCc;
			Hdr_type = ASC;
			Onecopy = 1;
			break;
		case 'd':	/* create directories as needed */
			Args |= OCd;
			break;
		case 'e':	/* extent info op */
			/* 
			 * We would like to have a bit defined for the
			 * -e option and set that in Args here but, 
			 * with the addition of -K and -T for version 
			 * 4, we've run out of bits.  There is no check
			 * on option conflicts for -e so we simply let
			 * it slide.     Args |= OCe; 
			 */ 
			if (!strcmp(optarg, "warn"))
				extent_op = OCe_WARN;
			else if (!strcmp(optarg, "ignore"))
				extent_op = OCe_IGNORE;
			else if (!strcmp(optarg, "force"))
				extent_op = OCe_FORCE;
			else
				msg(USAGE, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
			break;
		case 'f':	/* select files not in patterns */
			Args |= OCf;
			break;
		case 'i':	/* "copy in" */
			Args |= OCi;
			Archive = 0;
			break;
		case 'k':	/* retry after I/O errors */
			Args |= OCk;
			break;
		case 'l':	/* link files when possible */
			Args |= OCl;
			break;
		case 'm':	/* retain modification time */
			Args |= OCm;
			break;
		case 'o':	/* "copy out" */
			Args |= OCo;
			Archive = 1;
			break;
		case 'p':	/* "pass" */
			Args |= OCp;
			break;
		case 'r':	/* rename files interactively */
			Args |= OCr;
			break;
		case 's':	/* swap bytes */
			Args |= OCs;
			break;
		case 't':	/* table of contents */
			Args |= OCt;
			break;
		case 'u':	/* copy unconditionally */
			Args |= OCu;
			break;
		case 'v':	/* verbose - print file names */
			if (Vflag)  /* -V option */
				msg(USAGE, mutexid, mutex, 'v', 'V');
			else
				vflag = 1;
			break;
		case 'A':	/* append to existing archive */
			Args |= OCA;
			break;
		case 'B':	/* set block size to 5120 bytes */
			Args |= OCB;
			Bufsize = 5120;
			break;
		case 'C':	/* set arbitrary block size */
			if (Args & OCC)
				msg(USAGE, dupl_pid, dupl_p, 'C');
			else {
				Args |= OCC;
				/*
				 * Check arg for nondigit characters
				 * before converting to integer.
				 */
				if (nondigit(optarg))
					msg(USAGE, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
				else
					Bufsize = atoi(optarg);
			}
			break;
		case 'D':
			Dflag = 1;
			break;
		case 'E':	/* alternate file for pattern input */
			if (Args & OCE)
				msg(USAGE, dupl_pid, dupl_p, 'E');
			else {
				Args |= OCE;
				Efil_p = optarg;
			}
			break;
		case 'G':	/* alternate interface (other than /dev/tty) */
			if (Args & OCG)
				msg(USAGE, dupl_pid, dupl_p, 'G');
			else {
				Args |= OCG;
				Ttyname = optarg;
				Tty_p = fopen(Ttyname, "r+");
				if (Tty_p != (FILE *)NULL) 
					setbuf(Tty_p, NULL);
				else {
					msg(ERR, ":65", "Cannot open \"%s\"", Ttyname);
					exit(Error_cnt);
				}
			}
			break;
		case 'H':	/* select header type */
			if (Args & OCH)
				msg(USAGE, dupl_pid, dupl_p, 'H');
			else {
				Args |= OCH;
				Hdr_p = optarg;
			}
			break;
		case 'I':	/* alternate file for archive input */
			if (Args & OCI)
				msg(USAGE, dupl_pid, dupl_p, 'I');
			else {
				Args |= OCI;
				IOfil_p = optarg;
			}
			break;

		/* Enhanced Application Compatibility Support */
		case 'K':	/* media size option */
			Args |= OCK;
			Mediasize = atoi(optarg);
			if (Mediasize == 0) {
				msg(ERR, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
				exit(Error_cnt);
			}
			break;
		/* End Enhanced Application Compatibility Support */

		case 'L':	/* follow symbolic links */
			Args |= OCL;
			break;
		case 'M':	/* specify new end-of-media message */
			if (Args & OCM)
				msg(USAGE, dupl_pid, dupl_p, 'M');
			else {
				Args |= OCM;
				Eom_p = optarg;
			}
			break;
		case 'O':	/* alternate file for archive output */
			if (Args & OCO)
				msg(USAGE, dupl_pid, dupl_p, 'O');
			else {
				Args |= OCO;
				IOfil_p = optarg;
			}
			break;
		case 'R':	/* change owner/group of files */
			if (Args & OCR)
				msg(USAGE, dupl_pid, dupl_p, 'R');
			else {
				Args |= OCR;
				Own_p = optarg;
			}
			break;
		case 'S':	/* swap halfwords */
			Args |= OCS;
			break;

		/* Enhanced Application Compatibility Support */
		case 'T':	/* truncate long file names to 14 chars */
			Args |= OCT;
			break;
		/* End Enhanced Application Compatibility Support */

		case 'V':	/* print a dot '.' for each file */
			if (vflag)  /* -v option */
				msg(USAGE, mutexid, mutex, 'v', 'V');
			else
				Vflag = 1;
			break;
		case '6':	/* for old, sixth-edition files */
			Args |= OC6;
			Ftype = SIXTH;
			break;
		default:
			Error_cnt++;
			usage();
		} /* End option */

	}  /* End (option = getopt(largc, largv, opts_p)) != EOF */

	largc -= optind;
	largv += optind;
	ckopts(Args);

	/* Get the page size, to use with memalign(). */
	Pagesize = sysconf(_SC_PAGESIZE);

	/* Check for memory problems. */
	if ((Buf_p = memalign(Pagesize, CPIOBSZ)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Empty = memalign(Pagesize, TARSZ)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Args & OCr) && (Renam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Symlnk_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Over_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Nam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Fullnam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Lnknam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);

	Gen.g_nam_p = Nam_p;

	if (Args & OCi) {
		if (largc > 0) /* save patterns for -i option, if any */
			Pat_pp = largv;
		if (Args & OCE)
			getpats(largc, largv);

	} else if (Args & OCo) {
		if (largc != 0) {  /* error if arguments left with -o */
			Error_cnt++;
			msg(USAGE, ":928", "Extra arguments at end");
		}
		else if (fstat(Archive, &ArchSt) < 0)
			msg(EXTN, badaccarchid, badaccarch);

		switch (Hdr_type) {
		case BIN:
			Hdrsz = HDRSZ;
			Pad_val = HALFWD;
			Oldstat = 1;
			break;
		case CHR:
			Hdrsz = CHRSZ;
			Pad_val = 0;
			break;
		case ASC:
		case CRC:
			Hdrsz = ASCSZ;
			Pad_val = FULLWD;
			break;
		case TAR:
		/* FALLTHROUGH */
		case USTAR: /* TAR and USTAR */
			Hdrsz = TARSZ;
			Pad_val = FULLBK;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		}

	} else { /* directory must be specified */
		if (largc != 1)
			msg(USAGE, ":1081", "Directory must be specified with -p");
		else if (access(*largv, 2) < 0)
			msg(EXTN, badaccessid, badaccess, *largv);
	}
	
	if (Args & (OCi | OCo)) {
		if (!Dflag) {
			if (Args & (OCB | OCC)) {
				if (g_init(&Device, &Archive) < 0)
					msg(EXTN, badinitid, badinit);
			} else {
				if ((Bufsize = g_init(&Device, &Archive)) < 0)
					msg(EXTN, badinitid, badinit);
			}
		}

		blk_cnt = _20K / Bufsize;
		blk_cnt = (blk_cnt >= MX_BUFS) ? blk_cnt : MX_BUFS;
		while (blk_cnt > 1) {
			if ((Buffr.b_base_p = memalign(Pagesize, (Bufsize * blk_cnt))) != (char *)NULL) {
				Buffr.b_out_p = Buffr.b_in_p = Buffr.b_base_p;
				Buffr.b_cnt = 0L;
				Buffr.b_size = (long)(Bufsize * blk_cnt);
				Buffr.b_end_p = Buffr.b_base_p + Buffr.b_size;
				break;
			}
			blk_cnt--;
		}

		if (blk_cnt < 2 || Buffr.b_size < (2 * CPIOBSZ))
			msg(EXT, nomemid, nomem);
	}

	if (Args & OCp) { /* get destination directory */
		(void)strcpy(Fullnam_p, *largv);

		/*
		 * If the mldmode of the process is virtual, change to
		 * real mode for stat() and see if target dir is an MLD.
		 * This only applies for -m, since without -m, 
		 * everything comes in at the same level and so will
		 * go in the same effective directory; no special work
		 * has to be done.
		 */
		if ((Args & OCm) && (macpkg) && ((orig_mldmode = mldmode(MLD_QUERY)) == MLD_VIRT)) {
			if (mldmode(MLD_REAL) < 0) {
                        	pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
                                	strerror(errno));
                        	exit(1);
                	}
		}
		else if (orig_mldmode < 0) {
                	pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
                	exit(1);
		}

		if (stat(Fullnam_p, &DesSt) < 0)
			msg(EXTN, badaccessid, badaccess, Fullnam_p);
		if ((DesSt.st_mode & Ftype) != S_IFDIR)
			msg(EXT, ":130", "\"%s\" is not a directory", Fullnam_p);

		/*
		 * If the original mldmode of the process was virtual, 
		 * determine if target dir is an MLD.  If so, set flag.
		 * But if we were originally in real mode, we don't 
		 * want to do anything here; no special treatment needed.  
		 */
		if (orig_mldmode == MLD_VIRT) {
			if ((DesSt.st_flags & S_ISMLD) == S_ISMLD)
				Target_mld = 1;
			/* Change back to virtual mode. */
			if (mldmode(MLD_VIRT) < 0) {
                      		pfmt(stderr, MM_ERROR|MM_NOGET, 
					"%s\n", strerror(errno));
                       		exit(1);
			}
        	}

		/*
		 * For target MLD case, check if user has P_SETFLEVEL 
		 * privilege.  If not, unset Target_mld flag.
		 * (This method for checking for P_SETFLEVEL is pretty
		 * kludgy, but we need to know this before we actually
		 * try to change the level of a file.)
	 	 */
		if (Target_mld) {
			/* Get level of target. */
			if (lvlfile(Fullnam_p, MAC_GET, &File_lvl) == 0) {
				/* Try to set level (using same level, so *
				 * that nothing actually changes).        */
				result = lvlfile(Fullnam_p, MAC_SET, &File_lvl);
				if ((result != 0) && (errno == EPERM))
					Target_mld = 0;
			} else
				Target_mld = 0;
		}

	}

	Full_p = Fullnam_p + strlen(Fullnam_p) - 1;
	if (*Full_p != '/') {
		Full_p++;
		*Full_p = '/';
	}
	Full_p++; 
	*Full_p = '\0';
	(void)strcpy(Lnknam_p, Fullnam_p);

	Lnkend_p = Lnknam_p + strlen(Lnknam_p);
	Max_filesz = ulimit(1, 0L);
	Lnk_hd.L_nxt_p = Lnk_hd.L_bck_p = &Lnk_hd;
	Lnk_hd.L_lnk_p = (struct Lnk *)NULL;
}

/*
 * Procedure:     set_tym
 *
 * Restrictions:
 *                utime(2):	none
 */

/*
 * set_tym: Set the access and modification times for a file.
 */

static
void
set_tym(nam_p, atime, mtime)
register char *nam_p;
register time_t atime, mtime;
{
	struct utimbuf timev;

	timev.actime = atime;
	timev.modtime = mtime;
	if (utime(nam_p, &timev) < 0) {
		if (Args & OCa)
			msg(ERRN, ":131", "Cannot reset access time for \"%s\"", nam_p);
		else
			msg(ERRN, ":132", "Cannot reset modification time for \"%s\"", nam_p);
	}
}

/*
 * Procedure:     sigint
 *
 * Restrictions:
 *                link(2):	none
 *                unlink(2):	none
 */

/*
 * sigint:  Catch interrupts.  If an interrupt occurs during the extraction
 * of a file from the archive with the -u option set, and the filename did
 * exist, remove the current file and restore the original file.  Then exit.
 */

static
void
sigint()
{
	register char *nam_p;

	(void)signal(SIGINT, SIG_IGN); /* block further signals */
	if (!Finished) {
		if (Args & OCi)
			nam_p = G_p->g_nam_p;
		else /* OCp */
			nam_p = Fullnam_p;
		if (*Over_p != '\0') { /* There is a temp file */
			if (remove(nam_p) == -1) {
				if (errno == EEXIST)
					/* If EEXIST, it's a non-empty directory. */
					msg(WARN, badremincdirid, badremincdir, nam_p);
				else
					msg(WARNN, badremincid, badreminc, nam_p);
			}
			if (link(Over_p, nam_p) == -1)
				msg(ERRN, badorigid, badorig, nam_p);
			if (remove(Over_p) == -1) {
				if (errno == EEXIST)
					/* If EEXIST, it's a non-empty directory. */
					msg(WARN, badremtmpdirid, badremtmpdir, Over_p);
				else
					msg(WARNN, badremtmpid, badremtmp, Over_p);
			}
		} else if (remove(nam_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory. */
				msg(WARN, badremincdirid, badremincdir, nam_p);
			else
				msg(WARNN, badremincid, badreminc, nam_p);
		}
	}
	exit(Error_cnt);
}

/*
 * Procedure:     swap
 *
 * Restrictions:  none
 */

/*
 * swap: Swap bytes (-s), halfwords (-S) or or both halfwords and bytes (-b).
 */

static
void
swap(buf_p, cnt)
register char *buf_p;
register int cnt;
{
	register unsigned char tbyte;
	register int tcnt;
	register int rcnt;

        rcnt = cnt % 4;
	cnt /= 4;
	if (Args & (OCb | OCs)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			tbyte = Swp_p->s_byte[0];
			Swp_p->s_byte[0] = Swp_p->s_byte[1];
			Swp_p->s_byte[1] = tbyte;
			tbyte = Swp_p->s_byte[2];
			Swp_p->s_byte[2] = Swp_p->s_byte[3];
			Swp_p->s_byte[3] = tbyte;
			Swp_p++;
		}
                if (rcnt >= 2) {
                        tbyte = Swp_p->s_byte[0];
                        Swp_p->s_byte[0] = Swp_p->s_byte[1];
                        Swp_p->s_byte[1] = tbyte;
                        tbyte = Swp_p->s_byte[2];
		}
	}
	if (Args & (OCb | OCS)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			tbyte = Swp_p->s_byte[0];
			Swp_p->s_byte[0] = Swp_p->s_byte[2];
			Swp_p->s_byte[2] = tbyte;
			tbyte = Swp_p->s_byte[1];
			Swp_p->s_byte[1] = Swp_p->s_byte[3];
			Swp_p->s_byte[3] = tbyte;
			Swp_p++;
		}
	}
}

/*
 * Procedure:     usage
 *
 * Restrictions:
 *                fflush	none
 *                pfmt		none
 */

/*
 * usage: Print the usage message on stderr and exit.
 */

static
void
usage()
{
	(void)fflush(stdout);
	(void)pfmt(stderr, MM_ACTION, ":133:Usage:\n");
	(void)pfmt(stderr, MM_NOSTD, ":1118:\tcpio -i[bcdfkmrstuvBSTV6] [-C size] ");
	(void)pfmt(stderr, MM_NOSTD, ":1098:[-E file] [-G file] [-H hdr] [[-I file] [-M msg]] ");
	(void)pfmt(stderr, MM_NOSTD, ":1088:[-R id] [-e ignore|warn|force] [patterns]\n");
	(void)pfmt(stderr, MM_NOSTD, ":1099:\tcpio -o[acvABLV] [-C size] [-G file]");
	(void)pfmt(stderr, MM_NOSTD, ":1089:[-H hdr] [-e ignore|warn|force] [-K mediasize] [[-M msg] [-O file]]\n");
	(void)pfmt(stderr, MM_NOSTD, ":1090:\tcpio -p[adlmuvLV] [-e ignore|warn|force] [-R id] directory\n");
	(void)fflush(stderr);
	exit(Error_cnt);
}

/*
 * Procedure:     verbose
 *
 * Restrictions:
 *                getpwuid:	none
 *                getgrgid:	P_MACREAD
 *                cftime:	P_MACREAD
 *                fputs:		none
 *                fputc:		none
 *                fflush:	none
 */

/*
 * verbose: For each file, print either the filename (-v) or a dot (-V).
 * If the -t option (table of contents) is set, print either the filename,
 * or if the -v option is also set, print an "ls -l"-like listing.
 * -v -> vflag=1
 * -V -> Vflag=1
 */

static
void
verbose(nam_p)
register char *nam_p;
{
	register int i, j, temp;
	mode_t mode;
	char modestr[11];

	for (i = 0; i < 10; i++)
		modestr[i] = '-';
	modestr[i] = '\0';

	if ((Args & OCt) && vflag) {
		mode = Gen.g_mode;
		for (i = 0; i < 3; i++) {
			temp = (mode >> (6 - (i * 3)));
			j = (i * 3) + 1;
			if (S_IROTH & temp)
				modestr[j] = 'r';
			if (S_IWOTH & temp)
				modestr[j + 1] = 'w';
			if (S_IXOTH & temp)
				modestr[j + 2] = 'x';
		}
		temp = Gen.g_mode & Ftype;
		switch (temp) {
		case (S_IFIFO):
			modestr[0] = 'p';
			break;
		case (S_IFCHR):
			modestr[0] = 'c';
			break;
		case (S_IFDIR):
			modestr[0] = 'd';
			break;
		case (S_IFBLK):
			modestr[0] = 'b';
			break;
		case (S_IFNAM):
                        if (Gen.g_rdev == S_INSEM)  /* Xenix semaphore */
                                modestr[0] = 's';
                        else if (Gen.g_rdev == S_INSHD)  /* Xenix shared data */
                                modestr[0] = 'm';
			else
				msg(ERR, ":140", "Impossible file type");
                        break;
		case (S_IFREG): /* was initialized to '-' */
			break;
		case (S_IFLNK):
			modestr[0] = 'l';
			break;
		default:
			msg(ERR, ":140", "Impossible file type");
		}
		if ((S_ISUID & Gen.g_mode) == S_ISUID)
			modestr[3] = 's';
		if ((S_ISVTX & Gen.g_mode) == S_ISVTX)
			modestr[9] = 't';
		if ((S_ISGID & G_p->g_mode) == S_ISGID && modestr[6] == 'x')
			modestr[6] = 's';
		else if ((S_ENFMT & Gen.g_mode) == S_ENFMT && modestr[6] != 'x')
			modestr[6] = 'l';
		if ((Hdr_type == USTAR || Hdr_type == TAR) && Gen.g_nlink == 0)
			(void)printf("%s%4d ", modestr, Gen.g_nlink+1);
		else
			(void)printf("%s%4d ", modestr, Gen.g_nlink);
		if (Lastuid == (int)Gen.g_uid)
			(void)printf("%-9s", Curpw_p->pw_name);
		else {
			if (Curpw_p = getpwuid((int)Gen.g_uid)) {
				(void)printf("%-9s", Curpw_p->pw_name);
				Lastuid = (int)Gen.g_uid;
			} else {
				(void)printf("%-9d", Gen.g_uid);
				Lastuid = -1;
			}
		}
		/*
		 * restrict the use of P_MACREAD privilege for opening 
		 * /etc/group in all the group related library routines.
		 */
		procprivl (CLRPRV, MACREAD_W, 0);
		if (Lastgid == (int)Gen.g_gid)
			(void)printf("%-9s", Curgr_p->gr_name);
		else {
			if (Curgr_p = getgrgid((int)Gen.g_gid)) {
				(void)printf("%-9s", Curgr_p->gr_name);
				Lastgid = (int)Gen.g_gid;
			} else {
				(void)printf("%-9d", Gen.g_gid);
				Lastgid = -1;
			}
		}
		if (!Aspec || (Gen.g_mode & Ftype) == S_IFIFO || (Gen.g_mode & Ftype) == S_IFNAM)
			(void)printf("%-7ld ", Gen.g_filesz);
		else
			(void)printf("%3d,%3d ", major(Gen.g_rdev), minor(Gen.g_rdev));
		(void)cftime(Time, gettxt(FORMATID, FORMAT), (time_t *)&Gen.g_mtime);
		procprivl (SETPRV, MACREAD_W, 0);
		(void)printf("%s, %s", Time, nam_p);
		if ((Gen.g_mode & Ftype) == S_IFLNK) {
			if (Hdr_type == USTAR)
				(void)strcpy(Symlnk_p, Thdr_p->tbuf.t_linkname);
			else {
				(void)strncpy(Symlnk_p, Buffr.b_out_p, Gen.g_filesz);
				*(Symlnk_p + Gen.g_filesz) = '\0';
			}
			(void)printf(" -> %s", Symlnk_p);
		}
		(void)printf("\n");
	} else if ((Args & OCt) || vflag) {
		(void)fputs(nam_p, Out_p);
		(void)fputc('\n', Out_p);
	} else { /* -V */
		(void)fputc('.', Out_p);
		if (Verbcnt++ >= 49) { /* start a new line of dots */
			Verbcnt = 0;
			(void)fputc('\n', Out_p);
		}
	}
	(void)fflush(Out_p);
}

/*
 * Procedure:     write_hdr
 *
 * Restrictions:
 *                sprintf	none
 */

/*
 * write_hdr: Transfer header information for the generic structure
 * into the format for the selected header and bwrite() the header.
 */

static
void
write_hdr()
{
	register int cnt, pad;

	switch (Hdr_type) {
	case BIN:
	case CHR:
	case ASC:
	case CRC:
		cnt = Hdrsz + G_p->g_namesz;
		break;
	case TAR:
	/*FALLTHROUGH*/
	case USTAR: /* TAR and USTAR */
		cnt = TARSZ;
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	FLUSH(cnt);
	switch (Hdr_type) {
	case BIN:
		Hdr.h_magic = (short)G_p->g_magic;
		Hdr.h_dev = (short)G_p->g_dev;
		Hdr.h_ino = (ushort)G_p->g_ino;
		Hdr.h_mode = G_p->g_mode;
		Hdr.h_uid = G_p->g_uid;
		Hdr.h_gid = G_p->g_gid;
		Hdr.h_nlink = G_p->g_nlink;
		Hdr.h_rdev = (short)G_p->g_rdev;
		mkshort(Hdr.h_mtime, (long)G_p->g_mtime);
		Hdr.h_namesize = (short)G_p->g_namesz;
		mkshort(Hdr.h_filesize, (long)G_p->g_filesz);
		(void)strcpy(Hdr.h_name, G_p->g_nam_p);
		(void)memcpy(Buffr.b_in_p, &Hdr, cnt);
		break;
	case CHR:
		(void)sprintf(Buffr.b_in_p, "%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.11lo%.6lo%.11lo%s",
			G_p->g_magic, G_p->g_dev, G_p->g_ino, G_p->g_mode, G_p->g_uid, G_p->g_gid, 
			G_p->g_nlink, G_p->g_rdev, G_p->g_mtime, G_p->g_namesz, G_p->g_filesz, G_p->g_nam_p);
		break;
	case ASC:
	case CRC:
		/* save extent info after name */
		if (extent_op != OCe_IGNORE && 
		    G_p->g_namesz <= (EXPNLEN - VX_CPIONEED) &&
		    (vx_extcpio.extinfo.ext_size ||
		     vx_extcpio.extinfo.reserve ||
		     vx_extcpio.extinfo.a_flags)) {
			(void)sprintf(Buffr.b_in_p + cnt,
				"%.8lx%.8lx%.8lx%.8lx",
				vx_extcpio.magic, vx_extcpio.extinfo.ext_size,
				vx_extcpio.extinfo.reserve,
				vx_extcpio.extinfo.a_flags);
			cnt += VX_CPIONEED;
			G_p->g_namesz += VX_CPIONEED;
			memset(&vx_extcpio.extinfo, '\0', 
				sizeof (struct vx_ext));
		}
		(void)sprintf(Buffr.b_in_p, "%.6lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%s",
			G_p->g_magic, G_p->g_ino, G_p->g_mode, G_p->g_uid, G_p->g_gid, G_p->g_nlink, G_p->g_mtime,
			G_p->g_filesz, major(G_p->g_dev), minor(G_p->g_dev), major(G_p->g_rdev), minor(G_p->g_rdev),
			G_p->g_namesz, G_p->g_cksum, G_p->g_nam_p);
		break;
	case USTAR: /* USTAR */
		Thdr_p = (union tblock *)Buffr.b_in_p;
		(void)memcpy(Thdr_p, Empty, TARSZ);
		(void)strncpy(Thdr_p->tbuf.t_name, G_p->g_tname, strlen(G_p->g_tname));
		(void)sprintf(Thdr_p->tbuf.t_mode, "%07o", G_p->g_mode);
		(void)sprintf(Thdr_p->tbuf.t_uid, "%07o", G_p->g_uid);
		(void)sprintf(Thdr_p->tbuf.t_gid, "%07o", G_p->g_gid);
		(void)sprintf(Thdr_p->tbuf.t_size, "%011lo", G_p->g_filesz);
		(void)sprintf(Thdr_p->tbuf.t_mtime, "%011lo", G_p->g_mtime);
		Thdr_p->tbuf.t_typeflag = G_p->g_typeflag;
		if (T_lname[0] != '\0')
			(void)strncpy(Thdr_p->tbuf.t_linkname, T_lname, strlen(T_lname));
		(void)sprintf(Thdr_p->tbuf.t_magic, "%s", TMAGIC);
		(void)sprintf(Thdr_p->tbuf.t_version, "%2s", TVERSION);
		(void)sprintf(Thdr_p->tbuf.t_uname, "%s",  G_p->g_uname);
		(void)sprintf(Thdr_p->tbuf.t_gname, "%s", G_p->g_gname);
		if (Aspec) {
			(void)sprintf(Thdr_p->tbuf.t_devmajor, "%07o", major(G_p->g_rdev));
			(void)sprintf(Thdr_p->tbuf.t_devminor, "%07o", minor(G_p->g_rdev));
		} else {
			/* devmajor and devminor will not be used if 
			 * not a special file, so set them to 0.
			 */
			(void)strcpy(Thdr_p->tbuf.t_devmajor, "00000000");
			(void)strcpy(Thdr_p->tbuf.t_devminor, "00000000");
		}
		(void)sprintf(Thdr_p->tbuf.t_prefix, "%s", Gen.g_prefix);
		(void)sprintf(Thdr_p->tbuf.t_cksum, "%07o", (int)cksum(TARTYP, 0));
		break;
	case TAR:
		Thdr_p = (union tblock *)Buffr.b_in_p;
		(void)memcpy(Thdr_p, Empty, TARSZ);
		(void)strncpy(Thdr_p->tbuf.t_name, G_p->g_nam_p, G_p->g_namesz);
		(void)sprintf(Thdr_p->tbuf.t_mode, "%07o ", G_p->g_mode);
		(void)sprintf(Thdr_p->tbuf.t_uid, "%07o ", G_p->g_uid);
		(void)sprintf(Thdr_p->tbuf.t_gid, "%07o ", G_p->g_gid);
		(void)sprintf(Thdr_p->tbuf.t_size, "%011o ", G_p->g_filesz);
		(void)sprintf(Thdr_p->tbuf.t_mtime, "%011o ", G_p->g_mtime);
		if (T_lname[0] != '\0')
			Thdr_p->tbuf.t_typeflag = LNKTYPE;
		else
			Thdr_p->tbuf.t_typeflag = AREGTYPE;
		(void)strncpy(Thdr_p->tbuf.t_linkname, T_lname, strlen(T_lname));
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	} /* Hdr_type */
	Buffr.b_in_p += cnt;
	Buffr.b_cnt += cnt;
	pad = ((cnt + Pad_val) & ~Pad_val) - cnt;
	if (pad != 0) {
		if (Buffr.b_in_p + pad >= Buffr.b_end_p)
			noeraseflag=1;
		FLUSH(pad);
		(void)memcpy(Buffr.b_in_p, Empty, pad);
		Buffr.b_in_p += pad;
		Buffr.b_cnt += pad;
	}
}

/*
 * Procedure:     write_trail
 *
 * Restrictions:  none
 */

/*
 * write_trail: Create the appropriate trailer for the selected header type
 * and bwrite the trailer.  Pad the buffer with nulls out to the next Bufsize
 * boundary, and force a write.  If the write completes, or if the trailer is
 * completely written (but not all of the padding nulls (as can happen on end
 * of medium)) return.  Otherwise, the trailer was not completely written out,
 * so re-pad the buffer with nulls and try again.
 */

static
void
write_trail()
{
	register int cnt, need;

	switch (Hdr_type) {
	case BIN:
	case CHR:
	case ASC:
	case CRC:
		Gen.g_mode = Gen.g_uid = Gen.g_gid = 0;
		Gen.g_nlink = 1;
		Gen.g_mtime = Gen.g_ino = Gen.g_dev = 0;
		Gen.g_rdev = Gen.g_cksum = Gen.g_filesz = 0;
		Gen.g_namesz = strlen("TRAILER!!!") + 1;
		(void)strcpy(Gen.g_nam_p, "TRAILER!!!");
		G_p = &Gen;
		write_hdr();
		break;
	case TAR:
	/*FALLTHROUGH*/
	case USTAR: /* TAR and USTAR */
		for (cnt = 0; cnt < 3; cnt++) {
			FLUSH(TARSZ);
			(void)memcpy(Buffr.b_in_p, Empty, TARSZ);
			Buffr.b_in_p += TARSZ;
			Buffr.b_cnt += TARSZ;
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	need = Bufsize - (Buffr.b_cnt % Bufsize);
	if(need == Bufsize)
		need = 0;
	
	while (Buffr.b_cnt > 0) {
		while (need > 0) {
			cnt = (need < TARSZ) ? need : TARSZ;
			need -= cnt;
			FLUSH(cnt);
			(void)memcpy(Buffr.b_in_p, Empty, cnt);
			Buffr.b_in_p += cnt;
			Buffr.b_cnt += cnt;
		}
		bflush();
	}
}

/*
 * Procedure:     chg_lvl_proc
 *
 * Restrictions:
 *                lvlfile(2):	none
 *                lvlproc(2):	none
 */

/*
 * Change the level of a process to that of the given file:
 *
 * 1. If the user does not have the P_SETPLEVEL privilege
 *    or user cannot obtain level of process,
 *    no level change is necessary.
 * 2  If the user has the P_SETPLEVEL privilege and the process level
 *    is successfully obtained,
 *    a. get level of the file
 *    b. get level of process if not obtained yet previously;
 *	 if the process level is not obtained successfully,
 *	 lvlpriv is set to 0, so that subsequent level change is
 *	 not attempted.
 *    c. change level of process to the file's level via lvlproc() only
 * 	 if the file's level is different from the level of the process
 *    d. If lvlproc() returns failure with errno EPERM, 
 *	 indicating that the invoking user does not have the P_SETPLEVEL
 *	 privilege to change process level, lvlpriv is set to 0,
 *	 so that subsequent level change is not attempted.
 *
 * Return value : 
 *	lvl_change - indicate whether level of process is changed :
 *			1 - changed
 *			0 - not changed
 *			
 */
static
int
chg_lvl_proc(namep)
char *namep;
{
	int result;
	int lvl_change = 0;
	char *p;  /* pointer to file name */


	/*
	 * User does not have P_SETPLEVEL privilege
  	 * or user cannot get level of process.  
	 */
	if (!lvlpriv) {
		/* If user can't change process level, unset
		 * Target_mld flag.  There's no need for
		 * special processing for Target_mld case if
		 * we can't change process level. */
		Target_mld = 0;
		return (lvl_change);
	}

	/*
	 * If target dir is MLD and process is in virtual mode,
	 * find out appropriate place (path component) at which
	 * to deflect in the MLD.  This component's level is the
	 * level to which we want to change the process before
	 * creating the file.  NOTE:  Don't check for virtual mode 
	 * here.  It's enough to check if Target_mld is set, since 
	 * it only gets set if we're in virtual mode (see setup()).
	 */
	if (Target_mld) {
		/* Get copy of file name; we don't want
		 * to alter original file name string. */
		if ((p = strdup(namep)) == NULL)
			msg(EXT, nomemid, nomem);
		namep = p;

		/* Skip over "/"s at beginning of string. */
		while (*p == '/')
			p++;

		/* Skip over "./"s and multiple "/"s at beginning of string. */
		while (*p == '.' && p[1] == '/') {
			p += 2;
			while (*p == '/')
				p++;
		}

		/*
		 * If no more slashes in p, then the appropriate
                 * effective directory of the target MLD is the
                 * level of the file itself.  Otherwise, it's the level
                 * of the first component of the pathname of the file.
		 */
		if ((strchr(p, '/')) != NULL) {
			/* There is at least another '/' in the path. */
			while (*p != '/') 
				p++;
			*p = '\0';
		}
	}

      	result = lvlfile(namep, MAC_GET, &File_lvl);

	/* unable to get level of file */
	if (result == -1) {
		msg(ERRN, ":774", "Cannot get level of \"%s\"", namep);
		Target_mld = 0;
		return (lvl_change);
	}
	
	/* level of process is not obtained yet */
	if (!Proc_lvl) {
		result = lvlproc(MAC_GET, &Proc_lvl);
		/* unable to get level of process */
		if (result == -1) {
			/* no need to do level change the next time and    *
			 * no need to do anything for target MLDs anymore. */
			lvlpriv = 0;  
			Target_mld = 0;
			msg(ERRN, ":775", "Cannot get level of process");
			return (lvl_change);
		}
	}

	if (Proc_lvl == File_lvl)
		return (lvl_change);

	result = lvlproc(MAC_SET, &File_lvl);

	/* process of level is changed successfully */
	if (!result)
		lvl_change = 1;
	/* user does not have P_SETPLEVEL priv */
	else if ((result == -1) && (errno == EPERM)) {
		lvlpriv = 0;     /* no need to do level change the next time */
		Target_mld = 0;  /* no need to do processing for target MLDs anymore */
 	}

	return (lvl_change);
}

/*
 * Procedure:     restore_lvl_proc
 *
 * Restrictions:  lvlproc(2):	none
 */

/* restore level of process to what is stored in Proc_lvl */
static
void
restore_lvl_proc()
{
	if (lvlproc(MAC_SET, &Proc_lvl) == -1)
		msg(WARNN, ":776", "Cannot restore level of process");
}

/*
 * Procedure:     chg_lvl_file
 *
 * Restrictions:  lvlfile(2):	none
 */

/* 
 * Change the level of a file.  This is for the case
 * where the target directory is an MLD, and we had to
 * create the file at a different level to get it in
 * the appropriate effective directory.
 * If the user doesn't have P_SETFLEVEL privilege, we 
 * don't even call this routine; the level of the file 
 * will be the level of the process.
 */
static
void
chg_lvl_file(src, target)
char *src, *target;
{
	lid_t srcfile_lvl;  /* level of source file */
	int result;

	/* Get level of source file. */
	result = lvlfile(src, MAC_GET, &srcfile_lvl);

	if (result == 0) {
		/* Level of source file obtained.  If level of source   
		 * file equals level of file from chg_lvl_proc(), level 
	 	 * of the new file is correct; don't have to change it. */
		if (srcfile_lvl != File_lvl)
			/* Set level of file to that of source file. */
			result = lvlfile(target, MAC_SET, &srcfile_lvl);
	}

	if (result == -1)
		msg(WARNN, ":905", "Cannot restore level of \"%s\"", target);
}

/*
 * Procedure:     creat_mld
 *
 * Restrictions:
 *                stat(2):	none
 *                mkmld(2):	none
 */

/*
 * Attempt to create the target directory as an MLD if the given source
 * directory is an MLD.
 *
 * 1. If the user does not have the P_MULTIDIR privilge, no MLD is created.
 *    A regular directory will be created on return.
 * 2. If the user has the P_MULTIDIR privilege:
 *    a. test whether the source directory is an MLD.
 *    b. If the source directory is not an MLD, 
 *	 a regular directory will be created on return.
 *    c. If the source directory is an MLD, an MLD is created via mkmld().  
 *	 - If mkmld() returns errno EPERM,
 *  	   indicating that the invoking user lacks the P_MULTIDIR priv,
 *         mldpriv is set to 0, so that subsequent creation of MLD is
 *	   not attempted.  A regular directory will be created on return.  
 *	 - If mkmld() returns error other than EPERM,
 *         -1 is returned.  No dircectory got created.
 *
 * return value:
 *	-1 : Failure in creating an MLD
 * 	 0 : Success in creating an MLD
 * 	 1 : A regular directory is to be created on return
 *
 */
static
int
creat_mld(src_dir, target_dir, mode)
char *src_dir;		/* source directory */
char *target_dir;	/* target directory to be created */
mode_t mode;
{
	int result;
	int ret = 1;
	int changed_mode = 0;	/* flag to tell if we changed MLD mode */
	int curr_mode;		/* holds return from MLD_QUERY */


	/* user does not have the P_MULTIDIR privilege to create an MLD */
	if (!mldpriv)
		/* a regular directory is to be created on return */
		return (ret);

	/* If process is not in real mode, change to real MLD mode. */
	if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
		if (mldmode(MLD_REAL) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				strerror(errno));
			exit(Error_cnt);
		}
		changed_mode = 1;
	} else if (curr_mode < 0) {
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
		exit(Error_cnt);
	}
	/* determine whether the source directory is an MLD */
	if ( stat(src_dir, &SrcSt) < 0) {
		msg(ERRN, badaccessid, badaccess, src_dir);
	}
	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(Error_cnt);
		}
	}

	if (SrcSt.st_flags & S_ISMLD == S_ISMLD) { /* an MLD */
		/* create the target directory as an MLD */
		result = mkmld(target_dir, mode);
		/* error in creating the MLD */
		if (result == -1)
		{
			/* user does not have the P_MULTIDIR priv */
			if (errno == EPERM)
			/* no need to create MLD the next time */
		   	/* a regular dir to be created on return */
				mldpriv = 0;
			/* fail to create the MLD other than ENOSYS */
			/* no directory is created */
			else if (errno != ENOSYS)
				ret = -1;
			/* ENOSYS:a regular dir to be created on return */
		}
		else  /* succeed in creating an MLD */
			ret = 0;
	} /* an MLD */

	return (ret);
}

/*
 * Procedure:     transferdac
 *
 * Restrictions:
 *                acl(2):	none
 *                chmod(2):	none
 */

/*
 * transferdac() will transfer the discretionary access control information
 * of the source to the target.
 *
 * If ACLs are not supported, it sets the target file's permission bits
 * via chmod() to the source file's permission bits.
 *
 * If ACLs are installed, it performs the following actions:
 *   1. It calls acl() with command ACL_GET to get the source file's ACL.
 *   2. If acl() with ACL_GET command returns successfully,
 *      it sets the target file's ACL via acl() with command ACL_SET.
 *      If acl() with command ACL_SET returns ENOSYS, indicating
 *      that the target file system does not support ACLs
 *	and additional entries are specified,
 *      it sets the target file's permission bits via chmod()
 *      to the source file's permission bits,
 *      with the middle 3 bits set to the
 *      the source file's ACL class entry permissions masked by
 *      the group entry permissions.
 *   3. If acl() with ACL_GET command returns error,
 *      it skips the transfer of ACL.
 *      However, if the error is ENOSPC, indicating that there is not
 *      enough space to hold the ACL,
 *      it doubly allocates more buffer space and calls acl() again.
 *      This is repeated until acl() returns succesfully or when
 *      there is not enough memory for more buffers.
 *      If acl() returns successfully, it proceeds as in (2).
 *      If there is not enough memory for more buffers,
 *      it skips the transfer of ACL.
 */

void
transferdac(source, target, modebits)
char *source;
char *target;
ulong modebits;
{
	struct acl aclbuf[NENTRIES]; /* initial number of entries allocated */
	struct acl *aclbufp = aclbuf;/* pointer to allocated buffer */
	int max_entries = NENTRIES;  /* max entries allowed for buffer */
	int nentries;		     /* actual number of entries */
	int aclerr = 0;
	struct acl *tmpbufp;
	int tmpentries;
	int gpbits, clbits = 0;	     /* ACL group entry and class entry */
	int gpdone, cldone = 0;	
	int i;
	char *parentp;	     	     /* pointer to parent directory path */
	char *targetcopy;	     /* copy of target pathname */


	if (aclpkg) { /* ACL security package is installed */

		/*
		 * Must first check if parent directory of target has
		 * default ACL entry.  If so, for the unprivileged user,
		 * we don't want to transfer the ACL from the source.  
		 * We want the default ACL to remain (from creat()).
		 */

		if (!privileged) {
			/*
			 * Get pathname of parent directory.  A copy
			 * of target is used because dirname() may
			 * alter its pathname argument, so the argument
			 * must be disposable.
			 */
			if ((targetcopy = strdup(target)) == NULL) 
				msg(EXT, nomemid, nomem);
			parentp = dirname(targetcopy);

			/* Get acl of the parent directory. */
			nentries = acl(parentp, ACL_GET, max_entries, aclbufp);
			if (nentries == -1) {
				msg(ERR, ":899", "Cannot get ACL for parent directory for \"%s\"", target);
				return;
			}
			
			/*
			 * Look through the ACL entries to see if there
			 * are any with default type (aclbufp->a_type). 
			 */
			for (i = 0; i < nentries; i++) {
				if ((aclbufp->a_type & ACL_DEFAULT) == ACL_DEFAULT)
					return;
				aclbufp++;
			}
		}

		/*
		 * If here, proceed with transfer of dac information 
		 * from source to target.  (Either privileged user or
		 * parent directory has no default ACL entry.)
		 */

		nentries = -1;
		while (nentries == -1) {
			nentries = acl(source, ACL_GET, max_entries, aclbufp);
			if ((nentries != -1) ||
			    ((nentries == -1) && (errno != ENOSPC)))
				break;
			tmpentries = max_entries * 2;
			if ((tmpbufp = 
(struct acl *)malloc(tmpentries * sizeof(struct acl))) == (struct acl *)NULL) {
				msg(ERR, ":777",
				"Not enough memory to get ACL for \"%s\"", source);
				break;
			}
			if (aclbufp != aclbuf)
				free(aclbufp);
			aclbufp = tmpbufp;
			max_entries = tmpentries;
		} /* end while */

		errno = 0;
		if (nentries != -1) {
			aclerr = acl(target, ACL_SET, nentries, aclbufp);
			/* If user is not privileged, clear umask bits. */
			if ((aclerr == 0) && (!privileged)) {
				modebits = modebits & ~Orig_umask;
				(void) chmod(target, modebits);
			}
		}
	} /* end "if (aclpkg)" */

	if (aclpkg && aclerr != 0 && errno == ENOSYS) {
		tmpbufp = aclbufp;
		/* search ACL for group and class entries */
		for (i = 0; i < nentries; i++, tmpbufp++) {
			if (tmpbufp->a_type == GROUP_OBJ) {
				gpbits = tmpbufp->a_perm & 07;
				gpdone = 1;
			}
			else if (tmpbufp->a_type == CLASS_OBJ) {
				clbits = tmpbufp->a_perm & 07;
				cldone = 1;
			}
			if (gpdone && cldone)
				break;
		} /* end for */	

		/* both ACL group and class entries are obtained */
		if (gpdone && cldone)
			modebits = (modebits & ~070) | ((gpbits & clbits) << 3);
		/* If not privileged, clear umask bits. */
		if (!privileged)
			modebits = modebits & ~Orig_umask;
		(void)chmod(target, modebits);
	} /* end if (aclerr != 0 && errno == ENOSYS) */	
}

/* Enhanced Application Compatibility Support 	*/
/* 	Implementation of [-T] option		*/
/* 	Truncate filenames greater than 14 characters */

static
void
truncate_sco(fname)
register char *fname;
{
	register char *cp;

	cp = fname + Gen.g_namesz;

	while (*cp != '/' && cp > fname)
		cp--;
	if (*cp == '/')
		cp++;
	if ((int)strlen(cp) <= MAX_NAMELEN)
		return;

	if (cp == fname)
		*(fname + MAX_NAMELEN) = '\0';
	else
		*(cp + MAX_NAMELEN) = '\0';
	return;
}
/* End Enhanced Application Compatibility Support */

/*
 * nondigit(arg):  Returns 1 if a character other than
 * a digit is found in the string <arg>; return 0 otherwise.
 * 
 */
static
int
nondigit(arg)
char *arg;
{
	char ch;
	char *ch_p;

	for (ch_p = arg; *ch_p; ch_p++) {
		ch = *ch_p;
		if (!isdigit(ch))
			return(1);
	}
	return(0);
}
