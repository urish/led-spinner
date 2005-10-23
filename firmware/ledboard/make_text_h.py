# File: make_text_h.py
# Revision: $Id$

data = file('text.txt', 'r')
cols = [[]]
for line in data.readlines():
	for index, char in enumerate(line.rstrip('\r\n')):
		if len(cols) <= index:
			cols.append([])
		cols[index].append(char)

print """
#ifndef __TEXT_H_
#define __TEXT_H_

#define BITMAP_HEIGHT	= (%d)
#define BITMAP_WIDTH	= (%d)

static unsigned char g_bitmap[] = {
	""" % (len(cols[0]), len(cols)),

for col_index, col_data in enumerate(cols):
	col_value = 0;
	for index, value in enumerate(col_data):
		if value == '*':
			col_value |= (1 << index)
	print "0x%02x," % col_value,
	if (col_index + 1) % 10 == 0:
		print "\n\t",
	

print """
};

#endif /* __TEXT_H_ */
"""