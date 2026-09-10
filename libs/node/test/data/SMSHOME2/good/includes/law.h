# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

### start of law.h, normally included from config.h ####################

# Override for local area wave model
if [[ "%FSFAMILY%" = "euroshelf" ]] ; then
  set -a
  PPFRQ=6
  WAMRESOL=medite10
  WAMANLEN=12
  WAMFCLEN=120
  WAMSAR=yes
  WAMIDELRES=6
  INIWDEXPVER=%INIWDEXPVER:0001%     # expver for wind forcing
  INIWDCLASS=%INIWDCLASS:od%
  INIWDSTREAM=DA
  WGRIBIN=yes
  gflag=T
  nsysnb=-1
  nmetnb=-1

  ALTIMETER=ers2
  SAR=ers2
  ERS=`echo $SAR | cut -c4`

  model=medite
  FSALT=$FSBASE/${ALTIMETER}/alt/$model
  FSALT_B=$FSBASE/${ALTIMETER}_b/alt/$model
  FSALT_S=$FSBASE/${ALTIMETER}_s/alt/$model
  FSSAR=$FSBASE/${SAR}/sar/$model
  FSSAR_B=$FSBASE/${SAR}_b/sar/$model
  FSSAR_S=$FSBASE/${SAR}_s/sar/$model

  WAVE_ROOT=$FDB_ROOT
  WAVE_IROOT=$FDB_IROOT

fi
# end of law.h #################################
