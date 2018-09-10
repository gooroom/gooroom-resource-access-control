#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess

from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
from grac_util import red_alert,bluetooth_exists
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
        mac = param[1].replace('.', ':').strip('\n').upper()
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
        red_alert(JSON_RULE_BLUETOOTH, mode, data_center)
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

