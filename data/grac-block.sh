#!/bin/bash

#
# parameter uasage 
#    $1  permission     allow, disallow, read_only
#    $2  device_type		disk, partition
#    $3  device_name	   /dev/%k in the udev rules
#

RESPERM="$1"
DEVTYPE="$2"
DEVPATH="$3"

DEVTYPE=${DEVTYPE,,}
RESPERM=${RESPERM,,}

#
# Write the syslog message
#  $1 - message
#
out_log()
{
	/usr/bin/logger --id=$$ -p 3 -t "GRAC" "$1"
}

echo "grac-block.sh : start" >> /var/log/gooroom/grac.log

if [ "${RESPERM}" = "disallow" ] ; then
	echo "grac-block.sh : try grac-tail" >> /var/log/gooroom/grac.log
   SYSNAME="$(echo ${DEVPATH} | /usr/bin/cut -d '/' -f 3)"
	echo "${SYSNAME}" >> /etc/gooroom/grac.d/blocks/block-list
	exit 0
fi

if [ "${RESPERM}" = "read_only" ] ; then
	echo "grac-block.sh : try grac-tail" >> /var/log/gooroom/grac.log
   	SYSNAME="$(echo ${DEVPATH} | /usr/bin/cut -d '/' -f 3)"
	echo "${SYSNAME}" >> /etc/gooroom/grac.d/blocks/block-list
	exit 0
fi

exit 0


if [ "${RESPERM}" = "disallow" ] ; then
		#  1st try : eject
		/usr/bin/eject ${DEVPATH}
		echo "grac-block.sh : try eject" >> /var/log/gooroom/grac.log

		# if not eject, unmount all partitions, and power off the disk
		BLOCKLIST="$(/bin/lsblk -n -p -l ${DEVPATH} | /bin/grep part)"
		if [ $? = 0 ]; then
			while read line
			do
				PARTDEV="$(echo ${line} | /usr/bin/awk '{print $1}')"
				MOUNTPOS="$(echo ${line} | /usr/bin/awk '{print $7}')"
				echo "grac-block.sh : ${line}" >> /var/log/gooroom/grac.log
				echo "grac-block.sh : ${PARTDEV} ${MOUNTPOS}" >> /var/log/gooroom/grac.log
				if [ "${MOUNTPOS}" != "" ] ; then
					/usr/bin/udisksctl unmount -b ${PARTDEV}
				else  
					echo "grac-block.sh : not yet mounted ${PARTDEV}" >> /var/log/gooroom/grac.log
				fi
			done <<< ${BLOCKLIST}

			/usr/bin/udisksctl power-off -b ${DEVPATH}  
			STATUS="$?"
			if [ ${STATUS} != "0" ]; then
				echo "grac-block.sh : power off error ${STATUS}" >> /var/log/gooroom/grac.log
			fi
		else
			echo "grac-block.sh : not found list" >> /var/log/gooroom/grac.log
		fi
		exit 0
	fi


	if [ "${DEVTYPE}" = "partition" ] ; then

		devlen=${#DEVPATH}-1
		DEVPATH="${DEVPATH:0:devlen}"

		#  1st try : eject
		/usr/bin/eject ${DEVPATH}
		echo "grac-block.sh : try eject by partition" >> /var/log/gooroom/grac.log

		# if not eject, unmount all partitions, and power off the disk
		BLOCKLIST="$(/bin/lsblk -n -p -l ${DEVPATH} | /bin/grep part)"
		if [ $? = 0 ]; then
			while read line
			do
				PARTDEV="$(echo ${line} | /usr/bin/awk '{print $1}')"
				MOUNTPOS="$(echo ${line} | /usr/bin/awk '{print $7}')"
				if [ "${MOUNTPOS}" != "" ] ; then
					/usr/bin/udisksctl unmount -b ${PARTDEV}
				else  
					echo "grac-block.sh : not yet mounted ${PARTDEV}" >> /var/log/gooroom/grac.log
				fi
			done <<< ${BLOCKLIST}

			/usr/bin/udisksctl power-off -b ${DEVPATH}  
			STATUS="$?"
			if [ ${STATUS} != "0" ]; then
				echo "grac-block.sh : power off error ${STATUS}" >> /var/log/gooroom/grac.log
			fi
		else
			echo "grac-block.sh : not found list" >> /var/log/gooroom/grac.log
		fi
		exit 0
	fi

	exit 0
fi


