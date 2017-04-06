#!/bin/bash

# daq_control.sh - start and monitor the DAQ process 
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

log_level="onubajadaq"     # name of main program
daqc="daq"                 # name of executable c file

# listen for button press
python waitforpress.py
echo "$log_level: Logging session initiated"
# change baud rate
echo "$log_level: Changing baud rate"
echo -e "\$PMTK251,115200*1F\r\n" > /dev/serial0
stty -F /dev/serial0 115200
sleep 5
# change update rate
echo "$log_level: Changing update rate"
echo -e "\$PMTK220,100*2F\r\n" > /dev/serial0
# start gpsd
echo "$log_level: Starting gpsd"
sudo gpsd /dev/serial0 -b -F /var/run/gpsd.sock
# tell gpsd about the UR change
echo "$log_level: Telling gpsd about BR change"
gpsctl -s 115200
echo "$log_level: Telling gpsd about UR change"
gpsctl -c 0.1
# wait for 5 seconds
echo "$log_level: Waiting..."
sleep 5
# mount USB
echo "$log_level: Mounting USB" 
"./mount.sh"
status=$?
if [ $status -eq 0 ]; then # if success
    if [ -f $daqc ]; then
        echo "$log_level: Starting logger" 
        "./$daqc" &
        pid=$!
        # listen for button press
        # "python waitforpress.py"
        read stop
        if ps -p $pid > /dev/null
        then 
            kill -2 $pid
            wait $pid
            if [ $? -eq 0 ]; then # if success
                "./mount.sh" # umount usb
                if [ $? -eq 0 ]; then # if success
                    echo "$log_level: Logging session terminated"
                else
                    echo "$log_level: error in mount"
                fi
            else
                # show error on LED
                echo "$log_level: error in $daqc"
                "./mount.sh" # umount usb
            fi
        else
            "./mount.sh" # umount usb
            if [ $? -eq 0 ]; then # if success
                echo "$log_level: Logging session terminated"
            else
                echo "$log_level: error in mount"
            fi
        fi
    else
        # show error on LED
        echo "$log_level: $daqc does not exist"
        "./mount.sh" # umount usb
    fi
else
    # show error on LED
    echo "$log_level: error in mount"
fi
# stop gpsd
sudo killall gpsd
