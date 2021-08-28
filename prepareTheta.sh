#!/bin/bash

PTPCAM=/home/tkrueger/funprojects/libptp2/build/ptpcam

#list devices
#$PTPCAM -l

#info
#$PTPCAM -i

echo "reset"
$PTPCAM -r

#switch sleep mode off
#$PTPCAM --set-property=0xD80E --val=0x001

#echo "waiting"
#sleep 5

#reset
#$PTPCAM -r

echo "swap mode"
#switch to liv streaming mode
$PTPCAM --set-property=0x5013 --val=0x0001


