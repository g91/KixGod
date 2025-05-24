import time
import sys
import threading

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Error: PySerial not installed.")
    print("Please install it with: pip install pyserial")
    sys.exit(1)

def list_available_ports():
    """List all available serial ports"""
    ports = list_ports.comports()
    print("\nAvailable serial ports:")
    for port in ports:
        print(f"- {port.device}: {port.description}")
    print("")

def monitor_signal_level(ser):
    """Monitor if any signal is being detected at all (run in a separate thread)"""
    last_report = time.time()
    while not stop_threads:
        # Check if any data has been received
        if ser.in_waiting > 0:
            return  # Data is being received, no need to continue monitoring

        # Every 5 seconds if no data, try to detect any signal
        if time.time() - last_report > 5:
            try:
                # Try different control line functions
                cts = ser.cts
                dsr = ser.dsr
                cd = ser.cd
                ri = ser.ri
                print(f"Status check - CTS: {cts}, DSR: {dsr}, CD: {cd}, RI: {ri}")
            except:
                pass  # Some adapters don't support these
            
            last_report = time.time()
        time.sleep(0.5)

def enhanced_listener(port="COM6", baud=9600, auto_detect=True):
    """Enhanced serial port listener with diagnostic features"""
    global stop_threads
    stop_threads = False
    
    if auto_detect:
        list_available_ports()
    
    print(f"Attempting to open {port} at {baud} baud...")
    
    try:
        # Try to open the serial port with more explicit settings
        ser = serial.Serial(
            port=port, 
            baudrate=baud, 
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1,
            xonxoff=False,     # Disable software flow control
            rtscts=False,      # Disable hardware (RTS/CTS) flow control
            dsrdtr=False       # Disable hardware (DSR/DTR) flow control
        )
        
        print(f"Connected to {port}")
        print("Press Ctrl+C to exit")
        print("-" * 40)
        
        # Start the signal level monitor in a separate thread
        monitor_thread = threading.Thread(target=monitor_signal_level, args=(ser,))
        monitor_thread.daemon = True
        monitor_thread.start()
        
        # Try different baud rates if requested
        baud_rates_to_try = [9600, 115200, 57600, 38400, 19200, 4800] if auto_detect else [baud]
        current_baud_index = baud_rates_to_try.index(baud) if baud in baud_rates_to_try else 0
        
        # Simple continuous reading loop
        last_baud_change = time.time()
        bytes_received = 0
        
        while True:
            try:
                # Read any available data
                if ser.in_waiting:
                    raw_data = ser.read(ser.in_waiting)
                    bytes_received += len(raw_data)
                    
                    # Try to print as text, fall back to hex
                    try:
                        text = raw_data.decode('utf-8')
                        print(f"Received: {text.strip()}")
                    except:
                        # Print as hex if we can't decode as text
                        hex_data = ' '.join([f'{b:02X}' for b in raw_data])
                        print(f"Received (hex): {hex_data}")
                
                # If auto-detect is enabled and we haven't received any data for 15 seconds,
                # try switching to another baud rate
                if auto_detect and bytes_received == 0 and time.time() - last_baud_change > 15:
                    current_baud_index = (current_baud_index + 1) % len(baud_rates_to_try)
                    new_baud = baud_rates_to_try[current_baud_index]
                    print(f"\nTrying different baud rate: {new_baud}")
                    
                    ser.baudrate = new_baud
                    last_baud_change = time.time()
                
                # Small delay to prevent CPU hogging
                time.sleep(0.1)
                
            except KeyboardInterrupt:
                print("\nExiting...")
                break
    except Exception as e:
        print(f"Error: {e}")
    finally:
        stop_threads = True  # Signal threads to stop
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Closed connection to {port}")

if __name__ == "__main__":
    # Allow command-line arguments for port and baud
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    auto_detect = "--auto" in sys.argv
    
    enhanced_listener(port, baud, auto_detect)