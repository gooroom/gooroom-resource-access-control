#!/bin/bash

# Disallow the usb storage	 
/usr/bin/eject "/dev/$1"
	
# Write Syslog
/usr/bin/logger -t "grac" "USB-Storage-Disallow (/dev/$1)"

# Print Messsage Box
export DISPLAY=:0
#export XAUTHORITY=/home/ultract/.Xauthority
export XAUTHORITY="/home/$(users | awk '{print $1}')/.Xauthority"
/usr/bin/zenity --warning --title "Warning" --text "USB-Storage-Disallow (/dev/$1)"
#/usr/bin/logger "$(/usr/bin/env)"

# Write Syslog
# /usr/bin/logger -t "gooroom-resource-access-control" "$1"

# Print Messsage Box
# /usr/bin/zenity --info --text "$1"

