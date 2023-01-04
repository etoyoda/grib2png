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
b20221220T1200Z      T f+0    d+0    v101302.5 m-1 
b20221220T1200Z      T f+0    d+0    v100000.0 m-1 
b20221220T1200Z      T f+0    d+0    v92500.0  m-1 
b20221220T1200Z      T f+0    d+0    v85000.0  m-1 
b20221220T1200Z      T f+0    d+0    v70000.0  m-1 
b20221220T1200Z      T f+0    d+0    v60000.0  m-1 
b20221220T1200Z      T f+0    d+0    v50000.0  m-1 
b20221220T1200Z      T f+0    d+0    v40000.0  m-1 
b20221220T1200Z      T f+0    d+0    v30000.0  m-1 
b20221220T1200Z      T f+0    d+0    v25000.0  m-1 
b20221220T1200Z      T f+0    d+0    v20000.0  m-1 
b20221220T1200Z      T f+0    d+0    v15000.0  m-1 
b20221220T1200Z      T f+0    d+0    v10000.0  m-1 
b20221220T1200Z      T f+0    d+0    v7000.0   m-1 
b20221220T1200Z      T f+0    d+0    v5000.0   m-1 
b20221220T1200Z      T f+0    d+0    v3000.0   m-1 
b20221220T1200Z      T f+0    d+0    v2000.0   m-1 
b20221220T1200Z      T f+0    d+0    v1000.0   m-1 
b20221220T1200Z      T f+360  d+0    v101302.5 m-1 
b20221220T1200Z      T f+360  d+0    v100000.0 m-1 
b20221220T1200Z      T f+360  d+0    v92500.0  m-1 
b20221220T1200Z      T f+360  d+0    v85000.0  m-1 
b20221220T1200Z      T f+360  d+0    v70000.0  m-1 
b20221220T1200Z      T f+360  d+0    v60000.0  m-1 
b20221220T1200Z      T f+360  d+0    v50000.0  m-1 
b20221220T1200Z      T f+360  d+0    v40000.0  m-1 
b20221220T1200Z      T f+360  d+0    v30000.0  m-1 
b20221220T1200Z      T f+360  d+0    v25000.0  m-1 
b20221220T1200Z      T f+360  d+0    v20000.0  m-1 
b20221220T1200Z      T f+360  d+0    v15000.0  m-1 
b20221220T1200Z      T f+360  d+0    v10000.0  m-1 
b20221220T1200Z      T f+360  d+0    v7000.0   m-1 
b20221220T1200Z      T f+360  d+0    v5000.0   m-1 
b20221220T1200Z      T f+360  d+0    v3000.0   m-1 
b20221220T1200Z      T f+360  d+0    v2000.0   m-1 
b20221220T1200Z      T f+360  d+0    v1000.0   m-1 
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
