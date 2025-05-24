"""
Simplified Raspberry Pi Pico Serial Test
Basic communication test with Amiga
"""
import time
from machine import UART, Pin

# Set up the UART with simplest possible configuration
uart = UART(0, 
           baudrate=9600,  # Match Amiga setting exactly
           bits=8, 
           parity=None, 
           stop=1, 
           tx=Pin(0),
           rx=Pin(1),
           timeout=100,
           flow=0)  # Explicitly no flow control

# Set up LED for visual feedback
led = Pin(25, Pin.OUT)

# Clear buffers at startup
uart.read()

def main():
    counter = 1
    
    print("Simplified Pico Serial Test")
    print("-------------------------")
    
    # Blink LED to indicate program start
    for _ in range(3):
        led.value(1)
        time.sleep(0.2)
        led.value(0)
        time.sleep(0.2)
    
    while True:
        # Simple message
        message = f"Hello Amiga #{counter}!\r\n"
        print(f"Sending: {message.strip()}")
        
        # Visual indicator
        led.value(1)
        
        # Send message
        uart.write(message)
        
        # Brief pause with LED on
        time.sleep(0.1)
        led.value(0)
        
        # Check for any response
        time.sleep(0.5)  # Wait for potential response
        
        if uart.any():
            data = uart.read()
            if data:
                print(f"Received: {data}")
                led.value(1)
                time.sleep(0.05)
                led.value(0)
                counter += 1
        
        # Wait between messages
        time.sleep(3)  # Shorter interval for testing

if __name__ == "__main__":
    main()