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
42.50 141.25 42.5N141.3E_Shiraoi
43.75 141.25 43.8N141.3E_Mashike
45.00 141.25 45.0N141.3E_Rishiri
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}hokkaido.png

# wajima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC64.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
36.25 135.00 36.3N135.0E_Tango
36.25 136.25 36.3N136.3E_Awara
37.50 137.50 37.5N137.5E_Suzu
38.75 138.75 38.8N138.8E_Awashima
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}noto.png

# kagoshima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC70.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
31.25 130.0 31.3N130E_Makurazaki
32.50 130.0 32.5N130E_Amakusa
33.75 130.0 33.8N130E_Karatsu
30.00 130.0 33.0N130E_Yaku
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}kyushu.png

# naze
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC71.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
28.75 130.00 28.8N130.0E_Amami
26.25 127.50 26.3N127.5E_Okinawa
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}amami.png

# ishigaki
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC72.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
23.75 123.75 23.8N123.8E_Yaeyama
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}yaeyama.png

# minamidaito
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC73.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
26.25 131.25 26.3N131.3N_Daito
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}daito.png

# chichijima
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC73.RJTD.${dd}${hh} > zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUSC03.RJTD.${dd}${hh} | grep ,47991 >> zobs.txt || :
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
27.50 142.50 27.5N142.5E_Ogasawara
25.00 153.75 25.0N153.8E_M.torishima
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}ogasawara.png

# kanto
test ! -f zobs.txt || rm -f zobs.txt
ruby /nwp/bin/bufr2pick ${obsbf}:AHL=^IUKC65.RJTD.${dd}${hh} > zobs.txt
test ! -f zmodel.txt || rm -f zmodel.txt
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt <<PICK
33.75 140 33.8N140E_Mikurajima
35 140    35.0N140E_Tateyama
36.25 140 36.3N140E_Chikusei
37.5 140  37.5N140E_Aizu
38.75 140 38.8N140E_Shonai
40 140    40.0N140E_Ogata
PICK
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png ${yy}${mm}${dd}${hh}kanto.png

rm -f z*.txt
cd ..
mv ${work} ${target}
