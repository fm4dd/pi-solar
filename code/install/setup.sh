#!/bin/bash
##########################################################
# setup.sh 20180401 Frank4DD
#
# This script installs the pre-requsite software packages
# that are needed to compile the "pi-solar" programs,
# defines the web home, installs the database, etc.
#
# It is the first script to be run after downloading or
# cloning the git package. Before running it, edit the
# file etc/pi-solar.conf to adjust the configuration 
# parameters.
#
# The script must be run as user "pi", it will use sudo
# if needed.
#
# Note: It adds approx 100MB of SW packages from required
# RRD dependencies, PHP and lighthttpd webserver packages.
# If a pi-weather instance is found (/home/pi/pi-wsXX)
# the web pages will integrate there, otherwise the web
# server gets configured to use /home/pi/pi-solar/web.
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
echo "# 1. Check for "pi-solar.conf" configfile and source it"
echo "##########################################################"
CONFIG="../etc/pi-solar.conf"
if [[ ! -f $CONFIG ]]; then
   echo "Error - cannot find config file [$CONFIG]" >&2
   exit 1
fi
readconfig MYCONFIG < "$CONFIG"
HOMEDIR=${MYCONFIG[pi-solar-dir]}
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
echo "# 3. Check if the Raspberry Pi OS version is sufficient"
echo "##########################################################"
MAJOR=`lsb_release -r -s | cut -d "." -f1` # lsb_release -r -s 
MINOR=`lsb_release -r -s | cut -d "." -f2` # returns e.g. "9.4"

   echo "Rasbian release check: identified version $MAJOR.$MINOR"
if (( $MAJOR != 9 )); then
   echo "Error - pi-solar supports Rasbian Stretch Release 9 and up."
   exit 1;
fi
echo "Done."
echo

echo "##########################################################"
echo "# 4. Before we get SW packages, refresh the SW catalogue"
echo "# and remove any unneeded SW packages."
echo "##########################################################"
EXECUTE="sudo apt-get update -qq"
echo "Updating SW catalogue through [$EXECUTE]. Please wait approx 60s"
`$EXECUTE`
echo

DELPKGS="libraspberrypi-doc samba-common cifs-utils libtalloc2 libwbclient0"
EXECUTE="sudo apt-get remove $DELPKGS -y -q"
echo "Removing SW packages [$DELPKGS]. Please wait ..."
$EXECUTE
if [[ $? > 0 ]]; then
   echo "Error removing packages [$DELPKGS], exiting setup script."
   echo "Check the apt function, network, and DNS."
   exit 1
fi
echo "Done."
echo

echo "##########################################################"
echo "# 5. Install the RRD database tools and development files"
echo "##########################################################"
APPLIST="rrdtool librrd8 librrd-dev"
EXECUTE="sudo apt-get install $APPLIST -y -q"
echo "Getting SW packages [$APPLIST]. Please wait ..."
$EXECUTE
if [[ $? > 0 ]]; then
   echo "Error getting packages [$APPLIST], exiting setup script."
   echo "Check the apt function, network, and DNS."
   exit 1
fi
echo "Done."
echo

echo "##########################################################"
echo "# 6. Install webserver: lighttpd, lighttpd-doc, php-cgi"
echo "##########################################################"
APPLIST="lighttpd lighttpd-doc php-cgi php-mbstring"
EXECUTE="sudo apt-get install $APPLIST -y -q"
echo "Getting SW packages [$APPLIST]. Please wait ..."
$EXECUTE
if [[ $? > 0 ]]; then
   echo "Error getting packages [$APPLIST], exiting setup script."
   echo "Check the apt function, network, and DNS."
   exit 1
fi
echo "Done."
echo

echo "##########################################################"
echo "# 7. Check for pi-weather integration"
echo "##########################################################"
if [ ${MYCONFIG[pi-solar-int]} == "none" ]; then
   echo "Found pi-solar-int=none, no integration with pi-weather"
   WEBHOME=$HOMEDIR/web
   LOGHOME=$HOMEDIR/var
   echo "Setting pi-solar web directory: $WEBHOME"
else
   WEBHOME=${MYCONFIG[pi-solar-int]}/web
   echo "Check existing pi-weather web directory: $WEBHOME"
