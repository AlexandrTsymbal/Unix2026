#!/bin/bash

FILE=A
SIZE=$((4*1024*1024 + 1))

dd if=/dev/zero of=$FILE bs=1M count=4 status=none

printf "\x01" | dd of=$FILE bs=1 seek=0 conv=notrunc 2>/dev/null
printf "\x01" | dd of=$FILE bs=1 seek=10000 conv=notrunc 2>/dev/null
printf "\x01" | dd of=$FILE bs=1 seek=$((SIZE-1)) conv=notrunc 2>/dev/null