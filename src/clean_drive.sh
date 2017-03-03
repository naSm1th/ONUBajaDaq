#!/bin/sh

./mount.sh
echo "Are you sure you want to remove the following?"
ls /mnt/bajadaq/*/
read ok
while true; do
    case $ok in
        [Yy]* ) rm -rf /mnt/bajadaq/*/; ./mount.sh; break;;
        [Nn]* ) exit;;
        * ) echo "(y)es or (n)o";;
    esac
done