fi

if [ -d $WEBHOME ]; then
   echo "OK. Found existing web directory: $WEBHOME"
   LOGHOME=${MYCONFIG[pi-solar-int]}/var
   echo "Setting pi-solar log directory: $LOGHOME"
else
   echo "Error - Could not find web directory: $WEBHOME"
   exit 1
fi
echo "Done."
echo

echo "##########################################################"
echo "# 8. Define HOMEDIR/var as tmpfs to reduce SD card wear"
echo "##########################################################"
TODAY=`date +'%Y%m%d'`
if [ -f ../backup/$TODAY-fstab.backup ]; then
   echo "Found existing backup of /etc/fstab file:"
   ls -l ../backup/$TODAY-fstab.backup
else
   echo "Create new backup of current /etc/fstab file:"
   cp /etc/crontab ../backup/$TODAY-fstab.backup
   ls -l ../backup/$TODAY-fstab.backup
fi
echo

GREP=`grep $HOMEDIR/var /etc/fstab`
if [[ $? > 0 ]]; then
   LINE1="##########################################################"
   sudo sh -c "echo \"$LINE1\" >> /etc/fstab"
   LINE2="# pi-solar: Define $HOMEDIR/var as tmpfs, reducing sdcard wear"
   sudo sh -c "echo \"$LINE2\" >> /etc/fstab"
   LINE3="tmpfs           $HOMEDIR/var    tmpfs   nodev,nosuid,noexec,nodiratime,size=64M        0       0"
   sudo sh -c "echo \"$LINE3\" >> /etc/fstab"
   echo "Adding 3 lines to /etc/fstab file:"
   tail -4 /etc/fstab
else
   echo "Found $HOMEDIR/var tmpfs line in /etc/fstab file:"
   echo "$GREP"
fi

GREP=`mount | grep $HOMEDIR/var`
if [[ $? > 0 ]]; then
   sudo mount -v $HOMEDIR/var
else
   echo "Found $HOMEDIR/var tmpfs is already mounted"
fi
mount | grep $HOMEDIR/var
echo "Done."
echo

echo "##########################################################"
echo "# 9. Compile 'C' source code in ../src"
echo "##########################################################"
cd ../src
echo "Cleanup any old binaries in src:"
#make clean
echo
echo "Compile new binaries in src:"
make
echo

echo "##########################################################"
echo "# 10. Install programs and scripts to $HOMEDIR/bin"
echo "##########################################################"
export BINDIR=$HOMEDIR/bin
echo "Installing application binaries into $BINDIR."
#env | grep BINDIR
make install
echo
echo "List installed files in $BINDIR:"
ls -l $BINDIR
echo
unset BINDIR
echo -n "Returning to installer directory: "
cd -
echo "Done."
echo

echo "##########################################################"
echo "# 11. rrdcreate creates the empty RRD database"
echo "##########################################################"
./rrdcreate.sh
RRD_DIR=${MYCONFIG[pi-solar-dir]}/rrd
RRD=$RRD_DIR/${MYCONFIG[pi-solar-rrd]}

if [[ ! -e $RRD ]]; then
   echo "Error creating the RRD database."
   exit 1
fi

ls -l $RRD
echo "Done."
echo

echo "##########################################################"
echo "# 12. Test the Victron charger serial line connectivity"
echo "##########################################################"
./serialtest.sh
echo "./serialtest.sh returned [$?]"

if [[ $? > 0 ]]; then
   echo "Error - Test with ./serialtest.sh failed, exiting install." >&2
   exit 1
fi
echo "Done."
echo

echo "##########################################################"
echo "# 13. Check SSH key for the data upload to Internet server"
echo "##########################################################"
PRIVKEY=`ls ~/.ssh/id_rsa`
PUBKEY=`ls ~/.ssh/id_rsa.pub`

if [ "$PRIVKEY" == "" ] || [ "$PUBKEY" == "" ]; then
   echo "Could not find existing SSH keys, generating 2048 bit RSA:"
   ssh-keygen -t rsa -b 2048 -C "pi-ws03" -f ~/.ssh/id_rsa -P "" -q
   PRIVKEY=`ls ~/.ssh/id_rsa`
   PUBKEY=`ls ~/.ssh/id_rsa.pub`
