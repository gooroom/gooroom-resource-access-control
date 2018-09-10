#! /usr/bin/env python3

#-----------------------------------------------------------------------
import os
os.environ['XTABLES_LIBDIR'] = '/usr/lib/x86_64-linux-gnu/xtables/'
import iptc

#-----------------------------------------------------------------------
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

    def reset_iptables(self, policy_state):
        """
        ipv4 and ipv6 tables reset
        """

        accept_policy = iptc.Policy('ACCEPT')
        drop_policy = iptc.Policy('DROP')

        #v4 flush
        input_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'INPUT')
        output_chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), 'OUTPUT')
        input_chain.flush()
        output_chain.flush()

        #v4 policy
        if policy_state == JSON_RULE_ALLOW:
            input_chain.set_policy(accept_policy)
            output_chain.set_policy(accept_policy)
        else:
            input_chain.set_policy(drop_policy)
            output_chain.set_policy(drop_policy)

        #v6 flush
        input_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'INPUT')
        output_chain6 = iptc.Chain(iptc.Table6(iptc.Table6.FILTER), 'OUTPUT')
        input_chain6.flush()
        output_chain6.flush()

        #v6 policy
        if policy_state == JSON_RULE_ALLOW:
            input_chain6.set_policy(accept_policy)
            output_chain6.set_policy(accept_policy)
        else:
            input_chain6.set_policy(drop_policy)
            output_chain6.set_policy(drop_policy)

    def reload(self):
        """
        reload
        """
            
        try:
            json_rules = self.data_center.get_json_rules()
            network_json = json_rules[JSON_RULE_NETWORK]
            policy_state = network_json[JSON_RULE_NETWORK_STATE]

            #reset
            self.reset_iptables(policy_state)

            #rules
            if not JSON_RULE_NETWORK_RULES in network_json:
                return

            rules_json = network_json[JSON_RULE_NETWORK_RULES]
            for rule_json in rules_json:
                ip = rule_json[JSON_RULE_NETWORK_IPADDRESS]
                direction_org = rule_json[JSON_RULE_NETWORK_DIRECTION]
                rule_state = rule_json[JSON_RULE_NETWORK_STATE]
                ports_json = rule_json[JSON_RULE_NETWORK_PORTS]

                directions = None
                if direction_org == JSON_RULE_NETWORK_ALLBOUND:
                    directions = (JSON_RULE_NETWORK_INBOUND,
                                JSON_RULE_NETWORK_OUTBOUND)
                else:
                    directions = (direction_org, None)

                for direction in directions:
                    if not direction:
                        continue

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

                            if src_ports:
                                self.logger.debug('src ports={}'.format(src_ports))
                                sp_match = iptc.Match(rule, 'multiport')
                                sp_match.sports = src_ports
                                rule.add_match(sp_match)
                            if dst_ports:
                                self.logger.debug('dst prots={}'.format(dst_ports))
                                dp_match = iptc.Match(rule, 'multiport')
                                dp_match.dports = dst_ports
                                rule.add_match(dp_match)

                        self.insert_rule(rule, direction, ip, protocol)
        except:
            self.logger.error(grac_format_exc())
                
    def ip_type(self, ip):
        """
        check ipv4 | ipv6 | domain
        """

        try:
            it = ipaddress.ip_address(ip)
            if isinstance(it, ipaddress.IPv4Address):
                return 'v4'
            return 'v6'
        except:
            #not checking value
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

        if rule_state == JSON_RULE_ALLOW:
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
            if '-' in ranged_port:
                ranged_port = ranged_port.replace('-', ':')
                ports.append(ranged_port)
            else:
                ports.append(ranged_port)

        if len(ports) == 0:
            return None
        else:
            return ','.join(ports)