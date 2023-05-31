#!/bin/bash
screen -S 6tisch -X -p 1 stuff "^c"
sleep 1
screen -S 6tisch -X -p 0 stuff "^c"
sleep 1

screen -S 6tisch -X quit

#sudo bash -c "echo 0 > /sys/kernel/debug/musb-hdrc.1/softconnect"
sudo uhubctl -a0 -l1-1 &> /dev/null
