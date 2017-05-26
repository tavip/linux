#!/usr/bin/python3 -u

import argparse, glob, os.path, re, sys

parser = argparse.ArgumentParser(description='Generate skeletons sources from full sources')
parser.add_argument('dirs', metavar='dir', nargs='+', help='list of directories to process')
parser.add_argument('output', help='output dir to copy processed files')
parser.add_argument('--todo', type=int, help='don\'t remove TODOs less then this', default=1)
args = parser.parse_args()

for d in args.dirs:
	paths = glob.glob(d + "/*.c")
	paths += glob.glob(d + "/*.h")
	paths += glob.glob(d + "/Kbuild")
	for p in paths:
		name=os.path.basename(p)
		try:
			os.makedirs(os.path.join(args.output, os.path.dirname(p)))
		except:
			pass
		if name == "Kbuild":
			pattern="(#\s*TODO)([0-9]*)\/?([0-9]*)(.*)"
		else:
			pattern="(\s*/\*\s*TODO)([0-9]*)/?([0-9]*)(.*)"
		f = open(p)
		g = open(os.path.join(args.output, p), "w")
		skip_lines = 0
		for l in f.readlines():
			if skip_lines > 0:
				skip_lines -= 1
				continue
			m = re.search(pattern, l)
			if m:
				todo=1
				if m.group(2):
					todo = int(m.group(2))
				if todo >= args.todo:
					if m.group(3):
						skip_lines = int(m.group(3))
					else:
						skip_lines = 1
				l = "%s%s%s\n" % (m.group(1), m.group(2), m.group(4))
			g.write(l)

