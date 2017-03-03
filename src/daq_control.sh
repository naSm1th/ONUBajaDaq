#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

log_level="onubajadaq"     # name of main program
daqc="daq"        # name of executable c file
mount="mount.sh"  # command for usb mount script
# command for python GPIO button script
wait4bp="python waitforpress.py"

# listen for button press
$wait4bp
echo "$log_level: Logging session initiated"
# mount USB
$mount
status=$?
if [ $status -eq 0 ]; then # if success
    if [ -f $daqc ]; then
        "./$daqc" &
        pid=$!
        # listen for button press
        # $wait4bp
        read stop
        kill -2 $pid
        wait $pid
        if [ $? -eq 0 ]; then # if success
            "./$mount" # umount usb
            if [ $? -eq 0 ]; then # if success
                echo "$log_level: Logging session terminated"
            else
                echo "$log_level: error in $mount"
            fi
        else
            # show error on LED
            echo "$log_level: error in $daqc"
            "./$mount" # umount usb
        fi
    else
        # show error on LED
        echo "$log_level: $daqc does not exist"
        "./$mount" # umount usb
    fi
else
    # show error on LED
    echo "$log_level: Mounting error"
fi
