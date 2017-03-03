#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

prog="daq"
mount="./mount.sh"
wait4bp="python waitforpress.py"

# listen for button press
$wait4bp
echo "Logging session initiated"
# mount USB
$mount
status=$?
if [ $status -eq 0 ]; then # if success
    if [ -f $prog ]; then
        "./$prog" &
        pid=$!
        # listen for button press
        $wait4bp
        kill -2 $pid
        wait $pid
        # umount usb
        $mount &
        pid=$!
        # wait for umount to finish
        wait $pid
        if [ $? -eq 0 ]; then # if success
            echo "Logging session terminated"
        else
            $mount
        fi
    else
        echo "$prog does not exist"
        $mount
    fi
else
    echo "Mounting error"
fi
