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
        if mode == JSON_RULE_ALLOW:
            return GRAC_OK

        vendorid = param[2].strip()
        productid = param[3].strip()

        for devs in data_center.get_usb_etc():
            for dev_name, dev_items in devs.items():
                if dev_items['state'] == JSON_RULE_ALLOW:
                    continue
                for item in dev_items['items']:
                    vid = item['vid']
                    pid = item['pid']

                    if vendorid == vid and productid == pid:
                        devpath = param[1]
                        authorized_path = search_file_reversely(
                                                                '/sys/'+devpath, 
                                                                'authorized', 
                                                                REVERSE_LOOKUP_LIMIT)

                        with open(authorized_path, 'w') as f:
                            f.write('0')
                            logmsg, notimsg, grmcode = \
                                make_media_msg(dev_name, mode)
                            logger.debug('logmsg={} notimsg={} grmcode={}'.format(logmsg, notimsg, grmcode))
                            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                            logger.debug('***** USB ETC disallow {}'.format(param[1]))

                        return GRAC_OK
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

