#ident	"@(#)lp.admin:printers/classes/classes.ch	1.1.2.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/lp.admin/printers/classes/classes.ch,v 1.1 91/02/28 18:13:24 ccs Exp $"
menu=Classes

lifetime=shortterm

multiselect
init=`cocreate -i lpdata -R dests.ch -e "-EOT-" $SPOOLDIR/bin/lpdata -f $datafile`
done=CLOSE
close=`
	getitems "," | set -l x;
	echo $x | regex
	    ',all$'	'all'
	    '(.*)$0'	'$m0' |
	set -l Form_Choice;
	unset -l all -l x;
	codestroy -R dests.ch lpdata;
`

`
	cosend lpdata list_classes |
	regex '^(..*)$0'	'name="$m0"' && echo name=all | \
	    shell 'sort';
`
