#!/usr/bin/python

from netaddr import *


class IPv6PrefixBaseException(BaseException):
    pass


class IPv6PrefixNoSubnets(IPv6PrefixBaseException):
    pass


class IPv6PrefixOutOfRange(IPv6PrefixBaseException):
    pass


class IPv6Prefix(object):
    """Class for abstracting a single IPv6 prefix"""
    
    def __init__(self, prefix, prefixlen):
        self.prefix = IPAddress(prefix)
        self.prefixlen = int(prefixlen)


    def __str__(self, *args, **kwargs):
        return '<IPv6Prefix [' + str(self.prefix) + '/' + str(self.prefixlen) + '] ' + '>' 
    #+ object.__str__(self, *args, **kwargs) + ' >'

    def __eq__(self, obj):
        if not isinstance(obj, IPv6Prefix):
            return False
        return ((self.prefix == obj.prefix) and (self.prefixlen == obj.prefixlen))
    
    def get_subprefix(self, n):
        'get subprefix n from overall prefix => /56 -> /64 with n = 0, 1, 2, ...'
        # our range is effectively (64 - self.prefixlen), as radvd announce /64 prefixes
        # only. n denotes the index of all available networks like this:
        #
        # self.prefixlen = 56
        # number of available n_networks = 2**(64-56) = 2**8 = 256 / n must be between 0 and (256-1)
        # n: 0, 1, 2, ...
        if self.prefixlen > 64:
            raise IPv6PrefixNoSubnets
        n_networks = 2**(64 - self.prefixlen)
        if n >= n_networks or n_networks > 2**16:
            raise IPv6PrefixOutOfRange
        # calculate subprefix
        value = IPAddress(self.prefix.value + (n << 64))
        return IPv6Prefix(str(value), 64)
        
        