else
   echo "Found existing SSH keys:"
fi

ls -l $PRIVKEY
ls -l $PUBKEY
echo "Done."
echo

echo "##########################################################"
echo "# 14. Create crontab entries for automated data collection"
echo "##########################################################"
TODAY=`date +'%Y%m%d'`
if [ -f ../backup/$TODAY-crontab.backup ]; then
   echo "Found existing backup of /etc/crontab file:"
   ls -l ../backup/$TODAY-crontab.backup
else
   echo "Create new backup of current /etc/crontab file:"
   cp /etc/crontab ../backup/$TODAY-crontab.backup
   ls -l ../backup/$TODAY-crontab.backup
fi
echo

GREP=`grep $HOMEDIR/bin/solar-data.sh /etc/crontab`
if [[ $? > 0 ]]; then
   LINE1="##########################################################"
   sudo sh -c "echo \"$LINE1\" >> /etc/crontab"
   LINE2="# pi-solar: Get solar data in 1-min intervals, + upload"
   sudo sh -c "echo \"$LINE2\" >> /etc/crontab"
   LINE3="*  *    * * *   pi      $HOMEDIR/bin/solar-data.sh > $LOGHOME/solar-data.log 2>&1"
   sudo sh -c "echo \"$LINE3\" >> /etc/crontab"
   echo "Adding 3 lines to /etc/crontab file:"
   tail -4 /etc/crontab
else
   echo "Found solar-data.sh line in /etc/crontab file:"
   echo "$GREP"
fi
echo "Done."
echo

echo "##########################################################"
echo "# 15. Configuring local web server for data visualization"
echo "##########################################################"
GREP=`grep "/home/pi/" /etc/lighttpd/lighttpd.conf`
if [[ $? > 0 ]]; then
   sudo sh -c "sed -i -e 's/= \"\/var\/www\/html\"/= \"\/home\/pi\/pi-solar\/web\"/' /etc/lighttpd/lighttpd.conf"
   sudo lighty-enable-mod fastcgi
   sudo lighty-enable-mod fastcgi-php
   sudo systemctl restart lighttpd
else
   echo "Found HOMEDIR line in /etc/lighttpd/lighttpd.conf file:"
   echo "$GREP"
fi
echo "Done."
echo

echo "##########################################################"
echo "# 16. Installing local web server documents and images"
echo "##########################################################"
if [ ${MYCONFIG[pi-solar-int]} != "none" ]; then
   echo "cp ../web/images/mppt-75-10.jpg $WEBHOME/images"
   cp ../web/images/mppt-75-10.jpg $WEBHOME/images
   echo "chmod 644 $WEBHOME/images/mppt-75-10.jpg"
   chmod 644 $HOMEDIR/web/images/mppt-75-10.jpg

   echo "cp ../web/images/solar-system-01.jpg $WEBHOME/images"
   cp ../web/images/solar-system-01.jpg $WEBHOME/images
   echo "chmod 644 $WEBHOME/images/solar-system-01.jpg"
   chmod 644 $HOMEDIR/web/images/solar-system-01.jpg

   echo "cp ../web/solar.css $WEBHOME"
   cp ../web/solar.css $WEBHOME
   echo "chmod 644 $WEBHOME/solar.css"
   chmod 644 $WEBHOME/solar.css

   echo "cp ../web/solar.php $WEBHOME"
   cp ../web/solar.php $WEBHOME
   echo "chmod 644 $WEBHOME/solar.php"
   chmod 644 $WEBHOME/solar.php

   FIXVI=./ws-vmenu-update.vi
else
   echo "No pi-weather integration, using $WEBHOME"
fi

echo "Done."
echo

echo "##########################################################"
echo "# 17. For WS integration, update vmenu.htm with solar.php "
echo "##########################################################"

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [[ ! -f $FIXVI ]]; then
   echo "Error - cannot find VIM update file [$FIXVMENU]" >&2
   exit 1
fi

VMENU=$WEBHOME/vmenu.htm
GREP=`grep solar.php $VMENU`  
# ---------------------------------------------------------------
# This will catch previous install, and also catches 'local'
# mode if we don't integrate and already have those entries
# ---------------------------------------------------------------
if [[ $? == 0 ]]; then
   SOLARLINKFLAG=1
   echo "Found existing solar.php integration in $VMENU"
