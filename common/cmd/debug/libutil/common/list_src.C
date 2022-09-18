#ident	"@(#)debugger:libutil/common/list_src.C	1.4"

#include "utility.h"
#include "SrcFile.h"
#include "LWP.h"
#include "Interface.h"
#include "Location.h"
#include "global.h"
#include <regexpr.h>

static int	fprintn(LWP *lwp, SrcFile *, long, long);
static SrcFile	*check_srcfile(LWP *, char *, long, long &);
static int	dore(LWP *, SrcFile *, long, const char *, int);

int 
list_src(LWP *lwp, int count, Location *l, const char *re, int mode)
{
	long		lcount = (count > 0) ? count : num_line;
	long		numlines = 0;
	SrcFile		*sf;
	char		*file;
	char		*func;
	char		*name;
	long		line;
	Frame		*f;

	// if no lwp specified in location, uses current
	if (l)
	{
		LWP	*l_lwp;
		if (!l->get_lwp(l_lwp))
		{
			return 0;
		}
		if (l_lwp)
			lwp = l_lwp;
	}
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	if (!lwp->state_check(E_DEAD))
	{
		return 0;
	}
	if (l)
	{
		f = lwp->curframe();

		if (l->get_file(lwp, f, file) == 0)
			return 0;
		if (!file && 
			(!lwp->state_check(E_RUNNING|E_DEAD)))
		{
			// list on running process allowed
			// only where fully qualified locaction
			// is given
			return 0;
		}
		switch(l->get_type())
		{
			case lk_none:
			case lk_addr:
			default:
				printe(ERR_bad_list_loc, E_ERROR);
				return 0;
			case lk_fcn:
				if ((l->get_func(lwp, f, func) == 0) || 
					(func == 0))
					return 0;
				if ((name = find_fcn(lwp, file,
					func, line )) == 0)
					return 0;
				break;
			case lk_stmt:
				if (!file)
				{
					if (!current_loc(lwp, f, name, 
						func, line))
						return 0;
				}
				else 
					name = file;
				if (l->get_line(lwp, f, 
					(unsigned long &)line) == 0)
					return 0;
		}
		if ((sf = check_srcfile(lwp, name, line, numlines)) == 0)
			return 0;
		lwp->set_current_stmt(name, line);
	}
	else
	{
		if (!lwp->state_check(E_RUNNING))
		{
			return 0;
		}
		if ((sf = check_srcfile(lwp, lwp->curr_src(), 
			lwp->current_line(), numlines)) == 0)
			return 0;
		if (re)
		{
			if (!dore(lwp, sf, numlines, re, mode))
				return 0;
			if (count <= 0)
			{
				long currline = lwp->current_line();
				printm(MSG_line_src, currline,
					sf->line((int)currline));
				return 1;
			}
		}
	}
	return fprintn(lwp, sf, lcount, numlines );
}

static SrcFile *
check_srcfile(LWP *lwp, char *fname, long line, long &num)
{
	SrcFile	*sf;

	if (fname == 0 || *fname == 0)
	{
		printe(ERR_no_cur_src, E_ERROR);
		return 0;
	}
	if ((sf = find_srcfile( lwp, fname )) == 0) 
	{
		printe(ERR_no_source_info, E_ERROR, fname);
		return 0;
	}
	if ((num = sf->num_lines()) == 0)
	{
		printe(ERR_no_lines, E_ERROR, sf->filename());
		return 0;
	}
	else if (num < line)
	{
		printe(ERR_only_n_lines, E_ERROR, num, sf->filename());
		return 0;
	}
	return sf;
}

// Print n lines.
static int
fprintn(LWP *lwp, SrcFile *sf, long count, long numlines)
{
	long	firstline, lastline;
	char	*fname;

	if ((fname = lwp->curr_src()) == 0)
	{
		printe(ERR_no_cur_src, E_ERROR);
		return 0;
	}
	// firstline would be zero for a file compiled w/o -g - 
	// no line number info
	if ((firstline = lwp->current_line()) == 0)
		firstline = 1;
	lastline = firstline + count - 1;

	if (lastline > numlines)
	{
		lastline = numlines;
		count = lastline - firstline + 1;
	}

	while (firstline <= lastline)
	{
		printm(MSG_line_src, firstline, sf->line((int)firstline));
		firstline++;
	}
	lwp->set_current_stmt(fname, lastline);

	return count;
}

// regular expression parsing and searching
static int
dore(LWP *lwp, SrcFile *sf, long mline, const char *nre, int backwds)
{
	long 			cline, tline;
	static char		*re;
	static size_t		len;		
	char	*fname;

	if ((fname = lwp->curr_src()) == 0)
	{
		printe(ERR_no_cur_src, E_ERROR);
		return 0;
	}

	if (*nre != '\0')
	{
		free(re);
		if ((re = compile(nre, 0, 0)) == 0)
		{
			printe(ERR_bad_re, E_ERROR);
			return 0;
		}
		
	}
	else if (!re)
	{
		printe(ERR_no_previous_re, E_ERROR);
		return 0;
	}

	tline = cline = lwp->current_line();

	do {
		if (backwds)
		{
			if (tline == mline)
				tline = 1;
			else
				tline += 1;
		}
		else
		{
			if (tline == 1)
				tline = mline;
			else
				tline -= 1;
		}
		if (step(sf->line((int)tline), re))
		{
			lwp->set_current_stmt(fname, tline);
			return 1;
		}
	} while (tline != cline);
	printe(ERR_no_re_match, E_WARNING);
	return 0;
}
