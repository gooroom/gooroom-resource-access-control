#! /usr/bin/env python3

#-----------------------------------------------------------------------
from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
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
        remove_path = search_file_reversely(
                                            '/sys/'+param[1], 
                                            'remove', 
                                            REVERSE_LOOKUP_LIMIT)
        with open(remove_path, 'w') as f:
            f.write('0')
            with open(META_FILE_PCI_RESCAN, 'a') as f2:
                f2.write('microphone')
            logger.info('mode has changed to {}'.format(mode))
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_MICROPHONE, mode)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

