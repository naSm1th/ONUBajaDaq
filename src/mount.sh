#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

DIR="/mnt/bajadaq"
log_level="mount"

if [ $# -eq 0 ]; then
    # no cl arg
    echo "$log_level: please specify mount or umount"
else
    if [ $1 = "-m" ]; then
        # mount
        sudo mkdir "$DIR"
        sudo mount -t vfat -o uid=logger,gid=logger /dev/sda1 $DIR &> /dev/null
        # check for failure 
        if ! mount | grep "$DIR" > /dev/null; then
            sudo rm -rf "$DIR" &> /dev/null
            echo "$log_level: could not mount"
            exit 1 # return error
        fi
    elif [ $1 = "-u" ]; then
        # unmount
        sudo umount "$DIR" &> /dev/null
        # check for success
        if ! mount | grep "$DIR" > /dev/null; then
            sudo rm -rf "$DIR" &> /dev/null
        else
            exit 1 # return error
        fi
    else
        # invalid cl arg
        echo "Usage:\n-m: mount\n-u: umount"
    fi
fi
