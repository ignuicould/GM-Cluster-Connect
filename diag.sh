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

# Helper for pulsing clock
pulse() {
    gpio write $SK 1
    gpio write $SK 0
}

# Clear chip state
gpio write $CS 0
gpio write $SK 0

# Loop through all 256 addresses (0 to 255)
for addr in {0..255}; do
    gpio write $CS 1
    
    # 1. Start Bit
    gpio write $DI 1; pulse
    
    # 2. Opcode '10' (Read)
    gpio write $DI 1; pulse
    gpio write $DI 0; pulse
    
    # 3. Address (8 bits)
    for i in {7..0}; do
        bit=$(( (addr >> i) & 1 ))
        gpio write $DI $bit
        pulse
    done
    
    # 4. Read Data (8 bits)
    byte_val=0
    for i in {7..0}; do
        gpio write $SK 1
        # Capture DO state
        bit=$(gpio read $DO)
        # Shift into byte_val
        byte_val=$(( (byte_val << 1) | bit ))
        gpio write $SK 0
    done
    
    # Output address and value in hex
    printf "Addr 0x%02X: 0x%02X\n" $addr $byte_val
    
    gpio write $CS 0
done