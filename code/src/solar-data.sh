#!/bin/bash
##########################################################
# solar-data.sh 20180402 Frank4DD
#
# This script runs in 1-min intervals through cron.
# It has the following 2 tasks:
# 	1. Read the serial data   -> web/getsolar.htm
#	2. Update RRD database    -> rrd/solar.rrd
#	3. Internet server upload -> weather.fm4dd.com
#
# Please set config file path to your installations value!
##########################################################
echo "solar-data.sh: Run at `date`"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd -P`
popd > /dev/null
CONFIG=$SCRIPTPATH/../etc/pi-solar.conf
echo "solar-data.sh: using $CONFIG"

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
# Check for the config file, source it and set global vars
##########################################################
if [[ ! -f $CONFIG ]]; then
  echo "solar-data.sh: Error - cannot find config file [$CONFIG]" >&2
  exit -1
fi
readconfig MYCONFIG < "$CONFIG"

VHOME=${MYCONFIG[pi-solar-dir]}
STATION=${MYCONFIG[pi-solar-sid]}
SDEV=${MYCONFIG[pi-solar-ser]}

##########################################################
# Set RRD values, get the RRD DB name from config file
##########################################################
RRD=$VHOME/rrd/${MYCONFIG[pi-solar-rrd]}
RRDTOOL="/usr/bin/rrdtool"
RRDGRAPH=$VHOME/bin/solar-rrd.sh

##########################################################
# Check for pi-weather integration
##########################################################
if [ ${MYCONFIG[pi-solar-int]} == "none" ]; then
   echo "solar-data.sh: pi-solar-int=none, no integration with pi-weather"
   WEBHOME=$HOMEDIR/web
   LOGHOME=$HOMEDIR/var
   echo "solar-data.sh: Setting pi-solar web directory: $WEBHOME"
else
   WEBHOME=${MYCONFIG[pi-solar-int]}/web
   echo "solar-data.sh: Check existing pi-weather web directory: $WEBHOME"
fi

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [ -d $WEBHOME ]; then
   echo "solar-data.sh: OK. Found existing web directory: $WEBHOME"
   LOGHOME=${MYCONFIG[pi-solar-int]}/var
   echo "solar-data.sh: Setting pi-solar log directory: $LOGHOME"
else
   echo "Error - Could not find web directory: $WEBHOME"
   exit 1
fi

##########################################################
# 1. Take the serial reading, save it to html for local
# webpage display and Internet server upload. 
##########################################################
echo "solar-data.sh: Getting serial data from $SDEV";
EXECUTE="$VHOME/bin/getvictron -s $SDEV -o $WEBHOME/getsolar.htm"
echo "solar-data.sh: $EXECUTE";
RRDUPDATE=`$EXECUTE`
RET=$?
echo "solar-data.sh: command returned code [$RET]";

if [[ $RET < '0' || $RRDUPDATE == "Error"* ]]; then
  echo "Error: - Failure during $EXECUTE"
fi

##########################################################
# Timestamp
##########################################################
TIME=`echo $RRDUPDATE | cut -d ":" -f 1`
if [ "$TIME" == "" ] || [ "$TIME" == "Error" ]; then
  echo "solar-data.sh: Error getting timestamp from sensor data"
  exit
fi
echo "solar-data.sh: Got timestamp from getvictron RRD output: [$TIME]"

##########################################################
# 2. Get daytime flag (TZ is taken from local system env)
# ./daytcalc -t 1486784589 -x 12.45277778 -y 51.340277778
##########################################################
DAYTCALC="${MYCONFIG[pi-solar-dir]}/bin/daytcalc"
LON="${MYCONFIG[pi-solar-lon]}"
LAT="${MYCONFIG[pi-solar-lat]}"
DAYTIME=('day' 'night')

echo "solar-data.sh: daytime flag $DAYTCALC -t $TIME -x $LON -y $LAT"
`$DAYTCALC -t $TIME -x $LON -y $LAT`

DAYT=$?
if [ "$DAYT" == "" ]; then
  echo "solar-data.sh: Error getting daytime information, setting 0."
  DAYT=0
else
  echo "solar-data.sh: daytcalc $TIME returned [$DAYT] [${DAYTIME[$DAYT]}]."
fi

##########################################################
# 3. Update the RRD database. Add the daytcalc flag to the
# getvictron RRD update string before calling rrdupdate.
##########################################################
echo "solar-data.sh: Updating RRD database $RRD"
echo "$RRDTOOL update $RRD $RRDUPDATE:$DAYT"
$RRDTOOL updatev $RRD "$RRDUPDATE:$DAYT"

##########################################################
# 4. Update RRD graphs in a separate process and continue
##########################################################
echo "solar-data.sh: Creating graphs with $RRDGRAPH"
$RRDGRAPH &

##########################################################
# 5. Create solar.txt, send it together with getsolar.htm
# to the Internet server
##########################################################
echo "solar-data.sh: Data upload"
if [ ${MYCONFIG[pi-solar-sftp]} == "none" ]; then
   echo "solar-data.sh: pi-solar-sftp=none, remote data upload disabled."
   exit
fi

echo "solar-data.sh: echo \"$RRDUPDATE:$DAYT\" > $VHOME/var/solar.txt"
echo "$RRDUPDATE:$DAYT" > $LOGHOME/solar.txt

SFTPDEST=$STATION@${MYCONFIG[pi-solar-sftp]}

if [ -f $VHOME/etc/sftp-dat.bat ]; then
   echo "solar-data.sh: Sending files to $SFTPDEST"
   sftp -q -b $VHOME/etc/sftp-dat.bat $SFTPDEST
else
   echo "solar-data.sh: Cannot find $VHOME/etc/sftp-dat.bat"
fi
############# end of solar-data.sh ########################
