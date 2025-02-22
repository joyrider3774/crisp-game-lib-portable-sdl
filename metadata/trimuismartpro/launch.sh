#!/bin/sh
echo $0 $*
PROGDIR=/mnt/SDCARD/Apps/cglpSDL2

cd $PROGDIR
HOME=$PROGDIR/ ./cglpsdl2 -f -w 1280 -h 720 -nsd -a
