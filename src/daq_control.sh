#!/bin/bash

# daq_control.sh - start and monitor the DAQ process 
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

# Modes
PROD=0
DEV=1
DEBUG=2

mode=$DEV                  # controls logging
log_level="onubajadaq"     # name of main program
daqc="daq"                 # name of executable c file

# listen for button press
python waitforpress.py
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$log_level: Logging session initiated"
fi
# change baud rate
if [ $mode = $DEBUG ]; then
    echo "$log_level: Changing baud rate"
fi
echo -e "\$PMTK251,115200*1F\r\n" > /dev/serial0
stty -F /dev/serial0 115200
sleep 5
# change update rate
if [ $mode = $DEBUG ]; then
    echo "$log_level: Changing update rate"
fi
echo -e "\$PMTK220,100*2F\r\n" > /dev/serial0
# start gpsd
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$log_level: Starting gpsd"
fi
sudo gpsd /dev/serial0 -b -F /var/run/gpsd.sock
# tell gpsd about the UR change
if [ $mode = $DEBUG ]; then
    echo "$log_level: Telling gpsd about BR change"
fi
gpsctl -s 115200
if [ $mode = $DEBUG ]; then
    echo "$log_level: Telling gpsd about UR change"
fi
gpsctl -c 0.1
# wait for 3 seconds
if [ $mode = $DEBUG ]; then
    echo "$log_level: Waiting for changes to take effect"
fi
sleep 3
# mount USB
echo "$log_level: Mounting USB" 
"./mount.sh"
status=$?
if [ $status -eq 0 ]; then # if success
    if [ -f $daqc ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Starting logger" 
        fi
        "./$daqc" &
        pid=$!
        # listen for button press
        # "python waitforpress.py"
        read stop
        # check to see if daqc is still running
        if ps -p $pid > /dev/null
        then 
            if [ $mode = $DEBUG ]; then
                echo "$log_level: $daqc already terminated"
            fi
            kill -2 $pid
            wait $pid
            if [ $? -eq 0 ]; then # if success
                "./mount.sh" # umount usb
                if [ $? -eq 0 ]; then # if success
                    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                        echo "$log_level: Logging session terminated"
                    fi
                else
                    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                        echo "$log_level: error in mount"
                    fi
                    # LED ERROR
                fi
            else
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$log_level: error in $daqc"
                fi
                "./mount.sh" # umount usb
                # LED ERROR
            fi
        else
            "./mount.sh" # umount usb
            if [ $? -eq 0 ]; then # if success
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$log_level: Logging session terminated"
                fi
            else
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$log_level: error in mount"
                fi
                # LED ERROR
            fi
        fi
    else
        echo "$log_level: $daqc does not exist"
        "./mount.sh" # umount usb
        # LED ERROR
    fi
else
    # show error on LED
    echo "$log_level: error in mount"
fi
# stop gpsd
sudo killall gpsd
