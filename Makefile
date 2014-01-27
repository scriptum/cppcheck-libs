TARGETS = geany.cfg glib.cfg selinux.cfg
COMPILER = python compile.py
all: $(TARGETS)

geany.cfg: geany.rules glib.cfg
	/bin/cat $^ | $(COMPILER) > $@

glib.cfg: glib.rules glib-functions.rules
	/bin/cat $^ | $(COMPILER) > $@

selinux.cfg: selinux.rules
	/bin/cat $^ | $(COMPILER) > $@

clean:
	/bin/rm -f $(TARGETS)
