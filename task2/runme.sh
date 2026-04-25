#!/bin/bash

rm -f stat.txt result.txt testfile.lck

make

echo "Starting test..." | tee result.txt

for i in {1..10}
do
    ./lock -f testfile &
    pids[$i]=$!
done

sleep 300

for pid in ${pids[*]}
do
    kill -SIGINT $pid
done

wait

echo "Test finished" | tee -a result.txt

echo "---- STAT FILE ----" | tee -a result.txt
cat stat.txt | tee -a result.txt

echo "---- CHECKS ----" | tee -a result.txt

lines=$(wc -l < stat.txt)
echo "Processes reported: $lines (expected 10)" | tee -a result.txt

avg=$(awk '{sum+=$3} END {print sum/NR}' stat.txt)
echo "Average locks: $avg" | tee -a result.txt