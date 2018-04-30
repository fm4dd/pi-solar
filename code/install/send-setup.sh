#!/bin/bash
##########################################################
# send-setup.sh 20180409 Frank4DD
#
# This script runs once after setup is complete and local
# station is operational. It is sending the files to also
# integrate the online server with pi-solar.
# 	1. get the following files-> etc/pi-solar.conf
#                                 -> web/solar.php
#                                 -> web/solar.css
#                                 -> web/images/mppt-75-10.jpg
#                                 -> web/images/solar-system-01.jpg
#                                 -> src/solar-rrd.sh (create graphs)
#                                 -> var/rrdcopy.xml.gz (fresh dump)
#       2. zip above files        -> pi-ws<XX>-solar.zip
# 	3. scp it to destination  -> weather.fm4dd.com
#    or 4. mail it to destination -> support@frank4dd.com
#
# Please set config file path to your installations value!
##########################################################
echo "send-setup.sh: Run at `date`"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd -P`
popd > /dev/null
CONFIG=$SCRIPTPATH/../etc/pi-solar.conf
echo "send-setup.sh: using $CONFIG"

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

echo "##########################################################"
echo "# 1. Check for the configfile, and source it"
echo "##########################################################"
if [[ ! -f $CONFIG ]]; then
  echo "send-setup.sh: Error - cannot find config file [$CONFIG]" >&2
  exit -1
fi
readconfig MYCONFIG < "$CONFIG"

echo "Done."
echo


echo "##########################################################"
echo "# 2. Check if this script runs as user "pi", not as root."
echo "##########################################################"
if (( $EUID != 1000 )); then
   echo "This script must be run as user \"pi\"."
   exit 1;
fi

echo "Done."
echo

echo "##########################################################"
echo "# 3. Check for and collect the required files into tmp"
echo "##########################################################"
WHOME=${MYCONFIG[pi-solar-dir]}
STATION=${MYCONFIG[pi-solar-sid]}
WTEMP="$WHOME/var/$STATION-solar"
BACKUP="$WHOME/var/$STATION-solar.zip"

mkdir $WTEMP
if [ ! -d $WTEMP ]; then
   echo "send-setup.sh: Error - cannot create temp dir [$WTEMP]"
   exit 1
fi

if [ -f $WHOME/etc/pi-solar.conf ]; then
   echo "cp $WHOME/etc/pi-solar.conf $WTEMP/$STATION.conf"
   cp $WHOME/etc/pi-solar.conf $WTEMP/pi-solar.conf
else
   echo "send-setup.sh: Error - cannot find [$WHOME/etc/pi-solar.conf]"
   exit 1
fi

if [ -f $WHOME/web/solar.php ]; then
   echo "cp $WHOME/web/solar.php $WTEMP/solar.php"
   cp $WHOME/web/solar.php $WTEMP/solar.php
else
   echo "send-setup.sh: Error - cannot find [$WHOME/web/solar.php]"
   exit 1
fi

if [ -f $WHOME/web/solar.css ]; then
   echo "cp $WHOME/web/solar.css $WTEMP/solar.css"
   cp $WHOME/web/solar.css $WTEMP/solar.css
else
   echo "send-setup.sh: Error - cannot find [$WHOME/web/solar.css]"
   exit 1
fi

if [ -f $WHOME/web/images/mppt-75-10.jpg ]; then
   echo "cp $WHOME/web/images/mppt-75-10.jpg $WTEMP/mppt-75-10.jpg"
   cp $WHOME/web/images/mppt-75-10.jpg $WTEMP/mppt-75-10.jpg
else
   echo "send-setup.sh: Error - cannot find [$WHOME/web/images/mppt-75-10.jpg]"
   exit 1
fi

if [ -f $WHOME/web/images/solar-system-01.jpg ]; then
   echo "cp $WHOME/web/images/solar-system-01.jpg $WTEMP/solar-system-01.jpg"
   cp $WHOME/web/images/solar-system-01.jpg $WTEMP/solar-system-01.jpg
else
   echo "send-setup.sh: Error - cannot find [$WHOME/web/images/solar-system-01.jpg]"
   exit 1
fi

if [ -f $WHOME/src/solar-rrd.sh ]; then
   echo "cp $WHOME/src/solar-rrd.sh $WTEMP/solar-rrd.sh"
   cp $WHOME/src/solar-rrd.sh $WTEMP/solar-rrd.sh
else
   echo "send-setup.sh: Error - cannot find [$WHOME/src/solar-rrd.sh]"
   exit 1
fi

echo "rrdtool dump $WHOME/rrd/${MYCONFIG[pi-solar-rrd]} > $WTEMP/rrdcopy.xml"
rrdtool dump $WHOME/rrd/${MYCONFIG[pi-solar-rrd]} > $WTEMP/rrdcopy.xml
if [[ $? > 0 ]]; then
   echo "send-setup.sh: Error - cannot extract rrd data"
   exit 1
fi

gzip $WTEMP/rrdcopy.xml

echo
echo "ls -l $WTEMP"
ls -l $WTEMP

echo "Done."
echo

echo "##########################################################"
echo "# 4. Zip up the data for transmission"
echo "##########################################################"
echo "zip -j $BACKUP $WTEMP/*"
zip -j $BACKUP $WTEMP/*
if [[ $? > 0 ]]; then
   echo "send-setup.sh: Error - cannot zip $WTEMP"
fi

echo
echo "rm $WTEMP/$STATION.*"
rm $WTEMP/$STATION.*

echo "rmdir $WTEMP"
rmdir $WTEMP

echo
echo "ls -l $BACKUP"
ls -l $BACKUP
echo "Done."
echo

echo "##########################################################"
echo "# 5. Send the zip package to weather.fm4dd.com"
echo "##########################################################"
echo "scp $BACKUP fm@weather.fm4dd.com:/tmp"
scp $BACKUP fm@weather.fm4dd.com:/tmp

echo
echo "rm $BACKUP"
rm $BACKUP

echo "Done."
echo
############ end of send-setup.sh #######################
