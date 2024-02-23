#!/bin/bash

# . env-setup

ln -s /usr/include/locale.h /usr/include/xlocale.h

cd deps.src

make all

cd ../ga

make

make install