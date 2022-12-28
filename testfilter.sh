#!/bin/bash
set -Ceuo pipefail

test -f sample.gsm || bash prepsample.sh
test ! -f z.log || rm -f z.log

./gribslim -fp0= -o/dev/null sample.gsm > z.log

diff -- z.log - <<EOF
b20221220T1200Z      T f+0    d+0    v101302.5
b20221220T1200Z      T f+0    d+0    v100000.0
b20221220T1200Z      T f+0    d+0    v92500.0 
b20221220T1200Z      T f+0    d+0    v85000.0 
b20221220T1200Z      T f+0    d+0    v70000.0 
b20221220T1200Z      T f+0    d+0    v60000.0 
b20221220T1200Z      T f+0    d+0    v50000.0 
b20221220T1200Z      T f+0    d+0    v40000.0 
b20221220T1200Z      T f+0    d+0    v30000.0 
b20221220T1200Z      T f+0    d+0    v25000.0 
b20221220T1200Z      T f+0    d+0    v20000.0 
b20221220T1200Z      T f+0    d+0    v15000.0 
b20221220T1200Z      T f+0    d+0    v10000.0 
b20221220T1200Z      T f+0    d+0    v7000.0  
b20221220T1200Z      T f+0    d+0    v5000.0  
b20221220T1200Z      T f+0    d+0    v3000.0  
b20221220T1200Z      T f+0    d+0    v2000.0  
b20221220T1200Z      T f+0    d+0    v1000.0  
b20221220T1200Z      T f+360  d+0    v101302.5
b20221220T1200Z      T f+360  d+0    v100000.0
b20221220T1200Z      T f+360  d+0    v92500.0 
b20221220T1200Z      T f+360  d+0    v85000.0 
b20221220T1200Z      T f+360  d+0    v70000.0 
b20221220T1200Z      T f+360  d+0    v60000.0 
b20221220T1200Z      T f+360  d+0    v50000.0 
b20221220T1200Z      T f+360  d+0    v40000.0 
b20221220T1200Z      T f+360  d+0    v30000.0 
b20221220T1200Z      T f+360  d+0    v25000.0 
b20221220T1200Z      T f+360  d+0    v20000.0 
b20221220T1200Z      T f+360  d+0    v15000.0 
b20221220T1200Z      T f+360  d+0    v10000.0 
b20221220T1200Z      T f+360  d+0    v7000.0  
b20221220T1200Z      T f+360  d+0    v5000.0  
b20221220T1200Z      T f+360  d+0    v3000.0  
b20221220T1200Z      T f+360  d+0    v2000.0  
b20221220T1200Z      T f+360  d+0    v1000.0  
b20221220T1200Z      T f+720  d+0    v101302.5
b20221220T1200Z      T f+720  d+0    v100000.0
b20221220T1200Z      T f+720  d+0    v92500.0 
b20221220T1200Z      T f+720  d+0    v85000.0 
b20221220T1200Z      T f+720  d+0    v70000.0 
b20221220T1200Z      T f+720  d+0    v60000.0 
b20221220T1200Z      T f+720  d+0    v50000.0 
b20221220T1200Z      T f+720  d+0    v40000.0 
b20221220T1200Z      T f+720  d+0    v30000.0 
b20221220T1200Z      T f+720  d+0    v25000.0 
b20221220T1200Z      T f+720  d+0    v20000.0 
b20221220T1200Z      T f+720  d+0    v15000.0 
b20221220T1200Z      T f+720  d+0    v10000.0 
b20221220T1200Z      T f+720  d+0    v7000.0  
b20221220T1200Z      T f+720  d+0    v5000.0  
b20221220T1200Z      T f+720  d+0    v3000.0  
b20221220T1200Z      T f+720  d+0    v2000.0  
b20221220T1200Z      T f+720  d+0    v1000.0  
b20221220T1200Z      T f+1080 d+0    v101302.5
b20221220T1200Z      T f+1080 d+0    v100000.0
b20221220T1200Z      T f+1080 d+0    v92500.0 
b20221220T1200Z      T f+1080 d+0    v85000.0 
b20221220T1200Z      T f+1080 d+0    v70000.0 
b20221220T1200Z      T f+1080 d+0    v60000.0 
b20221220T1200Z      T f+1080 d+0    v50000.0 
b20221220T1200Z      T f+1080 d+0    v40000.0 
b20221220T1200Z      T f+1080 d+0    v30000.0 
b20221220T1200Z      T f+1080 d+0    v25000.0 
b20221220T1200Z      T f+1080 d+0    v20000.0 
b20221220T1200Z      T f+1080 d+0    v15000.0 
b20221220T1200Z      T f+1080 d+0    v10000.0 
b20221220T1200Z      T f+1080 d+0    v7000.0  
b20221220T1200Z      T f+1080 d+0    v5000.0  
b20221220T1200Z      T f+1080 d+0    v3000.0  
b20221220T1200Z      T f+1080 d+0    v2000.0  
b20221220T1200Z      T f+1080 d+0    v1000.0  
b20221220T1200Z      T f+1440 d+0    v101302.5
b20221220T1200Z      T f+1440 d+0    v100000.0
b20221220T1200Z      T f+1440 d+0    v92500.0 
b20221220T1200Z      T f+1440 d+0    v85000.0 
b20221220T1200Z      T f+1440 d+0    v70000.0 
b20221220T1200Z      T f+1440 d+0    v60000.0 
b20221220T1200Z      T f+1440 d+0    v50000.0 
b20221220T1200Z      T f+1440 d+0    v40000.0 
b20221220T1200Z      T f+1440 d+0    v30000.0 
b20221220T1200Z      T f+1440 d+0    v25000.0 
b20221220T1200Z      T f+1440 d+0    v20000.0 
b20221220T1200Z      T f+1440 d+0    v15000.0 
b20221220T1200Z      T f+1440 d+0    v10000.0 
b20221220T1200Z      T f+1440 d+0    v7000.0  
b20221220T1200Z      T f+1440 d+0    v5000.0  
b20221220T1200Z      T f+1440 d+0    v3000.0  
b20221220T1200Z      T f+1440 d+0    v2000.0  
b20221220T1200Z      T f+1440 d+0    v1000.0  
b20221220T1200Z      T f+1800 d+0    v101302.5
b20221220T1200Z      T f+1800 d+0    v100000.0
b20221220T1200Z      T f+1800 d+0    v92500.0 
b20221220T1200Z      T f+1800 d+0    v85000.0 
b20221220T1200Z      T f+1800 d+0    v70000.0 
b20221220T1200Z      T f+1800 d+0    v60000.0 
b20221220T1200Z      T f+1800 d+0    v50000.0 
b20221220T1200Z      T f+1800 d+0    v40000.0 
b20221220T1200Z      T f+1800 d+0    v30000.0 
b20221220T1200Z      T f+1800 d+0    v25000.0 
b20221220T1200Z      T f+1800 d+0    v20000.0 
b20221220T1200Z      T f+1800 d+0    v15000.0 
b20221220T1200Z      T f+1800 d+0    v10000.0 
b20221220T1200Z      T f+1800 d+0    v7000.0  
b20221220T1200Z      T f+1800 d+0    v5000.0  
b20221220T1200Z      T f+1800 d+0    v3000.0  
b20221220T1200Z      T f+1800 d+0    v2000.0  
b20221220T1200Z      T f+1800 d+0    v1000.0  
b20221220T1200Z      T f+2160 d+0    v101302.5
b20221220T1200Z      T f+2160 d+0    v100000.0
b20221220T1200Z      T f+2160 d+0    v92500.0 
b20221220T1200Z      T f+2160 d+0    v85000.0 
b20221220T1200Z      T f+2160 d+0    v70000.0 
b20221220T1200Z      T f+2160 d+0    v60000.0 
b20221220T1200Z      T f+2160 d+0    v50000.0 
b20221220T1200Z      T f+2160 d+0    v40000.0 
b20221220T1200Z      T f+2160 d+0    v30000.0 
b20221220T1200Z      T f+2160 d+0    v25000.0 
b20221220T1200Z      T f+2160 d+0    v20000.0 
b20221220T1200Z      T f+2160 d+0    v15000.0 
b20221220T1200Z      T f+2160 d+0    v10000.0 
b20221220T1200Z      T f+2160 d+0    v7000.0  
b20221220T1200Z      T f+2160 d+0    v5000.0  
b20221220T1200Z      T f+2160 d+0    v3000.0  
b20221220T1200Z      T f+2160 d+0    v2000.0  
b20221220T1200Z      T f+2160 d+0    v1000.0  
b20221220T1200Z      T f+2520 d+0    v101302.5
b20221220T1200Z      T f+2520 d+0    v100000.0
b20221220T1200Z      T f+2520 d+0    v92500.0 
b20221220T1200Z      T f+2520 d+0    v85000.0 
b20221220T1200Z      T f+2520 d+0    v70000.0 
b20221220T1200Z      T f+2520 d+0    v60000.0 
b20221220T1200Z      T f+2520 d+0    v50000.0 
b20221220T1200Z      T f+2520 d+0    v40000.0 
b20221220T1200Z      T f+2520 d+0    v30000.0 
b20221220T1200Z      T f+2520 d+0    v25000.0 
b20221220T1200Z      T f+2520 d+0    v20000.0 
b20221220T1200Z      T f+2520 d+0    v15000.0 
b20221220T1200Z      T f+2520 d+0    v10000.0 
b20221220T1200Z      T f+2520 d+0    v7000.0  
b20221220T1200Z      T f+2520 d+0    v5000.0  
b20221220T1200Z      T f+2520 d+0    v3000.0  
b20221220T1200Z      T f+2520 d+0    v2000.0  
b20221220T1200Z      T f+2520 d+0    v1000.0  
b20221220T1200Z      T f+2880 d+0    v101302.5
b20221220T1200Z      T f+2880 d+0    v100000.0
b20221220T1200Z      T f+2880 d+0    v92500.0 
b20221220T1200Z      T f+2880 d+0    v85000.0 
b20221220T1200Z      T f+2880 d+0    v70000.0 
b20221220T1200Z      T f+2880 d+0    v60000.0 
b20221220T1200Z      T f+2880 d+0    v50000.0 
b20221220T1200Z      T f+2880 d+0    v40000.0 
b20221220T1200Z      T f+2880 d+0    v30000.0 
b20221220T1200Z      T f+2880 d+0    v25000.0 
b20221220T1200Z      T f+2880 d+0    v20000.0 
b20221220T1200Z      T f+2880 d+0    v15000.0 
b20221220T1200Z      T f+2880 d+0    v10000.0 
b20221220T1200Z      T f+2880 d+0    v7000.0  
b20221220T1200Z      T f+2880 d+0    v5000.0  
b20221220T1200Z      T f+2880 d+0    v3000.0  
b20221220T1200Z      T f+2880 d+0    v2000.0  
b20221220T1200Z      T f+2880 d+0    v1000.0  
b20221220T1200Z      T f+3240 d+0    v101302.5
b20221220T1200Z      T f+3240 d+0    v100000.0
b20221220T1200Z      T f+3240 d+0    v92500.0 
b20221220T1200Z      T f+3240 d+0    v85000.0 
b20221220T1200Z      T f+3240 d+0    v70000.0 
b20221220T1200Z      T f+3240 d+0    v60000.0 
b20221220T1200Z      T f+3240 d+0    v50000.0 
b20221220T1200Z      T f+3240 d+0    v40000.0 
b20221220T1200Z      T f+3240 d+0    v30000.0 
b20221220T1200Z      T f+3240 d+0    v25000.0 
b20221220T1200Z      T f+3240 d+0    v20000.0 
b20221220T1200Z      T f+3240 d+0    v15000.0 
b20221220T1200Z      T f+3240 d+0    v10000.0 
b20221220T1200Z      T f+3240 d+0    v7000.0  
b20221220T1200Z      T f+3240 d+0    v5000.0  
b20221220T1200Z      T f+3240 d+0    v3000.0  
b20221220T1200Z      T f+3240 d+0    v2000.0  
b20221220T1200Z      T f+3240 d+0    v1000.0  
b20221220T1200Z      T f+3600 d+0    v101302.5
b20221220T1200Z      T f+3600 d+0    v100000.0
b20221220T1200Z      T f+3600 d+0    v92500.0 
b20221220T1200Z      T f+3600 d+0    v85000.0 
b20221220T1200Z      T f+3600 d+0    v70000.0 
b20221220T1200Z      T f+3600 d+0    v60000.0 
b20221220T1200Z      T f+3600 d+0    v50000.0 
b20221220T1200Z      T f+3600 d+0    v40000.0 
b20221220T1200Z      T f+3600 d+0    v30000.0 
b20221220T1200Z      T f+3600 d+0    v25000.0 
b20221220T1200Z      T f+3600 d+0    v20000.0 
b20221220T1200Z      T f+3600 d+0    v15000.0 
b20221220T1200Z      T f+3600 d+0    v10000.0 
b20221220T1200Z      T f+3600 d+0    v7000.0  
b20221220T1200Z      T f+3600 d+0    v5000.0  
b20221220T1200Z      T f+3600 d+0    v3000.0  
b20221220T1200Z      T f+3600 d+0    v2000.0  
b20221220T1200Z      T f+3600 d+0    v1000.0  
b20221220T1200Z      T f+3960 d+0    v101302.5
b20221220T1200Z      T f+3960 d+0    v100000.0
b20221220T1200Z      T f+3960 d+0    v92500.0 
b20221220T1200Z      T f+3960 d+0    v85000.0 
b20221220T1200Z      T f+3960 d+0    v70000.0 
b20221220T1200Z      T f+3960 d+0    v60000.0 
b20221220T1200Z      T f+3960 d+0    v50000.0 
b20221220T1200Z      T f+3960 d+0    v40000.0 
b20221220T1200Z      T f+3960 d+0    v30000.0 
b20221220T1200Z      T f+3960 d+0    v25000.0 
b20221220T1200Z      T f+3960 d+0    v20000.0 
b20221220T1200Z      T f+3960 d+0    v15000.0 
b20221220T1200Z      T f+3960 d+0    v10000.0 
b20221220T1200Z      T f+3960 d+0    v7000.0  
b20221220T1200Z      T f+3960 d+0    v5000.0  
b20221220T1200Z      T f+3960 d+0    v3000.0  
b20221220T1200Z      T f+3960 d+0    v2000.0  
b20221220T1200Z      T f+3960 d+0    v1000.0  
b20221220T1200Z      T f+4320 d+0    v101302.5
b20221220T1200Z      T f+4320 d+0    v100000.0
b20221220T1200Z      T f+4320 d+0    v92500.0 
b20221220T1200Z      T f+4320 d+0    v85000.0 
b20221220T1200Z      T f+4320 d+0    v70000.0 
b20221220T1200Z      T f+4320 d+0    v60000.0 
b20221220T1200Z      T f+4320 d+0    v50000.0 
b20221220T1200Z      T f+4320 d+0    v40000.0 
b20221220T1200Z      T f+4320 d+0    v30000.0 
b20221220T1200Z      T f+4320 d+0    v25000.0 
b20221220T1200Z      T f+4320 d+0    v20000.0 
b20221220T1200Z      T f+4320 d+0    v15000.0 
b20221220T1200Z      T f+4320 d+0    v10000.0 
b20221220T1200Z      T f+4320 d+0    v7000.0  
b20221220T1200Z      T f+4320 d+0    v5000.0  
b20221220T1200Z      T f+4320 d+0    v3000.0  
b20221220T1200Z      T f+4320 d+0    v2000.0  
b20221220T1200Z      T f+4320 d+0    v1000.0  
b20221220T1200Z      T f+4680 d+0    v101302.5
b20221220T1200Z      T f+4680 d+0    v100000.0
b20221220T1200Z      T f+4680 d+0    v92500.0 
b20221220T1200Z      T f+4680 d+0    v85000.0 
b20221220T1200Z      T f+4680 d+0    v70000.0 
b20221220T1200Z      T f+4680 d+0    v60000.0 
b20221220T1200Z      T f+4680 d+0    v50000.0 
b20221220T1200Z      T f+4680 d+0    v40000.0 
b20221220T1200Z      T f+4680 d+0    v30000.0 
b20221220T1200Z      T f+4680 d+0    v25000.0 
b20221220T1200Z      T f+4680 d+0    v20000.0 
b20221220T1200Z      T f+4680 d+0    v15000.0 
b20221220T1200Z      T f+4680 d+0    v10000.0 
b20221220T1200Z      T f+4680 d+0    v7000.0  
b20221220T1200Z      T f+4680 d+0    v5000.0  
b20221220T1200Z      T f+4680 d+0    v3000.0  
b20221220T1200Z      T f+4680 d+0    v2000.0  
b20221220T1200Z      T f+4680 d+0    v1000.0  
b20221220T1200Z      T f+5040 d+0    v101302.5
b20221220T1200Z      T f+5040 d+0    v100000.0
b20221220T1200Z      T f+5040 d+0    v92500.0 
b20221220T1200Z      T f+5040 d+0    v85000.0 
b20221220T1200Z      T f+5040 d+0    v70000.0 
b20221220T1200Z      T f+5040 d+0    v60000.0 
b20221220T1200Z      T f+5040 d+0    v50000.0 
b20221220T1200Z      T f+5040 d+0    v40000.0 
b20221220T1200Z      T f+5040 d+0    v30000.0 
b20221220T1200Z      T f+5040 d+0    v25000.0 
b20221220T1200Z      T f+5040 d+0    v20000.0 
b20221220T1200Z      T f+5040 d+0    v15000.0 
b20221220T1200Z      T f+5040 d+0    v10000.0 
b20221220T1200Z      T f+5040 d+0    v7000.0  
b20221220T1200Z      T f+5040 d+0    v5000.0  
b20221220T1200Z      T f+5040 d+0    v3000.0  
b20221220T1200Z      T f+5040 d+0    v2000.0  
b20221220T1200Z      T f+5040 d+0    v1000.0  
b20221220T1200Z      T f+5400 d+0    v101302.5
b20221220T1200Z      T f+5400 d+0    v100000.0
b20221220T1200Z      T f+5400 d+0    v92500.0 
b20221220T1200Z      T f+5400 d+0    v85000.0 
b20221220T1200Z      T f+5400 d+0    v70000.0 
b20221220T1200Z      T f+5400 d+0    v60000.0 
b20221220T1200Z      T f+5400 d+0    v50000.0 
b20221220T1200Z      T f+5400 d+0    v40000.0 
b20221220T1200Z      T f+5400 d+0    v30000.0 
b20221220T1200Z      T f+5400 d+0    v25000.0 
b20221220T1200Z      T f+5400 d+0    v20000.0 
b20221220T1200Z      T f+5400 d+0    v15000.0 
b20221220T1200Z      T f+5400 d+0    v10000.0 
b20221220T1200Z      T f+5400 d+0    v7000.0  
b20221220T1200Z      T f+5400 d+0    v5000.0  
b20221220T1200Z      T f+5400 d+0    v3000.0  
b20221220T1200Z      T f+5400 d+0    v2000.0  
b20221220T1200Z      T f+5400 d+0    v1000.0  
b20221220T1200Z      T f+5760 d+0    v101302.5
b20221220T1200Z      T f+5760 d+0    v100000.0
b20221220T1200Z      T f+5760 d+0    v92500.0 
b20221220T1200Z      T f+5760 d+0    v85000.0 
b20221220T1200Z      T f+5760 d+0    v70000.0 
b20221220T1200Z      T f+5760 d+0    v60000.0 
b20221220T1200Z      T f+5760 d+0    v50000.0 
b20221220T1200Z      T f+5760 d+0    v40000.0 
b20221220T1200Z      T f+5760 d+0    v30000.0 
b20221220T1200Z      T f+5760 d+0    v25000.0 
b20221220T1200Z      T f+5760 d+0    v20000.0 
b20221220T1200Z      T f+5760 d+0    v15000.0 
b20221220T1200Z      T f+5760 d+0    v10000.0 
b20221220T1200Z      T f+5760 d+0    v7000.0  
b20221220T1200Z      T f+5760 d+0    v5000.0  
b20221220T1200Z      T f+5760 d+0    v3000.0  
b20221220T1200Z      T f+5760 d+0    v2000.0  
b20221220T1200Z      T f+5760 d+0    v1000.0  
b20221220T1200Z      T f+6120 d+0    v101302.5
b20221220T1200Z      T f+6120 d+0    v100000.0
b20221220T1200Z      T f+6120 d+0    v92500.0 
b20221220T1200Z      T f+6120 d+0    v85000.0 
b20221220T1200Z      T f+6120 d+0    v70000.0 
b20221220T1200Z      T f+6120 d+0    v60000.0 
b20221220T1200Z      T f+6120 d+0    v50000.0 
b20221220T1200Z      T f+6120 d+0    v40000.0 
b20221220T1200Z      T f+6120 d+0    v30000.0 
b20221220T1200Z      T f+6120 d+0    v25000.0 
b20221220T1200Z      T f+6120 d+0    v20000.0 
b20221220T1200Z      T f+6120 d+0    v15000.0 
b20221220T1200Z      T f+6120 d+0    v10000.0 
b20221220T1200Z      T f+6120 d+0    v7000.0  
b20221220T1200Z      T f+6120 d+0    v5000.0  
b20221220T1200Z      T f+6120 d+0    v3000.0  
b20221220T1200Z      T f+6120 d+0    v2000.0  
b20221220T1200Z      T f+6120 d+0    v1000.0  
b20221220T1200Z      T f+6480 d+0    v101302.5
b20221220T1200Z      T f+6480 d+0    v100000.0
b20221220T1200Z      T f+6480 d+0    v92500.0 
b20221220T1200Z      T f+6480 d+0    v85000.0 
b20221220T1200Z      T f+6480 d+0    v70000.0 
b20221220T1200Z      T f+6480 d+0    v60000.0 
b20221220T1200Z      T f+6480 d+0    v50000.0 
b20221220T1200Z      T f+6480 d+0    v40000.0 
b20221220T1200Z      T f+6480 d+0    v30000.0 
b20221220T1200Z      T f+6480 d+0    v25000.0 
b20221220T1200Z      T f+6480 d+0    v20000.0 
b20221220T1200Z      T f+6480 d+0    v15000.0 
b20221220T1200Z      T f+6480 d+0    v10000.0 
b20221220T1200Z      T f+6480 d+0    v7000.0  
b20221220T1200Z      T f+6480 d+0    v5000.0  
b20221220T1200Z      T f+6480 d+0    v3000.0  
b20221220T1200Z      T f+6480 d+0    v2000.0  
b20221220T1200Z      T f+6480 d+0    v1000.0  
b20221220T1200Z      T f+6840 d+0    v101302.5
b20221220T1200Z      T f+6840 d+0    v100000.0
b20221220T1200Z      T f+6840 d+0    v92500.0 
b20221220T1200Z      T f+6840 d+0    v85000.0 
b20221220T1200Z      T f+6840 d+0    v70000.0 
b20221220T1200Z      T f+6840 d+0    v60000.0 
b20221220T1200Z      T f+6840 d+0    v50000.0 
b20221220T1200Z      T f+6840 d+0    v40000.0 
b20221220T1200Z      T f+6840 d+0    v30000.0 
b20221220T1200Z      T f+6840 d+0    v25000.0 
b20221220T1200Z      T f+6840 d+0    v20000.0 
b20221220T1200Z      T f+6840 d+0    v15000.0 
b20221220T1200Z      T f+6840 d+0    v10000.0 
b20221220T1200Z      T f+6840 d+0    v7000.0  
b20221220T1200Z      T f+6840 d+0    v5000.0  
b20221220T1200Z      T f+6840 d+0    v3000.0  
b20221220T1200Z      T f+6840 d+0    v2000.0  
b20221220T1200Z      T f+6840 d+0    v1000.0  
b20221220T1200Z      T f+7200 d+0    v101302.5
b20221220T1200Z      T f+7200 d+0    v100000.0
b20221220T1200Z      T f+7200 d+0    v92500.0 
b20221220T1200Z      T f+7200 d+0    v85000.0 
b20221220T1200Z      T f+7200 d+0    v70000.0 
b20221220T1200Z      T f+7200 d+0    v60000.0 
b20221220T1200Z      T f+7200 d+0    v50000.0 
b20221220T1200Z      T f+7200 d+0    v40000.0 
b20221220T1200Z      T f+7200 d+0    v30000.0 
b20221220T1200Z      T f+7200 d+0    v25000.0 
b20221220T1200Z      T f+7200 d+0    v20000.0 
b20221220T1200Z      T f+7200 d+0    v15000.0 
b20221220T1200Z      T f+7200 d+0    v10000.0 
b20221220T1200Z      T f+7200 d+0    v7000.0  
b20221220T1200Z      T f+7200 d+0    v5000.0  
b20221220T1200Z      T f+7200 d+0    v3000.0  
b20221220T1200Z      T f+7200 d+0    v2000.0  
b20221220T1200Z      T f+7200 d+0    v1000.0  
b20221220T1200Z      T f+7560 d+0    v101302.5
b20221220T1200Z      T f+7560 d+0    v100000.0
b20221220T1200Z      T f+7560 d+0    v92500.0 
b20221220T1200Z      T f+7560 d+0    v85000.0 
b20221220T1200Z      T f+7560 d+0    v70000.0 
b20221220T1200Z      T f+7560 d+0    v60000.0 
b20221220T1200Z      T f+7560 d+0    v50000.0 
b20221220T1200Z      T f+7560 d+0    v40000.0 
b20221220T1200Z      T f+7560 d+0    v30000.0 
b20221220T1200Z      T f+7560 d+0    v25000.0 
b20221220T1200Z      T f+7560 d+0    v20000.0 
b20221220T1200Z      T f+7560 d+0    v15000.0 
b20221220T1200Z      T f+7560 d+0    v10000.0 
b20221220T1200Z      T f+7560 d+0    v7000.0  
b20221220T1200Z      T f+7560 d+0    v5000.0  
b20221220T1200Z      T f+7560 d+0    v3000.0  
b20221220T1200Z      T f+7560 d+0    v2000.0  
b20221220T1200Z      T f+7560 d+0    v1000.0  
b20221220T1200Z      T f+7920 d+0    v101302.5
b20221220T1200Z      T f+7920 d+0    v100000.0
b20221220T1200Z      T f+7920 d+0    v92500.0 
b20221220T1200Z      T f+7920 d+0    v85000.0 
b20221220T1200Z      T f+7920 d+0    v70000.0 
b20221220T1200Z      T f+7920 d+0    v60000.0 
b20221220T1200Z      T f+7920 d+0    v50000.0 
b20221220T1200Z      T f+7920 d+0    v40000.0 
b20221220T1200Z      T f+7920 d+0    v30000.0 
b20221220T1200Z      T f+7920 d+0    v25000.0 
b20221220T1200Z      T f+7920 d+0    v20000.0 
b20221220T1200Z      T f+7920 d+0    v15000.0 
b20221220T1200Z      T f+7920 d+0    v10000.0 
b20221220T1200Z      T f+7920 d+0    v7000.0  
b20221220T1200Z      T f+7920 d+0    v5000.0  
b20221220T1200Z      T f+7920 d+0    v3000.0  
b20221220T1200Z      T f+7920 d+0    v2000.0  
b20221220T1200Z      T f+7920 d+0    v1000.0  
EOF

