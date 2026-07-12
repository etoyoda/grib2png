#!/bin/bash
set -Ceuo pipefail
# (C) TOYODA Eizi, 2024
#
PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
LANG=C
TZ=UTC

test -d /nwp/p0 -a -d /nwp/p1/jmagrib -a -d /nwp/p2
cd /nwp/p2

# 推定最新時刻
set $(LANG=C TZ=UTC date --date='45 minutes ago' +'%Y %m %d %H')
: date $*
yy=$1
mm=$2
dd=$3
hh=$4
case ${hh} in
0[1-5]) hh=00 ;;
0[6-9]|10|11) hh=06 ;;
1[2-7]) hh=12 ;;
18|19|2?) hh=18 ;;
esac

# 推定最新時刻フォルダができていれば終了
target=${yy}-${mm}-${dd}T${hh}Z-ema
test ! -d ${target}

# 入力ファイルチェック
prev6=$(LANG=C TZ=UTC date --date="${yy}-${mm}-${dd}T${hh}:00Z -6 hours" +'%Y%m%dT%H')
gsmfile=/nwp/p1/jmagrib/${prev6}Z/gsm${prev6}.bin
test -f ${gsmfile}
obsbf=
try=/nwp/p0/${yy}-${mm}-${dd}.new/obsbf-${yy}-${mm}-${dd}.tar
if test -f ${try} ; then
  obsbf=${try}
else
  try=/nwp/p0/${yy}-${mm}-${dd}/obsbf-${yy}-${mm}-${dd}.tar
  if test -f ${try} ; then
    obsbf=${try}
  else
    obsbf=/nwp/p0/${yy}-${mm}-${dd}/obsbf-${yy}-${mm}-${dd}.tar.gz
  fi
fi
test -f ${obsbf}

# 作業中フォルダがロックとなる
work=wk.emagram
if timestamp=$(stat --format=%Z ${work} 2>/dev/null) ; then
  limit=$(LANG=C TZ=UTC date --date='1 hour ago' '+%s')
  if [[ $timestamp -lt $limit ]] ; then
    date --date="@${timestamp}" +'Lock file at %c - removed'
    rm -rf ${work}
  else
    date --date="@${timestamp}" +'Lock file at %c - aborted'
    false
  fi
fi
test ! -d ${work}
mkdir ${work}
cd ${work}


# sapporo
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC61.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
43.75 141.25 +44+141/Mashike
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s1 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}hokkaido.png

# wajima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC64.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
33.75 135.00 +34+135/Gobo
37.50 136.25 +38+136/Noto
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s2 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}noto.png

# kagoshima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC70.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
31.25 130.0 +31+130/Makurazaki
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s1 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}kyushu.png

# naze
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick "${obsbf}:AHL=^IUKC71.RJTD.${dd}${hh}" > zobs.txt
ruby /nwp/bin/bufr2pick "${obsbf}:AHL=^IUKC72.RJTD.${dd}${hh}" >> zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
25.00 125.00 +25+125/Miyakojima
28.75 130.00 +29+130/Amami
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s2 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}ryukyu.png

# minamidaito
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC73.RJTD.${dd}${hh} >> zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
26.25 131.25 +26+131/Minamidaitojima
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s1 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}daito.png

# chichijima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUSC03.RJTD.${dd}${hh} | grep ,47991 >> zobs.txt || :
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
25.00 153.75 +25+154/Minamitorishima
27.50 142.50 +28+143/Ogasawara
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s2 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}ogasawara.png

# kanto
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC65.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
36.25 140 +36+140/Chikusei
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s1 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}kanto.png

test ! -f zmodel.txt || rm -f zmodel.txt
ls -1 /nwp/p1/jmagrib/2*/gsm*.bin | tail -4 > zstn.txt
/nwp/bin/gribpick -f'g0=' -p $(cat zstn.txt) > zmodel.txt <<PICK
  36.25 140 +36+140/Chikusei
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram -s3 zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}kanto2.png

rm -f z*.txt
cd ..
mv ${work} ${target}
