#!/bin/bash
cd build
clang++ -DWIN32 -std=c++17 -march=native -O3 -maes -x c++ ../listUnreferenced.cpp -ogmslur

