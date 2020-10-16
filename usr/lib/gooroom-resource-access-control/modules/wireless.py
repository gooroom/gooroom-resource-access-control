#! /usr/bin/env python3

#-----------------------------------------------------------------------
import datetime
import os

from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
from grac_util import make_media_msg,red_alert2,write_event_log
from grac_define import *

#-----------------------------------------------------------------------
def do_task(param, data_center):
    """
    do task
    """

    logger = GracLog.get_logger()
    try:
        mode = param[0]
        remove_path = search_file_reversely(
                                        '/sys/'+param[1], 
                                        'remove', 
                                        REVERSE_LOOKUP_LIMIT)
        #v2.0
        if not os.path.exists(remove_path):
            logger.error('(wireless) REMOVE NOT FOUND')
            return

        with open(remove_path, 'w') as f:
            f.write('1')

        if os.path.exists(remove_path):
            remove_second = '/'.join(remove_path.split('/')[:-2]) + '/remove'
            if not os.path.exists(remove_second):
                logger.error('(wireless) FAIL TO REMOVE 1')
                return
            else:
                with open(remove_second, 'w') as sf:
                    sf.write('1')
                
        if os.path.exists(remove_path):
            logger.error('(wireless) FAIL TO REMOVE 2')
            return

        with open(META_FILE_PCI_RESCAN, 'a') as f2:
            f2.write('wireless=>{}'.format(remove_path))
        logger.info('mode has changed to {}'.format(mode))
        logmsg, notimsg, grmcode = \
            make_media_msg(JSON_RULE_WIRELESS, mode)
        red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
        write_event_log(SOMANSA, 
                        datetime.datetime.now().strftime('%Y%m%d %H:%M:%S'),
                        JSON_RULE_WIRELESS, 
                        SOMANSA_STATE_DISALLOW, 
                        'null', 
                        'null', 
                        'null', 
                        'null')
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

