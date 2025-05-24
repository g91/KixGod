/*
 * Amiga Packet Communication Framework
 * Minimal framework for receiving and processing serial packets
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/serial.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <stdio.h>
#include <string.h>

/* Global variables for serial communication */
struct MsgPort *SerialMP = NULL;
struct IOExtSer *SerialIO = NULL;
BOOL SerialOpen = FALSE;
struct MsgPort *TimerMP = NULL;
struct timerequest *TimerIO = NULL;
BOOL TimerOpen = FALSE;

/* Packet processing callback type */
typedef void (*PacketHandler)(const char *packet, ULONG length);

/* Function prototypes */
BOOL InitPacketFramework(void);
void CleanupPacketFramework(void);
BOOL SendPacket(const char *data, ULONG length);
ULONG ReceivePacket(char *buffer, ULONG maxLength);
void ProcessPackets(PacketHandler handler);
void DefaultPacketHandler(const char *packet, ULONG length);

/* Initialize the packet communication framework */
BOOL InitPacketFramework(void)
{
    /* Create message port for serial device */
    SerialMP = CreatePort(NULL, 0);
    if (!SerialMP) {
        printf("Failed to create serial message port\n");
        return FALSE;
    }

    /* Create I/O request for serial device */
    SerialIO = (struct IOExtSer *)CreateExtIO(SerialMP, sizeof(struct IOExtSer));
    if (!SerialIO) {
        printf("Failed to create serial I/O request\n");
        return FALSE;
    }

    /* Open serial device */
    if (OpenDevice("serial.device", 0, (struct IORequest *)SerialIO, 0) != 0) {
        if (OpenDevice("serial.device", 1, (struct IORequest *)SerialIO, 0) != 0) {
            printf("Failed to open serial device\n");
            return FALSE;
        }
    }
    
    SerialOpen = TRUE;

    /* Configure serial port: 9600 baud, 8N1, no flow control */
    SerialIO->io_Baud = 9600;
    SerialIO->io_ReadLen = 8;
    SerialIO->io_WriteLen = 8;
    SerialIO->io_StopBits = 1;
    SerialIO->io_RBufLen = 2048;
    SerialIO->io_SerFlags = SERF_XDISABLED;  /* Disable XON/XOFF */
    
    SerialIO->IOSer.io_Command = SDCMD_SETPARAMS;
    if (DoIO((struct IORequest *)SerialIO) != 0) {
        printf("Failed to set serial parameters\n");
        return FALSE;
    }
    
    /* Clear buffers */
    SerialIO->IOSer.io_Command = CMD_CLEAR;
    SerialIO->IOSer.io_Data = NULL;
    SerialIO->IOSer.io_Length = 0;
    DoIO((struct IORequest *)SerialIO);
    
    /* Setup timer */
    TimerMP = CreatePort(NULL, 0);
    if (TimerMP) {
        TimerIO = (struct timerequest *)CreateExtIO(TimerMP, sizeof(struct timerequest));
        if (TimerIO) {
            if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)TimerIO, 0) == 0) {
                TimerOpen = TRUE;
            }
        }
    }
    
    return TRUE;
}

/* Clean up framework resources */
void CleanupPacketFramework(void)
{
    if (SerialOpen) {
        CloseDevice((struct IORequest *)SerialIO);
        SerialOpen = FALSE;
    }
    
    if (SerialIO) {
        DeleteExtIO((struct IORequest *)SerialIO);
        SerialIO = NULL;
    }
    
    if (SerialMP) {
        DeletePort(SerialMP);
        SerialMP = NULL;
    }

    if (TimerOpen) {
        CloseDevice((struct IORequest *)TimerIO);
        TimerOpen = FALSE;
    }
    
    if (TimerIO) {
        DeleteExtIO((struct IORequest *)TimerIO);
        TimerIO = NULL;
    }
    
    if (TimerMP) {
        DeletePort(TimerMP);
        TimerMP = NULL;
    }
}

/* Send a packet */
BOOL SendPacket(const char *data, ULONG length)
{
    if (!SerialOpen || !SerialIO) 
        return FALSE;
    
    SerialIO->IOSer.io_Command = CMD_WRITE;
    SerialIO->IOSer.io_Data = (APTR)data;
    SerialIO->IOSer.io_Length = length;
    
    return (DoIO((struct IORequest *)SerialIO) == 0);
}

/* Receive a packet (non-blocking) */
ULONG ReceivePacket(char *buffer, ULONG maxLength)
{
    if (!SerialOpen || !SerialIO) 
        return 0;
    
    /* Check if data is available */
    SerialIO->IOSer.io_Command = SDCMD_QUERY;
    DoIO((struct IORequest *)SerialIO);
    
    if (SerialIO->IOSer.io_Actual > 0) {
        /* Read available data */
        SerialIO->IOSer.io_Command = CMD_READ;
        SerialIO->IOSer.io_Data = buffer;
        SerialIO->IOSer.io_Length = (SerialIO->IOSer.io_Actual < maxLength) ? 
                                   SerialIO->IOSer.io_Actual : maxLength;
        DoIO((struct IORequest *)SerialIO);
        return SerialIO->IOSer.io_Actual;
    }
    
    return 0;
}

/* Default packet handler - just prints received packets */
void DefaultPacketHandler(const char *packet, ULONG length)
{
    printf("Received packet (%lu bytes): ", length);
    fwrite(packet, 1, length, stdout);
    printf("\n");
    
    /* Auto-respond to "Hello Amiga" messages */
    if (strstr(packet, "Hello Amiga")) {
        char response[] = "Hello Pi!\r\n";
        SendPacket(response, strlen(response));
        printf("Sent auto-response: Hello Pi!\n");
    }
}

/* Main packet processing loop */
void ProcessPackets(PacketHandler handler)
{
    char buffer[1024];
    ULONG bytesRead;
    BOOL running = TRUE;
    
    if (!handler) {
        handler = DefaultPacketHandler;
    }
    
    printf("Packet framework started. Press Ctrl+C to exit.\n");
    
    while (running) {
        /* Check for incoming packets */
        bytesRead = ReceivePacket(buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            /* Null-terminate for string operations */
            buffer[bytesRead] = '\0';
            
            /* Process the packet */
            handler(buffer, bytesRead);
        }
        
        /* Small delay to prevent busy waiting */
        Delay(1);
        
        /* Check for break signal (simplified - in real use you'd want proper signal handling) */
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) {
            running = FALSE;
        }
    }
}

/* Example usage */
int main(void)
{
    printf("Amiga Packet Communication Framework\n");
    printf("===================================\n");
    
    if (InitPacketFramework()) {
        /* Process packets with default handler */
        ProcessPackets(NULL);
        
        CleanupPacketFramework();
    } else {
        printf("Framework initialization failed\n");
        CleanupPacketFramework();
    }
    
    return 0;
}