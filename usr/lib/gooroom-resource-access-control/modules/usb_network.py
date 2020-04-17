#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import threading
import psutil
import time
import os
import re

from grac_util import remount_readonly,search_file_reversely
from grac_util import GracConfig,GracLog,grac_format_exc
from grac_util import make_media_msg,red_alert2
from grac_define import *

#-----------------------------------------------------------------------
def do_task(param, data_center):
    """
    do task
    """

    logger = GracLog.get_logger()

    try:
        mode = param[0]

        if mode == JSON_RULE_DISALLOW:
            devpath = param[1]

            #################### WHITE LIST ####################
            usb_port_blacklist = \
                data_center.get_usb_network_port_blacklist()
            logger.debug('(UNW MOD) usb_port_blacklist={}'.format
                                                (usb_port_blacklist))
            if not usb_port_blacklist:
                logger.info('(UNW MOD) blacklist is empty')
                return
            
            block_usb_regex = re.compile(usb_port_blacklist)
            if not block_usb_regex.search(devpath):
                logger.info('(UNW MOD) path={} not in blacklist={}'.format(
                                                devpath, 
                                                usb_port_blacklist))
                return
            ###################################################
                
            authorized_path = search_file_reversely(
                                                    '/sys/'+devpath, 
                                                    'authorized', 
                                                    REVERSE_LOOKUP_LIMIT)

            with open(authorized_path, 'w') as f:
                f.write('0')
                logger.info('mode has changed to {}'.format(mode))
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_USB_NETWORK, mode)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                logger.debug('***** USB NETWORK MODULE disallow {}'.format(param[1]))
    except:
        e = grac_format_exc()
        GracLog.get_logger().error(e)
        logger.error(e)

    return GRAC_OK

#-----------------------------------------------------------------------
def remount_thread(devnode, mode, data_center):
    """
    remount
    """

    logger = GracLog.get_logger()

    for i in range(600): #1 mins
        partis = psutil.disk_partitions()
        for parti in partis:
            if parti.device == devnode:
                remount_readonly(parti.device, parti.mountpoint)
                logger.info('{} mode has changed'.format(devnode))
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_USB_MEMORY, mode)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                return
        time.sleep(0.1)
    logger.error('{} fail to change mode'.format(devnode))

