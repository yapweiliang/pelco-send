// PelcoSend.c
// by Wei Liang Yap
// Version 1.0 - 2021-06-12
// adapted from SerialSend (https://batchloaf.wordpress.com/serialsend/) by TedBurke
//
// does not check for valid baud rates
    
#include <windows.h>
#include <stdio.h>

#define PELCO_PACKET_LENGTH 7
#define PELCO_SYNC 0xFF
#define PELCO_CALL_PRESET 0x07
    
int main(int argc, char *argv[])
{
    // Declare variables and structures
    unsigned char pelco_packet[PELCO_PACKET_LENGTH];
    int baudrate = 2400;  // default baud rate
    int comport = 3; // default com port
    int checksum = 0;
    char dev_name[MAX_PATH];
    int camera_id = 1;
    int camera_preset = -1;
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};
    int n;
    DWORD bytes_written;

    // Parse command line arguments
    int argn = 1;

    if (argc > 1) {
        // expect first argument to be the desired preset number
        camera_preset = atoi(argv[argn++]);
        if (camera_preset < 1) {
            fprintf(stderr, "%s: invalid preset specified", argv[0]);
            return 1;
        }
        
        // parse remaining arguments
        while(argn < argc)
        {
            if (strncmp(argv[argn], "/baudrate", 5) == 0)
            {
                // Parse baud rate
                if (++argn < argc && ((baudrate = atoi(argv[argn])) > 0))
                {
                }
                else
                {
                    fprintf(stderr, "%s: Baud rate error\n", argv[0]);
                    return 1;
                }
            }
            else if (strncmp(argv[argn], "/comNN", 4) == 0)
            {
                // Parse device number. 
                char s[MAX_PATH];
                strcpy(s, &argv[argn][4]);
                if (strlen(s) > 0) {
                    comport = atoi(s);
                }
                else
                {
                    fprintf(stderr, "%s: Com Port number error\n", argv[0]);
                    return 1;                    
                }
            }      
            else if (strcmp(argv[argn], "/camera") == 0)
            {   
                // Parse Camera ID
                if (++argn < argc)
                {
                    camera_id = atoi(argv[argn]);
                }
                else
                {
                    fprintf(stderr, "%s: camera_id error\n", argv[0]);
                    return 1;
                }
            }   
            
            // Next command line argument
            argn++;
        }

    } 
    else
    {
        fprintf(stderr, "Usage:\n\t%s CAMERA_PRESET [/camera CAMERA_ID] [/baudrate BAUDRATE] [/comN]\n\n", argv[0]);
        fprintf(stderr, "Example:\n\t%s 1 /camera 2 /com%d\n\n", argv[0], comport);
        return 1;
    }
    
    sprintf(dev_name, "\\\\.\\COM%d", comport);
    
    hSerial = CreateFile(
        dev_name, 
        GENERIC_READ|GENERIC_WRITE, 
        0, 
        NULL,
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL );
    if (hSerial == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "%s: Unable to open specified serial port %s\n", argv[0], dev_name);
        return 1;
    }

    // Set device parameters
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "%s: Error getting device state\n", argv[0]);
        CloseHandle(hSerial);
        return 1;
    }
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "%s: Error setting device parameters\n", argv[0]);
        CloseHandle(hSerial);
        return 1;
    }
   
    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &timeouts) == 0)
    {
        fprintf(stderr, "%s: Error setting timeouts\n", argv[0]);
        CloseHandle(hSerial);
        return 1;
    }

    // Compose Pelco packet
    pelco_packet[0] = PELCO_SYNC;
    pelco_packet[1] = camera_id;
    pelco_packet[2] = 0;
    pelco_packet[3] = PELCO_CALL_PRESET;
    pelco_packet[4] = 0;
    pelco_packet[5] = camera_preset;

    // Add checksum
    for (n=1; n < PELCO_PACKET_LENGTH-1; ++n ) {
        checksum += pelco_packet[n];
    }
    pelco_packet[PELCO_PACKET_LENGTH-1] = checksum % 256;

    // summarise
    fprintf(stderr, "%s: Set preset (%d) on camera (%d) via COM%d (%s), baud rate (%d)\n", argv[0], camera_preset, camera_id, comport, dev_name, baudrate);

    // send Pelco packet
    if(!WriteFile(hSerial, pelco_packet, PELCO_PACKET_LENGTH, &bytes_written, NULL))
    { 
        fprintf(stderr, "%s: Error writing to %s\n", argv[0], dev_name);
        CloseHandle(hSerial);
        return 1;
    }
    if (bytes_written == PELCO_PACKET_LENGTH) {
        fprintf(stderr, "%s: %02X %02X %02X %02X %02X %02X %02X (%d bytes written to %s)\n", argv[0],  
            pelco_packet[0], pelco_packet[1], pelco_packet[2], pelco_packet[3], pelco_packet[4], pelco_packet[5], pelco_packet[6],
            bytes_written, dev_name);
    }
    else
    {
        fprintf(stderr, "%s: Error sending packet. %d bytes written to %s)\n", argv[0], bytes_written, dev_name);
    }
     
    // Flush transmit buffer before closing serial port
    FlushFileBuffers(hSerial);

    // Close serial port
    CloseHandle(hSerial);
   
    // exit normally
    return 0;
}