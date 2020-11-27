#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import threading
import datetime
import psutil
import time
import os

from grac_util import make_media_msg,red_alert2,write_event_log
from grac_util import remount_readonly,search_file_reversely
from grac_util import GracConfig,GracLog,grac_format_exc
from grac_util import catch_user_id
from grac_define import *

#-----------------------------------------------------------------------
def do_task(param, data_center):
    """
    do task
    """

    logger = GracLog.get_logger()
    try:
        mode = param[0]
        serial = param[2].strip() if param[2] else ''

        #whitelist
        if serial: #param[2]:
            #serial = param[2].strip('\n')
            for s in data_center.get_usb_memory_whitelist():
                if s == serial:
                    logger.info('serial({}) is in whitelist'.format(serial))

                    #usb whitelist register signal
                    signal_msg = ['except', '103', 'already in whitelist']
                    data_center.GRAC.media_usb_info(','.join(signal_msg))
                    return GRAC_OK

            #usb whitelist register signal
            product = param[3].strip() if param[3] else ''
            vendor = param[4].strip() if param[4] else ''
            model = param[5].strip() if param[5] else ''

            user_id, _  = catch_user_id()

            if not user_id or user_id[0] == '-':
                signal_msg = ['except', '101', 'not login']
            if user_id[0] == '+':
                signal_msg = ['except', '102', 'local user not supported']
            else:
                signal_msg = ['normal', user_id, serial, '', product, '', vendor, model]
            data_center.GRAC.media_usb_info(','.join(signal_msg))
        else:
            signal_msg = ['except', '104', 'serial not found']
            data_center.GRAC.media_usb_info(','.join(signal_msg))

        if mode == JSON_RULE_DISALLOW:
            devpath = param[1]
            authorized_path = search_file_reversely(
                                                    '/sys/'+devpath, 
                                                    'authorized', 
                                                    REVERSE_LOOKUP_LIMIT)

            with open(authorized_path, 'w') as f:
                f.write('0')
                logger.info('mode has changed to {}'.format(mode))
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_USB_MEMORY, mode)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                write_event_log(SOMANSA, 
                                datetime.datetime.now().strftime('%Y%m%d %H:%M:%S'),
                                JSON_RULE_USB_MEMORY, 
                                SOMANSA_STATE_DISALLOW, 
                                'null', 
                                'null', 
                                'null', 
                                'null')
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
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_USB_MEMORY, mode)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                write_event_log(SOMANSA, 
                                datetime.datetime.now().strftime('%Y%m%d %H:%M:%S'),
                                JSON_RULE_USB_MEMORY, 
                                SOMANSA_STATE_READONLY, 
                                'null', 
                                'null', 
                                'null', 
                                'null')
                return
        time.sleep(0.1)
    logger.error('{} fail to change mode'.format(devnode))

