/*
 * Amiga Simple Serial Test
 * Minimal implementation for bidirectional communication with Raspberry Pi Pico
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/serial.h>
#include <devices/timer.h>
#include <devices/console.h>
#include <devices/inputevent.h>
#include <proto/console.h>

/* Include prototypes for standard functions */
#include <stdio.h>
#include <string.h>

/* Include Amiga-specific prototypes */
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>  /* For CreatePort, CreateExtIO */

/* Define missing console constants */
#define MY_RAWKEYS 1    /* Replace with actual value if available */
#define MY_SETF_RAW 1   /* Replace with actual value if available */

/* Global variables */
struct MsgPort *SerialMP = NULL;     /* Message port for serial device */
struct IOExtSer *SerialIO = NULL;    /* I/O request for serial device */
BOOL SerialOpen = FALSE;             /* Flag for device open state */
struct MsgPort *TimerMP = NULL;      /* Message port for timer device */
struct timerequest *TimerIO = NULL;  /* I/O request for timer device */
BOOL TimerOpen = FALSE;              /* Flag for timer open state */
struct MsgPort *ConsoleMP = NULL;    /* Message port for console device */
struct IOStdReq *ConsoleIO = NULL;   /* I/O request for console device */
BOOL ConsoleOpen = FALSE;            /* Flag for console device open state */

/* Function prototypes */
BOOL InitSerial(void);
void CleanupSerial(void);
BOOL SendData(const char *data, ULONG length);
ULONG ReceiveData(char *buffer, ULONG length);
BOOL ResetSerial(void);
BOOL ConfigureTerminal(void);
BOOL InitConsole(void);
void CleanupConsole(void);
BOOL CheckKeyboard(void);
char GetKey(void);
char GetKeyNonBlocking(void);

/* Debug flag */
#define DEBUG 1

/* Initialize the serial device */
BOOL InitSerial(void)
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

    /* Try unit 0 first, then unit 1 */
    if (OpenDevice("serial.device", 0, (struct IORequest *)SerialIO, 0) != 0) {
        printf("Unit 0 failed, trying unit 1...\n");
        if (OpenDevice("serial.device", 1, (struct IORequest *)SerialIO, 0) != 0) {
            printf("Failed to open serial device\n");
            return FALSE;
        }
    }
    
    SerialOpen = TRUE;
    printf("Serial device opened successfully\n");

    /* Configure serial port: 9600 baud, 8 bits, no parity, 1 stop bit */
    SerialIO->io_Baud = 9600;
    SerialIO->io_ReadLen = 8;
    SerialIO->io_WriteLen = 8;
    SerialIO->io_StopBits = 1;
    SerialIO->io_RBufLen = 1024;  /* Double to 1024 bytes */
    
    /* CRITICAL CHANGE: Disable flow control from the start */
    SerialIO->io_SerFlags &= ~SERF_7WIRE;     /* Disable RTS/CTS */
    SerialIO->io_SerFlags |= SERF_XDISABLED;  /* Disable XON/XOFF */
    
    SerialIO->IOSer.io_Command = SDCMD_SETPARAMS;
    
    if (DoIO((struct IORequest *)SerialIO) != 0) {
        printf("Failed to set serial parameters\n");
        return FALSE;
    }
    
    /* Clear input and output buffers */
    SerialIO->IOSer.io_Command = CMD_CLEAR;
    SerialIO->IOSer.io_Data = NULL;
    SerialIO->IOSer.io_Length = 0;
    DoIO((struct IORequest *)SerialIO);
    
    /* Setup timer for timeouts */
    TimerMP = CreatePort(NULL, 0);
    if (TimerMP) {
        TimerIO = (struct timerequest *)CreateExtIO(TimerMP, sizeof(struct timerequest));
        if (TimerIO) {
            if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)TimerIO, 0) == 0) {
                TimerOpen = TRUE;
                printf("Timer device opened successfully\n");
            }
        }
    }
    
    printf("Serial port initialization complete\n");
    
    /* Brief delay after initialization to allow buffers to settle */
    if (TimerOpen && TimerIO) {
        TimerIO->tr_node.io_Command = TR_ADDREQUEST;
        TimerIO->tr_time.tv_secs = 0;
        TimerIO->tr_time.tv_micro = 500000;  /* 500ms */
        DoIO((struct IORequest *)TimerIO);
    }
    
    return TRUE;
}

/* Clean up and close devices */
void CleanupSerial(void)
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

    printf("Serial port closed\n");
}

