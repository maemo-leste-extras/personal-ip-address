#!/bin/sh

set -x
glib-gettextize --copy --force
libtoolize --automake
intltoolize --copy --force --automake

aclocal-1.9
autoconf
automake-1.9 --add-missing --foreign

