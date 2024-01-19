#!/bin/sh
cd /nwp/p1
find * -name '*.png' > files.lst.new
find ../p2 -name himdst\*.png >> files.lst.new
mv -f files.lst.new files.lst
