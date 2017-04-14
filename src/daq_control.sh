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
success=1                  # error detect flag

# one of these will be called on exit
function finish1 {
    # show error on LED (exited early)
    python updateled.py red

    # stop gpsd
    sudo killall gpsd
}

function finish2 {
    # program ran successfully,
    # errored out,
    # or was terminated via cmd line
    ./mount.sh -u
    if [ $? -ne 0 ]; then # if exit code non-zero 
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: error in mount"
            # show error on LED
            python updateled.py red
        fi
    fi

    # stop gpsd
    sudo killall gpsd

    # check for successful execution
    if [ $success -eq 0 ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: error"
        fi
        # show error on LED
        python updateled.py red
    else
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Logging session terminated"
        fi
        # LED SUCCESS
        python updateled.py green
    fi
}

# change baud rate
if [ $mode = $DEBUG ]; then
    echo "$log_level: Changing baud rate"
fi
echo -e "\$PMTK251,115200*1F\r\n" > /dev/serial0
stty -F /dev/serial0 115200
# wait for BR to change
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

# set inital exit handler
trap finish1 EXIT

# communicate the BR & UR changes to gpsd
if [ $mode = $DEBUG ]; then
    echo "$log_level: Telling gpsd about BR change"
fi
gpsctl -s 115200
if [ $mode = $DEBUG ]; then
    echo "$log_level: Telling gpsd about UR change"
fi
gpsctl -c 0.1

# mount removable storage 
if [ $mode = $DEBUG ]; then
    echo "$log_level: Mounting storage" 
fi
./mount.sh -m
mnt_status=$?

# replace exit handler
trap finish2 EXIT

# wait for 3 seconds
if [ $mode = $DEBUG ]; then
    echo "$log_level: Waiting for changes to take effect"
fi
sleep 3

# make sure removable storage is mounted
if [ $mnt_status -eq 0 ]; then
    if [ -f $daqc ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Starting logger" 
        fi
        # trap update for led
        trap "python updateled.py green" SIGUSR1
        python updateled.py red

        # listen for button press
        python waitforpress.py

        # start logger
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: Logging session initiated"
        fi
        python updateled.py blue
        "./$daqc" "$$" &
        logger_pid=$!
        # wait for feedback before listening again
        sleep 5
        python waitforpress.py &
        button_pid=$!
        # wait for button press or logger termination (error)
        wait -n
        # check to see if waitforpress is still running
        if ps -p $button_pid > /dev/null
        then
            # waitforpress is still running, daqc terminated early
            if [ $mode = $DEBUG ]; then
                echo "$log_level: $daqc terminated early"
            fi
            kill -2 $button_pid
            wait $button_pid
            # show error on LED 
            python updateled.py red
            success=0
        fi
        # check to see if daqc is still running
        if ps -p $logger_pid > /dev/null
        then 
            if [ $mode = $DEBUG ]; then
                echo "$log_level: Terminating $daqc"
            fi
            kill -2 $logger_pid
            wait $logger_pid
            if [ $? -ne 0 ]; then
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$log_level: error in $daqc"
                fi
                # show error on LED 
                python updateled.py red
                success=0
            fi
        fi
    else
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$log_level: $daqc does not exist"
        fi
        # show error on LED 
        python updateled.py red
        success=0
    fi
else
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$log_level: error in mount"
    fi
    # show error on LED
    python updateled.py red
    success=0
fi
