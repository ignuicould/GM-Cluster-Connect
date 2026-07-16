#!/bin/bash

# Pins
CS=3
SK=14
DI=12
DO=13

# Setup
gpio mode $CS out
gpio mode $SK out
gpio mode $DI out
gpio mode $DO in
gpio mode $DO up

# Reset
gpio write $CS 0
gpio write $SK 0
gpio write $DI 0

# Helper
pulse() {
    gpio write $SK 1
    gpio write $SK 0
}

# Read Command (1, 1, 0, addr[8..0])
gpio write $CS 1
gpio write $DI 1; pulse
gpio write $DI 1; pulse
gpio write $DI 0; pulse
for i in {8..0}; do
    gpio write $DI 0; pulse
done

# Read Data
echo -n "Result: "
for i in {7..0}; do
    gpio write $SK 1
    gpio read $DO
    gpio write $SK 0
done
echo ""
gpio write $CS 0