#! /usr/bin/env python3

#-----------------------------------------------------------------------
from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
from grac_util import red_alert
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
            red_alert(JSON_RULE_MICROPHONE, mode, data_center)
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

