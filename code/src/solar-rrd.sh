#!/bin/bash
##########################################################
# solar-rrd.sh 20180325 Frank4DD
# 
# This script runs in 1-min intervals through cron. It
# generates the graph images as well as the energy tables.
#
# Please set config file path to your installations value!
##########################################################
sleep 5 # 5 sec past 00 to let solar-data.sh update first
echo "solar-rrd.sh: Run at `date`"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd -P`
popd > /dev/null
CONFIG=$SCRIPTPATH/../etc/pi-solar.conf
echo "solar-rrd.sh: Config file [$CONFIG]"

##########################################################
# readconfig() function to read the config file variables
##########################################################
readconfig() {
   local ARRAY="$1"
   local KEY VALUE
   local IFS='='
   declare -g -A "$ARRAY"
   while read; do
      # here assumed that comments may not be indented
      [[ $REPLY == [^#]*[^$IFS]${IFS}[^$IFS]* ]] && {
          read KEY VALUE <<< "$REPLY"
          [[ -n $KEY ]] || continue
          eval "$ARRAY[$KEY]=\"\$VALUE\""
      }
   done
}

##########################################################
# Check for the config file, and source it
##########################################################
if [[ ! -f $CONFIG ]]; then
  echo "solar-rrd.sh: Error - cannot find config file [$CONFIG]" >&2
  exit -1
fi
readconfig MYCONFIG < "$CONFIG"

PVPOWER="${MYCONFIG[pi-solar-dir]}/bin/pvpower"

RRD=${MYCONFIG[pi-solar-dir]}/rrd/${MYCONFIG[pi-solar-rrd]}
RRDTOOL="/usr/bin/rrdtool"

##########################################################
# Check for pi-weather integration
##########################################################
if [ ${MYCONFIG[pi-solar-int]} == "none" ]; then
   echo "solar-data.sh: pi-solar-int=none, no integration with pi-weather"
   WEBPATH="${MYCONFIG[pi-solar-dir]}/web"
   IMGPATH="${MYCONFIG[pi-solar-dir]}/web/images"
   VARPATH="${MYCONFIG[pi-solar-dir]}/var"
   LOGPATH="${MYCONFIG[pi-solar-dir]}/log"
   echo "solar-rrd.sh: Set directories web, web/images, var, log under: ${MYCONFIG[pi-solar-dir]}"
else
   WEBPATH=${MYCONFIG[pi-solar-int]}/web
   echo "solar-rrd.sh: Check existing pi-weather web directory: $WEBPATH"
fi

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [ -d $WEBPATH ]; then
   echo "solar-rrd.sh: OK. Found existing web directory: $WEBPATH"
   IMGPATH="${MYCONFIG[pi-solar-int]}/web/images"
   VARPATH="${MYCONFIG[pi-solar-int]}/var"
   LOGPATH="${MYCONFIG[pi-solar-int]}/log"
   echo "solar-rrd.sh: Set directories web, web/images, var, log under: ${MYCONFIG[pi-solar-int]}"
else
   echo "Error - Could not find web directory: $WEBPATH"
   exit 1
fi

##########################################################
# Check if RRD database exists, exit if it doesn't
##########################################################
if [[ ! -e $RRD ]]; then
   echo "solar-rrd.sh: Error cannot find RRD database."
   exit 1
fi

##########################################################
# Create the daily graph images
##########################################################
VPNLPNG=$IMGPATH/daily_vpnl.png # PV Panel Voltage
VBATPNG=$IMGPATH/daily_vbat.png # BAT Voltage
PBALPNG=$IMGPATH/daily_pbal.png # Energy Balance

echo -n "Creating image $VPNLPNG... "
$RRDTOOL graph $VPNLPNG -a PNG \
  --start -16h \
  --title='Panel Voltage' \
  --step=60s  \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:opcs1=$RRD:opcs:AVERAGE \
  DEF:vpnl1=$RRD:vpnl:AVERAGE \
  CDEF:v1=opcs1,0,GT,vpnl1,UNKN,IF \
  DEF:dayt1=$RRD:dayt:AVERAGE \
  CDEF:dayt2=dayt1,0,GT,INF,UNKN,IF \
  AREA:dayt2#cfcfcf \
  CDEF:tneg1=dayt1,0,GT,NEGINF,UNKN,IF \
  AREA:tneg1#cfcfcf \
  AREA:vpnl1#00447755:'Volt Off' \
  AREA:v1#00447799:'Volt Charging' \
  LINE1:vpnl1#004477FF:''  \
  GPRINT:vpnl1:MIN:'Min\: %3.2lf %sV' \
  GPRINT:vpnl1:MAX:'Max\: %3.2lf %sV' \
  GPRINT:vpnl1:LAST:'Last\: %3.2lf %sV'

echo -n "Creating image $VBATPNG... "
$RRDTOOL graph $VBATPNG -a PNG \
  --start -16h \
  --title='Battery Voltage' \
  --step=60s  \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  --alt-autoscale \
  --alt-y-grid \
  DEF:vbat1=$RRD:vbat:AVERAGE \
  DEF:dayt1=$RRD:dayt:AVERAGE \
  CDEF:dayt2=dayt1,0,GT,INF,UNKN,IF \
  AREA:dayt2#cfcfcf \
  CDEF:tneg1=dayt1,0,GT,NEGINF,UNKN,IF \
  AREA:tneg1#cfcfcf \
  AREA:vbat1#99001F55:'' \
  LINE1:vbat1#99001FFF:'Volt' \
  GPRINT:vbat1:MIN:'Min\: %3.2lf V' \
  GPRINT:vbat1:MAX:'Max\: %3.2lf V' \
  GPRINT:vbat1:LAST:'Last\: %3.2lf V'

echo -n "Creating image $PBALPNG... "
$RRDTOOL graph $PBALPNG -a PNG \
  --start -16h \
  --title='Power Balance' \
  --step=60s  \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vb1=$RRD:vbat:AVERAGE \
  DEF:ib1=$RRD:ibat:AVERAGE \
  CDEF:pbat=vb1,ib1,* \
  CDEF:psur=pbat,0,GE,pbat,0,IF \
  CDEF:pdef=pbat,0,LT,pbat,0,IF \
  DEF:dayt1=$RRD:dayt:AVERAGE \
  CDEF:dayt2=dayt1,0,GT,INF,UNKN,IF \
  AREA:dayt2#cfcfcf \
  CDEF:tneg1=dayt1,0,GT,NEGINF,UNKN,IF \
  AREA:tneg1#cfcfcf \
  AREA:psur#00994455:'' \
  AREA:pdef#99001F55:'' \
  LINE1:psur#009944FF:'Watt Surplus' \
  LINE1:pdef#99001FFF:'Watt Deficit' \
  CDEF:zero=dayt1,5,EQ,UNKN,0,IF \
  LINE1:zero#cfcfcfFF:''  \
  GPRINT:pbat:MIN:'Min\: %3.2lf %sW' \
  GPRINT:pbat:MAX:'Max\: %3.2lf %sW' \
  GPRINT:pbat:LAST:'Last\: %3.2lf %sW'

##########################################################
# Create the monthly graph images
##########################################################
MVPNLPNG=$IMGPATH/monthly_vpnl.png
MVBATPNG=$IMGPATH/monthly_vbat.png
MPBALPNG=$IMGPATH/monthly_pbal.png
midnight=$(date -d "00:00" +%s)

##########################################################
# Check if monthly panel file has already
# been updated today, otherwise generate.
##########################################################
if [ -f $MVPNLPNG ]; then FILEAGE=$(date -r $MVPNLPNG +%s); fi
if [ ! -f $MVPNLPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $MVPNLPNG... "

  # --------------------------------------- #
  # --x-grid defines the x-axis spacing.    #
  # format: GTM:GST:MTM:MST:LTM:LST:LPR:LFM #
  # GTM:GST base grid (Unit:How Many)       #
  # MTM:MST major grid (Unit:How Many)      #
  # LTM:LST how often labels are placed     #
  # --------------------------------------- #
  $RRDTOOL graph $MVPNLPNG -a PNG \
  --start end-21d --end 00:00 \
  --x-grid HOUR:8:DAY:1:DAY:1:86400:%d \
  --title='Panel Voltage, 3 Weeks' \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:opcs1=$RRD:opcs:AVERAGE \
  DEF:vpnl1=$RRD:vpnl:AVERAGE \
  CDEF:v1=opcs1,0,GT,vpnl1,UNKN,IF \
  AREA:vpnl1#00447755:'Volt Off' \
  AREA:v1#00447799:'Volt Charging' \
  LINE1:vpnl1#004477FF:''  \
  GPRINT:vpnl1:MIN:'Min\: %3.2lf %sV' \
  GPRINT:vpnl1:MAX:'Max\: %3.2lf %sV' \
  GPRINT:vpnl1:LAST:'Last\: %3.2lf %sV'
fi

##########################################################
# Check if monthly battery file has already
# been updated today, otherwise generate.
##########################################################
if [ -f $MVBATPNG ]; then FILEAGE=$(date -r $MVBATPNG +%s); fi
if [ ! -f $MVBATPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $MVBATPNG... "

  $RRDTOOL graph $MVBATPNG -a PNG \
  --start end-21d --end 00:00 \
  --title='Battery Voltage, 3 Weeks' \
  --x-grid HOUR:8:DAY:1:DAY:1:86400:%d \
  --alt-autoscale \
  --lower-limit=12.0 \
  --width=619 \
  --height=77 \
  --border=1  \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vbat1=$RRD:vbat:AVERAGE \
  AREA:vbat1#99001F55:'' \
  LINE1:vbat1#99001FFF:'Volt' \
  GPRINT:vbat1:MIN:'Min\: %3.2lf V' \
  GPRINT:vbat1:MAX:'Max\: %3.2lf V' \
  GPRINT:vbat1:LAST:'Last\: %3.2lf V'
fi

##########################################################
# Check if monthly power file has already
# been updated today, otherwise generate.
##########################################################
if [ -f $MPBALPNG ]; then FILEAGE=$(date -r $MPBALPNG +%s); fi
if [ ! -f $MPBALPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $MPBALPNG... "

  $RRDTOOL graph $MPBALPNG -a PNG \
  --start end-21d --end 00:00 \
  --x-grid HOUR:8:DAY:1:DAY:1:86400:%d \
  --title='Power Balance, 3 Weeks' \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vb1=$RRD:vbat:AVERAGE \
  DEF:ib1=$RRD:ibat:AVERAGE \
  CDEF:pbat=vb1,ib1,* \
  CDEF:psur=pbat,0,GE,pbat,UNKN,IF \
  CDEF:pdef=pbat,0,LT,pbat,UNKN,IF \
  AREA:psur#00994455:'' \
  LINE1:psur#009944FF:'Watt Surplus' \
  AREA:pdef#99001F55:'' \
  LINE1:pdef#99001FFF:'Watt Deficit' \
  GPRINT:pbat:MIN:'Min\: %3.2lf %sW' \
  GPRINT:pbat:MAX:'Max\: %3.2lf %sW' \
  GPRINT:pbat:LAST:'Last\: %3.2lf %sW'
fi

##########################################################
# Create the yearly graph images
##########################################################
YVPNLPNG=$IMGPATH/yearly_vpnl.png
YVBATPNG=$IMGPATH/yearly_vbat.png
YPBALPNG=$IMGPATH/yearly_pbal.png

##########################################################
# Check if yearly VPNL file has already
# been updated today, otherwise generate
##########################################################
if [ -f $YVPNLPNG ]; then FILEAGE=$(date -r $YVPNLPNG +%s); fi
if [ ! -f $YVPNLPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $YVPNLPNG... "

  $RRDTOOL graph $YVPNLPNG -a PNG \
  --start end-18mon --end 00:00 \
  --x-grid MONTH:1:YEAR:1:MONTH:1:2592000:%b \
  --title='Panel Voltage, Yearly View' \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vpnl1=$RRD:vpnl:AVERAGE \
  AREA:vpnl1#00447799:'' \
  LINE1:vpnl1#004477FF:'Volt'  \
  GPRINT:vpnl1:MIN:'Min\: %3.2lf %sV' \
  GPRINT:vpnl1:MAX:'Max\: %3.2lf %sV' \
  GPRINT:vpnl1:LAST:'Last\: %3.2lf %sV'
fi

##########################################################
# Check if yearly VBAT file has already 
# been updated today, otherwise generate
##########################################################
if [ -f $YVBATPNG ]; then FILEAGE=$(date -r $YVBATPNG +%s); fi
if [ ! -f $YVBATPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $YVBATPNG... "

  $RRDTOOL graph $YVBATPNG -a PNG \
  --start end-18mon --end 00:00 \
  --x-grid MONTH:1:YEAR:1:MONTH:1:2592000:%b \
  --title='Battery Voltage, Yearly View' \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vbat1=$RRD:vbat:AVERAGE \
  AREA:vbat1#99001F55:'' \
  LINE1:vbat1#99001FFF:'Volt' \
  GPRINT:vbat1:MIN:'Min\: %3.2lf V' \
  GPRINT:vbat1:MAX:'Max\: %3.2lf V' \
  GPRINT:vbat1:LAST:'Last\: %3.2lf V'
fi

##########################################################
# Check if yearly power file has already
# been updated today, otherwise generate
##########################################################
if [ -f $YPBALPNG ]; then FILEAGE=$(date -r $YPBALPNG +%s); fi
if [ ! -f $YPBALPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating image $YLOADPNG... "

  $RRDTOOL graph $YPBALPNG -a PNG \
  --start end-18mon --end 00:00 \
  --x-grid MONTH:1:YEAR:1:MONTH:1:2592000:%b \
  --title='Power Balance, Yearly View' \
  --width=619 \
  --height=77 \
  --border=1  \
  --slope-mode \
  --color SHADEA#000000 \
  --color SHADEB#000000 \
  DEF:vb1=$RRD:vbat:AVERAGE \
  DEF:ib1=$RRD:ibat:AVERAGE \
  CDEF:pbat=vb1,ib1,* \
  CDEF:psur=pbat,0,GE,pbat,UNKN,IF \
  CDEF:pdef=pbat,0,LT,pbat,UNKN,IF \
  AREA:psur#00994455:'' \
  LINE1:psur#009944FF:'Watt Surplus' \
  AREA:pdef#99001F55:'' \
  LINE1:pdef#99001FFF:'Watt Deficit' \
  GPRINT:pbat:MIN:'Min\: %3.2lf %sW' \
  GPRINT:pbat:MAX:'Max\: %3.2lf %sW' \
  GPRINT:pbat:LAST:'Last\: %3.2lf %sW'
fi

###########################################################
## Create the 18-year graph images
###########################################################
#TWYVBATPNG=$IMGPATH/twyear_temp.png
#TWYVPNLPNG=$IMGPATH/twyear_humi.png
#TWYLOADPNG=$IMGPATH/twyear_bmpr.png
#
###########################################################
## Check if the 18-year temp file has already
## been updated today, otherwise generate it.
###########################################################
#if [ -f $TWYVBATPNG ]; then FILEAGE=$(date -r $TWYVBATPNG +%s); fi
#if [ ! -f $TWYVBATPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
#  echo -n "Creating image $TWYVBATPNG... "
#
#  $RRDTOOL graph $TWYVBATPNG -a PNG \
#  --start end-18years --end 00:00 \
#  --x-grid YEAR:1:YEAR:10:YEAR:1:31536000:%Y \
#  --title='Temperature, 18-Year View' \
#  --slope-mode \
#  --width=619 \
#  --height=77 \
#  --border=1  \
#  --color SHADEA#000000 \
#  --color SHADEB#000000 \
#  DEF:temp1=$RRD:temp:AVERAGE \
#  'AREA:temp1#99001F:Temperature in °C' \
#  'GPRINT:temp1:MIN:Min\: %3.2lf' \
#  'GPRINT:temp1:MAX:Max\: %3.2lf' \
#  'GPRINT:temp1:LAST:Last\: %3.2lf'
#fi
#
############################################################
## Check if the 18-year humi file has already
## been updated today, otherwise generate it.
###########################################################
#if [ -f $TWYVPNLPNG ]; then FILEAGE=$(date -r $TWYVPNLPNG +%s); fi
#if [ ! -f $TWYVPNLPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
#  echo -n "Creating image $TWYVPNLPNG... "
#
#  $RRDTOOL graph $TWYVPNLPNG -a PNG \
#  --start end-18years --end 00:00 \
#  --x-grid YEAR:1:YEAR:10:YEAR:1:31536000:%Y \
#  --title='Humidity, 18-Year View' \
#  --upper-limit=100 \
#  --lower-limit=0 \
#  --slope-mode \
#  --width=619 \
#  --height=77 \
#  --border=1  \
#  --color SHADEA#000000 \
#  --color SHADEB#000000 \
#  DEF:humi1=$RRD:humi:AVERAGE \
#  'AREA:humi1#004477:Humidity in percent' \
#  'GPRINT:humi1:MIN:Min\: %3.2lf' \
#  'GPRINT:humi1:MAX:Max\: %3.2lf' \
#  'GPRINT:humi1:LAST:Last\: %3.2lf'
#fi
#
###########################################################
## Check if the 18-year bmpr file has already
## been updated today, otherwise generate it.
###########################################################
#if [ -f $TWYLOADPNG ]; then FILEAGE=$(date -r $TWYLOADPNG +%s); fi
#if [ ! -f $TWYLOADPNG ] || [[ "$FILEAGE" < "$midnight" ]]; then
#  echo -n "Creating image $TWYLOADPNG... "
#
#  $RRDTOOL graph $TWYLOADPNG -a PNG \
#  --start end-18years --end 00:00 \
#  --x-grid YEAR:1:YEAR:10:YEAR:1:31536000:%Y \
#  --title='Barometric Pressure, 18-Year View' \
#  --slope-mode \
#  --width=619 \
#  --height=77 \
#  --border=1  \
#  --alt-autoscale \
#  --alt-y-grid \
#  --units-exponent=0 \
#  --color SHADEA#000000 \
#  --color SHADEB#000000 \
#  DEF:bmpr1=$RRD:bmpr:AVERAGE \
#  'CDEF:bmpr2=bmpr1,100,/' \
#  'AREA:bmpr2#007744:Barometric Pressure in hPa' \
#  'GPRINT:bmpr2:MIN:Min\: %3.2lf' \
#  'GPRINT:bmpr2:MAX:Max\: %3.2lf' \
#  'GPRINT:bmpr2:LAST:Last\: %3.2lf'
#fi
#
##########################################################
# Daily update of the 12-days power generation htm file
##########################################################
DAYHTMFILE=$WEBPATH/daypower.htm

if [ -f $DAYHTMFILE ]; then FILEAGE=$(date -r $DAYHTMFILE +%s); fi
if [ ! -f $DAYHTMFILE ] || [[ "$FILEAGE" < "$midnight" ]]; then
  echo -n "Creating $DAYHTMFILE... "
  $PVPOWER -s $RRD -d $DAYHTMFILE
  cp $DAYHTMFILE $VARPATH/daypower.htm
  echo " Done."
fi

echo "solar-rrd.sh: Finished `date`"
###########################################################
## Daily update of the monthly Min/Max Temperature htm file
###########################################################
#MONHTMFILE=$WEBPATH/pvpower.htm
#
#if [ -f $MONHTMFILE ]; then FILEAGE=$(date -r $MONHTMFILE +%s); fi
#if [ ! -f $MONHTMFILE ] || [[ "$FILEAGE" < "$midnight" ]]; then
#  echo -n "Creating $MONHTMFILE... "
#  $PVPOWER -s $RRD -m $MONHTMFILE
#  cp $MONHTMFILE $VARPATH/monpower.htm
#  echo " Done."
#fi
#
##########################################################
# End of solar-rrd.sh
##########################################################
