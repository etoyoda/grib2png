#!/bin/bash
set -euo pipefail

# wis2ukwmc.bash - WIS2からUKWMC GPVをダウンロードし貼り合わせ・画像変換

PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
LANG=C
TZ=UTC

test -x /usr/local/bin/wnm-ukwmc.rb
test -x /nwp/bin/gribukwmc
test -x /nwp/bin/grib2png

test -d /nwp/p1/ukwmc || mkdir /nwp/p1/ukwmc
cd /nwp/p1/ukwmc

# 推定最新時刻
: ${xdate:='5 hours ago'}
set -- $(LANG=C TZ=UTC date --date="${xdate}" +'%Y %m %d %H')
ymd=$1$2$3
hh=$4
ym=${1}-${2}
hh=$(ruby -e 'printf("%02u", ARGV.first.to_i / 12 * 12)' $hh)
export TIMECARD="$1 $2 $3 $hh 00"

# 推定最新時刻フォルダができていれば終了
test ! -d ${ymd}T${hh}Z

# ロックディレクトリ work を検知したら処理して work を新設し入る
verbose=false
if timestamp=$(stat --format=%Z work 2>/dev/null) ; then
  limit=$(LANG=C TZ=UTC date --date='1 hour ago' '+%s')
  if [[ $timestamp -lt $limit ]] ; then
    date --date="@${timestamp}" +'Lock file at %c - removed'
    rm -rf work
    verbose=true
  else
    date --date="@${timestamp}" +'Lock file at %c - aborted'
    exit 16
  fi
fi
mkdir work
cd work

# --- データ処理 ---
# WNM から UKWMC データのダウンロード
/usr/local/bin/wnm-ukwmc.rb > wnm-ukwmc.log

# 6枚タイルを全球に張り合わせ
/nwp/bin/gribukwmc z.ukwmc.tar > gribukwmc.log
mv merged.grib ukwmc${ymd}T${hh}.bin

# V要素を抜き出し
/nwp/bin/gribslim -f'p[V]=' -oukwmc-v.bin ukwmc${ymd}T${hh}.bin

# 画像化。U→V で処理する必要があるが wnm-ukwmc.rb が順序を
# 保証できていないため V 要素を再度読ませる
/nwp/bin/grib2png ukwmc${ymd}T${hh}.bin ukwmc-v.bin > grib2png.log

# アーカイブ
test -d /nwp/a1/$ym || mkdir /nwp/a1/$ym
gzip -9 ukwmc${ymd}T${hh}.bin
ln -f ukwmc${ymd}T${hh}.bin.gz /nwp/a1/ukwmc${ymd}T${hh}.bin.gz

# 仮ディレクトリ work を日時入り名称に変更
cd ..
test ! -d ${ymd}T${hh}Z || rm -rf ${ymd}T${hh}Z
mv work ${ymd}T${hh}Z

# 古い同種ディレクトリを削除
keep_days=2
find . -maxdepth 1 -type d -name '20*Z' -ctime +${keep_days} -print0 | xargs -0 rm -rf

if $verbose ; then
  echo ... and all done okay
fi
