#!/bin/bash
cd build
clang++-13 -std=c++17 -march=native -O3 -maes -x c++ ../listUnreferenced.cpp -ogmslur