else
   echo "No solar.php link integration in $VMENU"
fi

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [ -z ${SOLARLINKFLAG+x} ]; then
   echo "Adding solar.php link to $VMENU"
   # -n disables vi's swap file for speed-up, but no backup exists
   echo "vi -n -s $FIXVI $VMENU"
   vi -n -s $FIXVI $VMENU
fi

echo "Done."
echo

echo "##########################################################"
echo "# 18. For WS integration add solar-data.log to showlog.php"
echo "##########################################################"
FIXVI=./ws-showlog-update.vi

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [[ ! -f $FIXVI ]]; then
   echo "Error - cannot find VIM update file [$FIXVMENU]" >&2
   exit 1
fi

SHOWLOG=$WEBHOME/showlog.php
GREP=`grep solar-data.log $SHOWLOG`
# ---------------------------------------------------------------
# This will catch previous install, and also catches 'local'
# mode if we don't integrate and already have those entries
# ---------------------------------------------------------------
if [[ $? == 0 ]]; then
   SOLARLINKFLAG=1
   echo "Found existing solar-data.log integration in $SHOWLOG"
else
   unset SOLARLINKFLAG
   echo "No solar-data.log integration in $SHOWLOG"
fi

if [ ${MYCONFIG[pi-solar-int]} != "none" ] && [ -z ${SOLARLINKFLAG+x} ]; then
   echo "Adding solar-data.log entry to $SHOWLOG"
   # -n disables vi's swap file for speed-up, but no backup exists
   echo "vi -n -s $FIXVI $SHOWLOG"
   vi -n -s $FIXVI $SHOWLOG
fi

echo "Done."
echo

echo "##########################################################"
echo "# 19. Create the SFTP batch files"
echo "##########################################################"
if [ ${MYCONFIG[pi-solar-int]} != "none" ]; then
   echo "Create the SFTP batch file for solar data upload"
   cat <<EOM >$HOMEDIR/etc/sftp-dat.bat
cd var
put $WEBHOME/getsolar.htm
put $LOGHOME/solar.txt
quit
EOM

   echo "Create the SFTP batch file for nightly XML-db upload"
   cat <<EOM >$HOMEDIR/etc/sftp-xml.bat
cd var
put $LOGHOME/solardb.xml.gz solardb.xml.gz.tmp
rename solardb.xml.gz.tmp solardb.xml.gz
quit
EOM
else
   echo "No pi-weather integration, skip SFTP batch files"
fi

echo "Done."
echo

echo "##########################################################"
echo "# 20. Create crontab entry for RRD XML file upload"
echo "##########################################################"

if [ ${MYCONFIG[pi-solar-int]} == "none" ]; then
   echo "No pi-weather integration, skip SFTP batch file"
fi

GREP=`grep $HOMEDIR/bin/solar-night.sh /etc/crontab`

if [[ $? > 0 ]] && [ ${MYCONFIG[pi-solar-int]} != "none" ]; then
   LINE1="##########################################################"
   sudo sh -c "echo \"$LINE1\" >> /etc/crontab"
   LINE2="# pi-solar: Upload RRD XML backup"
   sudo sh -c "echo \"$LINE2\" >> /etc/crontab"
   LINE3="40 0    * * *   pi      $HOMEDIR/bin/solar-night.sh > $LOGHOME/solar-night.log 2>&1"
   sudo sh -c "echo \"$LINE3\" >> /etc/crontab"
   echo "Adding 3 lines to /etc/crontab file:"
   tail -4 /etc/crontab
fi

GREP=`grep $HOMEDIR/bin/solar-night.sh /etc/crontab`

if [[ $? == 0 ]] && [ ${MYCONFIG[pi-solar-int]} != "none" ]; then
   echo "Found solar-night.sh line in /etc/crontab file:"
   echo "$GREP"
else
   echo "Cannot find solar-night.sh line in /etc/crontab file"
fi

echo "Done."
echo

echo "##########################################################"
echo "# End of pi-solar Installation. Review the script output. "
echo "# Please reboot the system to enable all changes. COMPLETE"
echo "##########################################################"
