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
let 'hh = hh / 6 * 6' || :
hh=$(printf '%02u' $hh)
# 推定最新時刻フォルダができていれば終了
test ! -d ${ymd}T${hh}Z

mkdir ${ymd}T${hh}Z
cd ${ymd}T${hh}Z


# syn がうまく動かないのでファイル決め打ちで攻める

URL="https://www.wis-jma.go.jp/d/o/RJTD/GRIB/Global_Spectral_Model\
/Latitude_Longitude/1.25_1.25/90.0_-90.0_0.0_358.75/Upper_air_layers\
/${ymd}/${hh}0000/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_${ymd}${hh}0000\
_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin"
wget -q -Obiggrib.bin ${URL}

/nwp/bin/grib2png biggrib.bin > grib2png.log
