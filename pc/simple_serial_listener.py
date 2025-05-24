import time
import sys

try:
    import serial
except ImportError:
    print("Error: PySerial not installed.")
    print("Please install it with: pip install pyserial")
    sys.exit(1)

def simple_listener(port="COM6", baud=9600):
    """Simple serial port listener with minimal dependencies"""
    print(f"Attempting to open {port} at {baud} baud...")
    
    try:
        # Try to open the serial port
        ser = serial.Serial(port, baud, timeout=1)
        print(f"Connected to {port}")
        print("Press Ctrl+C to exit")
        print("-" * 40)
        
        # Simple continuous reading loop
        while True:
            try:
                # Read any available data
                if ser.in_waiting:
                    raw_data = ser.read(ser.in_waiting)
                    
                    # Try to print as text, fall back to hex
                    try:
                        text = raw_data.decode('utf-8')
                        print(f"Received: {text}")
                    except:
                        # Print as hex if we can't decode as text
                        hex_data = ' '.join([f'{b:02X}' for b in raw_data])
                        print(f"Received (hex): {hex_data}")
                
                # Small delay to prevent CPU hogging
                time.sleep(0.1)
                
            except KeyboardInterrupt:
                print("\nExiting...")
                break
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Closed connection to {port}")

if __name__ == "__main__":
    # Allow command-line arguments for port and baud
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    
    simple_listener(port, baud)