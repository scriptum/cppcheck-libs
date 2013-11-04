TARGETS = geany.cfg gtk.cfg

all: $(TARGETS)

geany.cfg: geany.rules gtk.rules
	cat $^ | perl compile.pl > $@

gtk.cfg: gtk.rules
	cat $^ | perl compile.pl > $@

clean:
	/bin/rm -f $(TARGETS)