#!/bin/bash
set -Ceuo pipefail

test -f sample.gsm || bash prepsample.sh
test ! -f z.log || rm -f z.log

./gribslim -f'p[VVPa]=v70000=v30000=|&,p[rDIV]=v85000=v25000=|&,p[rVOR]=v50000=&,p[U]=p[V]=p[T]=p[RH]=p[Z]=||||,|||,g361<&,p[Z]=v50000=&p[RAIN]=p[Pmsl]=||,g720%0=g360=|&,|' -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z      U f+0    d+0    vz10      m-0  
b20221220T1200Z      V f+0    d+0    vz10      m-0  
b20221220T1200Z      T f+0    d+0    vz2       m-0  
b20221220T1200Z     RH f+0    d+0    vz2       m-0  
b20221220T1200Z   Pmsl f+0    d+0    vmsl      m-0  
b20221220T1200Z      U f+0    d+0    vp1000    m-0  
b20221220T1200Z      V f+0    d+0    vp1000    m-0  
b20221220T1200Z      T f+0    d+0    vp1000    m-0  
b20221220T1200Z     RH f+0    d+0    vp1000    m-0  
b20221220T1200Z      Z f+0    d+0    vp1000    m-0  
b20221220T1200Z      U f+0    d+0    vp925     m-0  
b20221220T1200Z      V f+0    d+0    vp925     m-0  
b20221220T1200Z      T f+0    d+0    vp925     m-0  
b20221220T1200Z     RH f+0    d+0    vp925     m-0  
b20221220T1200Z      Z f+0    d+0    vp925     m-0  
b20221220T1200Z      U f+0    d+0    vp850     m-0  
b20221220T1200Z      V f+0    d+0    vp850     m-0  
b20221220T1200Z      T f+0    d+0    vp850     m-0  
b20221220T1200Z     RH f+0    d+0    vp850     m-0  
b20221220T1200Z      Z f+0    d+0    vp850     m-0  
b20221220T1200Z      U f+0    d+0    vp700     m-0  
b20221220T1200Z      V f+0    d+0    vp700     m-0  
b20221220T1200Z      T f+0    d+0    vp700     m-0  
b20221220T1200Z     RH f+0    d+0    vp700     m-0  
b20221220T1200Z      Z f+0    d+0    vp700     m-0  
b20221220T1200Z   VVPa f+0    d+0    vp700     m-0  
b20221220T1200Z      U f+0    d+0    vp600     m-0  
b20221220T1200Z      V f+0    d+0    vp600     m-0  
b20221220T1200Z      T f+0    d+0    vp600     m-0  
b20221220T1200Z     RH f+0    d+0    vp600     m-0  
b20221220T1200Z      Z f+0    d+0    vp600     m-0  
b20221220T1200Z      U f+0    d+0    vp500     m-0  
b20221220T1200Z      V f+0    d+0    vp500     m-0  
b20221220T1200Z      T f+0    d+0    vp500     m-0  
b20221220T1200Z     RH f+0    d+0    vp500     m-0  
b20221220T1200Z      Z f+0    d+0    vp500     m-0  
b20221220T1200Z   rVOR f+0    d+0    vp500     m-0  
b20221220T1200Z      U f+0    d+0    vp400     m-0  
b20221220T1200Z      V f+0    d+0    vp400     m-0  
b20221220T1200Z      T f+0    d+0    vp400     m-0  
b20221220T1200Z     RH f+0    d+0    vp400     m-0  
b20221220T1200Z      Z f+0    d+0    vp400     m-0  
b20221220T1200Z      U f+0    d+0    vp300     m-0  
b20221220T1200Z      V f+0    d+0    vp300     m-0  
b20221220T1200Z      T f+0    d+0    vp300     m-0  
b20221220T1200Z     RH f+0    d+0    vp300     m-0  
b20221220T1200Z      Z f+0    d+0    vp300     m-0  
b20221220T1200Z   VVPa f+0    d+0    vp300     m-0  
b20221220T1200Z      U f+0    d+0    vp250     m-0  
b20221220T1200Z      V f+0    d+0    vp250     m-0  
b20221220T1200Z      T f+0    d+0    vp250     m-0  
b20221220T1200Z      Z f+0    d+0    vp250     m-0  
b20221220T1200Z   rDIV f+0    d+0    vp250     m-0  
b20221220T1200Z      U f+0    d+0    vp200     m-0  
b20221220T1200Z      V f+0    d+0    vp200     m-0  
b20221220T1200Z      T f+0    d+0    vp200     m-0  
b20221220T1200Z      Z f+0    d+0    vp200     m-0  
b20221220T1200Z      U f+0    d+0    vp150     m-0  
b20221220T1200Z      V f+0    d+0    vp150     m-0  
b20221220T1200Z      T f+0    d+0    vp150     m-0  
b20221220T1200Z      Z f+0    d+0    vp150     m-0  
b20221220T1200Z      U f+0    d+0    vp100     m-0  
b20221220T1200Z      V f+0    d+0    vp100     m-0  
b20221220T1200Z      T f+0    d+0    vp100     m-0  
b20221220T1200Z      Z f+0    d+0    vp100     m-0  
b20221220T1200Z      U f+0    d+0    vp70      m-0  
b20221220T1200Z      V f+0    d+0    vp70      m-0  
b20221220T1200Z      T f+0    d+0    vp70      m-0  
b20221220T1200Z      Z f+0    d+0    vp70      m-0  
b20221220T1200Z      U f+0    d+0    vp50      m-0  
b20221220T1200Z      V f+0    d+0    vp50      m-0  
b20221220T1200Z      T f+0    d+0    vp50      m-0  
b20221220T1200Z      Z f+0    d+0    vp50      m-0  
b20221220T1200Z      U f+0    d+0    vp30      m-0  
b20221220T1200Z      V f+0    d+0    vp30      m-0  
b20221220T1200Z      T f+0    d+0    vp30      m-0  
b20221220T1200Z      Z f+0    d+0    vp30      m-0  
b20221220T1200Z      U f+0    d+0    vp20      m-0  
b20221220T1200Z      V f+0    d+0    vp20      m-0  
b20221220T1200Z      T f+0    d+0    vp20      m-0  
b20221220T1200Z      Z f+0    d+0    vp20      m-0  
b20221220T1200Z      U f+0    d+0    vp10      m-0  
b20221220T1200Z      V f+0    d+0    vp10      m-0  
b20221220T1200Z      T f+0    d+0    vp10      m-0  
b20221220T1200Z      Z f+0    d+0    vp10      m-0  
b20221220T1200Z      U f+360  d+0    vz10      m-0  
b20221220T1200Z      V f+360  d+0    vz10      m-0  
b20221220T1200Z      T f+360  d+0    vz2       m-0  
b20221220T1200Z     RH f+360  d+0    vz2       m-0  
b20221220T1200Z   Pmsl f+360  d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+360  vsfc      m-0  
b20221220T1200Z      U f+360  d+0    vp1000    m-0  
b20221220T1200Z      V f+360  d+0    vp1000    m-0  
b20221220T1200Z      T f+360  d+0    vp1000    m-0  
b20221220T1200Z     RH f+360  d+0    vp1000    m-0  
b20221220T1200Z      Z f+360  d+0    vp1000    m-0  
b20221220T1200Z      U f+360  d+0    vp925     m-0  
b20221220T1200Z      V f+360  d+0    vp925     m-0  
b20221220T1200Z      T f+360  d+0    vp925     m-0  
b20221220T1200Z     RH f+360  d+0    vp925     m-0  
b20221220T1200Z      Z f+360  d+0    vp925     m-0  
b20221220T1200Z      U f+360  d+0    vp850     m-0  
b20221220T1200Z      V f+360  d+0    vp850     m-0  
b20221220T1200Z      T f+360  d+0    vp850     m-0  
b20221220T1200Z     RH f+360  d+0    vp850     m-0  
b20221220T1200Z      Z f+360  d+0    vp850     m-0  
b20221220T1200Z      U f+360  d+0    vp700     m-0  
b20221220T1200Z      V f+360  d+0    vp700     m-0  
b20221220T1200Z      T f+360  d+0    vp700     m-0  
b20221220T1200Z     RH f+360  d+0    vp700     m-0  
b20221220T1200Z      Z f+360  d+0    vp700     m-0  
b20221220T1200Z   VVPa f+360  d+0    vp700     m-0  
b20221220T1200Z      U f+360  d+0    vp600     m-0  
b20221220T1200Z      V f+360  d+0    vp600     m-0  
b20221220T1200Z      T f+360  d+0    vp600     m-0  
b20221220T1200Z     RH f+360  d+0    vp600     m-0  
b20221220T1200Z      Z f+360  d+0    vp600     m-0  
b20221220T1200Z      U f+360  d+0    vp500     m-0  
b20221220T1200Z      V f+360  d+0    vp500     m-0  
b20221220T1200Z      T f+360  d+0    vp500     m-0  
b20221220T1200Z     RH f+360  d+0    vp500     m-0  
b20221220T1200Z      Z f+360  d+0    vp500     m-0  
b20221220T1200Z   rVOR f+360  d+0    vp500     m-0  
b20221220T1200Z      U f+360  d+0    vp400     m-0  
b20221220T1200Z      V f+360  d+0    vp400     m-0  
b20221220T1200Z      T f+360  d+0    vp400     m-0  
b20221220T1200Z     RH f+360  d+0    vp400     m-0  
b20221220T1200Z      Z f+360  d+0    vp400     m-0  
b20221220T1200Z      U f+360  d+0    vp300     m-0  
b20221220T1200Z      V f+360  d+0    vp300     m-0  
b20221220T1200Z      T f+360  d+0    vp300     m-0  
b20221220T1200Z     RH f+360  d+0    vp300     m-0  
b20221220T1200Z      Z f+360  d+0    vp300     m-0  
b20221220T1200Z   VVPa f+360  d+0    vp300     m-0  
b20221220T1200Z      U f+360  d+0    vp250     m-0  
b20221220T1200Z      V f+360  d+0    vp250     m-0  
b20221220T1200Z      T f+360  d+0    vp250     m-0  
b20221220T1200Z      Z f+360  d+0    vp250     m-0  
b20221220T1200Z   rDIV f+360  d+0    vp250     m-0  
b20221220T1200Z      U f+360  d+0    vp200     m-0  
b20221220T1200Z      V f+360  d+0    vp200     m-0  
b20221220T1200Z      T f+360  d+0    vp200     m-0  
b20221220T1200Z      Z f+360  d+0    vp200     m-0  
b20221220T1200Z      U f+360  d+0    vp150     m-0  
b20221220T1200Z      V f+360  d+0    vp150     m-0  
b20221220T1200Z      T f+360  d+0    vp150     m-0  
b20221220T1200Z      Z f+360  d+0    vp150     m-0  
b20221220T1200Z      U f+360  d+0    vp100     m-0  
b20221220T1200Z      V f+360  d+0    vp100     m-0  
b20221220T1200Z      T f+360  d+0    vp100     m-0  
b20221220T1200Z      Z f+360  d+0    vp100     m-0  
b20221220T1200Z      U f+360  d+0    vp70      m-0  
b20221220T1200Z      V f+360  d+0    vp70      m-0  
b20221220T1200Z      T f+360  d+0    vp70      m-0  
b20221220T1200Z      Z f+360  d+0    vp70      m-0  
b20221220T1200Z      U f+360  d+0    vp50      m-0  
b20221220T1200Z      V f+360  d+0    vp50      m-0  
b20221220T1200Z      T f+360  d+0    vp50      m-0  
b20221220T1200Z      Z f+360  d+0    vp50      m-0  
b20221220T1200Z      U f+360  d+0    vp30      m-0  
b20221220T1200Z      V f+360  d+0    vp30      m-0  
b20221220T1200Z      T f+360  d+0    vp30      m-0  
b20221220T1200Z      Z f+360  d+0    vp30      m-0  
b20221220T1200Z      U f+360  d+0    vp20      m-0  
b20221220T1200Z      V f+360  d+0    vp20      m-0  
b20221220T1200Z      T f+360  d+0    vp20      m-0  
b20221220T1200Z      Z f+360  d+0    vp20      m-0  
b20221220T1200Z      U f+360  d+0    vp10      m-0  
b20221220T1200Z      V f+360  d+0    vp10      m-0  
b20221220T1200Z      T f+360  d+0    vp10      m-0  
b20221220T1200Z      Z f+360  d+0    vp10      m-0  
b20221220T1200Z   Pmsl f+720  d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+720  vsfc      m-0  
b20221220T1200Z      Z f+720  d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+1440 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+1440 vsfc      m-0  
b20221220T1200Z      Z f+1440 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+2160 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+2160 vsfc      m-0  
b20221220T1200Z      Z f+2160 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+2880 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+2880 vsfc      m-0  
b20221220T1200Z      Z f+2880 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+3600 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+3600 vsfc      m-0  
b20221220T1200Z      Z f+3600 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+4320 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+4320 vsfc      m-0  
b20221220T1200Z      Z f+4320 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+5040 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+5040 vsfc      m-0  
b20221220T1200Z      Z f+5040 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+5760 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+5760 vsfc      m-0  
b20221220T1200Z      Z f+5760 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+6480 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+6480 vsfc      m-0  
b20221220T1200Z      Z f+6480 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+7200 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+7200 vsfc      m-0  
b20221220T1200Z      Z f+7200 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+7920 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+7920 vsfc      m-0  
b20221220T1200Z      Z f+7920 d+0    vp500     m-0  
EOF

