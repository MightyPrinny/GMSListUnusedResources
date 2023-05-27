#!/bin/bash
cd build
g++ -std=c++17 -march=native -g -O1 -pg -maes -x c++ ../listUnreferenced.cpp
