#!/bin/sh

./mount.sh
echo "Are you sure you want to remove the following?"
ls /mnt/bajadaq/*/
read ok
rm -rf /mnt/bajadaq/*/
./mount.sh
