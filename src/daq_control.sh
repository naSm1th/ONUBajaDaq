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

mode=$DEV                # controls logging
log_level="onubajadaq"     # name of main program
daqc="daq"                 # name of executable c file
success=1                  # error detect flag

function finish1 {
    ./mount.sh -u
    if [ $? -ne 0 ]; then # if exit code non-zero 
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: error in mount"
        fi
        success=0
        # LED ERROR
    fi
}

function finish2 {
    ./mount.sh -u
    if [ $? -ne 0 ]; then # if exit code non-zero 
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: error in mount"
        fi
        success=0
        # LED ERROR
    fi

    # stop gpsd
    sudo killall gpsd
}

# set exit handler
trap finish1 EXIT

# check if removable memory can be mounted
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$log_level: Mounting USB" 
fi
./mount.sh -m
status=$?
if [ $status -eq 0 ]; then
    if [ $mode = $DEBUG ]; then
        echo "$log_level: USB mounted" 
    fi
fi

# listen for button press
python waitforpress.py
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$log_level: Logging session initiated"
fi

# if removable storage did not mount before
if [ $status -ne 0 ]; then
    ./mount.sh -m
    status=$?
fi

# check if removable storage is mounted
if [ $status -eq 0 ]; then
    if [ $mode = $DEBUG ]; then
        echo "$log_level: USB already mounted" 
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

    # set second exit handler
    trap finish2 EXIT
    
    # tell gpsd about the BR change
    if [ $mode = $DEBUG ]; then
        echo "$log_level: Telling gpsd about BR change"
    fi
    gpsctl -s 115200

    # tell gpsd about the UR change
    if [ $mode = $DEBUG ]; then
        echo "$log_level: Telling gpsd about UR change"
    fi
    gpsctl -c 0.1

    # wait for 3 seconds
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$log_level: Waiting for changes to take effect"
    fi
    sleep 3

    if [ -f $daqc ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Starting logger" 
        fi

        "./$daqc" &
        pid=$!
        # listen for button press
        python waitforpress.py

        # wait for button press or daqc stop
        # wait -n
        # check to see if daqc is still running
        if ps -p $pid > /dev/null
        then 
            if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                echo "$log_level: terminating session..." 
            fi
            kill -2 $pid
            wait $pid
            exit_status=$?
            if [ $exit_status -ne 0 ]; then # if exit code non-zero 
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$log_level: error in $daqc"
                fi
                success=0
                # LED ERROR
            fi
        else # daqc already terminated
            if [ $exit_status -ne 0 ]; then # if exit code non-zero
                success=0
                # LED ERROR
            fi
        fi
        #else
        #    if [ $exit_status -ne 0 ]; then # if exit code non-zero
        #        success=0
        #        # LED ERROR
        #    fi
        #fi
    else
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: $daqc does not exist"
        fi
        success=0
        # LED ERROR
    fi
    if [ $success -eq 0 ]; then
        # LED ERROR
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: error"
        fi
    else
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Logging session terminated"
        fi
        # LED SUCCESS
    fi
else
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$log_level: error in mount"
    fi
    # LED ERROR
fi
