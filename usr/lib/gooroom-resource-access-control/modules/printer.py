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
        authorized_path = search_file_reversely(
                                                '/sys/'+param[1], 
                                                'authorized', 
                                                REVERSE_LOOKUP_LIMIT)
        with open(authorized_path, 'w') as f:
            f.write('0')
            logger.info('mode has changed to {}'.format(mode))
            red_alert(JSON_RULE_PRINTER, mode, data_center)
            logger.debug('***** PRINTER MODULE disallow {}'.format(param[1]))
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

