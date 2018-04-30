#!/bin/bash
##########################################################
# solar-night.sh 20170624 Frank4DD
#
# This script runs daily after midnight, sending the
# RRD database XML export file to the Internet server.
# (var/solardb.xml). Even if upload is disabled, this
# creates a local nightly database backup.
#
# Please set config file path to your installations value!
##########################################################
echo "solar-night.sh: Run at `date`"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd -P`
popd > /dev/null
CONFIG=$SCRIPTPATH/../etc/pi-solar.conf
echo "solar-night.sh: using $CONFIG"

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
  echo "solar-night.sh: Error - cannot find config file [$CONFIG]" >&2
  exit -1
fi
readconfig MYCONFIG < "$CONFIG"

VHOME=${MYCONFIG[pi-solar-dir]}
STATION=${MYCONFIG[pi-solar-sid]}

##########################################################
# Get the RRD directory and RRD DB name from config file
##########################################################
RRD_DIR=${MYCONFIG[pi-solar-dir]}/rrd
RRD=$RRD_DIR/${MYCONFIG[pi-solar-rrd]}

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
# Create a database export into $LOGHOME/solardb.xml.gz
# Sends a nightly DB for import on the Internet server,
# its import there clears out upload gaps should the exist
##########################################################
if [ -f $LOGHOME/solardb.xml.gz ]; then
   echo "solar-night.sh: Deleting old file $LOGHOME/solardb.xml.gz"
   rm $LOGHOME/solardb.xml.gz
fi

echo "solar-night.sh: Creating XML export to $LOGHOME/solardb.xml"
echo "solar-night.sh: rrdtool dump $RRD $LOGHOME/solardb.xml"
rrdtool dump $RRD $LOGHOME/solardb.xml

if [ -f $LOGHOME/solardb.xml ]; then
  echo "solar-night.sh: Compressing XML export file $LOGHOME/solardb.xml"
  gzip $LOGHOME/solardb.xml
else
  echo "solar-night.sh: Error creating $LOGHOME/solardb.xml"
  exit 1
fi

##########################################################
# Check if destination is set, exit of set to "none"
##########################################################
if [ ${MYCONFIG[pi-solar-sftp]} == "none" ]; then
   echo "solar-night.sh: pi-solar-sftp=none, remote data upload disabled."
   echo "solar-night.sh: Finished `date`"
   exit 0
fi

SFTPDEST=$STATION@${MYCONFIG[pi-solar-sftp]}

##########################################################
# Upload xml DB archive /tmp/solardb.xml.gz to the server
##########################################################
if [ -f $LOGHOME/solardb.xml.gz ]; then
   echo "`date`: Uploading XML export file to $SFTPDEST"
   /usr/bin/sftp -b $VHOME/etc/sftp-xml.bat $SFTPDEST
else
   echo "Error no upload, can't find $LOGHOME/solardb.xml.gz"
fi

echo "solar-night.sh: Finished `date`"
############# end of solar-night.sh ########################
