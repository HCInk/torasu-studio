#!/bin/sh
echo "### SETUP START ###"
set -e

#
# General Stuff
#

echo "Setup: Generals"

mkdir -p thirdparty && cd thirdparty

#
# Submodules
#

echo "Setup: Submodules"

git submodule update --init --recursive

#
# Cleanup
#

echo "Setup: Cleanup"

cd ..

echo "### SETUP DONE ###"