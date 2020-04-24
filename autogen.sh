#!/bin/sh

set -x
glib-gettextize --copy --force
libtoolize --automake
intltoolize --copy --force --automake

aclocal
autoconf
automake --add-missing --foreign