rm -f z.log
./gribslim -ff0= -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z      U f+0    d+0    v101214.5
b20221220T1200Z      V f+0    d+0    v101214.5
b20221220T1200Z      T f+0    d+0    v101302.5
b20221220T1200Z     RH f+0    d+0    v101302.5
b20221220T1200Z   Pmsl f+0    d+0    v101324.0
b20221220T1200Z      U f+0    d+0    v100000.0
b20221220T1200Z      V f+0    d+0    v100000.0
b20221220T1200Z      T f+0    d+0    v100000.0
b20221220T1200Z     RH f+0    d+0    v100000.0
b20221220T1200Z      Z f+0    d+0    v100000.0
b20221220T1200Z   VVPa f+0    d+0    v100000.0
b20221220T1200Z      U f+0    d+0    v92500.0 
b20221220T1200Z      V f+0    d+0    v92500.0 
b20221220T1200Z      T f+0    d+0    v92500.0 
b20221220T1200Z     RH f+0    d+0    v92500.0 
b20221220T1200Z      Z f+0    d+0    v92500.0 
b20221220T1200Z   VVPa f+0    d+0    v92500.0 
b20221220T1200Z   rVOR f+0    d+0    v92500.0 
b20221220T1200Z   rDIV f+0    d+0    v92500.0 
b20221220T1200Z      U f+0    d+0    v85000.0 
b20221220T1200Z      V f+0    d+0    v85000.0 
b20221220T1200Z      T f+0    d+0    v85000.0 
b20221220T1200Z     RH f+0    d+0    v85000.0 
b20221220T1200Z      Z f+0    d+0    v85000.0 
b20221220T1200Z   VVPa f+0    d+0    v85000.0 
b20221220T1200Z    PSI f+0    d+0    v85000.0 
b20221220T1200Z    CHI f+0    d+0    v85000.0 
b20221220T1200Z      U f+0    d+0    v70000.0 
b20221220T1200Z      V f+0    d+0    v70000.0 
b20221220T1200Z      T f+0    d+0    v70000.0 
b20221220T1200Z     RH f+0    d+0    v70000.0 
b20221220T1200Z      Z f+0    d+0    v70000.0 
b20221220T1200Z   VVPa f+0    d+0    v70000.0 
b20221220T1200Z   rVOR f+0    d+0    v70000.0 
b20221220T1200Z   rDIV f+0    d+0    v70000.0 
b20221220T1200Z      U f+0    d+0    v60000.0 
b20221220T1200Z      V f+0    d+0    v60000.0 
b20221220T1200Z      T f+0    d+0    v60000.0 
b20221220T1200Z     RH f+0    d+0    v60000.0 
b20221220T1200Z      Z f+0    d+0    v60000.0 
b20221220T1200Z   VVPa f+0    d+0    v60000.0 
b20221220T1200Z      U f+0    d+0    v50000.0 
b20221220T1200Z      V f+0    d+0    v50000.0 
b20221220T1200Z      T f+0    d+0    v50000.0 
b20221220T1200Z     RH f+0    d+0    v50000.0 
b20221220T1200Z      Z f+0    d+0    v50000.0 
b20221220T1200Z   VVPa f+0    d+0    v50000.0 
b20221220T1200Z   rVOR f+0    d+0    v50000.0 
b20221220T1200Z      U f+0    d+0    v40000.0 
b20221220T1200Z      V f+0    d+0    v40000.0 
b20221220T1200Z      T f+0    d+0    v40000.0 
b20221220T1200Z     RH f+0    d+0    v40000.0 
b20221220T1200Z      Z f+0    d+0    v40000.0 
b20221220T1200Z   VVPa f+0    d+0    v40000.0 
b20221220T1200Z      U f+0    d+0    v30000.0 
b20221220T1200Z      V f+0    d+0    v30000.0 
b20221220T1200Z      T f+0    d+0    v30000.0 
b20221220T1200Z     RH f+0    d+0    v30000.0 
b20221220T1200Z      Z f+0    d+0    v30000.0 
b20221220T1200Z   VVPa f+0    d+0    v30000.0 
b20221220T1200Z      U f+0    d+0    v25000.0 
b20221220T1200Z      V f+0    d+0    v25000.0 
b20221220T1200Z      T f+0    d+0    v25000.0 
b20221220T1200Z      Z f+0    d+0    v25000.0 
b20221220T1200Z   rVOR f+0    d+0    v25000.0 
b20221220T1200Z   rDIV f+0    d+0    v25000.0 
b20221220T1200Z      U f+0    d+0    v20000.0 
b20221220T1200Z      V f+0    d+0    v20000.0 
b20221220T1200Z      T f+0    d+0    v20000.0 
b20221220T1200Z      Z f+0    d+0    v20000.0 
b20221220T1200Z    PSI f+0    d+0    v20000.0 
b20221220T1200Z    CHI f+0    d+0    v20000.0 
b20221220T1200Z      U f+0    d+0    v15000.0 
b20221220T1200Z      V f+0    d+0    v15000.0 
b20221220T1200Z      T f+0    d+0    v15000.0 
b20221220T1200Z      Z f+0    d+0    v15000.0 
b20221220T1200Z      U f+0    d+0    v10000.0 
b20221220T1200Z      V f+0    d+0    v10000.0 
b20221220T1200Z      T f+0    d+0    v10000.0 
b20221220T1200Z      Z f+0    d+0    v10000.0 
b20221220T1200Z      U f+0    d+0    v7000.0  
b20221220T1200Z      V f+0    d+0    v7000.0  
b20221220T1200Z      T f+0    d+0    v7000.0  
b20221220T1200Z      Z f+0    d+0    v7000.0  
b20221220T1200Z      U f+0    d+0    v5000.0  
b20221220T1200Z      V f+0    d+0    v5000.0  
b20221220T1200Z      T f+0    d+0    v5000.0  
b20221220T1200Z      Z f+0    d+0    v5000.0  
b20221220T1200Z      U f+0    d+0    v3000.0  
b20221220T1200Z      V f+0    d+0    v3000.0  
b20221220T1200Z      T f+0    d+0    v3000.0  
b20221220T1200Z      Z f+0    d+0    v3000.0  
b20221220T1200Z      U f+0    d+0    v2000.0  
b20221220T1200Z      V f+0    d+0    v2000.0  
b20221220T1200Z      T f+0    d+0    v2000.0  
b20221220T1200Z      Z f+0    d+0    v2000.0  
b20221220T1200Z      U f+0    d+0    v1000.0  
b20221220T1200Z      V f+0    d+0    v1000.0  
b20221220T1200Z      T f+0    d+0    v1000.0  
b20221220T1200Z      Z f+0    d+0    v1000.0  
b20221220T1200Z   RAIN f+0    d+360  v101325.0
b20221220T1200Z   RAIN f+0    d+720  v101325.0
b20221220T1200Z   RAIN f+0    d+1080 v101325.0
b20221220T1200Z   RAIN f+0    d+1440 v101325.0
b20221220T1200Z   RAIN f+0    d+1800 v101325.0
b20221220T1200Z   RAIN f+0    d+2160 v101325.0
b20221220T1200Z   RAIN f+0    d+2520 v101325.0
b20221220T1200Z   RAIN f+0    d+2880 v101325.0
b20221220T1200Z   RAIN f+0    d+3240 v101325.0
b20221220T1200Z   RAIN f+0    d+3600 v101325.0
b20221220T1200Z   RAIN f+0    d+3960 v101325.0
b20221220T1200Z   RAIN f+0    d+4320 v101325.0
b20221220T1200Z   RAIN f+0    d+4680 v101325.0
b20221220T1200Z   RAIN f+0    d+5040 v101325.0
b20221220T1200Z   RAIN f+0    d+5400 v101325.0
b20221220T1200Z   RAIN f+0    d+5760 v101325.0
b20221220T1200Z   RAIN f+0    d+6120 v101325.0
b20221220T1200Z   RAIN f+0    d+6480 v101325.0
b20221220T1200Z   RAIN f+0    d+6840 v101325.0
b20221220T1200Z   RAIN f+0    d+7200 v101325.0
b20221220T1200Z   RAIN f+0    d+7560 v101325.0
b20221220T1200Z   RAIN f+0    d+7920 v101325.0
EOF

