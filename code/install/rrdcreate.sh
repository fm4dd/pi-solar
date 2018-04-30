#!/bin/bash
##########################################################
# rrdcreate.sh 20180325 Frank4DD
#
# This script creates the RRD database that will hold the
# ve.direct controller data collected in 1-min intervals.
# 
# It only needs to run once at installation time.
##########################################################
echo "rrdcreate.sh: Creating RRD database file for pi-solar"

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
CONFIG=../etc/pi-solar.conf
if [[ ! -f $CONFIG ]]; then
  echo "rrdcreate.sh: Error - cannot find config file [$CONFIG]" >&2
  exit -1
fi
readconfig MYCONFIG < "$CONFIG"

##########################################################
# Get the RRD directory and RRD DB name from config file
##########################################################
RRD_DIR=${MYCONFIG[pi-solar-dir]}/rrd
RRD=$RRD_DIR/${MYCONFIG[pi-solar-rrd]}

##########################################################
# Check for the DB folder, create one if necessary
##########################################################
if [[ ! -e $RRD_DIR ]]; then
    echo "rrdcreate.sh: Creating database directory $RRD_DIR."
    mkdir $RRD_DIR
fi
echo "rrdcreate.sh: Using folder [$RRD_DIR]."

##########################################################
# Check if the database already exists - Don't overwrite!
##########################################################
if [[ -f $RRD ]]; then
  echo "rrdcreate.sh: Skipping creation, RRD database [$RRD] exists." >&2
  exit -1
fi

##########################################################
# Check if the rrdtool database system is installed      #
##########################################################
if ! [ -x "$(command -v rrdtool)" ]; then
  echo "rrdcreate.sh: Error - rrdtool is not installed." >&2
  exit -1
fi

##########################################################
# The solar RRD database will hold four data sources:
# ----------------------------------------------
# 1. battery voltage (in Volt)   -> V   -> min  -50 max   50
# 2. battery current (in Ampere) -> I   -> min -100 max  100 (can be negative!)
# 3. panel voltage (in Volt)     -> VPV -> min -150 max  150
# 4. panel power (in Watt)       -> PPV -> min    0 max 1000
# 5. load current (in Ampere)    -> IL  -> min    0 max  100
# 6. operational charge state    -> CS  -> min    0 max    5
# 7. Daytime (sunrise..sunset) flag (0=day, 1=night)
#
# The data slots are allocated as follows:
# ----------------------------------------
# 1. store 1-min readings in 20160 entries (14 days: 60s*24hr*14d)
# RRA:AVERAGE:0.5:1:20160
# 2. store one 60min average in 13200 entries (1.5 years: 24hr*550d)
# RRA:AVERAGE:0.5:60:13200
# 3. store one day average in 6580 entries (18 years: 6580d)
# RRA:AVERAGE:0.5:1440:6580
#
# Additionally, we store MIN and MAX values at the same intervals.
# Resulting DB size is 4.3 MB binary, 10 MB as XML dump.
##########################################################
echo "rrdcreate.sh: Creating RRD database [$RRD]."

rrdtool create $RRD          \
--start now --step 60s       \
DS:vbat:GAUGE:300:-50:50     \
DS:ibat:GAUGE:300:-100:100   \
DS:vpnl:GAUGE:300:-150:150   \
DS:ppnl:GAUGE:300:0:1000     \
DS:load:GAUGE:300:0:100      \
DS:opcs:GAUGE:300:0:5        \
DS:dayt:GAUGE:300:0:1        \
RRA:AVERAGE:0.5:1:20160      \
RRA:AVERAGE:0.5:60:13200     \
RRA:AVERAGE:0.5:1440:6580    \
RRA:MIN:0.5:60:13200         \
RRA:MAX:0.5:60:13200         \
RRA:MIN:0.5:1440:6580        \
RRA:MAX:0.5:1440:6580

if [[ -f $RRD ]]; then
  echo "rrdcreate.sh: Database [$RRD] created."
else
  echo "rrdcreate.sh: Could not create database [$RRD]."
  exit -1
fi

############# end of rrdcreate.sh ########################
