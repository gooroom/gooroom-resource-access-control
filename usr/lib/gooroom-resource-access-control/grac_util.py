#! /usr/bin/env python3

#-----------------------------------------------------------------------
from datetime import datetime
from systemd import journal
import simplejson as json
from pwd import getpwnam
import logging.handlers
import configparser
import subprocess
import traceback
import importlib
import logging
import struct
import glob
import dbus
import sys
import os
import re

from grac_error import GracError
from grac_define import *

#-----------------------------------------------------------------------
class GracConfig:
    """
    grac config
    """
    
    _parsers = {}

    @classmethod
    def get_config(cls, config_fullpath=CONFIG_FULLPATH, reload=False):
        """
        return singleton RawConfigParser
        """

        if not reload and config_fullpath in cls._parsers:
            
            return cls._parsers[config_fullpath]

        if not os.path.exists(config_fullpath):
            raise GracError(
                'config file(%s) specified' 
                ' in GracDefine.py does not be found' % config_fullpath)

        parser = configparser.RawConfigParser()
        parser.optionxform = str
        parser.read(config_fullpath)
        cls._parsers[config_fullpath] = parser
        return parser
        
    @classmethod
    def get_my_gracname(cls):
        """
        return grac-name calling this method
        """

        return sys.argv[0].split('/')[-1].split('.')[0]

#-----------------------------------------------------------------------
class GracLog:
    """
    grac log
    """

    #default logger
    _logger = None

    #named
    _named_logger = {}

    @classmethod
    def get_logger(cls, filename=None):
        """
        return singleton logger
        """

        logger = None

        if filename:
            if filename in cls._named_logger:
                return cls._named_logger[filename]
            else:
                logger = logging.getLogger(filename)
                cls._named_logger[filename] = logger

        else:
            if cls._logger:
                return cls._logger
            else:
                cls._logger = logging.getLogger('GRAC')
                logger = cls._logger

        conf = GracConfig.get_config()

        #log level
        logger.setLevel(eval('logging.%s' % conf.get('LOG', 'LEVEL')))

        #log path
        log_path = conf.get('LOG', 'PATH')
        if log_path[:-1] != '/':
            log_path += '/'

        #make dirs of log path
        try:
            os.makedirs(log_path)
        except OSError:
            if os.path.exists(log_path):
                pass
            else:
                raise

        #datetime
        today = datetime.now().strftime('%Y%m%d')

        #filename
        if not filename:
            filename = sys.argv[0].split('/')[-1].split('.')[0]

        #log fullpath
        log_fullpath = '%s%s_%s.log' % (log_path, filename, today)

        #max_bytes, backup_count 
        max_bytes = int(conf.get('LOG', 'MAX_BYTES'))
        backup_count = int(conf.get('LOG', 'BACKUP_COUNT'))

        file_handler = logging.handlers.RotatingFileHandler(
            log_fullpath, 
            maxBytes=max_bytes, 
            backupCount=backup_count)

        #formatter
        fmt = logging.Formatter(conf.get('LOG', 'FMT'))
        file_handler.setFormatter(fmt)

        logger.addHandler(file_handler)

        return logger

#-----------------------------------------------------------------------
def grac_format_exc():
    """
    reprlib version of format_exc of traceback
    """

    return '\n'.join(traceback.format_exc().split('\n')[-4:-1])

#-----------------------------------------------------------------------
def catch_user_id():
    """
    read current logined user_id from /var/run/utmp
    """

    with open('/var/run/utmp', 'rb') as f:
        fc = memoryview(f.read())

    utmp_fmt = '<ii32s4s32s'
    user_id = '-'

    for i in range(int(len(fc)/384)):
        ut_type, ut_pid, ut_line, ut_id, ut_user = \
            struct.unpack(utmp_fmt, fc[384*i:76+(384*i)])
        ut_line = ut_line.decode('utf8').strip('\00')
        ut_id = ut_id.decode('utf8').strip('\00')

        if ut_type == 7 and ut_id == ':0':
            user_id = ut_user.decode('utf8').strip('\00')

    #check if user_id is an online account
    with open('/etc/passwd') as f:
        pws = f.readlines()

    if user_id != '-':
        for pw in pws:
            splited = pw.split(':')
            if splited[0] == user_id:
                ps = '/var/run/user/%d/gooroom/.grm-user'  % getpwnam(user_id).pw_uid
                #user_id is a local account
                if not 'gooroom-online-account' in splited[4] or not os.path.exists(ps):
                    user_id = '+' + user_id
                break
        else:
            raise Exception('user_id(%s) does not existed in /etc/passwd')

    return user_id

#-----------------------------------------------------------------------
def search_dir(from_here, dir_regex_s):
    """
    search for directory
    """

    regex = re.compile(dir_regex_s)
    for root, dirs, files in os.walk(from_here):
        for d in dirs:
            if regex.match(d):
                return root+'/'+d

    return None