rm -f z.log
./gribslim -ff0=d0=p0=KK -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z      T f+0    d+0    v101302.5
b20221220T1200Z      T f+0    d+0    v100000.0
b20221220T1200Z      T f+0    d+0    v92500.0 
b20221220T1200Z      T f+0    d+0    v85000.0 
b20221220T1200Z      T f+0    d+0    v70000.0 
b20221220T1200Z      T f+0    d+0    v60000.0 
b20221220T1200Z      T f+0    d+0    v50000.0 
b20221220T1200Z      T f+0    d+0    v40000.0 
b20221220T1200Z      T f+0    d+0    v30000.0 
b20221220T1200Z      T f+0    d+0    v25000.0 
b20221220T1200Z      T f+0    d+0    v20000.0 
b20221220T1200Z      T f+0    d+0    v15000.0 
b20221220T1200Z      T f+0    d+0    v10000.0 
b20221220T1200Z      T f+0    d+0    v7000.0  
b20221220T1200Z      T f+0    d+0    v5000.0  
b20221220T1200Z      T f+0    d+0    v3000.0  
b20221220T1200Z      T f+0    d+0    v2000.0  
b20221220T1200Z      T f+0    d+0    v1000.0  
EOF

rm -f z.log
./gribslim -fpx108= -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z   RAIN f+0    d+360  v101325.0
b20221220T1200Z   RAIN f+0    d+720  v101325.0
b20221220T1200Z   RAIN f+0    d+1080 v101325.0
b20221220T1200Z   RAIN f+0    d+1440 v101325.0
b20221220T1200Z   RAIN f+0    d+1800 v101325.0
b20221220T1200Z   RAIN f+0    d+2160 v101325.0
b20221220T1200Z   RAIN f+0    d+2520 v101325.0
b20221220T1200Z   RAIN f+0    d+2880 v101325.0
b20221220T1200Z   RAIN f+0    d+3240 v101325.0
b20221220T1200Z   RAIN f+0    d+3600 v101325.0
b20221220T1200Z   RAIN f+0    d+3960 v101325.0
b20221220T1200Z   RAIN f+0    d+4320 v101325.0
b20221220T1200Z   RAIN f+0    d+4680 v101325.0
b20221220T1200Z   RAIN f+0    d+5040 v101325.0
b20221220T1200Z   RAIN f+0    d+5400 v101325.0
b20221220T1200Z   RAIN f+0    d+5760 v101325.0
b20221220T1200Z   RAIN f+0    d+6120 v101325.0
b20221220T1200Z   RAIN f+0    d+6480 v101325.0
b20221220T1200Z   RAIN f+0    d+6840 v101325.0
b20221220T1200Z   RAIN f+0    d+7200 v101325.0
b20221220T1200Z   RAIN f+0    d+7560 v101325.0
b20221220T1200Z   RAIN f+0    d+7920 v101325.0
EOF

rm -f z.log
./gribslim -fpx108=,d1440.L,K -o/dev/null sample.gsm > z.log
diff -- z.log - <<EOF
b20221220T1200Z   RAIN f+0    d+360  v101325.0
b20221220T1200Z   RAIN f+0    d+720  v101325.0
b20221220T1200Z   RAIN f+0    d+1080 v101325.0
EOF

rm -f z.log