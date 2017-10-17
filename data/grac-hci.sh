#!/bin/bash

#
# parameter uasage 
#    $1  command			allow, disallow
#    $2  address			MAC  
#

COMMAND="${1^^}"
MACADDR="${2^^}"

MACADDR="$(/bin/echo ${MACADDR} | /usr/bin/tr [.] [:])"

#
# Write the syslog message
#  $1 - message
#
out_log()
{
	/usr/bin/logger -t "[GRAC]" "app=\"grac-hci.sh\" msg=\"$1\""
}

if [ "${COMMAND}" = "DISALLOW" ]; then
	# can't run 'hcitoo' in udev mode
	#	echo "--> /usr/bin/hcitool dc ${MACADDR}" >> /etc/gooroom/grac.d/recover.udev
	#	/usr/bin/hcitool dc ${MACADDR}

	echo "--> echo disconnect ${MACADDR} | bluetoothctl" >> /etc/gooroom/grac.d/recover.udev
	out_log  "exec : echo disconnect ${MACADDR} | bluetoothctl"

	echo -e "disconnect ${MACADDR} \nquit" | /usr/bin/bluetoothctl

	exit 0
fi


if [ "${COMMAND}" = "ALLOW" ]; then
	# can't run 'hcitoo' in udev mode
	#	echo "--> /usr/bin/hcitool cc ${MACADDR}" >> /etc/gooroom/grac.d/recover.udev
	#	/usr/bin/hcitool cc ${MACADDR}

	echo "--> echo connect ${MACADDR} | bluetoothctl" >> /etc/gooroom/grac.d/recover.udev
	out_log  "exec : echo connect ${MACADDR} | bluetoothctl"

	echo -e "connect ${MACADDR} \nquit" | /usr/bin/bluetoothctl

	exit 0
fi


# can't run 'hcitoo' in udev mode
#while read DEVICE
#do
#	echo "/usr/bin/hcitool -i ${DEVICE} dc ${MACADDR}" >> /etc/gooroom/grac.d/recover.udev
#done <<< "$(/usr/bin/hcitool dev | /usr/bin/awk 'NR>=2 {print $1}')"
#

