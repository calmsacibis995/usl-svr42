#ident	"@(#)xpr:xgetopt.c	1.5"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "xgetopt.h"

#define STREQU(A,B)	(strcmp((A),(B)) == 0)

#ifndef MEMUTIL
extern char		*malloc();
#endif /* MEMUTIL */

extern int		optopt,
			optind,
			opterr,
			getopt();

extern char		*optarg;

static int		first_time	= 1;

static void		add_optlet();

#if !defined(SYSV) && !defined(SVR4)
extern int strlen();	/* Defined in string.h in SYSV */
#endif
/*
 * (Picked up from the source to "getopt()", version 1.14.)
 */
static char error1[] = ": option requires an argument -- ";
#define ERR(s, c)	if(opterr){\
	extern int write();\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}

/**
 ** xgetopt()
 **/

int			xgetopt (argc, argv, options)
	int			argc;
	char			**argv;
	struct xoptions		*options;
{
	static char		*option_list	= 0;

	struct xoptions		*po;

	int			optlet;


	if (first_time) {
		register int		noptions;

		register char		*p,
					*q;


		if (option_list)
			free (option_list);
		option_list = 0;

		for (noptions = 0, po = options; po->letter; po++)
			noptions++;
		if (!noptions)
			return (-1);
		option_list = malloc(2 * noptions + 1); /* worst case */

		/*
		 * Construct the option string needed for "getopt()".
		 * We put a ':' after a letter if (1) an argument
		 * is needed after the option or (2) the letter is
		 * the first letter in a word option--the balance of
		 * the word is treated here as an ``argument''.
		 * Since we may not know until after we've added a
		 * letter once that it will eventually need a ':',
		 * we put a ' ' as a place-holder, which is removed
		 * if never replaced with a ':'.
		 */
		for (po = options; po->letter; po++) {
			add_optlet (
				option_list,
				po->letter,
				(po->arg_needed? ':' : ' ')
			);
			if (po->word)
				add_optlet (option_list, po->word[0], ':');
		}
		for (p = q = option_list; *p; p++)
			if (*p != ' ')
				*q++ = *p;
		*q = 0;

		first_time = 0;
	}

	/*
	 * Because of the non-standard command line syntax,
	 * we play some games with the "getopt()" routine's
	 * variables. We allow single letter options, for
	 * conformance to the standard (where a single letter
	 * is ambiguous we choose one meaning and provide
	 * alternate single letter options for the other
	 * meanings). However, we also allow the full word
	 * option. When given, the balance of the word will
	 * be an ``argument'', which we discard in favor of
	 * the real argument, if any.
	 */

	optlet = getopt(argc, argv, option_list);
	if (optlet == -1)
		return (optlet);

	/*
	 * "getopt()" found something wrong; this can either be
	 * an unknown option, or an option that "getopt()" thinks
	 * needs an argument. If the latter case, but the option
	 * doesn't really need an argument, return the valid
	 * option. Note that this means that a full word couldn't
	 * have been given--otherwise the balance of the word would
	 * have been the argument. Thus we need only check "po->letter".
	 */
	if (optlet == '?') {
		for (po = options; po->letter; po++)
			if (po->letter == optopt && !po->arg_needed)
				return (optopt);
		return ('?');
	}

	/*
	 * If no ':' after this letter in the option list,
	 * then the letter cannot be the first in a word;
	 * so nothing needs to be fixed up.
	 */
	if (strchr(option_list, optlet)[1] != ':')
		return (optlet);

	/*
	 * See if a full word option was given. The full word is
	 * composed of "optlet" following by the letters in "optarg".
	 * If we find that a full word was given, adjust "optarg" IF
	 * the option REALLY takes an argument. (The ':' in the
	 * option list may be a fake to allow a full word.)
	 * If we don't find a full word, then (because we constructed
	 * "option_list" from the table) the value in "optarg" must
	 * be the real argument.
	 */
	for (po = options; po->letter; po++) {
		if (
			po->word
		     && optlet == po->word[0]
		     && STREQU(optarg, po->word + 1)
		) {
			optlet = optopt = po->letter;
			if (po->arg_needed) {
				if (optind >= argc) {
					ERR (error1, optlet);
					return ('?');
				}
				optarg = argv[optind++];
			}
			break;
		}
	}

	/*
	 * If we didn't find that a full word option was given,
	 * we MAY have to adjust "optind" so that the argument in
	 * "optarg" is considered next time as an option, IF the
	 * current option doesn't REALLY take an argument.
	 *
	 * THIS IS TRICKY! The single letter option may be in
	 * a string of single letter options, in which case
	 * the balance of the single letter options should
	 * be examined--i.e. we shouldn't bump "optind".
	 * If the option at hand ISN'T in a string of options,
	 * "optarg" will point to the next command line option,
	 * i.e. the one before where "optind" currently points.
	 * Checking "optarg" tells us if "optind" should be fixed.
	 */
	if (!po->letter)
		for (po = options; po->letter; po++)
			if (optlet == po->letter) {
				if (
					!po->arg_needed
				     && optarg == argv[optind - 1]
				)
					optind--;
				break;
			}

	return (optlet);
}

/**
 ** add_optlet()
 **/

static void		add_optlet (str, optlet, arglet)
	char			*str;
	int			optlet,
				arglet;
{
	char			*pair	= "X:",
				*p;


	if ((p = strchr(str, optlet))) {
		if (p[1] == ' ')
			p[1] = arglet;
	} else {
		pair[0] = optlet;
		pair[1] = arglet;
		strcat (str, pair);
	}
	return;
}

/**
 ** xgetopt_setup()
 **/

void			xgetopt_setup ()
{
	first_time = 1;
	return;
}
