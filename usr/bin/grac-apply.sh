#!/bin/bash

#
# check environment
#

USERNAME="$(/usr/bin/users | /usr/bin/awk 'NR==1 {print}')"


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


# display message
show_msg $1

