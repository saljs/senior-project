#!/bin/bash 

cat *.txt | grep "Time logged" | awk '{print $3}' | tr -d 'hrs' | awk '{s+=$1} END {print s}'
