from xml.dom.minidom import *
dictionary = {}

with open("gtk.rules") as f:
	for line in f:
		if "\t" in line and not line.strip() == "":
			dictionary[line.split()[0].strip()] = True
# print dictionary
# exit(0)
xml_glib = parse('/usr/share/gir-1.0/GLib-2.0.gir')
xml_gtk = parse('/usr/share/gir-1.0/Gtk-2.0.gir')

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

for func in xml_glib.getElementsByTagName('function') + xml_glib.getElementsByTagName('method') + xml_gtk.getElementsByTagName('function') + xml_gtk.getElementsByTagName('method'):
	param_has_ptr = False
	try:
		for param in func.getElementsByTagName('parameter'):
			p_type = getType(param)
			if p_type == "gpointer" or "*" in p_type:
				param_has_ptr = True
	except:
		pass
	# ret = func.getElementsByTagName('return-value')[0]
	# ret_type = getType(ret)
	# if ret_type == "void":
		# ret = 'noreturn'
	# else:
		# ret = 'return'
	f_name = getAttr(func, 'c:identifier')
	#if func.parentNode.nodeName == "record" or param_has_ptr:
	ret = ' leak-ignore'
	if f_name not in dictionary:
		print "\t", f_name + "\t" + ret



# for methods in gir.getElementsByTagName('record'):


# for rec in gir.getElementsByTagName('record'):
	# a = False
	# for func in rec.getElementsByTagName('constructor'):
		# ret = func.getElementsByTagName('return-value')[0]
		# ret_type = getType(ret)
		# if (ret_type == "gpointer" or "*" in ret_type) and "const" not in ret_type:
			# a = True
			# print getAttr(func, 'c:identifier'), ret_type
	# if a:
		# print rec.toxml()
