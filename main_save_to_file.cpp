#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <mutex>
#include <map>
#include "hikvision_sdk/incEn/HCISUPPublic.h"
#include "hikvision_sdk/incEn/HCISUPStream.h"

// Global variables for control
bool g_bExit = false;
LONG g_lListenHandle = -1;

// Mutex for thread safety
std::mutex g_mutex;

// Structure to hold stream file information
struct StreamFileInfo
{
    std::ofstream file;
    bool hasHeader;
    std::string deviceId;
    int channelNo;
};

// Map to store stream file handles by preview handle
std::map<LONG, StreamFileInfo> g_streamFiles;

// Signal handler for graceful termination
void SignalHandler(int signal)
{
    g_bExit = true;
    std::cout << "Termination signal received, exiting...\n";
}

// Callback function for handling stream data
void CALLBACK PreviewDataCB(LONG iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData)
{
    if (pPreviewCBMsg == NULL)
        return;
        
    std::lock_guard<std::mutex> lock(g_mutex);
    
    // Get or create file information for this handle
    auto& fileInfo = g_streamFiles[iPreviewHandle];
    
    // Handle different types of data
    switch (pPreviewCBMsg->byDataType)
    {
        case NET_EHOME_SYSHEAD:  // Stream header
            std::cout << "Received stream header from device " << fileInfo.deviceId 
                      << " channel " << fileInfo.channelNo 
                      << ", length: " << pPreviewCBMsg->dwDataLen << " bytes\n";
            
            // Save header to file
            if (fileInfo.file.is_open() && pPreviewCBMsg->pRecvdata != nullptr)
            {
                fileInfo.file.write(static_cast<char*>(pPreviewCBMsg->pRecvdata), pPreviewCBMsg->dwDataLen);
                fileInfo.hasHeader = true;
            }
            break;
            
        case NET_EHOME_STREAMDATA:  // Stream data
            if (pPreviewCBMsg->dwDataLen % 1000 == 0)
            {
                std::cout << "Received " << pPreviewCBMsg->dwDataLen << " bytes of stream data from device " 
                          << fileInfo.deviceId << " channel " << fileInfo.channelNo << "\n";
            }
            
            // Save data to file if we have a header
            if (fileInfo.file.is_open() && fileInfo.hasHeader && pPreviewCBMsg->pRecvdata != nullptr)
            {
                fileInfo.file.write(static_cast<char*>(pPreviewCBMsg->pRecvdata), pPreviewCBMsg->dwDataLen);
            }
            break;
            
        case NET_EHOME_STREAMEND:  // End of stream
            std::cout << "Stream ended for device " << fileInfo.deviceId 
                      << " channel " << fileInfo.channelNo << "\n";
            
            // Close the file
            if (fileInfo.file.is_open())
            {
                fileInfo.file.close();
            }
            
            // Remove from our map
            g_streamFiles.erase(iPreviewHandle);
            break;
            
        default:
            std::cout << "Unknown data type: " << (int)pPreviewCBMsg->byDataType << "\n";
            break;
    }
}

// Callback function for handling new connections
BOOL CALLBACK PreviewNewLinkCB(LONG iLinkHandle, NET_EHOME_NEWLINK_CB_MSG *pNewLinkCBMsg, void *pUserData)
{
    if (pNewLinkCBMsg == NULL)
    {
        std::cout << "Error: New link callback message is NULL!\n";
        return FALSE;
    }

    std::string deviceId = reinterpret_cast<char*>(pNewLinkCBMsg->szDeviceID);
    std::string serialNumber = pNewLinkCBMsg->sDeviceSerial;
    int channelNo = pNewLinkCBMsg->dwChannelNo;
    
    std::cout << "New connection established: \n";
    std::cout << "  Device ID: " << deviceId << "\n";
    std::cout << "  Session ID: " << pNewLinkCBMsg->iSessionID << "\n";
    std::cout << "  Channel: " << channelNo << "\n";
    std::cout << "  Device Serial: " << serialNumber << "\n";

    // Create a filename for this stream
    std::string filename = "stream_" + deviceId + "_channel_" + std::to_string(channelNo) + ".ps";
    
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        
        // Create file information structure
        StreamFileInfo fileInfo;
        fileInfo.deviceId = deviceId;
        fileInfo.channelNo = channelNo;
        fileInfo.hasHeader = false;
        
        // Open the file for writing
        fileInfo.file.open(filename, std::ios::binary);
        if (!fileInfo.file.is_open())
        {
            std::cout << "Failed to open file for writing: " << filename << "\n";
            return FALSE;
        }
        
        // Store in our map
        g_streamFiles[iLinkHandle] = std::move(fileInfo);
    }
    
    std::cout << "Saving stream to file: " << filename << "\n";
    
    // Set up callback to receive data from this connection
    NET_EHOME_PREVIEW_DATA_CB_PARAM struDataCBParam;
    memset(&struDataCBParam, 0, sizeof(struDataCBParam));
    
    // Define the callback for stream data
    struDataCBParam.fnPreviewDataCB = PreviewDataCB;
    struDataCBParam.pUserData = NULL;
    struDataCBParam.byStreamFormat = 0;  // PS format
    
    // Register the callback to receive data
    if (!NET_ESTREAM_SetPreviewDataCB(iLinkHandle, &struDataCBParam))
    {
        std::cout << "Failed to set preview data callback! Error: " << NET_ESTREAM_GetLastError() << "\n";
        
        // Clean up file
        std::lock_guard<std::mutex> lock(g_mutex);
        g_streamFiles.erase(iLinkHandle);
        
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
    
    // Close all open files
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto& pair : g_streamFiles)
        {
            if (pair.second.file.is_open())
            {
                pair.second.file.close();
            }
        }
        g_streamFiles.clear();
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