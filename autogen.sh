#!/bin/bash -e
# $File: autogen.sh
# $Date: Sat Dec 22 10:51:20 2012 +0800
# $Author: jiakai <jia.kai66@gmail.com>

aclocal -I m4
autoconf
intltoolize --force
autoheader
automake --add-missing
