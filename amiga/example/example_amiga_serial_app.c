/*
 * Example Packet Application
 * Demonstrates usage of the Amiga Packet Communication Framework
 */

#include "amiga_packet_framework.h"
#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* Application state */
typedef struct {
    ULONG packetCount;
    ULONG commandCount;
    BOOL echoMode;
    BOOL verboseMode;
} AppState;

static AppState appState = {0, 0, TRUE, FALSE};

/* Command structure */
typedef struct {
    const char *name;
    void (*handler)(const char *args);
    const char *description;
} Command;

/* Forward declarations */
void HandleStatusCommand(const char *args);
void HandleEchoCommand(const char *args);
void HandleVerboseCommand(const char *args);
void HandleHelpCommand(const char *args);
void HandlePingCommand(const char *args);
void HandleSendCommand(const char *args);
void HandleResetCommand(const char *args);
void CustomPacketHandler(const char *packet, ULONG length);

/* Command table */
static Command commands[] = {
    {"STATUS", HandleStatusCommand, "Show application status"},
    {"ECHO", HandleEchoCommand, "Toggle echo mode on/off"},
    {"VERBOSE", HandleVerboseCommand, "Toggle verbose mode on/off"},
    {"HELP", HandleHelpCommand, "Show this help message"},
    {"PING", HandlePingCommand, "Send ping to remote device"},
    {"SEND", HandleSendCommand, "Send custom message"},
    {"RESET", HandleResetCommand, "Reset packet counters"},
    {NULL, NULL, NULL}  /* End marker */
};

/* Command handlers */
void HandleStatusCommand(const char *args)
{
    char response[256];
    
    sprintf(response, "STATUS: Packets=%lu Commands=%lu Echo=%s Verbose=%s\r\n",
            appState.packetCount, 
            appState.commandCount,
            appState.echoMode ? "ON" : "OFF",
            appState.verboseMode ? "ON" : "OFF");
    
    SendPacket(response, strlen(response));
    
    if (appState.verboseMode) {
        printf("Sent status response\n");
    }
}

void HandleEchoCommand(const char *args)
{
    appState.echoMode = !appState.echoMode;
    
    char response[64];
    sprintf(response, "ECHO: %s\r\n", appState.echoMode ? "ON" : "OFF");
    SendPacket(response, strlen(response));
    
    printf("Echo mode: %s\n", appState.echoMode ? "ON" : "OFF");
}

void HandleVerboseCommand(const char *args)
{
    appState.verboseMode = !appState.verboseMode;
    
    char response[64];
    sprintf(response, "VERBOSE: %s\r\n", appState.verboseMode ? "ON" : "OFF");
    SendPacket(response, strlen(response));
    
    printf("Verbose mode: %s\n", appState.verboseMode ? "ON" : "OFF");
}

void HandleHelpCommand(const char *args)
{
    char response[512];
    int i;
    
    strcpy(response, "HELP: Available commands:\r\n");
    
    for (i = 0; commands[i].name != NULL; i++) {
        strcat(response, commands[i].name);
        strcat(response, " - ");
        strcat(response, commands[i].description);
        strcat(response, "\r\n");
    }
    
    SendPacket(response, strlen(response));
    
    if (appState.verboseMode) {
        printf("Sent help information\n");
    }
}

void HandlePingCommand(const char *args)
{
    char response[] = "PONG\r\n";
    SendPacket(response, strlen(response));
    
    printf("Received PING, sent PONG\n");
}

void HandleSendCommand(const char *args)
{
    char response[256];
    
    if (args && strlen(args) > 0) {
        sprintf(response, "ECHO: %s\r\n", args);
        SendPacket(response, strlen(response));
        
        if (appState.verboseMode) {
            printf("Echoed message: %s\n", args);
        }
    } else {
        strcpy(response, "ERROR: No message specified\r\n");
        SendPacket(response, strlen(response));
    }
}

