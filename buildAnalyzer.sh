#!/bin/sh

g++ -O2 -Iinclude -o analyzer src/analyzer.cc src/SpectrumProcessor.cc -l fftw3

g++ -O2 -Iinclude -o fileThrottler src/fileThrottler.cc

