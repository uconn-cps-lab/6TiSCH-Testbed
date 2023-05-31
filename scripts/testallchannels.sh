skip=0;
#for CH in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
for CH in 5; do
#for GREEDY in 0 1; do
GREEDY=1
#skips
if [ "$skip" -gt "0" ];then
	skip=`expr $skip - 1`
	echo "Skip"
	continue
fi
echo "channels=$CH"
ex -sc "%s/var greedy = [^;]*;/var greedy = $GREEDY;/g|x" scheduler.js
ex -sc "%s/\"channels\":\d*,/\"channels\":$CH,/g|x" settings.json
node simulation.js | grep "Total number"

#done
done

