# File: motorset.py
# Revision: $Id$

import struct
import sys
import time

# SUP board driverr
import supdrv

MOTOR_DEVICE_ID = 0x15

RESOLUTION_TABLE = {
	0: 	1,
	1:	8,
	2:	64,
	3:	256,
	4:	1024,
}

def send_command(cmd_code, cmd_data = ''):
	supdrv.i2c_init()
	supdrv.i2c_send(MOTOR_DEVICE_ID, chr(cmd_code) + cmd_data)
	
def read_response(size = None):
	if size:
		return supdrv.i2c_recv(MOTOR_DEVICE_ID, size)
	return supdrv.i2c_recv_dynamic(MOTOR_DEVICE_ID)

def cmd_play():
	try:
		pitch = int(sys.argv[2])
	except:
		usage()
	send_command(0, chr(pitch))

def cmd_show():
	send_command(4)
	resp = read_response()
	timer_scaler, timer_value = struct.unpack('=bH', resp[:3])
	print "Timer = %d" % (timer_value, )
	print "Timer scaler = %d" % (RESOLUTION_TABLE[timer_scaler], )
	seq = []
	for i in xrange(ord(resp[5])):
		seq += [ord(resp[6+i]), ]
	print "Sequence = %s" % (seq, )

def cmd_timer():
	try:
		value = int(sys.argv[2])
	except:
		usage()
	send_command(1, struct.pack('=H', value))

def cmd_res():
	try:
		value = int(sys.argv[2])
	except:
		usage()
	send_command(2, chr(value))

def cmd_seq():
	try:
		seq = []
		for item in sys.argv[2].split(','):
			seq += [int(item)]
	except:
		usage()
	send_command(3, chr(len(seq)) + ''.join(map(chr, seq)))

def usage():
	print >> sys.stderr, "Usage: %s <cmd> [args]" % sys.argv[0]
	print >> sys.stderr, "Available commands:"
	print >> sys.stderr, "\tSHOW - Dumps current chip configuration"
	print >> sys.stderr, "\tPLAY pitch - Plays a beep on the device"
	print >> sys.stderr, "\tSEQ num,num,... - Sets the output sequence"
	print >> sys.stderr, "\tTIMER num - Sets the timer interval"
	print >> sys.stderr, "\tRES num - Sets the timer interval resolution"
	
	sys.exit(-1)

COMMAND_TABLE = {
	"PLAY" : cmd_play,
	"SHOW" : cmd_show,
	"SEQ"  : cmd_seq,
	"TIMER": cmd_timer,
	"RES"  : cmd_res,
}

if len(sys.argv) < 2:
	usage()
	
if sys.argv[1].upper() in COMMAND_TABLE:
	COMMAND_TABLE[sys.argv[1].upper()]()
else:
	usage()
