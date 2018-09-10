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
        bConfigurationValue_path = search_file_reversely(
                                                        '/sys/'+param[1], 
                                                        'bConfigurationValue', 
                                                        REVERSE_LOOKUP_LIMIT)
        with open(bConfigurationValue_path, 'w') as f:
            f.write('0')
            with open('{}/{}.bcvp'.format(META_ROOT, JSON_RULE_CAMERA), 'w') as f2:
                f2.write(bConfigurationValue_path)
            logger.info('mode has changed to {}'.format(mode))
            red_alert(JSON_RULE_CAMERA, mode, data_center)
    except:
        e = grac_format_exc()
        logger.error(e)

    return GRAC_OK

