import machine
import utime

# Define pins separately so we can control them directly
tx_pin = machine.Pin(0, machine.Pin.OUT)
rx_pin = machine.Pin(1, machine.Pin.IN)
rts_pin = machine.Pin(2, machine.Pin.OUT)
cts_pin = machine.Pin(3, machine.Pin.IN)

# Status LED on Pico W
led = machine.Pin("LED", machine.Pin.OUT)

def blink_led(times=3, delay=0.2):
    """Blink LED to indicate status"""
    for _ in range(times):
        led.on()
        utime.sleep(delay)
        led.off()
        utime.sleep(delay)

def create_uart(baud_rate=9600):
    """Create and return a UART instance with the specified baud rate"""
    return machine.UART(0,
                       baudrate=baud_rate,
                       tx=tx_pin,
                       rx=rx_pin,
                       bits=8,
                       parity=None,
                       stop=1,
                       timeout=1000)

def max3232_loopback_test(baud_rate=9600):
    """Test loopback through MAX3232 by sending data and checking if it returns"""
    print(f"Starting MAX3232 loopback test at {baud_rate} baud")
    
    # Configure UART
    uart = create_uart(baud_rate)
    
    # Clear any pending data
    while uart.any():
        uart.read()
    
    # Signal test starting
    blink_led(3, 0.2)
    
    # Test patterns to send
    test_patterns = [
        b"MAX3232 TEST\r\n",
        b"LOOPBACK TESTING\r\n",
        b"12345678\r\n",
        b"\x55\xAA\x55\xAA\r\n"  # Alternating bit pattern
    ]
    
    success_count = 0
    total_tests = 0
    
    # Run tests for each pattern
    for pattern in test_patterns:
        print(f"Sending: {pattern}")
        
        # Send the pattern
        uart.write(pattern)
        
        # Wait a moment for data to loop back
        utime.sleep(0.1)
        
        # Check if we received data back
        if uart.any():
            received = uart.read()
            print(f"Received: {received}")
            
            # Check if received data matches what we sent
            if received == pattern:
                print("✓ MATCH - Loopback successful")
                success_count += 1
                led.on()
                utime.sleep(0.2)
                led.off()
            else:
                print("✗ MISMATCH - Data corrupted in loopback")
        else:
            print("✗ NO DATA RECEIVED - Loopback failed")
        
        total_tests += 1
        utime.sleep(0.5)
    
    # Print results
    print(f"\nLoopback test results: {success_count}/{total_tests} successful")
    if success_count == total_tests:
        print("MAX3232 loopback is working correctly!")
        blink_led(5, 0.1)  # Happy blinks
    else:
        print("Issues detected with MAX3232 loopback.")
        # Sad slow blinks
        for _ in range(3):
            led.on()
            utime.sleep(0.8)
            led.off() 
            utime.sleep(0.8)

def continuous_loopback_test():
    """Run a continuous loopback test to help with troubleshooting"""
    print("Starting continuous loopback test...")
    print("Press Ctrl+C to stop")
    
    # Configure UART
    uart = create_uart(9600)
    
    counter = 0
    while True:
        try:
            # Send a test message with counter
            test_msg = f"LOOPBACK-{counter}\r\n".encode()
            uart.write(test_msg)
            
            # Wait briefly for loopback
            utime.sleep(0.1)
            
            # Check for response
            if uart.any():
                received = uart.read()
                try:
                    decoded = received.decode('utf-8').strip()
                    print(f"SENT: LOOPBACK-{counter} | RECEIVED: {decoded}")
                    
                    # Visual indication of success/failure
                    if f"LOOPBACK-{counter}" in decoded:
                        led.on()
                        utime.sleep(0.05)
                        led.off()
                    else:
                        # Blink twice quickly for mismatch
                        led.on()
                        utime.sleep(0.05)
                        led.off()
                        utime.sleep(0.05)
                        led.on()
                        utime.sleep(0.05)
                        led.off()
                except:
                    # For binary data
                    hex_data = ' '.join([f'{b:02X}' for b in received])
                    print(f"SENT: LOOPBACK-{counter} | RECEIVED (hex): {hex_data}")
            else:
                print(f"SENT: LOOPBACK-{counter} | NO RESPONSE")
                # Slow blink for no response
                led.on()
                utime.sleep(0.5)
                led.off()
            
            counter += 1
            utime.sleep(0.5)
        except KeyboardInterrupt:
            print("\nLoopback test stopped by user")
            break

def test_signal_levels():
    """Test the MAX3232 by directly toggling pins"""
    print("Testing MAX3232 signal levels...")
    
    # Bypass UART and toggle TX pin directly
    for i in range(10):
        print(f"Toggling TX pin: {i}")
        tx_pin.value(1)  # Set high
        led.on()
        utime.sleep(0.5)
        tx_pin.value(0)  # Set low
        led.off()
        utime.sleep(0.5)
        
        # Check if we're receiving data on RX (loopback)
        rx_state = rx_pin.value()
        print(f"RX state: {rx_state}")
    
    print("Signal level test completed")

if __name__ == "__main__":
    print("MAX3232 Loopback Test")
    print("=====================")
    
    # Wait for a moment to let things settle
    utime.sleep(1)
    
    # Choose which test to run
    test_mode = 2  # Change this to select different tests
    
    if test_mode == 1:
        # Run a single loopback test
        max3232_loopback_test(9600)
    elif test_mode == 2:
        # Run a continuous loopback test
        continuous_loopback_test()
    elif test_mode == 3:
        # Test signal levels directly
        test_signal_levels()
    else:
        print("Invalid test mode selected")