#-----------------------------------------------------------------------
def search_file_reversely(path, file_name, path_level_limit):
    """
    search for file by reversing path
    """
    
    path_trail_splited = path.split('/')
    while len(path_trail_splited) > path_level_limit:
        path_trail = '/'.join(path_trail_splited)
        
        for fn in glob.glob(path_trail+'/*'):
            if fn.split('/')[-1] == file_name:
                return fn

        path_trail_splited = path_trail_splited[:-1]

    return None

#-----------------------------------------------------------------------
GRMCODE_MAP = {
    (JSON_RULE_USB_MEMORY, JSON_RULE_DISALLOW):GRMCODE_USB_MEMORY_DISALLOW,
    (JSON_RULE_USB_MEMORY, JSON_RULE_READONLY):GRMCODE_USB_MEMORY_READONLY,
    (JSON_RULE_PRINTER, JSON_RULE_DISALLOW):GRMCODE_PRINTER_DISALLOW,
    (JSON_RULE_CD_DVD, JSON_RULE_DISALLOW):GRMCODE_CD_DVD_DISALLOW,
    (JSON_RULE_CAMERA, JSON_RULE_DISALLOW):GRMCODE_CAMERA_DISALLOW,
    (JSON_RULE_SOUND, JSON_RULE_DISALLOW):GRMCODE_SOUND_DISALLOW,
    (JSON_RULE_MICROPHONE, JSON_RULE_DISALLOW):GRMCODE_MICROPHONE_DISALLOW,
    (JSON_RULE_WIRELESS, JSON_RULE_DISALLOW):GRMCODE_WIRELESS_DISALLOW,
    (JSON_RULE_BLUETOOTH, JSON_RULE_DISALLOW):GRMCODE_BLUETOOTH_DISALLOW,
    (JSON_RULE_KEYBOARD, JSON_RULE_DISALLOW):GRMCODE_KEYBOARD_DISALLOW,
    (JSON_RULE_MOUSE, JSON_RULE_DISALLOW):GRMCODE_MOUSE_DISALLOW }

def make_media_msg(item, state):
    """
    make journald and notification messages
    """

    grmcode = '009999'
    try:
        grmcode = GRMCODE_MAP[(item, state)]
    except:
        pass

    if state == 'disallow':
        logmsg = '$({}) is blocked by detecting unauthorized media'.format(item)
        notimsg = '{} 가(이) 차단되었습니다'.format(item)
    elif state == 'read_only':
        logmsg = '$({}) is blocked by detecting to write to read-only media'.format(item)
        notimsg = '{} 가(이) 읽기전용으로 설정되었습니다'.format(item)
    else:
        logmsg = ''
        notimsg = ''

    return logmsg, notimsg, grmcode

#-----------------------------------------------------------------------
def red_alert2(logmsg, notimsg, priority, grmcode, data_center, flag='all'):
    """
    RED ALERT
    """

    #JOURNALD
    journal.send(logmsg,
                SYSLOG_IDENTIFIER='GRAC',
                PRIORITY=priority,
                GRMCODE=grmcode)

    if flag != 'all':
        return

    #ALERT
    try:
        module_path = GracConfig.get_config().get('SECURITY', 'SECURITY_MODULE_PATH')
        sys.path.append(module_path)
        m = importlib.import_module('gooroom-security-logparser')
        notify_level = getattr(m, 'get_notify_level')('media')
        if priority > notify_level: 
            return
    except:
        pass

    data_center.GRAC.grac_noti('{}:{}'.format(grmcode, notimsg))
    #subprocess.call(['/usr/bin/grac-apply.sh', notimsg])

#-----------------------------------------------------------------------
def remount_readonly(src, dst):
    """
    remount usb as readonly
    """

    #subprocess.call(['/bin/umount', src])
    #os.makedirs(dst, exist_ok=True)
    #subprocess.call(['/bin/mount', '-o', 'ro', src, dst])
    subprocess.call(['/bin/mount', '-o', 'remount,ro', src, dst])

#-----------------------------------------------------------------------
def umount_mount_readonly(src, dst):
    """
    unmount and mount usb as readonly
    """

    subprocess.call(['/bin/umount', src])
    os.makedirs(dst, exist_ok=True)
    subprocess.call(['/bin/mount', '-o', 'ro', src, dst])

#-----------------------------------------------------------------------
def verify_rule(rule_name):
    """
    verity rule file
    """

    import OpenSSL
    import base64

    fn = rule_name.split('/')[-1] + '+signature'
    dn = rule_name.replace('/', '.')
    signature_file = META_SIGNATURE_PATH + '/{}/{}'.format(dn, fn)

    with open(signature_file) as f:
        signature = f.read()

    with open(rule_name) as f2:
        data = f2.read()

    cert = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, 
        open(META_SERVER_CERT_PATH).read())

    OpenSSL.crypto.verify(cert, 
        base64.b64decode(signature.encode('utf8')), 
        data.encode('utf8'), 'sha256')

#-----------------------------------------------------------------------
def bluetooth_exists():
    """
    check if bluetooth controller is in local
    """

    cs = subprocess.check_output(['/usr/bin/hcitool', 'dev'])
    cs_list = cs.decode('utf8').strip('\n').split('\n')
    if len(cs_list) < 2:
        return False
    else:
        return True
