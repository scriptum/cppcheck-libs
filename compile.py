#!/usr/bin/env python
#-*- coding:utf-8 -*-
import sys, re
allocs_i = 0
allocs = {}
funcs = []
for line in sys.stdin:
	line = line.strip()
	if line == "" or line[0] == "#":
		continue
	m = re.match('^alloc(\[(.*)\])?:$', line)
	if m:
		allocs_i += 1
		key = 'alloc'
		if m.group(2):
			if m.group(2) not in allocs:
				allocs[m.group(2)] = {}
			current = m.group(2)
		else:
			current = str(allocs_i)
			allocs[current] = {}
	elif line == 'dealloc:':
		key = 'dealloc'
	elif line == 'use:':
		key = 'use'
	elif line == 'functions:':
		key = 'functions'
	else:
		if key == 'functions':
			funcs.append(line)
		else:
			if not key in allocs[current]:
				allocs[current][key] = []
			allocs[current][key].append(line)

print "<?xml version=\"1.0\"?>\n<def>"

for key in allocs:
	print "\t<memory>";
	for k in allocs[key]:
		for vv in allocs[key][k]:
			print "\t\t<"+k+">"+vv+"</"+k+">"
	print "\t</memory>"

for v in funcs:
	opts = v.split()
	print "\t<function name=\""+opts[0]+"\">"
	for vv in opts[1:]:
		if vv == "leak-ignore":
			print "\t\t<leak-ignore/>"
		elif vv == "return":
			print "\t\t<noreturn>false</noreturn>"
		elif vv == "noreturn":
			print "\t\t<noreturn>true</noreturn>"
	print "\t</function>"

print "</def>"
