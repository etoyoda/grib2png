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
0?|10|11) hh=00 ;;
1*|2*) hh=12 ;;
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
echo 42.5 141.25 42.5N141.3E | \
/nwp/bin/gribpick -f'g360=' -p $gsmfile > zmodel.txt
test ! -f plot.png || rm -f plot.png
/nwp/bin/emagram zmodel.txt zobs.txt
mv -f plot.png e${yy}${mm}${dd}${hh}sapporo.png

cd ..
mv ${work} ${target}
