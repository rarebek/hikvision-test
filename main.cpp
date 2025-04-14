#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include "hikvision_sdk/incEn/HCISUPPublic.h"
#include "hikvision_sdk/incEn/HCISUPStream.h"

// Global variables for control
bool g_bExit = false;
LONG g_lListenHandle = -1;

// Signal handler for graceful termination
void SignalHandler(int signal)
{
    g_bExit = true;
    std::cout << "Termination signal received, exiting...\n";
}

// Callback function for handling new connections
BOOL CALLBACK PreviewNewLinkCB(LONG iLinkHandle, NET_EHOME_NEWLINK_CB_MSG *pNewLinkCBMsg, void *pUserData)
{
    if (pNewLinkCBMsg == NULL)
    {
        std::cout << "Error: New link callback message is NULL!\n";
        return FALSE;
    }

    std::cout << "New connection established: \n";
    std::cout << "  Device ID: " << pNewLinkCBMsg->szDeviceID << "\n";
    std::cout << "  Session ID: " << pNewLinkCBMsg->iSessionID << "\n";
    std::cout << "  Channel: " << pNewLinkCBMsg->dwChannelNo << "\n";
    std::cout << "  Device Serial: " << pNewLinkCBMsg->sDeviceSerial << "\n";

    // Set up callback to receive data from this connection
    NET_EHOME_PREVIEW_DATA_CB_PARAM struDataCBParam;
    memset(&struDataCBParam, 0, sizeof(struDataCBParam));
    
    // Define the callback for stream data
    struDataCBParam.fnPreviewDataCB = [](LONG iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData) -> void {
        if (pPreviewCBMsg == NULL)
            return;
            
        // Handle different types of data
        switch (pPreviewCBMsg->byDataType)
        {
            case NET_EHOME_SYSHEAD:  // Stream header
                std::cout << "Received stream header, length: " << pPreviewCBMsg->dwDataLen << " bytes\n";
                break;
                
            case NET_EHOME_STREAMDATA:  // Stream data
                std::cout << "Received stream data, length: " << pPreviewCBMsg->dwDataLen << " bytes\n";
                // Here you would typically:
                // 1. Save to file, or
                // 2. Decode and display, or
                // 3. Forward to another application
                break;
                
            case NET_EHOME_STREAMEND:  // End of stream
                std::cout << "Stream ended\n";
                break;
                
            default:
                std::cout << "Unknown data type: " << (int)pPreviewCBMsg->byDataType << "\n";
                break;
        }
    };
    
    struDataCBParam.pUserData = NULL;
    
    // Register the callback to receive data
    if (!NET_ESTREAM_SetPreviewDataCB(iLinkHandle, &struDataCBParam))
    {
        std::cout << "Failed to set preview data callback! Error: " << NET_ESTREAM_GetLastError() << "\n";
        return FALSE;
    }
    
    return TRUE;
}

int main(int argc, char* argv[])
{
    // Check command line arguments
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <listen_port>\n";
        return -1;
    }
    
    int listenPort = atoi(argv[1]);
    
    // Set up signal handler for graceful termination
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // Initialize the SDK
    if (!NET_ESTREAM_Init())
    {
        std::cout << "Failed to initialize the SDK! Error: " << NET_ESTREAM_GetLastError() << "\n";
        return -1;
    }
    
    // Set log file
    if (!NET_ESTREAM_SetLogToFile(3, const_cast<char*>("./logs"), TRUE))
    {
        std::cout << "Failed to set log file! Error: " << NET_ESTREAM_GetLastError() << "\n";
        // Continue anyway - non-critical
    }
    
    // Configure and start the preview listener
    NET_EHOME_LISTEN_PREVIEW_CFG struPreviewListenParam;
    memset(&struPreviewListenParam, 0, sizeof(struPreviewListenParam));
    
    // Set the listening IP to 0.0.0.0 (all interfaces)
    strcpy(struPreviewListenParam.struIPAdress.szIP, "0.0.0.0");
    struPreviewListenParam.struIPAdress.wPort = listenPort;
    
    // Set connection callback
    struPreviewListenParam.fnNewLinkCB = PreviewNewLinkCB;
    struPreviewListenParam.byLinkMode = 0;  // TCP mode
    
    // Start listening for preview connections
    g_lListenHandle = NET_ESTREAM_StartListenPreview(&struPreviewListenParam);
    if (g_lListenHandle < 0)
    {
        std::cout << "Failed to start preview listener! Error: " << NET_ESTREAM_GetLastError() << "\n";
        NET_ESTREAM_Fini();
        return -1;
    }
    
    std::cout << "Started preview listener on port " << listenPort << "\n";
    std::cout << "Waiting for connections... (Press Ctrl+C to exit)\n";
    
    // Main loop
    while (!g_bExit)
    {
        // Just sleep to avoid CPU usage
        sleep(1);
    }
    
    // Cleanup
    if (g_lListenHandle >= 0)
    {
        NET_ESTREAM_StopListenPreview(g_lListenHandle);
        g_lListenHandle = -1;
    }
    
    NET_ESTREAM_Fini();
    std::cout << "Application terminated normally.\n";
    
    return 0;
}