/* Send data through serial port with improved error handling */
BOOL SendData(const char *data, ULONG length)
{
    int retry;
    LONG error;
    
    if (!SerialOpen || !SerialIO) 
        return FALSE;
    
    /* Debug output */
    printf("[Sending %lu bytes] ", length);
    
    /* Try sending multiple times */
    for (retry = 0; retry < 3; retry++) {
        /* Make sure we're starting fresh */
        SerialIO->IOSer.io_Error = 0;
        
        /* Use synchronous I/O */
        SerialIO->IOSer.io_Command = CMD_WRITE;
        SerialIO->IOSer.io_Data = (APTR)data;
        SerialIO->IOSer.io_Length = length;
        
        error = DoIO((struct IORequest *)SerialIO);
        
        if (error == 0) {
            printf("[Sent OK]\n");
            
            /* Brief pause after successful send to ensure transmission */
            if (TimerOpen && TimerIO) {
                TimerIO->tr_node.io_Command = TR_ADDREQUEST;
                TimerIO->tr_time.tv_secs = 0;
                TimerIO->tr_time.tv_micro = 100000;  /* 100ms */
                DoIO((struct IORequest *)TimerIO);
            }
            
            return TRUE;
        } else {
            printf("[Send error %ld] ", error);
            
            /* Reset serial device on error */
            SerialIO->IOSer.io_Command = CMD_CLEAR;
            DoIO((struct IORequest *)SerialIO);
            
            /* Wait a moment before retrying */
            if (TimerOpen && TimerIO) {
                TimerIO->tr_node.io_Command = TR_ADDREQUEST;
                TimerIO->tr_time.tv_secs = 0;
                TimerIO->tr_time.tv_micro = 200000;  /* 200ms */
                DoIO((struct IORequest *)TimerIO);
            }
        }
    }
    
    printf("[Send failed]\n");
    return FALSE;
}

/* Terminal-optimized receive function */
ULONG ReceiveData(char *buffer, ULONG length)
{
    ULONG bytesRead = 0;
    
    if (!SerialOpen || !SerialIO) 
        return 0;
    
    /* First check quickly if data is available */
    SerialIO->IOSer.io_Command = SDCMD_QUERY;
    DoIO((struct IORequest *)SerialIO);
    
    if (SerialIO->IOSer.io_Actual > 0) {
        /* Data available, read it immediately */
        SerialIO->IOSer.io_Command = CMD_READ;
        SerialIO->IOSer.io_Data = buffer;
        SerialIO->IOSer.io_Length = length;
        DoIO((struct IORequest *)SerialIO);
        bytesRead = SerialIO->IOSer.io_Actual;
    }
    
    return bytesRead;
}

/* Reset serial device completely */
BOOL ResetSerial(void)
{
    printf("Performing complete serial reset...\n");
    
    /* First close existing device if open */
    if (SerialOpen) {
        CloseDevice((struct IORequest *)SerialIO);
        SerialOpen = FALSE;
    }
    
    /* Try reopening device */
    if (OpenDevice("serial.device", 0, (struct IORequest *)SerialIO, 0) != 0) {
        printf("Reset: Unit 0 failed, trying unit 1...\n");
        if (OpenDevice("serial.device", 1, (struct IORequest *)SerialIO, 0) != 0) {
            printf("Failed to reopen serial device\n");
            return FALSE;
        }
    }
    
    SerialOpen = TRUE;
    printf("Serial device reopened successfully\n");
    
    /* Reset ALL parameters to defaults */
    SerialIO->io_Baud = 9600;  /* Ensure this matches Pi */
    SerialIO->io_ReadLen = 8;
    SerialIO->io_WriteLen = 8;
    SerialIO->io_StopBits = 1;
    SerialIO->io_RBufLen = 1024;  /* Large buffer */
    SerialIO->io_ExtFlags = 0;    /* Clear all extended flags */
    SerialIO->io_SerFlags = 0;    /* Start with all flags cleared */
    SerialIO->io_SerFlags |= SERF_XDISABLED;  /* Disable XON/XOFF */
    
    SerialIO->IOSer.io_Command = SDCMD_SETPARAMS;
    if (DoIO((struct IORequest *)SerialIO) != 0) {
        printf("Failed to reset serial parameters\n");
        return FALSE;
    }
    
    /* Clear input and output buffers */
    SerialIO->IOSer.io_Command = CMD_CLEAR;
    SerialIO->IOSer.io_Data = NULL;
    SerialIO->IOSer.io_Length = 0;
    DoIO((struct IORequest *)SerialIO);
    
    /* Small delay after reset */
    if (TimerOpen && TimerIO) {
        TimerIO->tr_node.io_Command = TR_ADDREQUEST;
        TimerIO->tr_time.tv_secs = 1;  /* Full 1 second pause */
        TimerIO->tr_time.tv_micro = 0;
        DoIO((struct IORequest *)TimerIO);
    }
    
    return TRUE;
}

