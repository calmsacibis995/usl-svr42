/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/config.c	1.18"
/*copyright     "%c%"*/

/* $XConsortium: config.c,v 1.6 91/07/25 12:15:45 keith Exp $ */

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
 * $NCDId: @(#)config.c,v 4.6 1991/07/09 14:08:09 lemke Exp $
 *
 */

#include	<stdio.h>
#include 	<os.h>
#include 	<string.h>
#include	"confmac.h"

void ErrorF();
void FatalError();
#ifndef DEFAULTMAXPOINT
#define DEFAULTMAXPOINT 640
#endif
#ifndef MAXCACHESIZE
#define MAXCACHESIZE  4000
#endif

#ifndef MAXPFCACHESIZE
#define MAXPFCACHESIZE 200
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif

#include 	<Xrenderer.h>

#include	"config.h"
#define KBYTES 1024

static unsigned long pfcachesize=0;
static unsigned long cachesize=0;
static unsigned long mincachesize=0;
static int logcachestats=0;
static int renderer_count=0;	
static FontRendererPtr lastRenderer= NULL;
static FontRendererPtr renderer=NULL;
static RendererConfigPtr rendercptr=NULL;
static char rendereralloc=FALSE;
typedef struct _config_options {
    char       *parm_name;
    char       *(*set_func) ();
}           ConfigOptionRec, *ConfigOptionPtr;

int XReadConfigFile();
char *GetConfigFontPath();
long GetPFCachesize();
extern char *cmdline_fontpath;
extern defaultFontPath;
static char *fontpath = NULL;
char *check_partial_paths();

static char *config_set_int(),
           *config_set_bool(),
           *config_set_fontpath(),
           *config_set_file(),
           *config_set_download(),
           *config_set_fonttype(),
           *config_set_derived(),
           *config_set_renderer();

/* these need to be in lower case and alphabetical order so a
 * binary search lookup can be used
 */
static ConfigOptionRec config_options[] = {
    {"alloc-units", config_set_int},
    {"cachesize", config_set_int},
    {"defaultpoint", config_set_int},
    {"derived-instance-pointsizes", config_set_derived},
    {"download-glyphs", config_set_download},
    {"download-height", config_set_int},
    {"download-maxchars", config_set_int},
    {"download-width", config_set_int},
    {"font-type", config_set_fonttype},
    {"fontpath", config_set_fontpath},
    {"free-renderer", config_set_bool},
    {"logcachestats", config_set_int},
    {"mincachesize", config_set_int},
    {"pfcachesize", config_set_int},
    {"preallocate-glyphs", config_set_int},
    {"preload-renderer", config_set_bool},
    {"prerender-glyphs", config_set_bool},
    {"sharedlib-filename", config_set_file},
    {"startrenderer", config_set_renderer},
    {"use-renderer", config_set_bool},
    {(char *) 0, 0},
};

char       *XConfigErrors[] = {
    "",
    "CONFIG: insufficient memory to load configuration file \"%s\"\n",
    "CONFIG: can't open configuration file \"%s\"\n",
    "CONFIG: error reading configuration file \"%s\"\n",
    "CONFIG: bad value \"%s\" for parameter \"%s\"\n",
    "CONFIG: unknown parameter \"%s\"\n",
    "CONFIG: missing '=' after parameter \"%s\"\n",
    "CONFIG: value out of range for parameter \"%s\"\n",
    "CONFIG: syntax error near parameter \"%s\"\n",
    "CONFIG: missing value for parameter \"%s\"\n",
    "CONFIG: extra value for parameter \"%s\"\n",
};


static char *
next_assign(c)
    char       *c;
{
    int         nesting = 0;

    while (*c != '\0') {
	if (*c == '(')
	    nesting++;
	else if (*c == ')')
	    nesting--;
	else if (*c == '=' && nesting == 0)
	    return c;
	c++;
    }
    return (char *) 0;
}

static void
strip_comments(data)
    char       *data;
{
    char       *c;

    c = data;
    while ((c = strchr(c, '#')) != NULL) {
	if (c == data || *(c - 1) != '\\') {
	    blank_comment(c);
	} else {
	    c++;
	}
    }
}

static      ConfigOptionPtr
match_param_name(name)
    char       *name;
{
    int         pos,
                rc,
                low,
                high;

    low = 0;
    high = sizeof(config_options) / sizeof(ConfigOptionRec) - 2;
    pos = high >> 1;

    while (low <= high) {
	rc = strcmp(name, config_options[pos].parm_name);
	if (rc == 0) {
	    return &config_options[pos];
	} else if (rc < 0) {
	    high = pos - 1;
	} else {
	    low = pos + 1;
	}
	pos = ((high + low) >> 1);
    }
    return 0;
}

static int
parse_config(data)
    char       *data;
{
    char       *c,
               *val,
               *next_eq,
               *consumed,
               *p;
    int        iret;
    char        param_name[64];
    Bool        equals_missing;
    ConfigOptionPtr param;

    c = data;
    skip_whitespace(c);

    while (*c != '\0') {
	equals_missing = FALSE;
        iret = 0;
	/* get parm name in lower case */
	p = c;
	while (isalnum(*c) || *c == '-') {
	    if (isupper(*c))
		*c = tolower(*c);
	    c++;
	}
	memcpy(param_name, p, min(sizeof(param_name), (int) (c - p)));
	param_name[(int) (c - p)] = '\0';

	/* check for junk */
	if (!isspace(*c) && *c != '=') {
	    ErrorF(XConfigErrors[CONFIG_ERR_SYNTAX], param_name);
	    /* eat garbage */
	    while (!isspace(*c) && *c != '=' && *c != '\0')
		c++;
	}
	skip_whitespace(c);
	if (*c != '=') {
	    ErrorF(XConfigErrors[CONFIG_ERR_NOEQUALS], param_name);
	    equals_missing = TRUE;
	} else {
	    c++;
	}

	skip_whitespace(c);

	/* find next assignment to guess where the value ends */
	if ((next_eq = next_assign(c)) != NULL) {
	    /* back up over whitespace */
	    for (val = next_eq - 1; val >= c &&
		    (isspace(*val) || *val == ',');
		    val--);

	    /* back over parm name */
	    for (; val >= c && (isalnum(*val) || *val == '-'); val--);

	    if (val <= c) {
		/* no value, ignore */
		ErrorF(XConfigErrors[CONFIG_ERR_NOVALUE], param_name);
		continue;
	    }
	    *val = '\0';
	} else if (*c == '\0') {
	    /* no value, ignore */
	    ErrorF(XConfigErrors[CONFIG_ERR_NOVALUE], param_name);
	    continue;
	}
	/* match parm name */
	if (equals_missing) {
	    equals_missing = FALSE;
	} else if ((param = match_param_name(param_name)) == NULL) {
	    rendercptr->renderPriv = TRUE;
	    ErrorF(XConfigErrors[CONFIG_ERR_UNKNOWN], param_name);
	} else {
            consumed = (param->set_func) (param, c);

	    skip_whitespace(consumed);
	    if (*consumed != '\0') {
		ErrorF(XConfigErrors[CONFIG_ERR_EXTRAVALUE],
		       param_name);
	    }
	}

	if (next_eq != NULL)
	    c = val + 1;
	else			/* last setting */
	    break;
    }
   if (lastRenderer) 
	FontFileRegisterRenderer(lastRenderer);
   return Success;
}

/*
 * handles anything that should be set once the file is parsed
 */


char *
GetConfigFontPath(defaultFontPath)
char *defaultFontPath;
{
if (fontpath != NULL) {
	defaultFontPath = fontpath;
#ifdef DEBUG
	fprintf(stderr,"Setting defaultfontpath to use fontpath from Xwinfont config file: %s\n",fontpath);
#endif
	return(defaultFontPath);
	}
return(NULL);
}

int
XReadConfigFile(filename)
    char       *filename;
{
    FILE       *fp;
    int         ret;
    int         len;
    char       *data;

    data = (char *) xalloc(CONFIG_MAX_FILESIZE);
    if (!data) {
	ErrorF(XConfigErrors[CONFIG_ERR_MEMORY], filename);
	return -1;
    }
    if ((fp = fopen(filename, "r")) == NULL) {
	ErrorF(XConfigErrors[CONFIG_ERR_OPEN], filename);
	return -1;
    }
    ret = fread(data, sizeof(char), CONFIG_MAX_FILESIZE, fp);
    if (ret <= 0) {
	ErrorF(XConfigErrors[CONFIG_ERR_READ], filename);
	return -1;
    }
    len = ftell(fp);
    len = min(len, CONFIG_MAX_FILESIZE);
    data[len] = '\0';		/* NULL terminate the data */

    (void) fclose(fp);

    init_configrec();
    strip_comments(data);
    ret = parse_config(data);

    free_configrec();
    xfree(data);

    return ret;
}

static char *
config_parse_bool (c, ret, pval)
    char	*c;
    int		*ret;
    Bool	*pval;
{
    char       *start,
                *c1,t;
    int         i,
                len;

    start = c;
    get_conf_val(c);
     t = *c;
    *c = '\0';
    len = c - start;
    for (i=0,c1= start; i < len; i++,c1++) {
	if (isupper(*c1))
		*c1 = tolower(*c1);
   	}
 
    c1 = start;
     if ((*c1 == 't') || (*c1 == 'y') || (*c1 == '1') || (!strncmp(start,"on",2)))  {
	*ret = 1;
	*c =t;
	return c;
	}
	
     if ((*c1 == 'f') || (*c1 == 'n') || (*c1 == '0') || (!strncmp(start,"of",2)))  {
	*ret = 0;
	*c =t;
	return c;
	}
    ErrorF(XConfigErrors[CONFIG_ERR_VALUE], start);
    *c = t;
    *ret = -1;
    return c;
}

static char *
config_parse_int(c, ret, pval)
    char       *c;
    int        *ret;
    int        *pval;
{
    char       *start,
                t;

    start = c;
    while (*c != '\0' && !isspace(*c) && *c != ',') {
	if (!isdigit(*c)) {	/* error */
	    get_conf_val(c);
	    t = *c;
	    *c = '\0';
	    ErrorF(XConfigErrors[CONFIG_ERR_VALUE], start);
	    *ret = -1;
	    *c = t;
	    return c;
	}
	c++;
    }
    t = *c;
    *c = '\0';
    *ret = 0;
    *pval = atoi(start);
    *c = t;
    return c;
}


/* config option sets */
/* these have to know how to do the real work and tweak the proper things */
static char *
config_set_int(parm, val,iret)
    ConfigOptionPtr parm;
    char       *val;
    int         *iret;
{
    int         ival,
                ret;

    val = config_parse_int(val, &ret, &ival);
    if ((ival < 0) || (ret == -1)) {
	*iret = -1;
	return val;
	}

*iret = 0;
    /* now do individual attribute checks */
    if (!strcmp(parm->parm_name, "alloc_units")) {
	rendercptr->alloc_units = ival;
    } else if (!strcmp(parm->parm_name, "mincachesize")) {
	if (ival <= MAXCACHESIZE) mincachesize = KBYTES *ival;
    } else if (!strcmp(parm->parm_name, "cachesize")) {
		if (ival <= MAXCACHESIZE) cachesize = KBYTES* ival;
    } else if (!strcmp(parm->parm_name, "pfcachesize")) {
		if (ival <= MAXPFCACHESIZE) pfcachesize = KBYTES* ival;
    } else if (!strcmp(parm->parm_name, "download-maxchars")) {
	rendercptr->dload_maxchars = ival;
    } else if (!strcmp(parm->parm_name, "download-height")) {
	rendercptr->dload_height = ival;
    } else if (!strcmp(parm->parm_name, "download-width")) {
	rendercptr->dload_width = ival;
    } else if (!strcmp(parm->parm_name, "logcachestats")) {
		logcachestats = ival;
    } else if (!strcmp(parm->parm_name, "preallocate-glyphs")) {
	rendercptr->preallocate_val = ival;
	if (ival < 100) rendercptr->prerender = 0;
    } else if (!strcmp(parm->parm_name, "defaultpoint")) {
	*iret=SetRendererDefaultPointSize(ival);
    }
    return val;
}

static char *
config_set_bool(parm, val,iret)
    ConfigOptionPtr parm;
    char       *val;
    int  	*iret;
{
    int
                ret;
    Bool        bval;

    val = config_parse_bool(val, &ret, &bval);
    if (ret == -1)
	return val;

    /* now do individual attribute checks */

    if (!strcmp(parm->parm_name, "free-renderer")) {
		rendercptr->close_when=ret;
    } else if (!strcmp(parm->parm_name, "preload-renderer")) {
		rendercptr->preload_renderer = ret;
    } else if (!strcmp(parm->parm_name, "prerender-glyphs")) {
		rendercptr->prerender=ret;
    } else if (!strcmp(parm->parm_name, "use-renderer")) {
    		if (ret == 0) rendercptr->donotuse=1;
    }

return val;
}

static char * 
config_set_file(parm,val)
    ConfigOptionPtr parm;
    char       *val;
{
    char       *start = val,
                t;
   int len;
   char * ptr, *dir;

   char path[256];

    if (val == NULL) return 0;
    get_conf_val(val);
    t = *val;
    *val = '\0';
    len = val - start + 1;
	/* get sharedlib filename */

    strcpy(path,"");
    if (*start != '/') {
	dir = (char *) get_xwinhome("lib");

	if (dir !=  (char *) 0) 	 {
		strcpy(path, dir);
    if (val == NULL) return 0;
		strcat(path, "/");
		}
	}
    strncat(path, start,len);
    len = strlen(path);
    ptr = (char *) xalloc(len+1);
        if (!ptr) return val;
    rendercptr->sharedlib_filename = (char *) ptr;
    strcpy(ptr,path);
    *val = t;
    return val;
}

static char *
config_set_fontpath(parm, val)
    ConfigOptionPtr parm;
    char       *val;
{
    char       *b;
	int len;
	int count;
	char *ptr;

        if (val == NULL) return 0;
	if (fontpath != NULL) xfree((char *) fontpath);	
		/* dump any previous one */
	b = fontpath = (char *) xalloc(strlen(val) + 1);
	if (!fontpath)
	    FatalError("Insufficent memory for font path\n");
	while (*val) {		/* remove all the gunk */
	    if (!isspace(*val)) {
		*b++ = *val;
	    }
	    val++;
	}
	*b = '\0';

fontpath = check_partial_paths(fontpath);

    return val;
}


char *
check_partial_paths(name)
    char *	name;
{
    register char *start, *end;
    char prefix[MAXPATHLEN];
    char fullname[MAXPATHLEN];
    char dirName[MAXPATHLEN];
    FontPathPtr path;
    int status;
    char *ptr;
    char *tmp;
    char *pprefix;
    int len ;
    memset(fullname,0,MAXPATHLEN-1);
    strcpy(fullname,"");
    strcpy(dirName,"");
    
    ptr= &fullname[0];
     tmp = (char *) get_xwinhome("");
     if (tmp == (char *) 0)
        	strcpy(prefix, "/");	/* not likely! */
            else
           	strcpy(prefix, tmp);
 
    end = name;
    for (;;) {
	start = end;
	while ((*start == ' ') || (*start == '\t') || (*start == ','))
	    start++;
	if (*start == '\0')
	    break;
	end = start;
	while ((*end != ' ') && (*end != '\t') && (*end != ',') &&
					(*end != '\0'))
	   end++;
        /*
         * allow for partial paths
         */
        if (*start != '/')
        {
  		strcpy(dirName,prefix);
        	strncat(dirName, start, end - start);
		len = strlen(dirName);
		strcat(dirName, ",");
		strcat(dirName,'\0');
	
        }
        else
        {
            memcpy(ptr, start, end - start);
	    ptr += end -start;
	    memcpy(ptr, ",", 1);
	    ptr++;
        }
	 if (dirName != "") {
	    len = strlen(dirName);
	    memcpy(ptr, dirName, len);
	    ptr += len;
	    strcpy(dirName, "");
            }	
    }

	if (fullname!= "") {
        if (fontpath != NULL) xfree((char *) fontpath); 
			/* dump any previous one */
              tmp=strrchr(fullname,(int) ',');
	      if (tmp != NULL) {
		  *tmp++ = '/';
		  *tmp++ = '\0';
         }
	
	len=strlen(fullname);
	strcat(fullname,'\0');
        fontpath = (char *) xalloc(len+1);
        if (!fontpath)
            FatalError("Insufficent memory for font path\n");

	

	strncpy(fontpath,fullname,len);
	strcat(fontpath,'\0');

	}
return fontpath;
}



static char *
config_set_download(parm, val)
    ConfigOptionPtr parm;
    char       *val;
{
    char       *start = val,
                t;

    if (val == NULL) return 0;
    skip_list_val(val);
    t = *val;
    *val = '\0';
    if (!strcmp(start,"fixed")) {
	rendercptr->download = 1;
    } else if (!strcmp(start, "all")) {
	rendercptr->download = 2;
    } else if (!strcmp(start, "none")) {
	rendercptr->download = 3;
    } else {
	rendercptr->download = 0;
    } 
    *val = t;
    return val;

}



int
SetRendererDefaultPointSize(val)
int val;
{
if (val > DEFAULTMAXPOINT) return (-1);
rendercptr->renderer_defaults.point = val;
return(1);
}

char *
config_set_derived(parm,val)
    ConfigOptionPtr parm;
    char *val;
{
    int ret;
    int ival;
    int size;
    int newsize;
    int *nlength;
    if (val == NULL) return 0;
    val = config_parse_int(val, &ret, &ival);
    if ((ival < 0) || (ret == -1)) {
	return val;
	}

size = rendercptr->numDerived; 
newsize = size + 1;
nlength = (int *)xrealloc(renderer->config.derived_instances, newsize * sizeof (int));
if (!nlength) return 0;
rendercptr->derived_instances= nlength;
rendercptr->derived_instances[size] =  ival;
get_conf_num(val);

rendercptr->numDerived = newsize;
if (!(iseol(*val))) config_set_derived(parm,val);
return (NULL);
}


static char *
config_set_renderer(parm, val)
    ConfigOptionPtr parm;
    char       *val;
{
    char       *start = val,
                t;
   static FontRendererPtr thisrenderer=NULL;
   int len;
   char *ptr;

	
    char suffix[20];
    if (val == NULL) return 0;
    skip_specialwhitespace(val);
    start= val;
    get_conf_val2(val);
    t = *val;
    *val = '\0';
    len = (val-start+1);
  
/* insert code here to determine if we are starting a new
renderer without copying the saved info to the renderer info.
*/
			
	strncpy(suffix,start,len);
	strcat(suffix,"");
	thisrenderer = (FontRendererPtr) AllocateRenderer(start, len, XSERVER_TYPE);
	if (!thisrenderer) return NULL; 
	free_configrec();	/* free any allocated record */
				/* this should free any junk that
				   came in before the first startrenderer */
        if ((lastRenderer) && (lastRenderer != thisrenderer) &&
		(lastRenderer->config.donotuse != 1)) {
			
		        FontFileRegisterRenderer(lastRenderer);
        }
	

      lastRenderer = thisrenderer;
      rendercptr = &thisrenderer->config;
      renderer = thisrenderer;
    *val = t;
    return val;
}

void 
GetCachesize(min, max, log)
long *min;
long *max;
int *log;
{
*min = mincachesize;
*max = cachesize;
*log = logcachestats;
}

long 
GetPFCachesize()
{
return pfcachesize;
}

static char *
config_set_fonttype(parm, val)
    ConfigOptionPtr parm;
    char       *val;
{
    char       *start = val,
                t;

    if (val == NULL) return 0;
    skip_list_val(val);
    t = *val;
    *val = '\0';
    if (!strcmp(val,"both")) {
	rendercptr->type = 1;
    } else if (!strcmp(val,"bitmap")) {
	rendercptr->type = 2;
	}
	else rendercptr->type = 0;
 }

init_configrec()
{
rendereralloc=TRUE;
rendercptr = (RendererConfigPtr) xalloc(sizeof(RendererConfigRec));
if (!rendercptr) FatalError("Insufficent memory for fontconfiguration rec \n");
memset(rendercptr, 0, sizeof(RendererConfigRec));
}

free_configrec()
{
if ((rendereralloc == TRUE) && (rendercptr != NULL)) xfree(rendercptr);
rendereralloc=FALSE;
}


