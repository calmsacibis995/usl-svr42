#ident	"@(#)lp.admin:printers/requests/Menu.rq.ch	1.5.3.1"
#ident  "$Header: Menu.rq.ch 2.0 91/07/12 $"

menu=Choices
multiselect=true
framemsg="MARK choices then press ENTER"


`set -l name_1="/tmp/lp.n1$VPID";
if [ -z $ARG1 ];
	then set -l list=all;
	else set -l list=$ARG1;
fi;
if [ -n "$TFADMIN" ]; then $TFADMIN lpstat -o$list | fmlcut -d' ' -f1  > $name_1;
else lpstat -o$list  | fmlcut -d' ' -f1  > $name_1; fi;
if [ -s $name_1 ];
then
	:;
else
	echo "init=false";
	message -b "There are no with print requests available now.";
	rm  $name_1;
fi`

close=`rm  $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort -u $name_1 | regex '^(.*)$0$' 'name=$m0'`