/* Configure terminal settings */
BOOL ConfigureTerminal(void)
{
    printf("Configuring terminal settings...\n");
    
    /* Set serial parameters optimally for terminal use */
    SerialIO->io_Baud = 9600;        /* Standard baud rate */
    SerialIO->io_ReadLen = 8;        /* 8 data bits */
    SerialIO->io_WriteLen = 8;       /* 8 data bits */
    SerialIO->io_StopBits = 1;       /* 1 stop bit */
    SerialIO->io_RBufLen = 4096;     /* Larger input buffer */
    SerialIO->io_ExtFlags = 0;       /* No extended flags */
    
    /* Disable all flow control for maximum compatibility */
    SerialIO->io_SerFlags = 0;       /* Start with all flags clear */
    SerialIO->io_SerFlags |= SERF_XDISABLED;  /* Disable XON/XOFF */
    SerialIO->io_SerFlags &= ~SERF_7WIRE;     /* Disable RTS/CTS */
    
    /* Apply the settings */
    SerialIO->IOSer.io_Command = SDCMD_SETPARAMS;
    if (DoIO((struct IORequest *)SerialIO) != 0) {
        printf("Failed to set terminal parameters\n");
        return FALSE;
    }
    
    return TRUE;
}

/* Initialize console for keyboard input */
BOOL InitConsole(void)
{
    ConsoleMP = CreatePort(NULL, 0);
    if (!ConsoleMP) {
        printf("Failed to create console message port\n");
        return FALSE;
    }
    
    ConsoleIO = (struct IOStdReq *)CreateExtIO(ConsoleMP, sizeof(struct IOStdReq));
    if (!ConsoleIO) {
        printf("Failed to create console I/O request\n");
        DeletePort(ConsoleMP);
        ConsoleMP = NULL;
        return FALSE;
    }
    
    if (OpenDevice("console.device", -1, (struct IORequest *)ConsoleIO, 0) != 0) {
        printf("Failed to open console device\n");
        DeleteExtIO((struct IORequest *)ConsoleIO);
        ConsoleIO = NULL;
        DeletePort(ConsoleMP);
        ConsoleMP = NULL;
        return FALSE;
    }
    
    ConsoleOpen = TRUE;
    
    /* Set raw mode for console - use custom defines */
    ConsoleIO->io_Data = (APTR)MY_RAWKEYS;
    ConsoleIO->io_Command = MY_SETF_RAW;
    DoIO((struct IORequest *)ConsoleIO);
    
    return TRUE;
}

/* Clean up console device */
void CleanupConsole(void)
{
    if (ConsoleOpen) {
        /* Restore normal mode */
        ConsoleIO->io_Data = (APTR)0;
        ConsoleIO->io_Command = MY_SETF_RAW;
        DoIO((struct IORequest *)ConsoleIO);
        
        CloseDevice((struct IORequest *)ConsoleIO);
        ConsoleOpen = FALSE;
    }
    
    if (ConsoleIO) {
        DeleteExtIO((struct IORequest *)ConsoleIO);
        ConsoleIO = NULL;
    }
    
    if (ConsoleMP) {
        DeletePort(ConsoleMP);
        ConsoleMP = NULL;
    }
}

/* Check if a key is pressed - fixed return type */
BOOL CheckKeyboard(void)
{
    ULONG signals;
    ULONG consoleMask;
    
    if (!ConsoleOpen || !ConsoleIO)
        return FALSE;
    
    /* Get the signal mask for the console message port */
    consoleMask = 1 << ConsoleMP->mp_SigBit;
    
    /* Check if any messages are waiting (non-blocking) */
    signals = SetSignal(0, 0);
    
    /* Convert integer to proper BOOL value */
    if (signals & consoleMask)
        return TRUE;
    else
        return FALSE;
}

/* Get a key from keyboard (blocking) */
char GetKey(void)
{
    char keycode = 0;
    
    if (!ConsoleOpen || !ConsoleIO)
        return 0;
    
    /* Read a key from console */
    ConsoleIO->io_Command = CMD_READ;
    ConsoleIO->io_Data = &keycode;
    ConsoleIO->io_Length = 1;
    DoIO((struct IORequest *)ConsoleIO);
    
    return keycode;
}

