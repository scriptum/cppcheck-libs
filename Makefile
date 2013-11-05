TARGETS = geany.cfg gtk.cfg selinux.cfg
COMPILER = python compile.py
all: $(TARGETS)

geany.cfg: geany.rules gtk.rules gtk-functions.rules
	/bin/cat $^ | $(COMPILER) > $@

gtk.cfg: gtk.rules gtk-functions.rules
	/bin/cat $^ | $(COMPILER) > $@

selinux.cfg: selinux.rules
	/bin/cat $^ | $(COMPILER) > $@

clean:
	/bin/rm -f $(TARGETS)
