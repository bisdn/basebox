#!/usr/bin/python

class IPv6Prefix(object):
    """Class for abstracting a single IPv6 prefix"""
    
    def __init__(self, prefix, prefixlen):
        self.prefix = prefix
        self.prefixlen = prefixlen
        self.pref = [x for x in prefix.split(':') if x != '']


    def __str__(self, *args, **kwargs):
        s_pref = ''
        for v in self.pref:
            s_pref += '[' + str(v) + '] '
        return '<IPv6Prefix [' + str(self.prefix) + '/' + str(self.prefixlen) + '] ' + s_pref + '>' 
    #+ object.__str__(self, *args, **kwargs) + ' >'

    def __eq__(self, obj):
        if not isinstance(obj, IPv6Prefix):
            return False
        return ((self.prefix == obj.prefix) and (self.prefixlen == obj.prefixlen))
    
    def get_subprefix(self, n, subprefixlen):
        'get subprefix n from overall prefix => /56 -> /64 with n = 0, 1, 2, ...'
        if n >= 2**subprefixlen:
            print 'invalid subprefix network index'
            raise
        self.pref[len(self.pref)] = str(int(self.pref[len(self.pref)], 16) + n)
        
        
        