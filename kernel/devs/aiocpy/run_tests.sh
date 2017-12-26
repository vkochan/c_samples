#! /bin/bash

DEV="/dev/aiocpy"

rm -f $DEV 2> /dev/null
rmmod  aiocpy 2> /dev/null
sleep 1
insmod aiocpy.ko
sleep 1

DEV_MAJ=$(grep aiocpy /proc/devices | awk '{ print $1 }')
DEV_MIN="0"

mknod -m 666 $DEV c $DEV_MAJ $DEV_MIN

./aiocpy_test
