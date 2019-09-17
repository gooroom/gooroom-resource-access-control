#! /usr/bin/env python3

#-----------------------------------------------------------------------
import simplejson as json
import subprocess
import pyinotify
import re

from grac_define import *
from grac_util import *

#-----------------------------------------------------------------------
class SoundMicInotify(pyinotify.ProcessEvent):
    """
    sound/microphone inotify handler
    """

    def __init__(self, data_center):

        self.data_center = data_center

    def pactl_sound(self, state, notimsg=True):
        """
        allow/disallow sound with pactl
        """

        letter = {}
        letter['from'] = 'grac'
        letter['to'] = 'session-manager'
        letter['title'] = 'media-control'
        letter['body'] = {'media':'sound', 'control':state}
        self.data_center.GRAC.grac_letter(json.dumps(letter))
        GracLog.get_logger().debug('SIGNAL SOUND {}'.format(state))

        if notimsg and state == JSON_RULE_DISALLOW:
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_SOUND, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                grmcode, self.data_center)#, flag=RED_ALERT_ALERTONLY)
        '''
        p0 = subprocess.Popen(
            ['/usr/bin/pacmd', 'list-sinks'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        pp_out, pp_err = p0.communicate()
        pp_out = pp_out.decode('utf8')

        for l in pp_out.split('\n'):
            if not l:
                continue
            if not 'index' in l:
                continue

            sound_indexes = re.sub(r'\s+', '', l.split(':')[1])

            for i in sound_indexes:
                if state == JSON_RULE_DISALLOW:
                    p1 = subprocess.Popen(
                        ['/usr/bin/pactl', 'set-sink-mute', i, '1'],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)
                    pp_out, pp_err = p1.communicate()
                    GracLog.get_logger().debug('BLOCK SOUND')
                else:
                    p1 = subprocess.Popen(
                        ['/usr/bin/pactl', 'set-sink-mute', i, '0'],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)
                    pp_out, pp_err = p1.communicate()
                    GracLog.get_logger().debug('UNBLOCK SOUND')
        '''
        
    def pactl_mic(self, state, notimsg=True):
        """
        allow/disallow microphone with pactl
        """

        letter = {}
        letter['from'] = 'grac'
        letter['to'] = 'session-manager'
        letter['title'] = 'media-control'
        letter['body'] = {'media':'microphone', 'control':state}
        self.data_center.GRAC.grac_letter(json.dumps(letter))
        GracLog.get_logger().debug('SIGNAL MIC {}'.format(state))

        if notimsg and state == JSON_RULE_DISALLOW:
            logmsg, notimsg, grmcode = \
                make_media_msg(JSON_RULE_MICROPHONE, state)
            red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                grmcode, self.data_center)#, flag=RED_ALERT_ALERTONLY)
        '''
        p0 = subprocess.Popen(
            ['/usr/bin/pacmd', 'list-sources'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        pp_out, pp_err = p0.communicate()
        pp_out = pp_out.decode('utf8')

        for l in pp_out.split('\n'):
            if not l:
                continue
            if not 'index' in l:
                continue
            mic_indexes = re.sub(r'\s+', '', l.split[1])

            for i in mic_indexes:
                if state == JSON_RULE_DISALLOW:
                    p1 = subprocess.Popen(
                        ['/usr/bin/pactl', 'set-source-mute', i, '1'],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)
                    pp_out, pp_err = p1.communicate()
                    GracLog.get_logger().debug('BLOCK MICROPHONE')
                else:
                    p1 = subprocess.Popen(
                        ['/usr/bin/pactl', 'set-source-mute', i, '0'],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)
                    pp_out, pp_err = p1.communicate()
                    GracLog.get_logger().debug('UNBLOCK MICROPHONE')
        '''
        
    def process_IN_ACCESS(self, event):
        """
        access callback
        """

        sound_rule = self.data_center.get_media_state(JSON_RULE_SOUND)
        if sound_rule == JSON_RULE_DISALLOW:
            self.pactl_sound(JSON_RULE_DISALLOW)
        
        mic_rule = self.data_center.get_media_state(JSON_RULE_MICROPHONE)
        if mic_rule == JSON_RULE_DISALLOW:
            self.pactl_mic(JSON_RULE_DISALLOW)
