#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

prog="daq"
mount="./mount.sh"

# listen for button press
read start

if [ -n start ]; then
    echo "Logging session initiated"
    $mount &
    pid=$!
    # wait for mount to finish
    wait $pid
    if [ $? -eq 0 ]; then # if success
        if [ -f prog ]; then
            "./$prog"
        else
            echo "$prog does not exist"
        fi
    else
        echo "Mounting error"
    fi
else
    echo "Logging session aborted"
fi
