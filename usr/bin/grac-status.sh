#!/bin/bash

# parameter
#    $1 action  - 'started'  'stopped'
#    'started'
#			$2 - service proces-id   from systemd $MAINPID
#    'stopped'
#			$2 - result      (string)  from systemd $SERVICE_RESULT
#			$3 - exit_status (string)  from systemd $EXIT_CODE
#			$4 - exit_status (number)  from systemd $EXIT_STATUS


set -e

ACTION="${1,,}"
RESULT="${2,,}"
EXITCODE="$4"

#
# Write the syslog message
#  $1 - error code
#
out_log()
{
	/usr/bin/logger --id=$$ -p 3 -t GRAC "GRAC: errorcode=$1"
}

if [ "${ACTION}" = "started" ] ; then
	out_log  "0"

elif [ "${ACTION}" = "stopped" ] ; then
	if [ "${EXITCODE}" != "0" ] ; then
		out_log  "1"
	elif [ "${RESULT}" = "success" ] ; then
		out_log  "1"
	else
		out_log  "1"
	fi

fi

exit 0

