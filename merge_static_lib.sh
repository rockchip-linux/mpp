#!/usr/bin/env bash

cd $1
rm -rf mpp/lib'$2$'.a

SCRIPT=$'CREATE mpp/lib'$2$'.a\n'
SCRIPT=$SCRIPT$(find . -name '*.a' -exec echo 'ADDLIB {}' \;)
SCRIPT=$SCRIPT$'\nSAVE\nEND\n'

ar -M <<< $SCRIPT
