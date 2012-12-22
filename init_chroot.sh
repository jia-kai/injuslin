#!/bin/sh -e
#This file is part of injuslin, informatcis judge system for linux.
#
#Copyright (C) <2008, 2009>  Jiakai <jia.kai66@gmail.com>
#
#Injuslin is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#Injuslin is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with injuslin.  If not, see <http://www.gnu.org/licenses/>.

mkdir judge
cd judge

mkdir tmp
chmod 777 tmp

echo "Compiling a C program ..."

cat >test.c  << EOF
#include <stdio.h>
#include <math.h>

int main()
{
	double x = sin(0.5);
	printf("Hello, world!\n");
}
EOF

gcc test.c -o test -lm
rm test.c

ldd -v test > libs

echo "Compiling a C++ program ..."

cat >test.cpp  << EOF
#include <iostream>
using namespace std;

int main()
{
	cout << "Hello, world!\n";
}
EOF

g++ test.cpp -o test -lm
rm test.cpp

ldd -v test >> libs
rm test

files=`grep -o '\/.*\.[-1-9]*' libs | sort | uniq`
rm libs

for i in $files
do
	install -Dv $i .$i
done

echo "Please enter the password for root to change the owner to root"
sudo chown root:root -R .

cd ..
sudo mv judge /judge

