#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import glob
import re
import os

from grac_util import remount_readonly,bluetooth_exists
from grac_util import search_dir,search_file_reversely,GracLog 
from grac_util import make_media_msg,red_alert2,catch_user_id
from grac_util import umount_mount_readonly
from grac_define import *
from grac_error import *

#-----------------------------------------------------------------------
class GracSynchronizer:
    """
    synchronize json-rules and device
    """

    supported_media = (
                        (JSON_RULE_USB_MEMORY, MC_TYPE_AUTH),
                        (JSON_RULE_PRINTER, MC_TYPE_AUTH),
                        (JSON_RULE_SOUND, MC_TYPE_ALLOWSYNC),
                        (JSON_RULE_MICROPHONE, MC_TYPE_ALLOWSYNC),
                        (JSON_RULE_MOUSE, MC_TYPE_NA),
                        (JSON_RULE_KEYBOARD, MC_TYPE_NA),
                        (JSON_RULE_CD_DVD, MC_TYPE_AUTH),
                        (JSON_RULE_WIRELESS, MC_TYPE_REMOVE),
                        (JSON_RULE_CAMERA, MC_TYPE_BCONFIG),
                        (JSON_RULE_BLUETOOTH, MC_TYPE_NA),
                        (JSON_RULE_CLIPBOARD, MC_TYPE_NOSYNC),
                        (JSON_RULE_SCREEN_CAPTURE, MC_TYPE_ALLOWSYNC))

    _logger = GracLog.get_logger()

    @classmethod
    def sync_screen_capture(cls, state, data_center):
        """
        synchronize bluetooth
        """
        
        user_id, _  = catch_user_id()
        if user_id == '-':
            cls._logger.debug('screen_capture can not be blocked '\
                                'because of no user loggined')
            return
        if user_id[0] == '+':
            user_id = user_id[1:]
            
        for sn in ('/usr/bin/xfce4-screenshooter','/usr/bin/gnome-screenshot'):
            if not os.path.exists(sn):
                continue
            if state == JSON_RULE_DISALLOW:
                p0 = subprocess.Popen(
                    ['/usr/bin/setfacl', 
                        '-m', 
                        'u:{}:r'.format(user_id), 
                        sn],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE)
                pp_out, pp_err = p0.communicate()
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_SCREEN_CAPTURE, state)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                    grmcode, data_center, flag=RED_ALERT_ALERTONLY)
            else:
                p0 = subprocess.Popen(
                    ['/usr/bin/setfacl', 
                        '-m', 
                        'u:{}:rx'.format(user_id), 
                        sn],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE)
                pp_out, pp_err = p0.communicate()

    @classmethod
    def sync_bluetooth(cls, state, data_center):
        """
        synchronize bluetooth
        """

        mac_regex = re.compile(r'([0-9A-F]{2}[:-]){5}([0-9A-F]{2})')

        if not bluetooth_exists():
            cls._logger.error('bluetooth controller not found')
            return

        for controller in glob.glob('/var/lib/bluetooth/*'):
            for mac in glob.glob(controller + '/*'):
                mac = mac.split('/')[-1].upper()
                if not mac_regex.match(mac):
                    continue

                for wl in data_center.get_bluetooth_whitelist():
                    if wl.upper() == mac:
                        break
                else:
                    p1 = subprocess.Popen(['echo', '-e', 'disconnect {}\nquit'.format(mac)], 
                                            stdout=subprocess.PIPE)
                    p2 = subprocess.Popen(['bluetoothctl'], 
                                            stdin=p1.stdout, 
                                            stdout=subprocess.PIPE, 
                                            stderr=subprocess.PIPE)
                    p1.stdout.close()
                    cls._logger.info(
                        'disconnecting controller-mac={} device-mac={}'.format(
                                                                            controller, 
                                                                            mac))
                    cls._logger.info(p2.communicate()[0].decode('utf8'))
                    logmsg, notimsg, grmcode = \
                        make_media_msg(JSON_RULE_BLUETOOTH, state)
                    red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)

        cls._logger.info('SYNC state={}'.format(state))
            
    @classmethod
    def sync_printer(cls, state, data_center):
        """
        synchronize printer
        """

        dn = search_dir('/sys/devices', '^lp[0-9]+$')
        if not dn:
            return
        
        fn = search_file_reversely(dn, 'authorized', REVERSE_LOOKUP_LIMIT)
        if not fn:
            return

        with open(fn, 'w') as f:
            f.write('0')
            cls._logger.info('SYNC state={} authorized=0'.format(state))
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_PRINTER, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
            cls._logger.debug('***** PRINTER disallow {}'.format(dn))

    @classmethod
    def sync_usb_memory_readonly(cls, state, data_center):
        """
        synchronize usb-memory readonly
        """

        block_base_path = '/sys/class/block/*'
        block_node_regex = re.compile('^[a-zA-Z]+$')
        block_usb_regex = re.compile('/usb[0-9]*/')

        #block devices
        for block_device in glob.glob(block_base_path):
            device_node = block_device.split('/')[-1]
            if not block_node_regex.match(device_node):
                continue

            #/sys/devices
            device_sys_path = block_device + '/device'
            if not os.path.exists(device_sys_path):
                continue
                
            device_real_path = os.path.realpath(device_sys_path)

            #usb device
            if block_usb_regex.search(device_real_path):
                #whitelist
                serial_path = search_file_reversely(
                                            device_real_path, 
                                            'serial', 
                                            REVERSE_LOOKUP_LIMIT)
                if serial_path:
                    with open(serial_path) as f:
                        serial = f.read().strip('\n')
                    for s in data_center.get_usb_memory_whitelist():
                        if s == serial:
                            cls._logger.info(
                                'SYNC serial({}) is in whitelist'.format(serial))
                            return

                skeep_uuid = False
                for usb_label_path in ('/dev/disk/by-label/*', '/dev/disk/by-uuid/*'):
                    for usb_label in glob.glob(usb_label_path):
                        if usb_label.lower().endswith('efi'):
                            continue
                        usb_label_realpath = os.path.realpath(usb_label)
                        usb_label_node = re.split('[0-9]+', usb_label_realpath.split('/')[-1])[0]
                        if usb_label_node == device_node:
                            mount_point = '/media/gooroom/'+usb_label.split('/')[-1]
                            umount_mount_readonly(usb_label_realpath, mount_point)
                            cls._logger.info('SYNC state={} {} remounted '\
                                'as readonly'.format(state, usb_label_realpath))
                            cls._logger.debug('***** USB read_only {} {}'.format(
                                usb_label_realpath, mount_point))
                            logmsg, notimsg, grmcode = \
                                make_media_msg(JSON_RULE_USB_MEMORY, state)
                            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                            skeep_uuid = True
                            break
                    if skeep_uuid:
                        break

    @classmethod
    def sync_usb_memory(cls, state, data_center):
        """
        synchronize usb-memory
        """

        block_base_path = '/sys/class/block/*'
        block_node_regex = re.compile('^[a-zA-Z]+$')
        block_usb_regex = re.compile('/usb[0-9]*/')

        #block devices
        for block_device in glob.glob(block_base_path):
            device_node = block_device.split('/')[-1]
            if not block_node_regex.match(device_node):
                continue

            #/sys/devices
            device_sys_path = block_device + '/device'
            if not os.path.exists(device_sys_path):
                continue
                
            device_real_path = os.path.realpath(device_sys_path)

            #usb device
            if block_usb_regex.search(device_real_path):
                #whitelist
                serial_path = search_file_reversely(
                                            device_real_path, 
                                            'serial', 
                                            REVERSE_LOOKUP_LIMIT)
                if serial_path:
                    with open(serial_path) as f:
                        serial = f.read().strip('\n')
                    for s in data_center.get_usb_memory_whitelist():
                        if s == serial:
                            cls._logger.info(
                                'SYNC serial({}) is in whitelist'.format(serial))
                            return

                #authorized
                authorized = search_file_reversely(
                                            device_real_path, 
                                            'authorized', 
                                            REVERSE_LOOKUP_LIMIT)
                if not authorized:
                    cls._logger.error('{} not found authorized'.block_device)
                    return

                with open(authorized, 'w') as f:
                    f.write('0')
                    cls._logger.info('SYNC state={} authorized=0'.format(state))
                    logmsg, notimsg, grmcode = \
                        make_media_msg(JSON_RULE_USB_MEMORY, state)
                    red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                    cls._logger.debug('***** USB disallow {}'.format(block_device))


    @classmethod
    def sync_sound(cls, state, data_center):
        """
        synchronize sound card
        """

        data_center.sound_mic_inotify.pactl_sound(state)
        if state == JSON_RULE_DISALLOW:
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_SOUND, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                grmcode, data_center, flag=RED_ALERT_ALERTONLY)

    @classmethod
    def sync_microphone(cls, state, data_center):
        """
        synchronize microphone
        """

        data_center.sound_mic_inotify.pactl_mic(state)
        if state == JSON_RULE_DISALLOW:
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_MICROPHONE, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                grmcode, data_center, flag=RED_ALERT_ALERTONLY)

    @classmethod
    def sync_mouse(cls, state, data_center):
        """
        synchronize mouse
        """

        mouse_base_path = '/sys/class/input'
        mouse_regex = re.compile('mouse[0-9]*')

        for mouse in glob.glob(mouse_base_path+'/*'):
            mouse_node = mouse.split('/')[-1]
            if mouse_regex.match(mouse_node):
                mouse_real_path = os.path.realpath(mouse+'/device')
                bConfigurationValue = search_file_reversely(
                                                    mouse_real_path, 
                                                    'bConfigurationValue', 
                                                    REVERSE_LOOKUP_LIMIT)
                if not bConfigurationValue:
                    cls._logger.error('{} not found bConfigurationValue'.format(mouse_node))
                    continue

                with open(bConfigurationValue, 'w') as f:
                    f.write('0')
                    cls._logger.info('SYNC state={} bConfigurationValue=0'.format(state))
                    logmsg, notimsg, grmcode = \
                        make_media_msg(JSON_RULE_MOUSE, state)
                    red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)

    @classmethod
    def sync_keyboard(cls, state, data_center):
        """
        synchronize keyboard
        """

        keyboard_real_path = search_dir('/sys/devices', '.*::capslock')
        bConfigurationValue = search_file_reversely(
                                                keyboard_real_path,
                                                'bConfigurationValue',
                                                REVERSE_LOOKUP_LIMIT)
        if not bConfigurationValue:
            cls._logger.error('not found bConfigurationValue')
            return

        with open(bConfigurationValue, 'w') as f:
            f.write('0')
            cls._logger.info('SYNC state={} bConfigurationValue=0'.format(state))
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_KEYBOARD, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)

    @classmethod
    def sync_cd_dvd(cls, state, data_center):
        """
        synchronize cd_dvd
        """

        block_base_path = '/sys/class/block/*'
        block_node_regex = re.compile('^sr[0-9]+$')
        block_usb_regex = re.compile('/usb[0-9]*/')

        #block devices
        for block_device in glob.glob(block_base_path):
            device_node = block_device.split('/')[-1]
            if not block_node_regex.match(device_node):
                continue

            #/sys/devices
            device_sys_path = block_device + '/device'
            if not os.path.exists(device_sys_path):
                continue
                
            device_real_path = os.path.realpath(device_sys_path)

            #usb device
            if block_usb_regex.search(device_real_path):
                authorized = search_file_reversely(
                                            device_real_path, 
                                            'authorized', 
                                            REVERSE_LOOKUP_LIMIT)
                if not authorized:
                    cls._logger.error('{} not found authorized'.format(device_node))
                    continue

                with open(authorized, 'w') as f:
                    f.write('0')
                    cls._logger.info('SYNC state={} authorized=0'.format(state))
                    logmsg, notimsg, grmcode = \
                        make_media_msg(JSON_RULE_CD_DVD, state)
                    red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)
                    cls._logger.debug('***** DVD disallow {}'.format(block_device))

    @classmethod
    def sync_wireless(cls, state, data_center):
        """
        synchronize wireless
        """

        wl_base_path = '/sys/class/net'
        wl_inner_regex = re.compile('wireless')

        for wl in glob.glob(wl_base_path+'/*'):
            wl_node = wl.split('/')[-1]

            for wl_inner in glob.glob(wl+'/*'):
                file_name = wl_inner.split('/')[-1]
                if wl_inner_regex.match(file_name):
                    wl_inner_real_path = os.path.realpath(wl_inner+'/device')
                    remove = search_file_reversely(
                                            wl_inner_real_path, 
                                            'remove', 
                                            REVERSE_LOOKUP_LIMIT)
                    if not remove:
                        cls._logger.error('{} not found remove'.format(wl_inner))
                        continue

                    with open(remove, 'w') as f:
                        f.write('1')
                        with open(META_FILE_PCI_RESCAN, 'a') as f:
                            f.write('wireless')
                        cls._logger.info('SYNC state={} remove=1'.format(state))
                        logmsg, notimsg, grmcode = \
                            make_media_msg(JSON_RULE_WIRELESS, state)
                        red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)

    @classmethod
    def sync_camera(cls, state, data_center):
        """
        synchronize camera
        """

        camera_base_path = '/sys/class/video4linux'

        for camera in glob.glob(camera_base_path+'/*'):
            camera_real_path = os.path.realpath(camera+'/device')
            bConfigurationValue = search_file_reversely(
                                                camera_real_path, 
                                                'bConfigurationValue', 
                                                REVERSE_LOOKUP_LIMIT)
            if not bConfigurationValue:
                cls._logger.error('not found bConfigurationValue')
                continue

            with open(bConfigurationValue, 'w') as f:
                f.write('0')
                with open('{}/{}.bcvp'.format(META_ROOT, JSON_RULE_CAMERA), 'w') as f2:
                    f2.write(bConfigurationValue)
                cls._logger.info('SYNC state={} bConfigurationValue=0'.format(state))
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_CAMERA, state)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, data_center)

    @classmethod
    def recover_bConfigurationValue(cls, target):
        """
        recover bConfigurationValue
        """

        filepath = '{}/{}.bcvp'.format(META_ROOT, target)
        if not os.path.exists(filepath):
            return

        with open(filepath) as f:
            device_path = f.read().strip('\n')

        os.unlink(filepath)

        bConfigurationValue_path = search_file_reversely(
                                                    device_path, 
                                                    'bConfigurationValue', 
                                                    REVERSE_LOOKUP_LIMIT)

        with open(bConfigurationValue_path, 'w') as f2:
            f2.write('1')
            cls._logger.info('{}\'bConfigurationValue is set 1'.format(target))

