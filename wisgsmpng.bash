#!/bin/bash
set -Ceuo pipefail
# (C) TOYODA Eizi, 2021
# ナウキャストのレーダー実況画像をダウンロードする
# 定期的に起動されることを想定している。時刻は細かくチューニングしなくてよい。
#
PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
LANG=C
TZ=UTC

SYNURL='https://www.wis-jma.go.jp/data/syn?ContentType=Text&Resolution=1.25%201.25&Grid=Latitude/Longitude&Discipline=Global%20Spectral%20Model&Type=GRIB&Access=Open&Indicator=RJTD&Area=90.0%20-90.0%200.0%20358.75'

test -d /nwp/p1/jmagrib || mkdir /nwp/p1/jmagrib
cd /nwp/p1/jmagrib

test ! -f syn.txt || rm -f syn.txt
wget -Osyn.txt -q $SYNURL


