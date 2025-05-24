# file: serial_listener.py
import pyserial as serial  # Change this line to use pyserial explicitly
import time
import argparse
from datetime import datetime

def listen_to_serial(port="COM6", baud=9600, timeout=None):
    """
    Listen to the specified serial port and print any incoming data.
    
    Args:
        port (str): Serial port name (e.g. 'COM6' on Windows)
        baud (int): Baud rate (must match the sender's rate)
        timeout (float): Read timeout in seconds, None for blocking
    """
    try:
        # Open serial connection
        ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout
        )
        
        print(f"Listening on {port} at {baud} baud...")
        print("Press Ctrl+C to exit")
        print("-" * 50)
        
        # Continuous reading loop
        buffer = bytearray()
        
        while True:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
                # Process complete lines
                while b'\n' in buffer:
                    # Split on newline
                    line, buffer = buffer.split(b'\n', 1)
                    
                    # Try to decode as string, fallback to hex if it fails
                    try:
                        decoded = line.decode('utf-8').strip()
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        print(f"[{timestamp}] Text: {decoded}")
                    except UnicodeDecodeError:
                        # For binary data, show hex representation
                        hex_data = ' '.join([f'{b:02X}' for b in line])
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        print(f"[{timestamp}] Binary: {hex_data}")
                
                # If we have data but no newline after a while, print what we have
                if len(buffer) > 0 and not ser.in_waiting:
                    # Try to decode as string, fallback to hex if it fails
                    try:
                        decoded = buffer.decode('utf-8').strip()
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        print(f"[{timestamp}] Partial: {decoded}")
                    except UnicodeDecodeError:
                        # For binary data, show hex representation
                        hex_data = ' '.join([f'{b:02X}' for b in buffer])
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        print(f"[{timestamp}] Partial binary: {hex_data}")
                    buffer = bytearray()
            
            # Brief pause to prevent CPU hogging
            time.sleep(0.01)
            
    except Exception as e:  # Use a general exception to catch all errors
        print(f"Error with serial port {port}: {e}")
    except KeyboardInterrupt:
        print("\nExiting serial listener")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Closed connection to {port}")

def main():
    parser = argparse.ArgumentParser(description="Serial port listener")
    parser.add_argument("-p", "--port", default="COM6", help="Serial port (default: COM6)")
    parser.add_argument("-b", "--baud", type=int, default=9600, help="Baud rate (default: 9600)")
    parser.add_argument("-t", "--timeout", type=float, default=None, help="Read timeout in seconds (default: None)")
    
    args = parser.parse_args()
    listen_to_serial(args.port, args.baud, args.timeout)

if __name__ == "__main__":
    main()