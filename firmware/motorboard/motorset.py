# File: motorset.py
# Revision: $Id$

import sys
import time

sys.path.append('../spi')

import spi

RESOLUTION_TABLE = {
	0: 	1,
	1:	8,
	2:	64,
	3:	256,
	4:	1024,
}

def read_word():
	return (spi.inb() << 8) + spi.inb()

def write_word(value):
	spi.outb(value >> 8)
	spi.outb(value & 0xff)

def send_command(cmd_id):
	while (spi.iob(cmd_id) != 0xae):
		time.sleep(0.1)

def cmd_play():
	try:
		pitch = int(sys.argv[2])
	except:
		usage()
	send_command(0)
	spi.outb(pitch)

def cmd_show():
	send_command(4)
	spi.iob(0)
	print "Timer = %d" % read_word()
	print "Timer Resolution = %d" % (RESOLUTION_TABLE[spi.inb()], )
	seq = []
	for i in xrange(spi.inb()):
		seq += [spi.inb()]
	print "Sequence = %s" % seq

def cmd_timer():
	try:
		value = int(sys.argv[2])
	except:
		usage()
	send_command(1)
	write_word(value)

def cmd_res():
	try:
		value = int(sys.argv[2])
	except:
		usage()
	send_command(2)
	spi.outb(value)

def cmd_sense():
	send_command(7)
	spi.outb(0)

def cmd_seq():
	try:
		seq = []
		for item in sys.argv[2].split(','):
			seq += [int(item)]
	except:
		usage()
	send_command(3)
	spi.outb(len(seq))
	for item in seq:
		spi.outb(item)

def usage():
	print >> sys.stderr, "Usage: %s <cmd> [args]" % sys.argv[0]
	print >> sys.stderr, "Available commands:"
	print >> sys.stderr, "\tSHOW - Dumps current chip configuration"
	print >> sys.stderr, "\tPLAY pitch - Plays a beep on the device"
	print >> sys.stderr, "\tSEQ num,num,... - Sets the output sequence"
	print >> sys.stderr, "\tTIMER num - Sets the timer interval"
	print >> sys.stderr, "\tRES num - Sets the timer interval resolution"
	print >> sys.stderr, "\tSENSE - enters sense mode"
	
	sys.exit(-1)

COMMAND_TABLE = {
	"PLAY" : cmd_play,
	"SHOW" : cmd_show,
	"SEQ"  : cmd_seq,
	"TIMER": cmd_timer,
	"RES": cmd_res,
	"SENSE": cmd_sense,
}

if len(sys.argv) < 2:
	usage()
	
if sys.argv[1].upper() in COMMAND_TABLE:
	COMMAND_TABLE[sys.argv[1].upper()]()
else:
	usage()
