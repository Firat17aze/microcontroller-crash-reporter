#!/bin/bash
# Black Box Crash Detector Test Script
# Runs automated tests on the Arduino Uno

PORT="/dev/cu.usbmodem101"
BAUD=9600

echo "=================================================="
echo "Black Box Crash Detector - Automated Test Suite"
echo "=================================================="
echo ""

# Configure serial port
stty -f $PORT $BAUD cs8 -cstopb -parenb

echo "TEST 1: Read Current Stack Pointer"
echo "--------------------------------------------------"
printf "4" > $PORT
sleep 2
cat < $PORT &
CAT_PID=$!
sleep 2
kill $CAT_PID 2>/dev/null
echo ""

echo "TEST 2: Deep Recursion Test"
echo "--------------------------------------------------"
printf "5" > $PORT
sleep 4
cat < $PORT &
CAT_PID=$!
sleep 3
kill $CAT_PID 2>/dev/null
echo ""

echo "TEST 3: Explicit Crash Dump (System will reset)"
echo "--------------------------------------------------"
printf "2" > $PORT
sleep 5
cat < $PORT &
CAT_PID=$!
sleep 3
kill $CAT_PID 2>/dev/null
echo ""

echo "TEST 4: Watchdog Timeout Test (Infinite Loop)"
echo "--------------------------------------------------"
echo "System will hang for ~2 seconds, then watchdog will trigger..."
printf "1" > $PORT
sleep 6
cat < $PORT &
CAT_PID=$!
sleep 3
kill $CAT_PID 2>/dev/null
echo ""

echo "=================================================="
echo "All tests completed!"
echo "=================================================="
