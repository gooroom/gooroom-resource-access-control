#!/bin/bash

#
# parameter uasage 
#    $1  resource name  	resouce.interface : (USB, printer,........) (usb, bluetooth) 
#    $2  permission     	allow, disallow,.......
#    $3  method				eject, authority, hcitool....
#    $4  devicename		%k in the udev rules
#    $5  device path		$devpath in the udev rules
#

RESINFO="$1"
RESNAME=`echo $1 | awk -F'.' '{print $1}'`
INFTYPE=`echo $1 | awk -F'.' '{print $2}'`
RESPERM="$2"
METHOD="$3"
DEVNAME="$4"
DEVPATH="$5"

RESNAME=${RESNAME,,}
RESPERM=${RESPERM,,}

#
# check environment
#

USERNAME="$(/usr/bin/users | /usr/bin/awk 'NR==1 {print}')"

#DISPNAME="unknown"
#if [ "${USERNAME}" != "" ]; then
#	while read ddd
#	do
#		DISPNAME="${ddd}"
#	done <<< "$(who | awk 'NR==1 {print $NF}' | tr -d '()')"
#
#	export XAUTHORITY=/home/${USERNAME}/.Xauthority
#	export DISPLAY="${DISPNAME}"
#fi

#
# Write the syslog message
#  $1 - message
#
out_log()
{
	/usr/bin/logger -t "[GRAC]" "app=\"GRAC\" msg=\"$1\""
}


#
# Display the message to user
#  $1 - message
#
show_msg()
{
	if [ "${USERNAME}" != "" ]; then
#		/usr/bin/zenity --warning --title "GRAC - Warning" --text "$1"
	
#		/sbin/runuser -u "$USERNAME" -- /usr/bin/notify-send -i error "GRAC - Warning"  "$1"
		/bin/su "$USERNAME" -c "/usr/bin/notify-send -i error 'GRAC - Warning'  '$1'"
	fi
}


# --- save control information --
echo  "${RESINFO}" "${RESPERM}" "${METHOD}" "${DEVNAME}" "${DEVPATH}"  >> /etc/gooroom/grac.d/recover.udev


# make log and display message

if [ "${RESPERM}" = "disallow" ] ; then
   MSG="가(이) 차단되었읍니다."
	out_log  "Disallowed ${RESINFO} : ${DEVPATH}"
	show_msg "${RESINFO}${MSG}"
	exit 0
fi

if [ "${RESPERM}" = "read_only" ] ; then
   MSG="가(이) 읽기 전용으로 설정되었읍니다."
	out_log  "Set Read-only mode ${RESINFO} : ${DEVPATH}"
	show_msg "${RESINFO}${MSG}"
	exit 0
fi


