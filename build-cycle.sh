#!/bin/bash

while true; do
    make && cp release/quake2 ~/Desktop/quake2 \
         && cp release/baseq2/game.dylib ~/Desktop/quake2/baseq2 \
         && cd ~/Desktop/quake2 && ./quake2 +map base1 \
         && cd -
done
