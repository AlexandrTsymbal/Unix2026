#!/bin/bash

RESULT=result.txt

echo "" > $RESULT

echo "===== BUILD =====" >> $RESULT

make clean >> $RESULT 2>&1
make >> $RESULT 2>&1

echo "" >> $RESULT
echo "===== CLEAN OLD =====" >> $RESULT

pkill -f myinit 2>/dev/null
pkill -f testproc 2>/dev/null

rm -f /tmp/myinit.log

CONFIG=$(realpath config1.txt)

echo "" >> $RESULT
echo "===== START MYINIT =====" >> $RESULT

./myinit -c "$CONFIG"

sleep 3

echo "" >> $RESULT
echo "===== CHECK 3 PROCESSES =====" >> $RESULT

pgrep -af testproc >> $RESULT

COUNT=$(pgrep -af testproc | wc -l)

echo "Expected: 3" >> $RESULT
echo "Actual: $COUNT" >> $RESULT

echo "" >> $RESULT
echo "===== KILL PROC2 =====" >> $RESULT

pkill -f "proc2"

sleep 3

COUNT2=$(pgrep -af testproc | wc -l)

echo "Expected after restart: 3" >> $RESULT
echo "Actual: $COUNT2" >> $RESULT

echo "" >> $RESULT
echo "===== RELOAD CONFIG =====" >> $RESULT

cp config2.txt config1.txt

MYINIT_PID=$(pgrep myinit)

kill -HUP $MYINIT_PID

sleep 3

pgrep -af testproc >> $RESULT

COUNT3=$(pgrep -af testproc | wc -l)

echo "Expected after HUP: 1" >> $RESULT
echo "Actual: $COUNT3" >> $RESULT

echo "" >> $RESULT
echo "===== LOG CONTENT =====" >> $RESULT

cat /tmp/myinit.log >> $RESULT

echo "" >> $RESULT
echo "===== DONE =====" >> $RESULT

echo "Tests finished. See result.txt"