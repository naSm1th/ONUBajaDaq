#!/bin/sh

./mount.sh
more /mnt/bajadaq/1*/
read ok
rm -rf /mnt/bajadaq/1*/
./mount.sh
