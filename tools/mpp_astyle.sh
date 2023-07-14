#!/bin/bash
set -e
./astyle --quiet --options=astylerc --recursive "../*.cpp" "../*.c" "../*.h"

