for i in $(seq 40 70 )
do
   echo "$i"
   node addSchedule $i 5 3 1 2
done