/* Get a key from keyboard (non-blocking) */
char GetKeyNonBlocking(void)
{
    char keycode = 0;
    
    if (!ConsoleOpen || !ConsoleIO)
        return 0;
    
    /* Check if any input is available */
    if (CheckKeyboard()) {
        /* Read a key from console */
        ConsoleIO->io_Command = CMD_READ;
        ConsoleIO->io_Data = &keycode;
        ConsoleIO->io_Length = 1;
        DoIO((struct IORequest *)ConsoleIO);
        return keycode;
    }
    
    return 0;  /* No key pressed */
}

/* Simplified direct receive function - more reliable for terminal use */
ULONG ReceiveDataDirect(char *buffer, ULONG maxLength)
{
    ULONG bytesRead = 0;
    
    if (!SerialOpen || !SerialIO) 
        return 0;
    
    /* Use SDCMD_QUERY to check if data is available without blocking */
    SerialIO->IOSer.io_Command = SDCMD_QUERY;
    DoIO((struct IORequest *)SerialIO);
    
    if (SerialIO->IOSer.io_Actual > 0) {
        /* Only try to read if data is actually available */
        SerialIO->IOSer.io_Command = CMD_READ;
        SerialIO->IOSer.io_Data = buffer;
        /* Read only what's available */
        SerialIO->IOSer.io_Length = SerialIO->IOSer.io_Actual < maxLength ? 
                                   SerialIO->IOSer.io_Actual : maxLength;
        DoIO((struct IORequest *)SerialIO);
        bytesRead = SerialIO->IOSer.io_Actual;
    }
    
    return bytesRead;
}

