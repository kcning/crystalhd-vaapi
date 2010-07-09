#!/bin/bash -x
autoheader
aclocal
libtoolize
autoconf
automake --add-missing

autoheader
aclocal
libtoolize
autoconf
automake --add-missing

./configure ${@}
