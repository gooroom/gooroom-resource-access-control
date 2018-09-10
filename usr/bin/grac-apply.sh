#!/bin/bash

#
# parameter uasage 
#    $1  resource name  	resouce.interface:subsystem : (USB, printer,........) (usb, bluetooth) 
#    $2  permission     	allow, disallow,.......
#    $3  method				eject, authority, hcitool....
#    $4  devicename		%k in the udev rules
#    $5  device path		$devpath in the udev rules
#

RESNAME="$1" #`echo ${RESINFO} | awk -F'.' '{print $1}'`
RESPERM="$2"


RESNAME=${RESNAME^^}
RESPERM=${RESPERM,,}

#
# check environment
#

USERNAME="$(/usr/bin/users | /usr/bin/awk 'NR==1 {print}')"

#
# Write the syslog message
#  $1 - message
#
out_log()
{
	/usr/bin/logger --id=$$ -p 3 -t "GRAC" "$1"
}


#
# Display the message to user
#  $1 - message
#
show_msg()
{
	if [ "${USERNAME}" != "" ]; then
		/bin/su "$USERNAME" -c "/usr/bin/notify-send -i error 'GRAC - Warning'  '$1'"
	fi
}


# make log and display message

LOGMSG="GRAC: cause=\"${RESPERM}\" kind=\"${RESNAME}\" device=\"\""

if [ "${RESPERM}" = "disallow" ] ; then
   MSG="가(이) 차단되었습니다."
	out_log  "${LOGMSG}"
	show_msg "${RESNAME}${MSG}"
	exit 0
fi

if [ "${RESPERM}" = "read_only" ] ; then
   MSG="가(이)  읽기 전용으로 설정되었습니다."
	out_log  "${LOGMSG}"
	show_msg "${RESNAME}${MSG}"
	exit 0
fi