void HandleResetCommand(const char *args)
{
    appState.packetCount = 0;
    appState.commandCount = 0;
    
    char response[] = "RESET: Counters cleared\r\n";
    SendPacket(response, strlen(response));
    
    printf("Packet counters reset\n");
}

/* Process a command from the packet */
void ProcessCommand(const char *packet, ULONG length)
{
    char command[64];
    char args[192];
    int i;
    const char *space;
    
    appState.commandCount++;
    
    /* Extract command and arguments */
    space = strchr(packet, ' ');
    if (space) {
        /* Command with arguments */
        strncpy(command, packet, space - packet);
        command[space - packet] = '\0';
        strcpy(args, space + 1);
        
        /* Remove trailing newlines from args */
        char *newline = strchr(args, '\r');
        if (newline) *newline = '\0';
        newline = strchr(args, '\n');
        if (newline) *newline = '\0';
    } else {
        /* Command without arguments */
        strcpy(command, packet);
        args[0] = '\0';
        
        /* Remove trailing newlines from command */
        char *newline = strchr(command, '\r');
        if (newline) *newline = '\0';
        newline = strchr(command, '\n');
        if (newline) *newline = '\0';
    }
    
    if (appState.verboseMode) {
        printf("Processing command: '%s' with args: '%s'\n", command, args);
    }
    
    /* Find and execute command */
    for (i = 0; commands[i].name != NULL; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            commands[i].handler(args);
            return;
        }
    }
    
    /* Unknown command */
    char response[128];
    sprintf(response, "ERROR: Unknown command '%s'. Type HELP for available commands.\r\n", command);
    SendPacket(response, strlen(response));
    
    if (appState.verboseMode) {
        printf("Unknown command: %s\n", command);
    }
}

/* Custom packet handler for the application */
void CustomPacketHandler(const char *packet, ULONG length)
{
    appState.packetCount++;
    
    if (appState.verboseMode) {
        printf("Received packet #%lu (%lu bytes): ", appState.packetCount, length);
        fwrite(packet, 1, length, stdout);
        printf("\n");
    }
    
    /* Check if this is a command (starts with a letter) */
    if (length > 0 && ((packet[0] >= 'A' && packet[0] <= 'Z') || 
                       (packet[0] >= 'a' && packet[0] <= 'z'))) {
        ProcessCommand(packet, length);
    } else if (appState.echoMode) {
        /* Echo non-command data if echo mode is enabled */
        char response[256];
        sprintf(response, "ECHO: ");
        strncat(response, packet, length < 240 ? length : 240);
        strcat(response, "\r\n");
        SendPacket(response, strlen(response));
        
        if (appState.verboseMode) {
            printf("Echoed data packet\n");
        }
    }
}

/* Main application */
int main(void)
{
    printf("Amiga Packet Application Example\n");
    printf("===============================\n");
    printf("Commands: STATUS, ECHO, VERBOSE, HELP, PING, SEND, RESET\n");
    printf("Press Ctrl+C to exit\n\n");
    
    /* Initialize the packet framework */
    if (!InitPacketFramework()) {
        printf("Failed to initialize packet framework\n");
        return 1;
    }
    
    printf("Framework initialized successfully\n");
    printf("Echo mode: %s\n", appState.echoMode ? "ON" : "OFF");
    printf("Verbose mode: %s\n\n", appState.verboseMode ? "ON" : "OFF");
    
    /* Send startup notification */
    char startup[] = "READY: Amiga packet application started\r\n";
    SendPacket(startup, strlen(startup));
    
    /* Start packet processing with custom handler */
    ProcessPackets(CustomPacketHandler);
    
    /* Send shutdown notification */
    char shutdown[] = "SHUTDOWN: Amiga packet application stopping\r\n";
    SendPacket(shutdown, strlen(shutdown));
    
    /* Cleanup */
    CleanupPacketFramework();
    
    printf("\nApplication terminated\n");
    printf("Final statistics:\n");
    printf("  Total packets received: %lu\n", appState.packetCount);
    printf("  Commands processed: %lu\n", appState.commandCount);
    
    return 0;
}