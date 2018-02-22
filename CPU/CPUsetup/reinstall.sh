#!/bin/bash

# CPU REINSTALL SCRIPT for Mini-EUSO
# Francesca Capel February 2018
# capel.francesca@gmail.com
# Mini-EUSO software repository: https://github.com/cescalara/minieuso_cpu
# Please see documentation for correct setup procedure

HOME_DIR="/home/software/CPU"
ZYNQ_IP=192.168.7.10
CPU_IP=192.168.7.2
TEST_IP=8.8.8.8

echo "************************"
echo "Mini-EUSO CPU reinstall"
echo "***********************"

# Set up the Mini-EUSO software
mkdir $HOME_DIR/CPU/CPUsoftware/log > /dev/null 2>&1
make -C $HOME_DIR/CPU/CPUsoftware/lib > /dev/null 2>&1
make -C $HOME_DIR/CPU/CPUsoftware > /dev/null 2>&1
echo "Mini-EUSO software is set up"

# Set up the cameras 
echo "Setting up the camera software..."
chmod +x $HOME_DIR/cameras/flycapture2-2.3.2.14-amd64/install_flycapture.sh
(cd $HOME_DIR/cameras/flycapture2-2.3.2.14-amd64 && sh install_flycapture.sh)
#sh -c 'echo 1000 > /sys/module/usbcore/parameters/usbfs_memory_mb'
mkdir $HOME_DIR/cameras/multiplecam/bin > /dev/null 2>&1
chmod +x $HOME_DIR/cameras/multiplecam/src/install.sh
(cd $HOME_DIR/cameras/multiplecam/src && sh -c './install.sh $HOME_DIR/cameras/multiplecam')
echo "Camera software is set up"
