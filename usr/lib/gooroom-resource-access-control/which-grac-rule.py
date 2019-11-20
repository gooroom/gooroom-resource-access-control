#! /usr/bin/env python3

#-----------------------------------------------------------------------
import configparser
import OpenSSL
import base64
import os

from grac_util import GracConfig,verify_rule,grac_format_exc

#-----------------------------------------------------------------------
if __name__ == '__main__':

    default_json_rules_path = \
        GracConfig.get_config().get(
                                'MAIN', 
                                'GRAC_DEFAULT_JSON_RULES_PATH')
    user_json_rules_path = \
        GracConfig.get_config().get(
                                'MAIN', 
                                'GRAC_USER_JSON_RULES_PATH')

    if os.path.exists(user_json_rules_path):
        json_rules_path = user_json_rules_path
        try:
            verify_rule(json_rules_path)
        except:
            json_rules_path = default_json_rules_path
    else:
        json_rules_path = default_json_rules_path
        
    print(json_rules_path)

