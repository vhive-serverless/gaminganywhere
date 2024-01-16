#!/bin/bash

. env-setup.sh

sudo ln -s /usr/include/locale.h /usr/include/xlocale.h

cd deps.src

make

cd ../ga

make

make install