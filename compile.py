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

print "<?xml version=\"1.0\"?>"
print "<!-- THIS FILE IS GENERATED AUTOMATICALLY. See https://github.com/scriptum/cppcheck-libs -->"
print "<def>"

if len(sys.argv) > 1:
	print "  <include><![CDATA["
	with open(sys.argv[1], 'r') as f:
		print (f.read())
	print "  ]]></include>"

for key in allocs:
	print "  <memory>";
	for k in allocs[key]:
		for vv in allocs[key][k]:
			init = ''
			name = vv
			if k == "alloc":
				try:
					name, init = vv.split()
					if init == 'noinit':
						init = ''
					else:
						init = ' init="true"'
				except:
					init = ' init="true"'
			print "    <"+k+init+">"+name+"</"+k+">"
	print "  </memory>"

for v in funcs:
	opts = v.split()
	print "  <function name=\""+opts[0]+"\">"
	noret = False
	for vv in opts[1:]:
		if vv == "leak-ignore":
			print "    <leak-ignore/>"
		elif vv == "noreturn":
			noret = True
			print "    <noreturn>true</noreturn>"
	if not noret:
		print "    <noreturn>false</noreturn>"
	print "  </function>"

print "</def>"
