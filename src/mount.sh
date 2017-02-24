#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

DIR="/media/bajadaq"

if [ ! -d "$DIR" ]; then
    # mount
    msg="$(sudo mount -t vfat -o uid=logger,gid=logger /dev/sda1 $DIR)"
    echo "$msg"
else
    # unmount
    msg="$(sudo unmount $DIR)"
    echo "$msg"
fi
