import machine
import utime

# Configure UART - same pins as your original script
# For loopback test: TX (GP0) should be physically connected to RX (GP1)
uart = machine.UART(0,
                  baudrate=9600,
                  bits=8,
                  parity=None,
                  stop=1,
                  tx=machine.Pin(0),
                  rx=machine.Pin(1),
                  timeout=100)

# Status LED
led = machine.Pin("LED", machine.Pin.OUT)

# Test counters
tests_run = 0
tests_passed = 0

def blink_led(times=1, delay=0.2):
    """Blink LED to indicate status"""
    for _ in range(times):
        led.on()
        utime.sleep(delay)
        led.off()
        utime.sleep(delay)

def run_loopback_test(test_message):
    """Send a message and check if we receive it back through loopback"""
    global tests_run, tests_passed
    
    # Clear any pending data
    while uart.any():
        uart.read()
    
    print(f"\n--- TEST #{tests_run + 1} ---")
    print(f"Sending: {test_message!r}")
    
    # Send the test message
    uart.write(test_message)
    
    # Wait for data to loop back
    utime.sleep(0.1)
    
    # Check if we received data
    if uart.any():
        received = uart.read()
        print(f"Received: {received!r}")
        
        if received == test_message:
            print("✓ TEST PASSED: Loopback successful!")
            tests_passed += 1
            blink_led(1, 0.1)  # Quick blink for success
        else:
            print("✗ TEST FAILED: Data mismatch")
            print(f"  Expected: {[b for b in test_message]}")
            print(f"  Received: {[b for b in received]}")
            blink_led(3, 0.1)  # Three quick blinks for failure
    else:
        print("✗ TEST FAILED: No data received")
        blink_led(3, 0.1)  # Three quick blinks for failure
    
    tests_run += 1

# Main test sequence
print("=== UART LOOPBACK TEST ===")
print("Wiring: TX (GP0) should be connected to RX (GP1)")
print(f"UART Configuration: {uart}")

# Wait for setup to complete
utime.sleep(1)

# Test with various messages and encodings
test_messages = [
    b"HELLO",                    # Simple text
    b"TEST-1234",                # Text with numbers
    b"\x00\x01\x02\x03\x04",     # Binary data
    b"CR\r",                     # With CR
    b"LF\n",                     # With LF
    b"CRLF\r\n",                 # With CRLF
]

for message in test_messages:
    run_loopback_test(message)
    utime.sleep(0.5)

# Final results
print("\n=== TEST RESULTS ===")
print(f"Tests run: {tests_run}")
print(f"Tests passed: {tests_passed}")

if tests_passed == tests_run:
    print("✓ ALL TESTS PASSED - Your loopback is working correctly!")
    # Success pattern: long-short-long
    led.on(); utime.sleep(0.5); led.off(); utime.sleep(0.2)
    led.on(); utime.sleep(0.1); led.off(); utime.sleep(0.2)
    led.on(); utime.sleep(0.5); led.off()
else:
    print(f"✗ {tests_run - tests_passed} TESTS FAILED - Check your connections")
    # Failure pattern: several short blinks
    for _ in range(5):
        led.on(); utime.sleep(0.1); led.off(); utime.sleep(0.1)