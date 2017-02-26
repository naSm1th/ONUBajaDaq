#!/bin/bash

# mount.sh - mount and unmount USB flash drive
# Author: Ryan Carl

# This file is part of the Baja DAQ System for Olivet Nazarene University
# Copyright 2017 Nathanael A. Smith & Ryan Carl
# License: MIT License (see LICENSE for more details)

DIR="/mnt/bajadaq"

if [ ! -d "$DIR" ]; then
    # mount
    sudo mkdir "$DIR"
    sudo mount -t vfat -o uid=logger,gid=logger /dev/sda1 $DIR
    # check for failure 
    if ! mount | grep "$DIR"> /dev/null; then
        sudo rm -rf "$DIR"
    fi
else
    # unmount
    sudo umount "$DIR"
    # check for success
    if ! mount | grep "$DIR"> /dev/null; then
        sudo rm -rf "$DIR"
    fi
fi
