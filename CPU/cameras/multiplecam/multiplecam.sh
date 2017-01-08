#!/bin/bash          
          
echo ST Multiple Cameras Manager
PROGR="/home/minieusouser/CPU/cameras/multiplecam/bin" 
# create a new directory to store images
DATE=`date +%Y-%m-%d:%H:%M:%S`
mkdir -p $DATE
cd $DATE
# launch main data acquisition program
$PROGR/multiplecam  | tee -a log-$DATE.log
 

