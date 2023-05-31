#!/bin/bash

echo "YOU NEED WINDOWS 10 LINUX SUBSYSTEM ENABLED"
xdsdfu=/mnt/c/ti/ccs_base/common/uscif/xds110/xdsdfu.exe
config=uniflash/user_files/configs/cc2650f128.ccxml
hex=../bin/hex/coap_cc26xx/CoAP-sensortag.hex
list=$($xdsdfu -e | grep "Serial Num" |cut -d: -f2|grep -o "[0-9a-zA-Z]*")
#echo "$list"
for i in {1..8}; do
	if [ ! -d "uniflash.$i" ]; then
		cp -r uniflash uniflash.$i
	fi
done
i=1
while read -r number; do
#	echo $number
	sed -e "s/Value=\"[^\"]*\" id=\"--/Value=\"$number\" id=\"--/g" $config > $config.$number
	cp $hex $hex.$number
done  <<< "$list"

while read -r number; do
	echo $i,$number
#	uniflash.$i/ccs_base/DebugServer/bin/DSLite.exe flash -c $config.$number -e -f -v "$hex.$number" | grep Success &
	uniflash.$i/ccs_base/DebugServer/bin/DSLite.exe flash -c $config.$number -f -v -r 0 -t 60 "$hex.$number" | grep success &
	sleep 1
	i=$(( i % 8 + 1 ))
done  <<< "$list"
wait

while read -r number; do
	rm $config.$number
	rm $hex.$number
done  <<< "$list"

