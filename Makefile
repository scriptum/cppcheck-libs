TARGETS = geany.cfg gtk.cfg selinux.cfg
COMPILER = python compile.py

TMP = .tmp_test_result
REPLACE = sed -e 's/^</ NEW:/' -e 's/^>/LOST:/'
CHECK_TAIL = 2>&1 | sort > $(TMP)
CPPCHECK = ../cppcheck/cppcheck -q -j4 --max-configs=1 --std=posix  --enable=warning,performance,style,portability
CHECK_GTK = $(CPPCHECK) --library=gtk --include=gtk.h tests/gtk $(CHECK_TAIL)
CHECK_STD = $(CPPCHECK) --include=gnu.h tests/std $(CHECK_TAIL)
CHECK_SELINUX = $(CPPCHECK) --library=selinux --include=selinux.h tests/selinux $(CHECK_TAIL)

CHECK_FULL = $(CPPCHECK) --library=gtk --include=gtk.h --include=gnu.h tests/full $(CHECK_TAIL)

all: $(TARGETS)

geany.cfg: geany.rules
	cat geany.rules | $(COMPILER) > $@

gtk.cfg: gtk.rules gtk-functions.rules gtk.h compile.py
	grep -Ehor '#define G_\w+\(\w+\)' /usr/include/glib-2.0/ | sed -r 's/\((\w+)\)/(\1) (\1)/' | grep -v " G.*_IS_" | grep -v " G_VALUE_HOLDS_" | sort -u > .gtk-auto.h
	cat gtk.rules gtk-functions.rules | $(COMPILER) gtk.h .gtk-auto.h > $@
	rm .gtk-auto.h

selinux.cfg: selinux.rules selinux.h compile.py
	cat selinux.rules | $(COMPILER) selinux.h > $@

clean:
	rm -f $(TARGETS)

test_gtk: all
	@$(CHECK_GTK) && diff $(TMP) tests/test_result_gtk | $(REPLACE)

test_gtk_replace: all
	@$(CHECK_GTK) > tests/test_result_gtk

test_selinux: all
	@$(CHECK_SELINUX) && diff $(TMP) tests/test_result_selinux | $(REPLACE)

test_selinux_replace: all
	@$(CHECK_SELINUX) > tests/test_result_selinux

test_std: all
	@$(CHECK_STD) && diff $(TMP) tests/test_result_std | $(REPLACE)

test_std_replace: all
	@$(CHECK_STD) > tests/test_result_std

test_full: all
	@$(CHECK_FULL) && diff $(TMP) tests/test_result_full | $(REPLACE)

test_full_replace: all
	@$(CHECK_FULL) > tests/test_result_full

test: test_gtk test_selinux test_std

test_replace: test_gtk_replace test_selinux_replace test_std_replace
