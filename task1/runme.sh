#!/bin/bash

set -e

echo "=== Build ===" > result.txt
make >> result.txt 2>&1

echo "=== Generate file A ===" >> result.txt
./gen_test.sh

echo "=== Create sparse B ===" >> result.txt
./task1 A B

echo "=== Gzip A and B ===" >> result.txt
gzip -c A > A.gz
gzip -c B > B.gz

echo "=== Restore B -> C through program ===" >> result.txt
gzip -cd B.gz | ./task1 C

echo "=== Create D with block size 100 ===" >> result.txt
./task1 -b 100 A D

echo "=== File stats ===" >> result.txt
stat A >> result.txt
stat A.gz >> result.txt
stat B >> result.txt
stat B.gz >> result.txt
stat C >> result.txt
stat D >> result.txt

echo "=== Done ===" >> result.txt