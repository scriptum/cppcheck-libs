TARGETS = geany.cfg gtk.cfg selinux.cfg
COMPILER = python compile.py

TMP = .tmp_test_result
REPLACE = sed -e 's/^</ NEW:/' -e 's/^>/LOST:/'
CHECK_TAIL = 2>&1 | sort > $(TMP)
CPPCHECK = ../cppcheck/cppcheck -q -j4 --max-configs=1 --std=posix  --enable=warning,performance,style,portability
CHECK_ALL = $(CPPCHECK) --library=selinux --include=selinux.h tests $(CHECK_TAIL)
CHECK_GTK = $(CPPCHECK) --library=gtk --include=gtk.h tests/gtk $(CHECK_TAIL)
CHECK_STD = $(CPPCHECK) --include=gnu.h tests/std $(CHECK_TAIL)
CHECK_SELINUX = $(CPPCHECK) --library=gtk tests/selinux $(CHECK_TAIL)

all: $(TARGETS)

geany.cfg: geany.rules gtk.rules gtk-functions.rules gtk.cfg
	cat geany.rules gtk.rules gtk-functions.rules | $(COMPILER) > $@

gtk.cfg: gtk.rules gtk-functions.rules gtk.h compile.py
	cat gtk.rules gtk-functions.rules | $(COMPILER) gtk.h > $@

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

test: test_gtk test_selinux test_std

test_replace: test_gtk_replace test_selinux_replace test_std_replace
