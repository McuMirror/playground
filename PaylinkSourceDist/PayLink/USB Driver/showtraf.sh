#!/bin/sh
#-----------------------------------------------------------------------------#
# The AESCDriver / Paylink application catchs the signal of type USR1 and uses this to
# toggle the value of ShowTraffic. As there are two AESWDriver process running
# the killall command is used (only the process which cares about the 
# ShowTraffic variable changes it's state). 
#-----------------------------------------------------------------------------#
killall -USR1 Paylink    1>/dev/null 2>&1
killall -USR1 AESCDriver 1>/dev/null 2>&1
#-----------------------------------------------------------------------------#



