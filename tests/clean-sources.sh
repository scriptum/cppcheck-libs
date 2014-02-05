#!/bin/sh

find gtk selinux std full -not -name "*.[ch]" -and -not -name "*.cpp" -and -not -name "*.hpp" -and -not -name "*.cc" -and -not -name "*.hh" $@