/* Improved keyboard handling for command detection */
void RunSerialTerminal(void)
{
    char recvBuffer[1024];  /* Larger buffer for incoming data */
    char sendBuffer[256];
    char keyBuffer[32];     /* Buffer for keyboard input */
    char tempBuffer[32];    /* Buffer for case-insensitive comparison - MOVED TO TOP */
    int keyPos = 0;         /* Current position in key buffer */
    ULONG bytesRead;
    BOOL running = TRUE;
    BOOL localEcho = TRUE;  /* Enable local echo by default */
    int pollCounter = 0;
    int quickPoll;
    int i;                  /* Loop counter - MOVED TO TOP */
    char key;
    
    /* Clear the screen and set terminal colors */
    printf("\033[2J\033[H\033[31;47m");  /* Red text on white background */
    printf("=== Amiga Serial Terminal ===\n");
    printf("Baud: 9600  Data: 8N1  Flow: None  Echo: ON\n");
    printf("Type 'exit', 'close', or press ESC to quit\n\n");
    
    /* Initialize key buffer */
    keyBuffer[0] = '\0';
    
    /* Send a dummy character to establish connection */
    sendBuffer[0] = '\r';  /* Simple carriage return */
    SendData(sendBuffer, 1);
    
    /* Set status indicator */
    printf("[Terminal Ready - Listening for data]\n");
    
    /* Main terminal loop */
    while (running) {
        pollCounter++;
        
        /* ===== KEYBOARD HANDLING ===== */
        /* Check for keyboard input */
        if (CheckKeyboard()) {
            /* Read a character from keyboard */
            key = GetKeyNonBlocking();
            
            if (key != 0) {  /* Only process if we got a valid key */
                /* Check for ESC key (ASCII 27) - EXIT IMMEDIATELY */
                if (key == 27) {
                    printf("\n\nESC pressed. Exiting terminal...\n");
                    running = FALSE;
                    break;  /* Break out of the main loop immediately */
                }
                
                /* Check for Enter/Return key */
                if (key == '\r' || key == '\n' || key == 10 || key == 13) {
                    /* Always echo a newline */
                    printf("\n");
                    
                    /* Force null-terminate the buffer */
                    keyBuffer[keyPos] = '\0';
                    
                    /* Debug: print buffer content */
                    printf("[Buffer: '%s']\n", keyBuffer);
                    
                    /* Check for exit commands - case insensitive */
                    strcpy(tempBuffer, keyBuffer);
                    
                    /* Convert to lowercase for comparison - fixed for loop */
                    for (i = 0; i < keyPos; i++) {
                        if (tempBuffer[i] >= 'A' && tempBuffer[i] <= 'Z')
                            tempBuffer[i] = tempBuffer[i] + ('a' - 'A');
                    }
                    
                    /* Compare the lowercase version */
                    if (strcmp(tempBuffer, "exit") == 0 || 
                        strcmp(tempBuffer, "close") == 0 ||
                        strcmp(tempBuffer, "quit") == 0) {
                        printf("\n[Command recognized: Closing terminal...]\n");
                        running = FALSE;
                        break;  /* Break out of the main loop immediately */
                    }
                    
                    /* Send the line to the serial port */
                    if (keyPos > 0) {
                        /* Debug info */
                        printf("[Sending: '%s']\n", keyBuffer);
                        
                        /* Send with CR+LF */
                        SendData(keyBuffer, strlen(keyBuffer));
                        SendData("\r\n", 2);
                    }
                    
                    /* Clear key buffer after sending */
                    keyPos = 0;
                    keyBuffer[0] = '\0';
                } 
                /* Handle backspace and delete */
                else if (key == 8 || key == 127) {
                    if (keyPos > 0) {
                        keyPos--;
                        keyBuffer[keyPos] = '\0';
                        /* Erase character from screen */
                        printf("\b \b");
                    }
                }
                /* Normal printable character */
                else if (key >= 32 && key <= 126) {
                    /* Add to our key buffer if there's room */
                    if (keyPos < sizeof(keyBuffer) - 1) {
                        keyBuffer[keyPos++] = key;
                        keyBuffer[keyPos] = '\0';  /* Keep null-terminated */
                        
                        /* Echo if enabled */
                        if (localEcho) {
                            printf("%c", key);
                        }
                    }
                }
            }
        }
        
        /* ===== SERIAL DATA HANDLING ===== */
        /* Check for incoming data (multiple times per loop) */
        for (quickPoll = 0; quickPoll < 3; quickPoll++) {
            /* Read directly without querying first - more reliable */
            bytesRead = ReceiveDataDirect(recvBuffer, sizeof(recvBuffer) - 1);
            
            if (bytesRead > 0) {
                /* Null-terminate and display */
                recvBuffer[bytesRead] = '\0';
                printf("%s", recvBuffer);
                
                /* AUTOMATICALLY SEND RESPONSE when we receive data */
                /* Check if the received message contains "Hello Amiga" */
                if (strstr(recvBuffer, "Hello Amiga")) {
                    /* Wait a short time before responding */
                    TimerIO->tr_node.io_Command = TR_ADDREQUEST;
                    TimerIO->tr_time.tv_secs = 0;
                    TimerIO->tr_time.tv_micro = 150000;  /* 150ms */
                    DoIO((struct IORequest *)TimerIO);
                    
                    /* Format a response */
                    sprintf(sendBuffer, "Hello Pi #%d!\r\n", pollCounter % 100);
                    
                    /* Show what we're sending */
                    printf("\n[Auto-Response: %s]\n", sendBuffer);
                    
                    /* Send the response */
                    SendData(sendBuffer, strlen(sendBuffer));
                }
                
                /* Break out of polling loop since we got data */
                break;
            }
            
            /* Ultra-short delay between rapid polls */
            Delay(1);
        }
        
        /* Very short delay in main loop - keep checking data frequently */
        Delay(2);  /* Short delay, about 40ms */
        
        /* Send a ping every ~10 seconds to keep connection alive */
        if (pollCounter % 250 == 0) {
            /* Check serial device status */
            SerialIO->IOSer.io_Command = SDCMD_QUERY;
            DoIO((struct IORequest *)SerialIO);
        }
    }
    
    /* Reset colors and clean up before exit */
    printf("\033[0m\n\nTerminal session ended.\n");
}

/* Main program with console initialization */
int main(void)
{
    /* Clear screen and set colors for better visibility */
    printf("\033[2J\033[30;47m");   /* Clear screen, black on white */
    
    printf("Amiga Serial Terminal\n");
    printf("===================\n");
    
    /* Initialize both serial and console devices */
    if (InitSerial() && InitConsole()) {
        /* Configure for terminal operation */
        ConfigureTerminal();
        
        /* Run the terminal - FIXED: Call RunSerialTerminal instead of RunSerialTest */
        RunSerialTerminal();
        
        /* Reset colors before exiting */
        printf("\033[0m");
        
        /* Clean up resources */
        CleanupConsole();
        CleanupSerial();
    } else {
        printf("Initialization failed\n");
        
        /* Clean up any resources that were allocated */
        if (ConsoleOpen) CleanupConsole();
        if (SerialOpen) CleanupSerial();
    }
    
    return 0;
}

