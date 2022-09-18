#ident	"@(#)lp.admin:printers/filters/Menu.filter.ch	1.2.4.1"
#ident  "$Header: Menu.filter.ch 2.0 91/07/12 $"

menu=Choices
lifeterm=shortterm
multiselect=true
framemsg="MARK choices then press ENTER"

init=`indicator -w;`

`set -l name_1="/tmp/lp.fl.ch$VPID";
       fmlcut -d: -f5 /usr/spool/lp/admins/lp/filter.table > $name_1;
if [ -s $name_1 ];
then
	echo "all" >> $name_1;
else
	echo "init=false";
	message -b "There are no filters available";
	rm -f $name_1;
fi`


close=`/usr/bin/rm -f $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort $name_1 | regex '^(.*)$0$' 'name=$m0'`

