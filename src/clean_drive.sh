#!/bin/sh

./mount.sh
echo "Are you sure you want to remove the following?"
ls /mnt/bajadaq/1*/
read ok
while true; do
    case $ok in
        [Yy]* ) rm -rf /mnt/bajadaq/1*/; ./mount.sh; break;;
        [Nn]* ) ./mount.sh; exit;;
        * ) echo "(y)es or (n)o";;
    esac
done
