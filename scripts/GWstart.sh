#!/bin/bash
screen -dmS 6tisch
screen -S 6tisch -X screen

sudo rm -rf /home/ubuntu/6TiSCH-HARP/bin/webapp/db
sudo uhubctl -a2 -l1-1 &> /dev/null

sleep 3
screen -S 6tisch -X -p 0 stuff "cd /home/ubuntu/6TiSCH-UCONN/source/Projects/gateway/proj;sudo ./gwapp.exe -s /dev/ttyACM0 -f 100 -b 2 -n 6 | tee /home/ubuntu/log_gwapp.txt
"

sleep 3
# screen -S 6tisch -X -p 1 stuff "cd /home/ubuntu/6tisch/bin/webapp;sudo node --expose-gc --inspect=0.0.0.0:9222 app.js | tee /home/ubuntu/log_webapp.txt
# "
screen -S 6tisch -X -p 1 stuff "cd /home/ubuntu/6TiSCH-UCONN/bin/webapp;sudo node app.js | tee /home/ubuntu/log_webapp.txt
"

