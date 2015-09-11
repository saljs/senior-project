#!/bin/bash


logName=$(date +%F_%H-%M).txt

echo "Senior project log for $(date +%F_%H-%M)" > $logName
echo "Time logged: " >> $logName
vim $logName
