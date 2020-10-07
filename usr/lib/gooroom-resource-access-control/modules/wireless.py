#! /usr/bin/env python3

#-----------------------------------------------------------------------
import datetime

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
        remove = '/'.join(remove.split('/')[:-2]) + '/remove'
        if not os.path.exists(remove):
            cls._logger.error('(v2.0)In parent dir, {} not found remove'.format(wl_inner))
            return

        with open(remove_path, 'w') as f:
            f.write('1')
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

