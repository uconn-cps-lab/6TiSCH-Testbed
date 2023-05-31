TTY="/dev/ttyACM0"

timestamp(){
echo -n `date +"%Y-%m-%d %H:%M:%S"`
echo -n " "
}

skip=0;
for TOPO in 2 0 1; do
for ALG in "PART" "RAND";do

#skips
if [ "$skip" -gt "0" ];then
	skip=`expr $skip - 1`
	echo "Skip topology $TOPO with algorithm $ALG"
	continue
fi

# setup inputs
cp -T mytopology.$TOPO.json mytopology.json
ex -sc "%s/var algorithm = [^;]*;/var algorithm = $ALG;/g|x" scheduler.js
timestamp;echo "Now testing topology $TOPO with algorithm $ALG"

while true; do
# stuff ctrl-c's
screen -X -p "webapp-exe" stuff ^c
sleep 3
screen -X -p "gateway-exe" stuff ^c
sleep 5

timestamp; echo "firing gwapp and app.js"
#stuff commands
screen -X -p "gateway-exe" stuff "while true; do sudo ./gwapp.exe -s $TTY | tee ../../../../bin/webapp/log_gwapp_`date +'%Y-%m-%d-%H-%M-%S'`.log; sleep 1; done
"
sleep 3
screen -X -p "webapp-exe" stuff "sudo node app.js | tee log_appjs_`date +'%Y-%m-%d-%H-%M-%S'`.log
"

#check running state
## wait 30 sec or until strings found
for i in `seq 30`;do
	if grep --quiet "dio_report" log_appjs.log;then
		if grep --quiet "COAP Socket Open" log_gwapp.log;then
			break;
		fi
	fi
	sleep 1
done

## check no error found
if grep --quiet "dio_report" log_appjs.log; then
	if grep --quiet "COAP Socket Open" log_gwapp.log; then
		if ! cat log_gwapp.log | sed '/COAP Socket Open/q' | grep --quiet "Association" ; then
			timestamp; echo "gwapp and app.js fired successfully"
			break;
		fi
	fi
fi
timestamp; echo "something wrong with gwapp or app.js, restarting"
done

timestamp; echo "Setting up topology"
#stuff topology setup
screen -X -p "topologysetup" stuff ^c
sleep 3
screen -X -p "topologysetup" stuff "sudo node setup_my_topology.js | tee log_setup_my_topology.log
"
sleep 10

#wait topology setup finish
while true; do
	if grep --quiet "Finished" log_setup_my_topology.log;then
		break;
	fi
	sleep 1
done

timestamp; echo -n "Topology setup finished. Now wait..... [COPYME]: TOPO[$TOPO], ALG[$ALG] ";timestamp; echo
sleep $((3 * 3600))
timestamp; echo "Wait time passed. Killing a test"

echo "Test another algorithm"
done
echo "Test another topology"
done
