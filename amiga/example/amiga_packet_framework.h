/*
 * Amiga Packet Communication Framework - Header File
 * Minimal framework for receiving and processing serial packets
 */

#ifndef AMIGA_PACKET_FRAMEWORK_H
#define AMIGA_PACKET_FRAMEWORK_H

#include <exec/types.h>

/* Packet processing callback type */
typedef void (*PacketHandler)(const char *packet, ULONG length);

/* Function prototypes */

/**
 * Initialize the packet communication framework
 * Sets up serial communication at 9600 baud, 8N1, no flow control
 * Returns TRUE on success, FALSE on failure
 */
BOOL InitPacketFramework(void);

/**
 * Clean up framework resources
 * Closes devices and frees allocated memory
 */
void CleanupPacketFramework(void);

/**
 * Send a packet through the serial port
 * @param data - pointer to data buffer to send
 * @param length - number of bytes to send
 * Returns TRUE on success, FALSE on failure
 */
BOOL SendPacket(const char *data, ULONG length);

/**
 * Receive a packet from the serial port (non-blocking)
 * @param buffer - buffer to store received data
 * @param maxLength - maximum bytes to read
 * Returns number of bytes actually read (0 if no data available)
 */
ULONG ReceivePacket(char *buffer, ULONG maxLength);

/**
 * Main packet processing loop
 * Continuously checks for incoming packets and calls handler
 * @param handler - callback function to process packets (NULL for default)
 */
void ProcessPackets(PacketHandler handler);

/**
 * Default packet handler implementation
 * Prints received packets and auto-responds to "Hello Amiga" messages
 * @param packet - received packet data
 * @param length - length of packet
 */
void DefaultPacketHandler(const char *packet, ULONG length);

#endif /* AMIGA_PACKET_FRAMEWORK_H */