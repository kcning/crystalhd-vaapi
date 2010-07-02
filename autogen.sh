#!/bin/bash -x
autoheader
aclocal
libtoolize
autoconf
automake --add-missing

./configure ${@}
