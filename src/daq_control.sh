#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

daqsh="onubajadaq"     # name of main program
daqc="daq"        # name of executable c file
mount="./mount.sh"  # command for usb mount script
# command for python GPIO button script
wait4bp="python waitforpress.py"

# listen for button press
$wait4bp
echo "$daqsh: Logging session initiated"
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
        # umount usb
        $mount &
        pid=$!
        # wait for umount to finish
        wait $pid
        if [ $? -eq 0 ]; then # if success
            echo "$daqsh: Logging session terminated"
        else
            $mount
        fi
    else
        echo "$daqsh: $daqc does not exist"
        $mount
    fi
else
    echo "$daqsh: Mounting error"
fi
