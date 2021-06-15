#! /usr/bin/env python3

#-----------------------------------------------------------------------
import os
os.environ['XTABLES_LIBDIR'] = '/usr/lib/x86_64-linux-gnu/xtables/'
import iptc

#-----------------------------------------------------------------------
import subprocess
import ipaddress

from grac_util import GracLog,GracConfig,grac_format_exc
from grac_define import *

#-----------------------------------------------------------------------
class GracNetwork:
    """
    GracNetwork: iptables
    """

    def __init__(self, data_center):

        self.logger = GracLog.get_logger()

        #DATA CENTER
        self.data_center = data_center

    def flush_chain(self):
        """
        flush input/output chain
        """

        #v4 flush
        input_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'INPUT')
        output_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'OUTPUT')
        input_chain.flush()
        output_chain.flush()

        #v6 flush
        input_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'INPUT')
        output_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'OUTPUT')
        input_chain6.flush()
        output_chain6.flush()

    def set_chain_policy(self, policy_state):
        """
        set input/output chain policy
        """

        accept_policy = iptc.Policy('ACCEPT')
        drop_policy = iptc.Policy('DROP')

        #v4 policy
        input_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'INPUT')
        output_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'OUTPUT')

        if policy_state.lower() == JSON_RULE_NETWORK_ACCEPT \
            or policy_state.lower() == JSON_RULE_ALLOW:
            input_chain.set_policy(accept_policy)
            output_chain.set_policy(accept_policy)
        else:
            input_chain.set_policy(drop_policy)
            output_chain.set_policy(drop_policy)

        #v6 policy
        input_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'INPUT')
        output_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'OUTPUT')

        if policy_state.lower() == JSON_RULE_NETWORK_ACCEPT \
            or policy_state.lower() == JSON_RULE_ALLOW:
            input_chain6.set_policy(accept_policy)
            output_chain6.set_policy(accept_policy)
        else:
            input_chain6.set_policy(drop_policy)
            output_chain6.set_policy(drop_policy)

    def now_v2_x(self, policy_state, network_json):
        """
        processing more than v2.x with raw input
        """

        if not isinstance(network_json, str) and JSON_RULE_NETWORK_RULES in network_json:
            rules = network_json[JSON_RULE_NETWORK_RULES]
            rules = [(int(rule['seq']), 'MENUAL', rule) for rule in rules]
        else:
            rules = []

        if not isinstance(network_json, str) and JSON_RULE_NETWORK_RULES_RAW in network_json:
            rules_law = network_json[JSON_RULE_NETWORK_RULES_RAW]
            rules_law = [(int(rule_law['seq']), 'RAW', rule_law) for rule_law in rules_law]
        else:
            rules_law = []
        rules_all = sorted(rules+rules_law, key=lambda i:i[0])

        with open(GRAC_IPTABLES_SV_PATH, 'w') as f:
            f.write('/usr/sbin/iptables -F\n')
            f.write('/usr/sbin/iptables -P INPUT {}\n'.format(policy_state.upper()))
            f.write('/usr/sbin/iptables -P OUTPUT {}\n'.format(policy_state.upper()))
            f.write('/usr/sbin/iptables -P FORWARD {}\n'.format(policy_state.upper()))

            for rule in rules_all:
                try:
                    if rule[1] == 'MENUAL':
                        l = rule[2]
                        direction = l['direction']
                        protocol = l['protocol']
                        ipaddress = l['ipaddress']
                        src_ports = l['src_ports']
                        dst_ports = l['dst_ports']
                        state = l['state']

                        if direction.lower() == 'all':
                            directions = ('INPUT', 'OUTPUT')
                        else:
                            directions = (direction.upper(),)
                        for di in directions:
                            s = ''
                            s += '-A {}'.format(di)
                            s += ' -p {}'.format(protocol)
                            if ipaddress:
                                if di == 'INPUT':
                                    s += ' -s {}'.format(ipaddress)
                                else:
                                    s += ' -d {}'.format(ipaddress)
                            if src_ports and protocol != 'icmp':
                                if protocol.lower() != 'icmp':
                                    s += ' -m multiport'
                                s += ' --sports {}'.format(src_ports)
                            if dst_ports and protocol != 'icmp':
                                if protocol.lower() != 'icmp':
                                    s += ' -m multiport'
                                s += ' --dports {}'.format(dst_ports)
                            s += ' -j {}'.format(state.upper())
                            f.write('/usr/sbin/iptables ' +s+'\n')
                    else:
                        f.write('/usr/sbin/iptables ' + rule[2]['cmd']+'\n')
                except:
                    self.logger.error(grac_format_exc())

        p0 = subprocess.Popen(
            ['/bin/bash', GRAC_IPTABLES_SV_PATH],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        pp_out, pp_err = p0.communicate()
        if pp_err:
            self.logger.error(pp_err.decode('utf8'))
        
    def reload(self):
        """
        reload
        """
            
        try:
            json_rules = self.data_center.get_json_rules()
            network_json = json_rules[JSON_RULE_NETWORK]

            #ADDING VERSION PROCESS
            server_version = '1.0'

            if isinstance(network_json, str):
                policy_state = network_json
            else:
                policy_state = network_json[JSON_RULE_NETWORK_STATE]
                if 'version' in network_json:
                    server_version = network_json[JSON_RULE_NETWORK_VERSION]

            if server_version.startswith('2.'):
                self.now_v2_x(policy_state, network_json)
                return

            #flush
            self.flush_chain()
            self.flush_chain()

            #policy to accept
            self.set_chain_policy(JSON_RULE_NETWORK_ACCEPT)

            #rules
            if isinstance(network_json, str) \
                or not JSON_RULE_NETWORK_RULES in network_json:
                self.set_chain_policy(policy_state)
                return

            rules_json = network_json[JSON_RULE_NETWORK_RULES]
            rules_json.reverse()

            for rule_json in rules_json:
                ip = rule_json[JSON_RULE_NETWORK_IPADDRESS]
                if ip:
                    ip = ip.strip()
                direction_org = rule_json[JSON_RULE_NETWORK_DIRECTION].lower()
                rule_state = rule_json[JSON_RULE_NETWORK_STATE].lower()

                directions = None
                if direction_org == JSON_RULE_NETWORK_ALLBOUND:
                    directions = (JSON_RULE_NETWORK_INBOUND,
                                JSON_RULE_NETWORK_OUTBOUND)
                else:
                    directions = (direction_org, None)

                for direction in directions:
                    if not direction:
                        continue

                    #SERVER VERSION 1.0
                    if server_version.startswith('1.0'):
                        if JSON_RULE_NETWORK_PORTS in rule_json:
                            ports_json = rule_json[JSON_RULE_NETWORK_PORTS]
                        else:
                            ports_json = []

                        #no ports
                        if len(ports_json) == 0:
                            rule = self.create_rule(rule_state, ip)
                            self.insert_rule(rule, direction, ip, None)
                            continue
                            
                        #ports
                        for port_json in ports_json:
                            rule = self.create_rule(rule_state, ip)

                            protocol = port_json[JSON_RULE_NETWORK_PROTOCOL]
                            rule.protocol = protocol

                            if protocol != 'icmp':
                                src_ports = self.ranged_ports_to_string(
                                    port_json[JSON_RULE_NETWORK_SRC_PORT])
                                dst_ports = self.ranged_ports_to_string(
                                    port_json[JSON_RULE_NETWORK_DST_PORT])

                                if src_ports and len(src_ports) > 0 and src_ports[0]:
                                    self.logger.debug('src ports={}'.format(src_ports))
                                    sp_match = iptc.Match(rule, 'multiport')
                                    sp_match.sports = src_ports
                                    rule.add_match(sp_match)
                                if dst_ports and len(dst_ports) > 0 and dst_ports[0]:
                                    self.logger.debug('dst prots={}'.format(dst_ports))
                                    dp_match = iptc.Match(rule, 'multiport')
                                    dp_match.dports = dst_ports
                                    rule.add_match(dp_match)

                            self.insert_rule(rule, direction, ip, protocol)
                    #SERVER VERSION NOT 1.0
                    else:
                        protocol = rule_json[JSON_RULE_NETWORK_PROTOCOL].lower()

                        #ICMP HERE
                        if protocol == 'icmp':
                            rule = self.create_rule(rule_state, ip)
                            rule.protocol = protocol
                            self.insert_rule(rule, direction, ip, protocol)
                            continue
                        elif not protocol:
                            rule = self.create_rule(rule_state, ip)
                            self.insert_rule(rule, direction, ip, None)
                            continue

                        src_ports = rule_json[JSON_RULE_NETWORK_SRC_PORTS]
                        dst_ports = rule_json[JSON_RULE_NETWORK_DST_PORTS]

                        #no ports
                        if not src_ports and not dst_ports:
                            rule = self.create_rule(rule_state, ip)
                            rule.protocol = protocol
                            self.insert_rule(rule, direction, ip, None)
                            continue

                        #ports
                        rule = self.create_rule(rule_state, ip)
                        rule.protocol = protocol

                        if src_ports:
                            src_ports = \
                                self.ranged_ports_to_string(src_ports.split(','))
                            sp_match = iptc.Match(rule, 'multiport')
                            sp_match.sports = src_ports
                            rule.add_match(sp_match)

                        if dst_ports:
                            dst_ports = \
                                self.ranged_ports_to_string(dst_ports.split(','))
                            sp_match = iptc.Match(rule, 'multiport')
                            sp_match.dports = dst_ports
                            rule.add_match(sp_match)
                            
                        self.insert_rule(rule, direction, ip, protocol)

            self.set_chain_policy(policy_state)
        except:
            self.logger.error(grac_format_exc())
                
    def ip_type(self, ip):
        """
        check ipv4 | ipv6 | domain
        """

        try:
            ip = ip.split('/')[0]
            it = ipaddress.ip_address(ip)
            if isinstance(it, ipaddress.IPv4Address):
                return 'v4'
            return 'v6'
        except:
            #not checking value
            self.logger.info(grac_format_exc())
            return 'domain'

    def insert_rule(self, rule, direction, ip, protocol):
        """
        insert rule
        """

        #domain to ip
        resolved_list = []
        it = self.ip_type(ip)
        if it == 'domain':
            resolved_list = self.dns_resolver(ip)
        else:
            resolved_list.append(ip)
        self.logger.debug('resolved_list={}'.format(resolved_list))

        #chains
        input_chain = output_chain = None
        if it == 'v6':
            input_chain = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'INPUT')
            output_chain = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'OUTPUT')
        else:
            input_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'INPUT')
            output_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'OUTPUT')

        for resolved in resolved_list:
            if direction == JSON_RULE_NETWORK_INBOUND:
                rule.src = resolved
                self._insert_rule_with_log(input_chain, rule)
            elif direction == JSON_RULE_NETWORK_OUTBOUND:
                rule.dst = resolved 
                self._insert_rule_with_log(output_chain, rule)
            else:
                rule.src = resolved
                self._insert_rule_with_log(input_chain, rule)
                rule.dst = resolved
                self._insert_rule_with_log(output_chain, rule)

    def _insert_rule_with_log(self, chain, rule):
        """
        insert rule with log
        """

        chain.insert_rule(rule)

        if rule.target.name == 'DROP':
            backup_target = rule.target
            rule.create_target('LOG')
            rule.target.log_prefix = 'GRAC: Disallowed Network'
            rule.target.log_level = 'error'
            limit_match = iptc.Match(rule, 'limit')
            limit_match.limit = '1/min'
            limit_match.limit_burst = '1'
            rule.add_match(limit_match)
            chain.insert_rule(rule)
            rule.target = backup_target

    def create_rule(self, rule_state, ip):
        """
        create rule
        """

        rule = None
        if self.ip_type(ip) == 'v6':
            rule = iptc.Rule6()
        else:
            rule = iptc.Rule()

        if rule_state.lower() == JSON_RULE_NETWORK_ACCEPT \
            or rule_state.lower() == JSON_RULE_ALLOW:
            rule.create_target('ACCEPT')
        else:
            rule.create_target('DROP')
        return rule
            
    def dns_resolver(self, domain):
        """
        dns resolver
        """

        try:
            import DNS
            req = DNS.DnsRequest(domain, qtype='A').req().answers
            return [r['data'] for r in req if r['typename'] == 'A']
        except:
            self.logger.error(grac_format_exc())
            return []

    '''
    def dns_resolver(self, domain):
        """
        dns resolver
        """

        import dns.resolver

        resolver = dns.resolver.Resolver()
        resolved = resolver.query(domain, 'A')
        return [ip.address for ip in resolved]
    '''

    def ranged_ports_to_string(self, ranged_ports):
        """
        ranged ports to list
        """

        ports = []

        for ranged_port in ranged_ports:
            ranged_port = ranged_port.strip()
            if '-' in ranged_port:
                ranged_port = ranged_port.replace('-', ':')
                ports.append(ranged_port)
            else:
                ports.append(ranged_port)

        if len(ports) == 0:
            return None
        else:
            return ','.join(ports)
