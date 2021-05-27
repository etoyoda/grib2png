#!/bin/bash
set -Ceuo pipefail
# (C) TOYODA Eizi, 2021
#
PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
LANG=C
TZ=UTC

SYNURL='https://www.wis-jma.go.jp/data/syn?ContentType=Text&Resolution=1.25%201.25&Grid=Latitude/Longitude&Discipline=Global%20Spectral%20Model&Type=GRIB&Access=Open&Indicator=RJTD&Area=90.0%20-90.0%200.0%20358.75'

test -d /nwp/p1/jmagrib || mkdir /nwp/p1/jmagrib
cd /nwp/p1/jmagrib

# syn.txt works as a lock file
# just in case the script crashes and old one remains
if lastrun=$(stat --format=%Z syn.txt 2>/dev/null) ; then
  limit=$(date --date='6 hour ago' '+%s')
  if [[ $lastrun -lt $limit ]] ; then
    rm -f syn.txt
    date --date="@${lastrun}" +'Lock file at %c - removed'
  else
    date --date="@${lastrun}" +'Lock file at %c - aborted'
    false
  fi
fi
trap "rm -f ${PWD}/syn.txt" EXIT
wget -Osyn.txt -q $SYNURL

rm -f syn.out
ruby - syn.txt <<RUBY >syn.out
hit=nil
while line=gets
  line.chomp!
  w=line.split(/\//)[-3..-1]
  w.push line
  next unless /A_SZ/ === w[2]
  if hit.nil? then hit=w
  elsif w[2]>hit[2] then hit=w
  end
end
exit 1 unless hit
puts [hit[0], 'T', hit[1][0..1], ' ', hit[3]].join
RUBY

read reftime url < syn.out
rm -f syn.out
test ! -d ${reftime}
# exits if reftime already processed

mkdir ${reftime}
cd ${reftime}
wget -q -Obig.grib "${url}"
/nwp/bin/grib2png big.grib
