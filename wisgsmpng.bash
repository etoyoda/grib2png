#!/bin/bash
set -Ceuo pipefail
# (C) TOYODA Eizi, 2021
# GSM 予報値GRIBをダウンロードして画像変換する
#
PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
LANG=C
TZ=UTC

test -d /nwp/p1/jmagrib || mkdir /nwp/p1/jmagrib
cd /nwp/p1/jmagrib

# 推定最新時刻
set $(LANG=C TZ=UTC date --date='3 hours ago' +'%Y%m%d %H')
: date $*
ymd=$1
hh=$2
hh=$(ruby -e 'printf("%02u", ARGV.first.to_i / 6 * 6)' $hh)
# 推定最新時刻フォルダができていれば終了
test ! -d ${ymd}T${hh}Z

# 作業中フォルダがロックとなる
if timestamp=$(stat --format=%Z work 2>/dev/null) ; then
  limit=$(LANG=C TZ=UTC date --date='1 hour ago' '+%s')
  if [[ $timestamp -lt $limit ]] ; then
    date --date="@${timestamp}" +'Lock file at %c - removed'
    rm -rf work
  else
    date --date="@${timestamp}" +'Lock file at %c - aborted'
    false
  fi
fi
test ! -d work
mkdir work
cd work

# syn がうまく動かないのでファイル決め打ちで攻める

tbegin=$(date +%s)
URL="https://www.wis-jma.go.jp/d/o/RJTD/GRIB/Global_Spectral_Model\
/Latitude_Longitude/1.25_1.25/90.0_-90.0_0.0_358.75/Upper_air_layers\
/${ymd}/${hh}0000/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_${ymd}${hh}0000\
_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin"
wget -q -Obiggrib.bin ${URL}
tend=$(date +%s)
let 'elapsed = tend - tbegin'
logger -tsyndl --id=$$ 'elapsed '${elapsed}' wget {"tag"=>"gsm13", "200"=>1}'

/nwp/bin/grib2png biggrib.bin > grib2png.log

cd ..
mv work ${ymd}T${hh}Z

keep_days=1
find . -maxdepth 1 -ctime +${keep_days} | xargs -r rm -rf
