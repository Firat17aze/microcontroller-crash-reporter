#!/usr/bin/env python3
"""
Serial test script for Black Box Crash Detector
"""
import serial
import time
import sys

# Configuration
PORT = '/dev/cu.usbmodem101'
BAUD = 9600

def send_command(ser, command):
    """Send a command and read response"""
    print(f"\n{'='*60}")
    print(f"Sending command: {command}")
    print('='*60)
    
    # Send the command
    ser.write(command.encode())
    ser.flush()
    
    # Wait and read response
    time.sleep(0.5)
    
    # Read all available data
    response = ""
    while ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        response += data.decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    print(response)
    return response

def main():
    print("Connecting to Black Box Crash Detector...")
    
    try:
        # Open serial connection
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)  # Wait for Arduino to boot
        
        # Clear initial buffer
        if ser.in_waiting > 0:
            initial = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(initial)
        
        # Test 1: Read Current Stack Pointer
        print("\n" + "="*60)
        print("TEST 1: Reading Current Stack Pointer")
        print("="*60)
        send_command(ser, '4')
        time.sleep(1)
        
        # Test 2: Deep Recursion Test
        print("\n" + "="*60)
        print("TEST 2: Deep Recursion (Stack Stress)")
        print("="*60)
        send_command(ser, '5')
        time.sleep(3)
        
        # Test 3: Explicit Crash Dump
        print("\n" + "="*60)
        print("TEST 3: Explicit Crash Dump (System will reset)")
        print("="*60)
        send_command(ser, '2')
        time.sleep(3)
        
        # Wait for reboot and check for crash report
        print("\n" + "="*60)
        print("Waiting for system reboot...")
        print("="*60)
        time.sleep(3)
        
        # Read boot messages and crash report
        response = ""
        for _ in range(10):
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                response += data
                print(data, end='')
            time.sleep(0.5)
        
        # Test 4: Watchdog Timeout Test
        print("\n" + "="*60)
        print("TEST 4: Watchdog Timeout (Infinite Loop - will hang for ~2s)")
        print("="*60)
        send_command(ser, '1')
        
        print("\nWaiting for watchdog to trigger (~2 seconds)...")
        time.sleep(4)
        
        # Read watchdog crash report after reboot
        print("\n" + "="*60)
        print("Reading Watchdog Crash Report...")
        print("="*60)
        time.sleep(2)
        
        response = ""
        for _ in range(10):
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                response += data
                print(data, end='')
            time.sleep(0.5)
        
        print("\n" + "="*60)
        print("All tests completed!")
        print("="*60)
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        ser.close()
        sys.exit(0)

if __name__ == "__main__":
    main()
