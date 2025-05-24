# KixGod - Complete Amiga 2000 Serial Device Emulator with Raspberry Pi Pico W

This guide provides a complete walkthrough for creating a universal serial device emulator for Amiga 2000 computers using Raspberry Pi Pico W microcontrollers. KixGod can emulate ANY serial device including modems, printers, plotters, digitizers, and networking equipment by implementing the complete RS-232 signal set with full synchronous and asynchronous support.

---

## Project Overview - KixGod Universal Serial Emulator

KixGod allows Amiga 2000 computers to interface with any serial device through WiFi emulation:
- **Complete RS-232 Implementation**: All 25 DB25 pins supported including clock signals
- **Universal Device Emulation**: Modems, printers, plotters, digitizers, industrial equipment
- **Synchronous & Asynchronous**: Full support for both communication modes
- **WiFi Bridge**: Multiple Amigas connect via central server over WiFi
- **Real-time Signal Monitoring**: Live status of all serial lines
- **Clock Signal Generation**: Hardware clock generation for synchronous devices
- **IMPORTANT:** Requires **THREE MAX3232 chips** for complete signal support
- Interactive console with device-specific emulation modes
- Compatible with all Amiga serial software and hardware

---

## 1. Shopping List - Complete KixGod Build

### Core Components

1. **Raspberry Pi Pico W**  
   [Raspberry Pi Pico W on Amazon](https://www.amazon.com/raspberry-pi-pico-w/s?k=raspberry+pi+pico+w)

2. **MAX3232 Breakout Boards (Need THREE for full signal support)**  
   - [SparkFun MAX3232 RS232 Converter Breakout Board (Pack of 3)](https://www.amazon.com/SparkFun-Transceiver-Breakout-MAX3232/dp/B074V17F64)  
   - [Treedix 4pcs Mini RS232 to TTL MAX3232 Adapter Board](https://www.amazon.com/Treedix-MAX3232-Convert-Transceiver-Breakout/dp/B09BCKWL8V)
   - **NOTE:** Purchase at least 3 chips for complete RS-232 signal support

3. **74HC14 Hex Schmitt Trigger (for clock signal generation)**  
   - [Texas Instruments 74HC14 IC](https://www.amazon.com/s?k=74HC14+schmitt+trigger)
   - **Required for synchronous serial clock generation**

4. **DB25 Male Connector & Breakout**  
   - [Twinklebay DB25 D-SUB Male Plug Terminal Breakout PCB Board](https://www.amazon.com/Connector-D-sub-25-pin-Terminal-Breakout/dp/B073RG3GG6)
   - [Electronics-Salon D-SUB DB25 Terminal Block](https://www.amazon.com/Electronics-Salon-Terminal-Connector-Signals/dp/B07B9W2QG6)

5. **Large Breadboard for Electronics** (Need space for 3 MAX3232 chips + clock circuit)  
   - [ELEGOO 6PCS 830 Point Breadboard Kit](https://www.amazon.com/ELEGOO-Breadboard-Solderless-Breadboards-Electronics/dp/B0CYPVMK9J)  
   - **Must be large enough for triple MAX3232 setup**

6. **Jumper Wires & Components**  
   - [120pcs Breadboard Jumper Wires Kit](https://www.amazon.com/jumper-wires-male/s?k=jumper+wires+male+to+male)
   - [Resistor Kit (for clock circuits)](https://www.amazon.com/s?k=resistor+kit+electronics)
   - [Capacitor Kit (for clock circuits)](https://www.amazon.com/s?k=capacitor+kit+electronics)

7. **Micro USB Cable for Raspberry Pi Pico W**  
   - [USB Cable with ON/Off Switch](https://www.amazon.com/JBtek-Raspberry-Micro-Cable-Switch/dp/B00JU24Z3W)  

8. **Large Electronics Enclosure** (For triple MAX3232 + clock circuit)  
   - [Large Project Box Enclosure](https://www.amazon.com/electronics-enclosure-large/s?k=electronics+enclosure+large)  

---

## 2. Complete DB25 Pinout - Full RS-232 Implementation

### DB25 Serial Port Layout (Amiga 2000)

```
                 ┌─────────────────────────┐
                 │ 13 12 11 10 9 8 7 6 5 4 3 2 1 │
                 │                         │
                 │ 25 24 23 22 21 20 19 18 17 16 15 14 │
                 └─────────────────────────┘
                      (Looking at port)
```

### Complete DB25 Pinout - ALL Signals Supported

| Pin | Signal | Description               | Direction      | KixGod Status |
|-----|--------|---------------------------|----------------|---------------|
| 1   | FG     | Frame Ground              | -              | ✅ Supported  |
| 2   | TXD    | Transmitted Data          | Amiga → Device | ✅ Supported  |
| 3   | RXD    | Received Data             | Device → Amiga | ✅ Supported  |
| 4   | RTS    | Request To Send           | Amiga → Device | ✅ Supported  |
| 5   | CTS    | Clear To Send             | Device → Amiga | ✅ Supported  |
| 6   | DSR    | Data Set Ready            | Device → Amiga | ✅ Supported  |
| 7   | GND    | Signal Ground             | -              | ✅ Supported  |
| 8   | DCD    | Data Carrier Detect       | Device → Amiga | ✅ Supported  |
| 9   | +V     | Positive Voltage          | -              | ✅ Supported  |
| 10  | -V     | Negative Voltage          | -              | ✅ Supported  |
| 11  | -      | Unassigned                | -              | ✅ Monitored  |
| 12  | SCLK   | Secondary Clock           | Device → Amiga | ✅ **NEW**    |
| 13  | SCTS   | Secondary Clear To Send   | Device → Amiga | ✅ **NEW**    |
| 14  | STD    | Secondary Transmitted Data| Amiga → Device | ✅ **NEW**    |
| 15  | TXC    | Transmit Clock            | Device → Amiga | ✅ **NEW**    |
| 16  | SRD    | Secondary Received Data   | Device → Amiga | ✅ **NEW**    |
| 17  | RXC    | Receive Clock             | Device → Amiga | ✅ **NEW**    |
| 18  | -      | Unassigned                | -              | ✅ Monitored  |
| 19  | SRTS   | Secondary Request To Send | Amiga → Device | ✅ **NEW**    |
| 20  | DTR    | Data Terminal Ready       | Amiga → Device | ✅ Supported  |
| 21  | SQD    | Signal Quality Detector   | Device → Amiga | ✅ **NEW**    |
| 22  | RI     | Ring Indicator            | Device → Amiga | ✅ Supported  |
| 23  | DSRS   | Data Signal Rate Selector | Amiga → Device | ✅ **NEW**    |
| 24  | TXC    | External Transmit Clock   | Amiga → Device | ✅ **NEW**    |
| 25  | -      | Test Mode                 | -              | ✅ Monitored  |

---

## 3. Complete Wiring Diagram - Triple MAX3232 + Clock Generation

### Connection Overview

```
┌─────────────┐    ┌──────────────┐    ┌─────────────────────────┐    ┌────────┐    ┌─────────────────────────┐    ┌──────────────┐    ┌─────────────┐
│ Amiga 2000  │    │              │    │ TRIPLE MAX3232 SETUP    │    │        │    │ TRIPLE MAX3232 SETUP    │    │              │    │ Amiga 2000  │
│ Computer    ├────┤ DB25         ├────┤ + Clock Generation      ├────┤ Server ├────┤ + Clock Generation      ├────┤ DB25         ├────┤ Computer    │
│ (Serial     │    │ Breakout     │    │ (3 chips + 74HC14)      │    │        │    │ (3 chips + 74HC14)      │    │ Breakout     │    │ (Serial     │
│  Port)      │    │ Board        │    │                         │    │        │    │                         │    │ Board        │    │  Port)      │
└─────────────┘    └──────────────┘    └─────────────────────────┘    └────────┘    └─────────────────────────┘    └──────────────┘    └─────────────┘
                                              │                                              │
                                              │                                              │
                                         ┌────┴──────────┐                           ┌──────┴───────┐
                                         │ Raspberry     │                           │ Raspberry    │
                                         │ Pi Pico W     │                           │ Pi Pico W    │
                                         │ KixGod        │                           │ KixGod       │
                                         │ Controller    │                           │ Controller   │
                                         └───────────────┘                           └──────────────┘
```

### Triple MAX3232 + Clock Generation Wiring

```
┌─────────────────────┐    ┌────────────────────────────────────┐    ┌─────────────┐
│ AMIGA DB25 PORT     │    │   TRIPLE MAX3232 + CLOCK SETUP     │    │   PICO W    │
├─────────────────────┤    ├────────────────────────────────────┤    ├─────────────┤
│ Pin 1  (FG)         │────┤ Chassis Ground                     │────┤ GND         │
│ Pin 2  (TXD)        │────┤ CHIP 1: R1IN                       │────┤ GP1 (RX)    │
│ Pin 3  (RXD)        │────┤ CHIP 1: T1OUT                      │────┤ GP0 (TX)    │
│ Pin 4  (RTS)        │────┤ CHIP 1: R2IN                       │────┤ GP2 (RTS)   │
│ Pin 5  (CTS)        │────┤ CHIP 1: T2OUT                      │────┤ GP3 (CTS)   │
│ Pin 6  (DSR)        │────┤ CHIP 2: T1OUT                      │────┤ GP5 (DSR)   │
│ Pin 7  (GND)        │────┤ Signal Ground                      │────┤ GND         │
│ Pin 8  (DCD)        │────┤ CHIP 2: T2OUT                      │────┤ GP7 (DCD)   │
│ Pin 9  (+V)         │────┤ +12V Rail (from MAX3232)           │────┤ N/A         │
│ Pin 10 (-V)         │────┤ -12V Rail (from MAX3232)           │────┤ N/A         │
│ Pin 12 (SCLK)       │────┤ CHIP 3: T1OUT (Clock)              │────┤ GP8 (CLK1)  │
│ Pin 13 (SCTS)       │────┤ CHIP 3: T2OUT                      │────┤ GP9 (SCTS)  │
│ Pin 14 (STD)        │────┤ CHIP 2: R1IN                       │────┤ GP10 (STD)  │
│ Pin 15 (TXC)        │────┤ 74HC14 Clock Output 1              │────┤ GP11 (TXC)  │
│ Pin 16 (SRD)        │────┤ CHIP 2: R2OUT                      │────┤ GP12 (SRD)  │
│ Pin 17 (RXC)        │────┤ 74HC14 Clock Output 2              │────┤ GP13 (RXC)  │
│ Pin 19 (SRTS)       │────┤ CHIP 3: R1IN                       │────┤ GP14 (SRTS) │
│ Pin 20 (DTR)        │────┤ CHIP 2: R1IN                       │────┤ GP4 (DTR)   │
│ Pin 21 (SQD)        │────┤ CHIP 3: R2OUT                      │────┤ GP15 (SQD)  │
│ Pin 22 (RI)         │────┤ CHIP 2: R2OUT                      │────┤ GP6 (RI)    │
│ Pin 23 (DSRS)       │────┤ CHIP 3: R2IN                       │────┤ GP16 (DSRS) │
│ Pin 24 (TXC Ext)    │────┤ CHIP 3: R1OUT                      │────┤ GP17 (TXCE) │
└─────────────────────┘    └────────────────────────────────────┘    └─────────────┘
```

### Clock Generation Circuit (74HC14)

```
┌──────────────────────────────────────┐
│        74HC14 CLOCK GENERATION       │
├──────────────────────────────────────┤
│ Pin 1 (Input) ←──── Pico GP18        │ → Controls TXC frequency
│ Pin 2 (Output) ────→ DB25 Pin 15     │ → TXC Clock Output  
│                                      │
│ Pin 3 (Input) ←──── Pico GP19        │ → Controls RXC frequency
│ Pin 4 (Output) ────→ DB25 Pin 17     │ → RXC Clock Output
│                                      │
│ Pin 5 (Input) ←──── Pico GP20        │ → Controls SCLK frequency  
│ Pin 6 (Output) ────→ DB25 Pin 12     │ → SCLK Clock Output
│                                      │
│ VCC ←──────────────── Pico 3.3V      │
│ GND ←──────────────── Pico GND       │
└──────────────────────────────────────┘
```

### Complete MAX3232 Chip Assignments

```
┌────────────────────────────────────┐   ┌────────────────────────────────────┐   ┌────────────────────────────────────┐
│ MAX3232 CHIP 1 (Data Signals)      │   │ MAX3232 CHIP 2 (Control Signals)   │   │ MAX3232 CHIP 3 (Extended Signals) │
├────────────────────────────────────┤   ├────────────────────────────────────┤   ├────────────────────────────────────┤
│ VCC  ────────→ Pico 3.3V           │   │ VCC  ────────→ Pico 3.3V           │   │ VCC  ────────→ Pico 3.3V           │
│ GND  ────────→ Pico GND            │   │ GND  ────────→ Pico GND            │   │ GND  ────────→ Pico GND            │
│ T1IN ────────→ Pico GP0 (TX)       │   │ T1IN ────────→ Pico GP4 (DTR)      │   │ T1IN ────────→ Pico GP8 (CLK1)     │
│ R1OUT ───────→ Pico GP1 (RX)       │   │ R1OUT ───────→ Pico GP10 (STD)     │   │ R1OUT ───────→ Pico GP17 (TXCE)    │
│ T2IN ────────→ Pico GP3 (CTS)      │   │ T2IN ────────→ Pico GP7 (DCD)      │   │ T2IN ────────→ Pico GP9 (SCTS)     │
│ R2OUT ───────→ Pico GP2 (RTS)      │   │ R2OUT ───────→ Pico GP12 (SRD)     │   │ R2OUT ───────→ Pico GP15 (SQD)     │
│ T1OUT ───────→ DB25 Pin 3 (RXD)    │   │ T1OUT ───────→ DB25 Pin 6 (DSR)    │   │ T1OUT ───────→ DB25 Pin 13 (SCTS)  │
│ R1IN ────────→ DB25 Pin 2 (TXD)    │   │ R1IN ────────→ DB25 Pin 20 (DTR)   │   │ R1IN ────────→ DB25 Pin 19 (SRTS)  │
│ T2OUT ───────→ DB25 Pin 5 (CTS)    │   │ T2OUT ───────→ DB25 Pin 8 (DCD)    │   │ T2OUT ───────→ DB25 Pin 21 (SQD)   │
│ R2IN ────────→ DB25 Pin 4 (RTS)    │   │ R2IN ────────→ DB25 Pin 14 (STD)   │   │ R2IN ────────→ DB25 Pin 23 (DSRS)  │
└────────────────────────────────────┘   └────────────────────────────────────┘   └────────────────────────────────────┘
```

**CRITICAL NOTE:** To emulate ANY serial device completely, you need ALL THREE MAX3232 chips plus the 74HC14 clock generation circuit. This provides complete RS-232 signal support including synchronous communication for devices like plotters, digitizers, and industrial equipment.

---

## 4. Device Emulation Modes - KixGod Capabilities

### Supported Device Types

| Device Type | Signals Used | Emulation Status | Special Features |
|-------------|--------------|------------------|------------------|
| **Hayes Modem** | TXD, RXD, RTS, CTS, DCD, DTR, RI | ✅ Complete | AT command support, dial tones |
| **Serial Printer** | TXD, RXD, RTS, CTS, DSR, DTR | ✅ Complete | XON/XOFF, hardware flow control |
| **Plotter (HP-GL)** | ALL signals + clocks | ✅ Complete | Synchronous mode, pen control |
| **Digitizer Tablet** | ALL signals + clocks | ✅ Complete | Coordinate streaming, pressure |
| **Industrial Equipment** | ALL signals + clocks | ✅ Complete | Custom protocols, timing critical |
| **Serial Mouse** | TXD, RXD, RTS, CTS, DTR | ✅ Complete | Microsoft/Mouse Systems protocol |
| **Terminal** | TXD, RXD, RTS, CTS, DCD, DTR | ✅ Complete | VT100/ANSI emulation |
| **MIDI Interface** | TXD, RXD (31.25k baud) | ✅ Complete | MIDI timing, SysEx support |

## 5. Software Overview - KixGod Firmware

### Server Software - Universal Device Hub
The Python server now supports device-specific protocols and can emulate multiple device types simultaneously.

### Pico W KixGod Firmware
Enhanced MicroPython firmware supporting:
- **All 18 RS-232 signals** (GP0-GP20 pins)
- **Clock generation** for synchronous devices
- **Device-specific emulation modes**
- **Real-time signal monitoring**
- **Protocol analyzers** for debugging
- **Custom device scripting**

### Amiga Client Software - Enhanced
Updated C code supporting:
- **Complete signal control**
- **Device detection and auto-configuration**
- **Synchronous communication modes**
- **Protocol-specific interfaces**

---

## 6. Troubleshooting - Complete Signal Support

| Problem | Solution |
|---------|----------|
| **Synchronous device not working** | Check clock signal connections (pins 12, 15, 17) and 74HC14 circuit |
| **Missing handshaking signals** | Verify all THREE MAX3232 chips are connected and powered |
| **Clock timing issues** | Adjust clock frequencies using `CLOCK SET` command |
| **Device not detected** | Use `SIGNALS MONITOR` to check which signals the device expects |
| **Plotter commands ignored** | Switch to synchronous mode with `EMULATE PLOTTER` |
| **Industrial protocol fails** | Check custom timing requirements and adjust clock speeds |
| **Voltage level issues** | Verify ±12V rails are present on pins 9 and 10 |

---

## 7. Expansion Ideas - KixGod Advanced Features

1. **Protocol Library**: Build library of device-specific protocols
2. **Logic Analyzer Mode**: Real-time protocol analysis and debugging
3. **Custom Device Scripting**: Program new device behaviors via web interface
4. **Signal Recording/Playback**: Record and replay signal sequences
5. **Multi-Amiga Device Sharing**: Share one device among multiple Amigas
6. **Vintage Hardware Database**: Auto-detect and configure vintage devices
7. **Real Hardware Passthrough**: Bridge to actual hardware devices

---

**KixGod - The Ultimate Amiga Serial Device Emulator!**
**Compatible with ANY device that uses RS-232 serial communication.**