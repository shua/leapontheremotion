clang++ -Wall -g -lLeap  track.cpp -o track
clang -I/usr/include/alsa -lasound  -lm -pthread sine.c -o sine
