#!/bin/bash

for slot in 9 11 17 19 23 27 29 33 37 40 44 47 49 59 67 70 73 79 82 87 89 97 99
do
   echo "$slot"
   node addSchedule $slot 15 1 999 1
done