#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import datetime

from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
from grac_util import make_media_msg,red_alert2,write_event_log
from grac_util import bluetooth_exists
from grac_define import *

#-----------------------------------------------------------------------
def do_task(param, data_center):
    """
    do task
    """

    logger = GracLog.get_logger()

    if not bluetooth_exists():
        logger.error('bluetooth controller not found')
        return GRAC_OK

    try:
        mode = param[0]
        unique = param[1].strip() if param[1] else param[1]
        name = param[2].strip() if param[2] else param[2]
        mac = ''
        if unique:
            mac = unique
        elif name:
            name = name.upper()
            delim_cnt = 0
            num_cnt = 0
            for n in name:
                if n == '.' or n == ':':
                    delim_cnt += 1
                    continue
                if ord(n) >= ord('0') and ord(n) <= ord('9'):
                    num_cnt += 1
                    continue
                if ord(n) >= ord('A') and ord(n) <= ord('F'):
                    num_cnt += 1
                    continue
                break
            else:
                if (delim_cnt == 0 or delim_cnt == 5) and num_cnt == 12 :
                    mac = name
        if not mac:
            raise Exception('!! bluetooth mac not found')

        mac = mac.replace('.', ':').strip('\n').upper()
        for m in data_center.get_bluetooth_whitelist():
            if m.upper() == mac:
                logger.info('mac({}) is in whitelist'.format(mac))
                return GRAC_OK

        p1 = subprocess.Popen(['echo', '-e', 'disconnect {}\nquit'.format(mac)], 
                                stdout=subprocess.PIPE)
        p2 = subprocess.Popen(['bluetoothctl'], 
                                stdin=p1.stdout, 
                                stdout=subprocess.PIPE, 
                                stderr=subprocess.PIPE)
        p1.stdout.close()
        logger.info(p2.communicate()[0].decode('utf8'))
        logmsg, notimsg, grmcode = \
            make_media_msg(JSON_RULE_BLUETOOTH, mode)
        red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
        write_event_log(SOMANSA, 
                        datetime.datetime.now().strftime('%Y%m%d %H:%M:%S'),
                        JSON_RULE_BLUETOOTH, 
                        SOMANSA_STATE_DISALLOW, 
                        'null', 
                        'null', 
                        'null', 
                        'null')
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

