#! /usr/bin/env python3

#-----------------------------------------------------------------------
import subprocess
import sys
import re

#-----------------------------------------------------------------------
RULE_DISALLOW = 'disallow'
RULE_ALLOW = 'allow'

#-----------------------------------------------------------------------
def pactl_sound(state):
    """
    pactl sound allow/disallow
    """

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

        sound_index = re.sub(r'\s+', '', l.split(':')[1])

        if state == RULE_DISALLOW:
            p1 = subprocess.Popen(
                ['/usr/bin/pactl', 'set-sink-mute', sound_index, '1'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            pp_out, pp_err = p1.communicate()
        else:
            p1 = subprocess.Popen(
                ['/usr/bin/pactl', 'set-sink-mute', sound_index, '0'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            pp_out, pp_err = p1.communicate()

#-----------------------------------------------------------------------
def pactl_mic(state):
    """
    pactl microphone allow/disallow
    """

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
        mic_index = re.sub(r'\s+', '', l.split(':')[1])

        if state == RULE_DISALLOW:
            p1 = subprocess.Popen(
                ['/usr/bin/pactl', 'set-source-mute', mic_index, '1'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            pp_out, pp_err = p1.communicate()
        else:
            p1 = subprocess.Popen(
                ['/usr/bin/pactl', 'set-source-mute', mic_index, '0'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            pp_out, pp_err = p1.communicate()

#-----------------------------------------------------------------------
if __name__ == '__main__':

    if len(sys.argv) < 3:
        print('usage: grac-pactl.py sound|microphone allow/disallow')
        sys.exit(0)


    media = sys.argv[1]
    state = sys.argv[2]
    if state != RULE_ALLOW and state != RULE_DISALLOW:
        print('{} is wrong rule.'\
            'rule must be allow or disallow'.format(state))
        sys.exit(0)

    if media == 'sound':
        pactl_sound(state)
    elif media == 'microphone':
        pactl_mic(state)
    else:
        print('{} is wrong media.'\
            'media must be sound or microphone '.format(media))

