from xml.dom.minidom import *
dictionary = {}

with open("gtk.rules") as f:
	for line in f:
		if "g_" in line and "\t" in line:
			dictionary[line.split()[0].strip()] = True
# print dictionary
gir = parse('/usr/share/gir-1.0/GLib-2.0.gir')

memory = []

def getAttr(node, attr):
	try:
		return node.attributes[attr].nodeValue
	except:
		return ""

def getType(node):
	try:
		t = node.getElementsByTagName('type')[0]
		return t.attributes['c:type'].nodeValue
	except:
		t = node.getElementsByTagName('array')[0]
		return 'array ' + t.attributes['c:type'].nodeValue
def getOwner(node):
	try:
		return node.attributes['transfer-ownership'].nodeValue
	except:
		return 'none'

print "functions:"

for func in gir.getElementsByTagName('function') + gir.getElementsByTagName('method'):
	param_has_ptr = False
	try:
		for param in func.getElementsByTagName('parameter'):
			p_type = getType(param)
			if p_type == "gpointer" or "*" in p_type:
				param_has_ptr = True
	except:
		pass
	ret = func.getElementsByTagName('return-value')[0]
	ret_type = getType(ret)
	if ret_type == "void":
		ret = 'noreturn'
	else:
		ret = 'return'
	f_name = getAttr(func, 'c:identifier')
	if func.parentNode.nodeName == "record" or param_has_ptr:
		ret += ' leak-ignore'
	if f_name not in dictionary:
		print "\t", f_name + "\t" + ret

# for methods in gir.getElementsByTagName('record'):
# for method in gir.getElementsByTagName('method'):
	# print getAttr(method, 'c:identifier')