rm -f z.log
./gribslim -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z      U f+0    d+0    vz10      m-0  
b20221220T1200Z      V f+0    d+0    vz10      m-0  
b20221220T1200Z      T f+0    d+0    vz2       m-0  
b20221220T1200Z     RH f+0    d+0    vz2       m-0  
b20221220T1200Z   Pmsl f+0    d+0    vmsl      m-0  
b20221220T1200Z      U f+0    d+0    vp1000    m-0  
b20221220T1200Z      V f+0    d+0    vp1000    m-0  
b20221220T1200Z      T f+0    d+0    vp1000    m-0  
b20221220T1200Z     RH f+0    d+0    vp1000    m-0  
b20221220T1200Z      Z f+0    d+0    vp1000    m-0  
b20221220T1200Z      U f+0    d+0    vp925     m-0  
b20221220T1200Z      V f+0    d+0    vp925     m-0  
b20221220T1200Z      T f+0    d+0    vp925     m-0  
b20221220T1200Z     RH f+0    d+0    vp925     m-0  
b20221220T1200Z      Z f+0    d+0    vp925     m-0  
b20221220T1200Z      U f+0    d+0    vp850     m-0  
b20221220T1200Z      V f+0    d+0    vp850     m-0  
b20221220T1200Z      T f+0    d+0    vp850     m-0  
b20221220T1200Z     RH f+0    d+0    vp850     m-0  
b20221220T1200Z      Z f+0    d+0    vp850     m-0  
b20221220T1200Z      U f+0    d+0    vp700     m-0  
b20221220T1200Z      V f+0    d+0    vp700     m-0  
b20221220T1200Z      T f+0    d+0    vp700     m-0  
b20221220T1200Z     RH f+0    d+0    vp700     m-0  
b20221220T1200Z      Z f+0    d+0    vp700     m-0  
b20221220T1200Z   VVPa f+0    d+0    vp700     m-0  
b20221220T1200Z      U f+0    d+0    vp600     m-0  
b20221220T1200Z      V f+0    d+0    vp600     m-0  
b20221220T1200Z      T f+0    d+0    vp600     m-0  
b20221220T1200Z     RH f+0    d+0    vp600     m-0  
b20221220T1200Z      Z f+0    d+0    vp600     m-0  
b20221220T1200Z      U f+0    d+0    vp500     m-0  
b20221220T1200Z      V f+0    d+0    vp500     m-0  
b20221220T1200Z      T f+0    d+0    vp500     m-0  
b20221220T1200Z     RH f+0    d+0    vp500     m-0  
b20221220T1200Z      Z f+0    d+0    vp500     m-0  
b20221220T1200Z   rVOR f+0    d+0    vp500     m-0  
b20221220T1200Z      U f+0    d+0    vp400     m-0  
b20221220T1200Z      V f+0    d+0    vp400     m-0  
b20221220T1200Z      T f+0    d+0    vp400     m-0  
b20221220T1200Z     RH f+0    d+0    vp400     m-0  
b20221220T1200Z      Z f+0    d+0    vp400     m-0  
b20221220T1200Z      U f+0    d+0    vp300     m-0  
b20221220T1200Z      V f+0    d+0    vp300     m-0  
b20221220T1200Z      T f+0    d+0    vp300     m-0  
b20221220T1200Z     RH f+0    d+0    vp300     m-0  
b20221220T1200Z      Z f+0    d+0    vp300     m-0  
b20221220T1200Z   VVPa f+0    d+0    vp300     m-0  
b20221220T1200Z      U f+0    d+0    vp250     m-0  
b20221220T1200Z      V f+0    d+0    vp250     m-0  
b20221220T1200Z      T f+0    d+0    vp250     m-0  
b20221220T1200Z      Z f+0    d+0    vp250     m-0  
b20221220T1200Z   rDIV f+0    d+0    vp250     m-0  
b20221220T1200Z      U f+0    d+0    vp200     m-0  
b20221220T1200Z      V f+0    d+0    vp200     m-0  
b20221220T1200Z      T f+0    d+0    vp200     m-0  
b20221220T1200Z      Z f+0    d+0    vp200     m-0  
b20221220T1200Z      U f+0    d+0    vp150     m-0  
b20221220T1200Z      V f+0    d+0    vp150     m-0  
b20221220T1200Z      T f+0    d+0    vp150     m-0  
b20221220T1200Z      Z f+0    d+0    vp150     m-0  
b20221220T1200Z      U f+0    d+0    vp100     m-0  
b20221220T1200Z      V f+0    d+0    vp100     m-0  
b20221220T1200Z      T f+0    d+0    vp100     m-0  
b20221220T1200Z      Z f+0    d+0    vp100     m-0  
b20221220T1200Z      U f+0    d+0    vp70      m-0  
b20221220T1200Z      V f+0    d+0    vp70      m-0  
b20221220T1200Z      T f+0    d+0    vp70      m-0  
b20221220T1200Z      Z f+0    d+0    vp70      m-0  
b20221220T1200Z      U f+0    d+0    vp50      m-0  
b20221220T1200Z      V f+0    d+0    vp50      m-0  
b20221220T1200Z      T f+0    d+0    vp50      m-0  
b20221220T1200Z      Z f+0    d+0    vp50      m-0  
b20221220T1200Z      U f+0    d+0    vp30      m-0  
b20221220T1200Z      V f+0    d+0    vp30      m-0  
b20221220T1200Z      T f+0    d+0    vp30      m-0  
b20221220T1200Z      Z f+0    d+0    vp30      m-0  
b20221220T1200Z      U f+0    d+0    vp20      m-0  
b20221220T1200Z      V f+0    d+0    vp20      m-0  
b20221220T1200Z      T f+0    d+0    vp20      m-0  
b20221220T1200Z      Z f+0    d+0    vp20      m-0  
b20221220T1200Z      U f+0    d+0    vp10      m-0  
b20221220T1200Z      V f+0    d+0    vp10      m-0  
b20221220T1200Z      T f+0    d+0    vp10      m-0  
b20221220T1200Z      Z f+0    d+0    vp10      m-0  
b20221220T1200Z      U f+360  d+0    vz10      m-0  
b20221220T1200Z      V f+360  d+0    vz10      m-0  
b20221220T1200Z      T f+360  d+0    vz2       m-0  
b20221220T1200Z     RH f+360  d+0    vz2       m-0  
b20221220T1200Z   Pmsl f+360  d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+360  vsfc      m-0  
b20221220T1200Z      U f+360  d+0    vp1000    m-0  
b20221220T1200Z      V f+360  d+0    vp1000    m-0  
b20221220T1200Z      T f+360  d+0    vp1000    m-0  
b20221220T1200Z     RH f+360  d+0    vp1000    m-0  
b20221220T1200Z      Z f+360  d+0    vp1000    m-0  
b20221220T1200Z      U f+360  d+0    vp925     m-0  
b20221220T1200Z      V f+360  d+0    vp925     m-0  
b20221220T1200Z      T f+360  d+0    vp925     m-0  
b20221220T1200Z     RH f+360  d+0    vp925     m-0  
b20221220T1200Z      Z f+360  d+0    vp925     m-0  
b20221220T1200Z      U f+360  d+0    vp850     m-0  
b20221220T1200Z      V f+360  d+0    vp850     m-0  
b20221220T1200Z      T f+360  d+0    vp850     m-0  
b20221220T1200Z     RH f+360  d+0    vp850     m-0  
b20221220T1200Z      Z f+360  d+0    vp850     m-0  
b20221220T1200Z      U f+360  d+0    vp700     m-0  
b20221220T1200Z      V f+360  d+0    vp700     m-0  
b20221220T1200Z      T f+360  d+0    vp700     m-0  
b20221220T1200Z     RH f+360  d+0    vp700     m-0  
b20221220T1200Z      Z f+360  d+0    vp700     m-0  
b20221220T1200Z   VVPa f+360  d+0    vp700     m-0  
b20221220T1200Z      U f+360  d+0    vp600     m-0  
b20221220T1200Z      V f+360  d+0    vp600     m-0  
b20221220T1200Z      T f+360  d+0    vp600     m-0  
b20221220T1200Z     RH f+360  d+0    vp600     m-0  
b20221220T1200Z      Z f+360  d+0    vp600     m-0  
b20221220T1200Z      U f+360  d+0    vp500     m-0  
b20221220T1200Z      V f+360  d+0    vp500     m-0  
b20221220T1200Z      T f+360  d+0    vp500     m-0  
b20221220T1200Z     RH f+360  d+0    vp500     m-0  
b20221220T1200Z      Z f+360  d+0    vp500     m-0  
b20221220T1200Z   rVOR f+360  d+0    vp500     m-0  
b20221220T1200Z      U f+360  d+0    vp400     m-0  
b20221220T1200Z      V f+360  d+0    vp400     m-0  
b20221220T1200Z      T f+360  d+0    vp400     m-0  
b20221220T1200Z     RH f+360  d+0    vp400     m-0  
b20221220T1200Z      Z f+360  d+0    vp400     m-0  
b20221220T1200Z      U f+360  d+0    vp300     m-0  
b20221220T1200Z      V f+360  d+0    vp300     m-0  
b20221220T1200Z      T f+360  d+0    vp300     m-0  
b20221220T1200Z     RH f+360  d+0    vp300     m-0  
b20221220T1200Z      Z f+360  d+0    vp300     m-0  
b20221220T1200Z   VVPa f+360  d+0    vp300     m-0  
b20221220T1200Z      U f+360  d+0    vp250     m-0  
b20221220T1200Z      V f+360  d+0    vp250     m-0  
b20221220T1200Z      T f+360  d+0    vp250     m-0  
b20221220T1200Z      Z f+360  d+0    vp250     m-0  
b20221220T1200Z   rDIV f+360  d+0    vp250     m-0  
b20221220T1200Z      U f+360  d+0    vp200     m-0  
b20221220T1200Z      V f+360  d+0    vp200     m-0  
b20221220T1200Z      T f+360  d+0    vp200     m-0  
b20221220T1200Z      Z f+360  d+0    vp200     m-0  
b20221220T1200Z      U f+360  d+0    vp150     m-0  
b20221220T1200Z      V f+360  d+0    vp150     m-0  
b20221220T1200Z      T f+360  d+0    vp150     m-0  
b20221220T1200Z      Z f+360  d+0    vp150     m-0  
b20221220T1200Z      U f+360  d+0    vp100     m-0  
b20221220T1200Z      V f+360  d+0    vp100     m-0  
b20221220T1200Z      T f+360  d+0    vp100     m-0  
b20221220T1200Z      Z f+360  d+0    vp100     m-0  
b20221220T1200Z      U f+360  d+0    vp70      m-0  
b20221220T1200Z      V f+360  d+0    vp70      m-0  
b20221220T1200Z      T f+360  d+0    vp70      m-0  
b20221220T1200Z      Z f+360  d+0    vp70      m-0  
b20221220T1200Z      U f+360  d+0    vp50      m-0  
b20221220T1200Z      V f+360  d+0    vp50      m-0  
b20221220T1200Z      T f+360  d+0    vp50      m-0  
b20221220T1200Z      Z f+360  d+0    vp50      m-0  
b20221220T1200Z      U f+360  d+0    vp30      m-0  
b20221220T1200Z      V f+360  d+0    vp30      m-0  
b20221220T1200Z      T f+360  d+0    vp30      m-0  
b20221220T1200Z      Z f+360  d+0    vp30      m-0  
b20221220T1200Z      U f+360  d+0    vp20      m-0  
b20221220T1200Z      V f+360  d+0    vp20      m-0  
b20221220T1200Z      T f+360  d+0    vp20      m-0  
b20221220T1200Z      Z f+360  d+0    vp20      m-0  
b20221220T1200Z      U f+360  d+0    vp10      m-0  
b20221220T1200Z      V f+360  d+0    vp10      m-0  
b20221220T1200Z      T f+360  d+0    vp10      m-0  
b20221220T1200Z      Z f+360  d+0    vp10      m-0  
b20221220T1200Z   Pmsl f+720  d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+720  vsfc      m-0  
b20221220T1200Z      Z f+720  d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+1440 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+1440 vsfc      m-0  
b20221220T1200Z      Z f+1440 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+2160 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+2160 vsfc      m-0  
b20221220T1200Z      Z f+2160 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+2880 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+2880 vsfc      m-0  
b20221220T1200Z      Z f+2880 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+3600 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+3600 vsfc      m-0  
b20221220T1200Z      Z f+3600 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+4320 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+4320 vsfc      m-0  
b20221220T1200Z      Z f+4320 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+5040 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+5040 vsfc      m-0  
b20221220T1200Z      Z f+5040 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+5760 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+5760 vsfc      m-0  
b20221220T1200Z      Z f+5760 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+6480 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+6480 vsfc      m-0  
b20221220T1200Z      Z f+6480 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+7200 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+7200 vsfc      m-0  
b20221220T1200Z      Z f+7200 d+0    vp500     m-0  
b20221220T1200Z   Pmsl f+7920 d+0    vmsl      m-0  
b20221220T1200Z   RAIN f+0    d+7920 vsfc      m-0  
b20221220T1200Z      Z f+7920 d+0    vp500     m-0  
EOF

rm -f z.log
