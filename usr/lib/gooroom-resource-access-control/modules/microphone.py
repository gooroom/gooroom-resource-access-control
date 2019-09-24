#! /usr/bin/env python3

#-----------------------------------------------------------------------
import pyinotify
import os

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
        if data_center.sound_mic_inotify: 
            wm = data_center.sound_mic_inotify.WM
            dev_snd_path = "/dev/snd/"
            file_list = os.listdir(dev_snd_path)

            for control_file in file_list:
                if control_file.find("control") is not -1:
                    idx = control_file.split("controlC")
                    wm.add_watch(
                        dev_snd_path + control_file, 
                        pyinotify.IN_ACCESS, 
                        rec=True)
    except:
        logger.error(grac_format_exc())

    return GRAC_OK

