#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import threading
import psutil
import time
import os

from grac_util import GracConfig,GracLog,grac_format_exc
from grac_util import red_alert,remount_readonly,search_file_reversely
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
            authorized_path = search_file_reversely(
                                                    '/sys/'+devpath, 
                                                    'authorized', 
                                                    REVERSE_LOOKUP_LIMIT)

            with open(authorized_path, 'w') as f:
                f.write('0')
                logger.info('mode has changed to {}'.format(mode))
                red_alert(JSON_RULE_USB_MEMORY, mode, data_center)
                logger.debug('***** USB MODULE disallow {}'.format(param[1]))

        elif mode == JSON_RULE_READONLY:
            devnode = '/dev/' + param[1]
            thr = threading.Thread(target=remount_thread, 
                                args=(devnode,mode,data_center))
            thr.daemon = True
            thr.start()
            logger.info('{} mode is changing to {}'.format(devnode, mode))
            logger.debug('***** USB MODULE read_only {}'.format(param[1]))
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
                red_alert(JSON_RULE_USB_MEMORY, mode, data_center)
                return
        time.sleep(0.1)
    logger.error('{} fail to change mode'.format(devnode))

