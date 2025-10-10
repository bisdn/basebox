#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: © 2017 BISDN GmbH
# SPDX-License-Identifier: MPL-2.0-no-copyleft-exception
#
# Helper script for resetting KNET to its initial state by removing all
# added interfaces and filters, intended to be called after stopping
# baseboxd.

from ctypes import *
import fcntl
import os

class bkn_ioctl_t(Structure):
    _fields_ = [("rc", c_int),
                ("len", c_int),
                ("bufsz", c_int),
                ("reserved", c_int),
                ("buf", c_uint64)]

class kcom_msg_hdr_t(Structure):
    _fields_ = [("type", c_uint8),
                ("opcode", c_uint8),
                ("seqno", c_uint8),
                ("status", c_uint8),
                ("unit", c_uint8),
                ("reserved", c_uint8),
                ("id", c_uint16)]

class kcom_msg_list_t(Structure):
    _fields_ = [("hdr", kcom_msg_hdr_t),
                ("fcnt", c_uint16),
                ("id", c_uint16 * 128)]

KCOM_MSG_TYPE_CMD = 1

KCOM_M_NETIF_DESTROY = 12
KCOM_M_NETIF_LIST = 13

KCOM_M_FILTER_DESTROY = 22
KCOM_M_FILTER_LIST = 23

# Used for NETIF_DESTROY and FILTER_DESTROY.
#
# Both use only the id field of the header to identify the filter/netif
# to destroy.
def delete_kcom_object(fd, obj_id, opcode):
    hdr = kcom_msg_hdr_t()
    hdr.type = KCOM_MSG_TYPE_CMD
    hdr.opcode = opcode
    hdr.unit = 0
    hdr.id = obj_id

    io = bkn_ioctl_t()
    io.len = sizeof(hdr)
    io.buf = addressof(hdr)

    fcntl.ioctl(fd, 0, io)

# Used for NETIF_LIST and FILTER_LIST
#
# Technically they use different structs passed to the kernel module,
# practically they have identical layouts, so we can just use one type.
def get_kcom_obj_list(fd, opcode):
    obj_list = kcom_msg_list_t()
    obj_list.hdr.type = KCOM_MSG_TYPE_CMD
    obj_list.hdr.opcode = opcode
    obj_list.hdr.unit = 0

    io = bkn_ioctl_t()
    io.len = sizeof(obj_list)
    io.buf = addressof(obj_list)

    fcntl.ioctl(fd, 0, io)

    return obj_list

try:
    fd = os.open("/dev/linux-bcm-knet", os.O_RDWR | os.O_NONBLOCK)

    kcom_filters = get_kcom_obj_list(fd, KCOM_M_FILTER_LIST)
    for i in range(1, kcom_filters.fcnt + 1):
        # do not delete the default RxAPI filter
        if kcom_filters.id[i] == 1:
            continue
        delete_kcom_object(fd, kcom_filters.id[i], KCOM_M_FILTER_DESTROY)

    kcom_netifs = get_kcom_obj_list(fd, KCOM_M_NETIF_LIST)
    for i in range(1, kcom_netifs.fcnt + 1):
        delete_kcom_object(fd, kcom_netifs.id[i], KCOM_M_NETIF_DESTROY)
except FileNotFoundError:
    # BCM KNET support module not loaded or supported, so nothing to do for us
    pass
