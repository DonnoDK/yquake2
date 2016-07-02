#!/bin/bash

Q2ATIME=""
Q2DATIME=""
BASEATIME=""

while true; do
    newtime=$(stat -f '%m' "./release/quake2")
    if [ "$newtime" != "$Q2ATIME" ]; then
        echo "q2 changed!"
        cp release/quake2 ~/Desktop/quake2
        Q2ATIME=$newtime
    fi
    newtime=$(stat -f '%m' "./release/baseq2/game.dylib")
    if [ "$newtime" != "$BASETIME" ]; then
        echo "base changed!"
        cp release/baseq2/game.dylib ~/Desktop/quake2/baseq2
        BASETIME=$newtime
    fi
    sleep 1
done
