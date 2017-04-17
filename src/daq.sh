#!/bin/bash

# daq_control.sh - start and monitor the DAQ process (runs on RPi boot) 
# Contributors: Ryan Carl, Nathanael Smith

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

# Modes
PROD=0
DEV=1
DEBUG=2

mode=$DEBUG             # error/alert reporting 
name="onubajadaq"       # name of main program
daqc="logger"           # name of executable c file
fix=1                   # gps fix flag

# one of these will be called on exit
function finish1 {
    # show error on LED (exited early)
    ./updateled.py red

    ./mount.sh -u
    if [ $? -ne 0 ]; then # if exit code non-zero 
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$name: error in mount"
        fi
        # show error on LED
        ./updateled.py red
    fi

    # pause to show error 
    sleep 10
    # turn off LED 
    ./updateled.py
}

function finish2 {
    # program ran successfully,
    # errored out,
    # or was terminated via cmd line
    ./mount.sh -u
    if [ $? -ne 0 ]; then # if exit code non-zero 
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$name: error in mount"
        fi
        # show error on LED
        ./updateled.py red
        # pause to show error 
        sleep 10
    fi

    # stop gpsd
    sudo killall gpsd

    # turn off LED 
    ./updateled.py
}

function logger_ready {
    fix=1
    python waitforpress.py &
    button_pid=$!
}

# show program execution start
# since running on boot
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "\n\n------ Baja Data Acquisition ------\n"
fi
# clear LED if lit
./updateled.py

# check for c file
if [ ! -f $daqc ]; then
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$name: $daqc does not exist\nTerminating program..."
    fi
    # show error on LED 
    ./updateled.py red
    # pause to show error 
    sleep 10
    ./updateled.py
    exit
fi

# mount removable storage 
if [ $mode = $DEBUG ]; then
    echo "$name: Mounting storage" 
fi
./mount.sh -m
mnt_status=$?
# make sure removable storage is mounted
if [ $mnt_status -ne 0 ]; then
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$name: removable storage not found\nTerminating program..."
    fi
    # show error on LED
    ./updateled.py red
    # pause to show error 
    sleep 10
    ./updateled.py
    exit
fi

# set inital exit handler
trap finish1 EXIT

# change baud rate
if [ $mode = $DEBUG ]; then
    echo "$name: Changing baud rate"
fi
echo -e "\$PMTK251,115200*1F\r\n" > /dev/serial0
stty -F /dev/serial0 115200
# wait for BR to change
sleep 5

# change update rate
if [ $mode = $DEBUG ]; then
    echo "$name: Changing update rate"
fi
echo -e "\$PMTK220,100*2F\r\n" > /dev/serial0

# start gpsd
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$name: Starting gpsd"
fi
sudo gpsd /dev/serial0 -b -F /var/run/gpsd.sock

# communicate the BR & UR changes to gpsd
if [ $mode = $DEBUG ]; then
    echo "$name: Telling gpsd about BR change"
fi
gpsctl -s 115200
if [ $mode = $DEBUG ]; then
    echo "$name: Telling gpsd about UR change"
fi
gpsctl -c 0.1

# replace exit handler
trap finish2 EXIT

# wait for 3 seconds
if [ $mode = $DEBUG ]; then
    echo "$name: Waiting for changes to take effect"
fi
sleep 3

# trap update for led
trap logger_ready SIGUSR1

while [ 1 ]; do
    # set flags
    success=1
    fix=0

    # listen for button press
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$name: Waiting for button press"
    fi
    # ready for input
    ./updateled.py blue 
    python waitforpress.py

    # quit if button is held
    if [ $? -eq 1 ]; then
        break
    fi

    # start logger
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$name: Run initiated"
    fi
    ./updateled.py red 
    if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
        echo "$name: Starting logger" 
    fi
    "./$daqc" "$$" &
    logger_pid=$!
    
    # waitforpress and button_pid in trap function

    # give logger 10 seconds to get set up
    s=0
    while [ $((s++)) -lt 10 -a $fix -eq 0 ]; do
        sleep 1
    done
    
    if [ $fix -ne 1 ]; then
        success=0
    else
        # ready to log
        ./updateled.py green

        # wait for button press or logger termination (error)
        wait -n

        # check to see if waitforpress is still running
        if ps -p $button_pid > /dev/null
        then
            # check to see if daqc is still running
            if ps -p $logger_pid > /dev/null
            then 
                if [ $mode = $DEBUG ]; then
                    echo "$name: $daqc still running"
                fi
            else
                if [ $mode = $DEBUG ]; then
                    echo "$name: $daqc terminated early"
                fi
            fi
            kill -2 $button_pid
            wait $button_pid
            success=0
        fi

        # check to see if daqc is still running
        if ps -p $logger_pid > /dev/null
        then 
            if [ $mode = $DEBUG ]; then
                echo "$name: Terminating $daqc"
            fi
            kill -2 $logger_pid
            wait $logger_pid
            if [ $? -ne 0 ]; then
                if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
                    echo "$name: error in $daqc"
                fi
                success=0
            fi
        fi
    fi

    # check for successful execution
    if [ $fix -eq 0 ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$name: No GPS fix"
        fi
        # show error on LED
        ./updateled.py red    
        # pause to show error 
        sleep 10
    elif [ $success -ne 0 ]; then
        if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
            echo "$name: Run terminated successfully"
        fi
    else
        # show error on LED
        ./updateled.py red    
        # pause to show error 
        sleep 10
    fi
done

# finished logging
if [ $mode = $DEBUG ] || [ $mode = $DEV ]; then
    echo "$name: Logging session complete"
fi
