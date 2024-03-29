#!/bin/bash
set -Ceuo pipefail

if test ! -f sample.gsm
then
  if test -f "/nwp/s1/gsm/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_20221220120000_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin"
  then
    ln -s "/nwp/s1/gsm/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_20221220120000_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin" sample.gsm
  else
    wget -Osample.gsm "https://toyoda-eizi.net/nwp/s1/gsm/W_jp-JMA-tokyo,MODEL,JMA+gsm+gpv,C_RJTD_20221220120000_GSM_GPV_Rgl_Gll1p25deg_L-all_FD0000-0512_grib2.bin"
  fi
fi

if test ! -f sample.mepss
then
  if test -f "/nwp/s1/meps/Z__C_RJTD_20190605000000_MEPS_GPV_Rjp_Lsurf_FH00-15_grib2.bin"
  then
    ln -s "/nwp/s1/meps/Z__C_RJTD_20190605000000_MEPS_GPV_Rjp_Lsurf_FH00-15_grib2.bin" sample.mepss
  else
    wget -Osample.mepss "https://toyoda-eizi.net/nwp/s1/meps/Z__C_RJTD_20190605000000_MEPS_GPV_Rjp_Lsurf_FH00-15_grib2.bin"
  fi
fi
