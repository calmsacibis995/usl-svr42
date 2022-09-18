#ident	"@(#)debugger:catalog.d/common/GMsg.awk	1.1"

# This awk script reads the Message table description file GMsg.awk.in
# and creates three new files:
#
# gui_msg.h
#	enum declaration of the types of messages
#	There is one entry for each non-comment line in Msg.awk.in
#
# GMtable.c
#	The initialization code for the message table,
#	one line per Message type.  Each line looks like:
#		{catalog number, format string}, // Gui_msg_id
#
# GMcatalog
#	Message text, one per line.  This is the input to mkmsgs,
#	which transforms it into the message catalog, uxdebug
#

BEGIN {
	f_tab_c = "GMtable.c"
	f_msg_h = "gui_msg.h"
	f_cat = "GMcatalog"

	# print the necesary header information to each file
	print "/* file produced by ../common/GMsg.awk */\n" >f_msg_h
	print "#ifndef _GUI_MSG_H"	>f_msg_h
	print "#define _GUI_MSG_H\n"	>f_msg_h
	print "enum Gui_msg_id\n{"		>f_msg_h
	print "\tGM_none = 0,"	>f_msg_h

	print "/* file produced by ../common/GMsg.awk\n */"  >f_tab_c
	print "#include \"gui_msg.h\"" > f_tab_c
	print "struct GM_tab gmtable[] = {" > f_tab_c
	print "{0},\t/* GM_none */" >f_tab_c

	# ------ WARNING -------
	# Messages in an existing catalog cannot be modified or removed,
	# because we have no control over the translated catalogs,
	# also, calls to gettxt have hard-coded numbers in them.
	# Messages MUST stay in the same order - after the first release
	# the catalog probably should not be machine generated
	next_num = 0			# next catalog entry
}

# main loop
# the command line in the makefile is
# 	awk -f GMsg.awk GMsg.awk.in

{
	# The only lines we are interested in are the ones that
	# start with GM_ or GE_

	if (substr($1, 1, 3) != "GM_" && substr($1, 1, 3) != "GE_")
		next

	msg_num = next_num++
	printf "\t%s,\n", $1 >f_msg_h
	printf "{%d,\t\"", msg_num >f_tab_c
	fstring = ""
	mid = $1
	j = NF

	# messages may span multiple lines
	# each line is printed individually to the output file
	for (i = 2; i <= j; i++)
	{
		if ((i == j) && (substr($i, length($i), 1) == "\\")) # continuation line
		{
			tmp = substr($i, 1, (length($i) - 1))
			if (i == 1)
				fstring = tmp
			else
				fstring = fstring " " tmp
			printf "%s", fstring > f_tab_c
			printf "%s", fstring > f_cat
			fstring = ""
			getline
			i = 0
			j = NF
			continue
		}
		if (i > 1 && fstring != "")
			fstring = fstring " "
		if ($i != "\\")	#\space used to ensure spacing
			fstring = fstring $i
	}
	printf "%s\"},", fstring > f_tab_c
	print fstring > f_cat
	printf "\t/* %s */\n", mid >f_tab_c
}

END {
	# finish off the message table files
	print "\tGM_last\n};\n"					>f_msg_h
	print "struct GM_tab\n{"				>f_msg_h
	print "\tint catindex;"					>f_msg_h
	print "\tconst char *string;"				>f_msg_h
	print "\tconst char *istring;"				>f_msg_h
	print "};\n"						>f_msg_h
	print "extern struct GM_tab gmtable[];\n"		>f_msg_h
	print "const char *gm_format(enum Gui_msg_id);\n"	>f_msg_h
	print "#endif // _GUI_MSG_H"				>f_msg_h

	print "{0}\t/* GM_last */\n};" >f_tab_c
}
