#!/usr/bin/env bash

./MUSIChydro tests/test_1+1DRiemannTest_Cartesian/music_input_1+1DRiemannTest_z

python3 tests/test_1+1DRiemannTest_Cartesian/TestOutputFiles.py
STATUS=$?

if [ $STATUS == 0 ]; then
    echo "All tests passed! :)"
    rm -fr ./*.dat
else
    echo "Tests FAILED :("
    exit 1
fi