#!/bin/bash
set -Ceuo pipefail

test -f sample.gsm || bash prepsample.sh

rm -f z.sum
md5sum sample.gsm > z.sum
diff -- z.sum - <<EOF
8d024d08bc4127fa84d0cc1b97169ead  sample.gsm
EOF
rm -f z.sum

rm -f z.log
./gribslim -f'p0=,g361<,&' -oz.grib sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z      T f+0    d+0    vz2       m-0  
b20221220T1200Z      T f+0    d+0    vp1000    m-0  
b20221220T1200Z      T f+0    d+0    vp925     m-0  
b20221220T1200Z      T f+0    d+0    vp850     m-0  
b20221220T1200Z      T f+0    d+0    vp700     m-0  
b20221220T1200Z      T f+0    d+0    vp600     m-0  
b20221220T1200Z      T f+0    d+0    vp500     m-0  
b20221220T1200Z      T f+0    d+0    vp400     m-0  
b20221220T1200Z      T f+0    d+0    vp300     m-0  
b20221220T1200Z      T f+0    d+0    vp250     m-0  
b20221220T1200Z      T f+0    d+0    vp200     m-0  
b20221220T1200Z      T f+0    d+0    vp150     m-0  
b20221220T1200Z      T f+0    d+0    vp100     m-0  
b20221220T1200Z      T f+0    d+0    vp70      m-0  
b20221220T1200Z      T f+0    d+0    vp50      m-0  
b20221220T1200Z      T f+0    d+0    vp30      m-0  
b20221220T1200Z      T f+0    d+0    vp20      m-0  
b20221220T1200Z      T f+0    d+0    vp10      m-0  
b20221220T1200Z      T f+360  d+0    vz2       m-0  
b20221220T1200Z      T f+360  d+0    vp1000    m-0  
b20221220T1200Z      T f+360  d+0    vp925     m-0  
b20221220T1200Z      T f+360  d+0    vp850     m-0  
b20221220T1200Z      T f+360  d+0    vp700     m-0  
b20221220T1200Z      T f+360  d+0    vp600     m-0  
b20221220T1200Z      T f+360  d+0    vp500     m-0  
b20221220T1200Z      T f+360  d+0    vp400     m-0  
b20221220T1200Z      T f+360  d+0    vp300     m-0  
b20221220T1200Z      T f+360  d+0    vp250     m-0  
b20221220T1200Z      T f+360  d+0    vp200     m-0  
b20221220T1200Z      T f+360  d+0    vp150     m-0  
b20221220T1200Z      T f+360  d+0    vp100     m-0  
b20221220T1200Z      T f+360  d+0    vp70      m-0  
b20221220T1200Z      T f+360  d+0    vp50      m-0  
b20221220T1200Z      T f+360  d+0    vp30      m-0  
b20221220T1200Z      T f+360  d+0    vp20      m-0  
b20221220T1200Z      T f+360  d+0    vp10      m-0  
EOF

rm -f z.sum
md5sum z.grib > z.sum
diff -- z.sum - <<EOF
3d32d898842ba9b3ea3fa973258732e5  z.grib
EOF
rm -f z.sum

rm -f z2.log
./gribslim -a -oz2.grib z.grib > z2.log
diff -- z.log z2.log

rm -f z.sum
md5sum z2.grib > z.sum
diff -- z.sum - <<EOF
3d32d898842ba9b3ea3fa973258732e5  z2.grib
EOF
rm -f z.sum

rm -f z.log
./gribslim -oz.grib -f'p[T]=v92500<!g0=&&' sample.gsm
./gribslim -oz2.grib -f'p[T]=v92500<!g360=&&' sample.gsm
./gribslim -oz3.grib -a z.grib z2.grib
./gribslim -o/dev/null z3.grib

rm -f z*.log z*.grib
echo $0 okay
