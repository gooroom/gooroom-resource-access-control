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
        #skeep 'event' of mouse events
        #for processing task once
        if 'event' in param[1].split('/')[-1]:
            return GRAC_OK

        bConfigurationValue_path = search_file_reversely(
                                                        '/sys/'+param[1], 
                                                        'bConfigurationValue', 
                                                        REVERSE_LOOKUP_LIMIT)
        with open(bConfigurationValue_path, 'w') as f:
            f.write('0')
            logger.info('mode has changed to {}'.format(mode))
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_MOUSE, mode)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

