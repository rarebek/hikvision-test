#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <map>
#include "hikvision_sdk/incEn/HCISUPPublic.h"
#include "hikvision_sdk/incEn/HCISUPStream.h"
#include "hikvision_sdk/incEn/HCISUPSS.h"

// Global variables for control
bool g_bExit = false;
LONG g_lListenHandle = -1;

// Device info storage
struct DeviceInfo {
    std::string deviceID;
    std::string serialNumber;
    LONG sessionID;
    DWORD channelNo;
    BYTE streamType;
    BYTE streamFormat;
    bool isStreaming;
    time_t lastDataTime;
};

// Map to store device info by link handle
std::map<LONG, DeviceInfo> g_deviceMap;

// Signal handler for graceful termination
void SignalHandler(int signal)
{
    g_bExit = true;
    std::cout << "Termination signal received, exiting...\n";
}

// Display detailed device information
void DisplayDeviceInfo(const DeviceInfo& info)
{
    std::cout << "\n============ DEVICE INFORMATION ============\n";
    std::cout << "Device ID: " << info.deviceID << "\n";
    std::cout << "Serial Number: " << info.serialNumber << "\n";
    std::cout << "Channel: " << info.channelNo << "\n";
    std::cout << "Stream Type: " << (int)info.streamType << " (";
    
    // Interpret stream type
    switch(info.streamType) {
        case 0: std::cout << "Main stream"; break;
        case 1: std::cout << "Sub stream"; break;
        case 2: std::cout << "Third stream"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << ")\n";
    
    std::cout << "Stream Format: " << (int)info.streamFormat << " (";
    
    // Interpret stream format
    switch(info.streamFormat) {
        case 0: std::cout << "PS"; break;
        case 1: std::cout << "RTP"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << ")\n";
    
    std::cout << "Session ID: " << info.sessionID << "\n";
    std::cout << "===========================================\n\n";
}

// Callback for handling stream data
void CALLBACK PreviewDataCallback(LONG iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData)
{
    if (pPreviewCBMsg == NULL)
        return;
        
    // Find the device in our map
    auto it = g_deviceMap.find(iPreviewHandle);
    if (it != g_deviceMap.end()) {
        // Update last data time
        it->second.lastDataTime = time(NULL);
    }
    
    // Handle different types of data
    switch (pPreviewCBMsg->byDataType)
    {
        case NET_EHOME_SYSHEAD:  // Stream header
            std::cout << "Received stream header, length: " << pPreviewCBMsg->dwDataLen << " bytes\n";
            break;
            
        case NET_EHOME_STREAMDATA:  // Stream data
            // Only log occasionally to avoid flooding the console
            if (pPreviewCBMsg->dwDataLen % 1000 == 0) {
                std::cout << "Received stream data, length: " << pPreviewCBMsg->dwDataLen << " bytes\n";
            }
            break;
            
        case NET_EHOME_STREAMEND:  // End of stream
            std::cout << "Stream ended\n";
            
            // Update device status
            if (it != g_deviceMap.end()) {
                it->second.isStreaming = false;
            }
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

    std::cout << "New connection established!\n";
    
    // Step 1: Store device information
    DeviceInfo info;
    info.deviceID = reinterpret_cast<char*>(pNewLinkCBMsg->szDeviceID);
    info.serialNumber = pNewLinkCBMsg->sDeviceSerial;
    info.sessionID = pNewLinkCBMsg->iSessionID;
    info.channelNo = pNewLinkCBMsg->dwChannelNo;
    info.streamType = pNewLinkCBMsg->byStreamType;
    info.streamFormat = pNewLinkCBMsg->byStreamFormat;
    info.isStreaming = true;
    info.lastDataTime = time(NULL);
    
    // Step 2: Display detailed device information
    DisplayDeviceInfo(info);
    
    // Step 3: Store in global map
    g_deviceMap[iLinkHandle] = info;

    // Step 4: Set up callback to receive data from this connection
    NET_EHOME_PREVIEW_DATA_CB_PARAM struDataCBParam;
    memset(&struDataCBParam, 0, sizeof(struDataCBParam));
    
    // Set the callback for stream data
    struDataCBParam.fnPreviewDataCB = PreviewDataCallback;
    struDataCBParam.pUserData = NULL;
    struDataCBParam.byStreamFormat = pNewLinkCBMsg->byStreamFormat; // Use the device's stream format
    
    // Register the callback to receive data
    if (!NET_ESTREAM_SetPreviewDataCB(iLinkHandle, &struDataCBParam))
    {
        std::cout << "Failed to set preview data callback! Error: " << NET_ESTREAM_GetLastError() << "\n";
        g_deviceMap.erase(iLinkHandle);
        return FALSE;
    }
    
    std::cout << "Successfully registered data callback for device " << info.deviceID << "\n";
    return TRUE;
}

// Function to display a summary of all connected devices
void DisplayConnectedDevices()
{
    if (g_deviceMap.empty()) {
        std::cout << "No devices connected.\n";
        return;
    }
    
    std::cout << "\n======== CONNECTED DEVICES SUMMARY ========\n";
    std::cout << "Total connected devices: " << g_deviceMap.size() << "\n\n";
    
    int index = 1;
    time_t currentTime = time(NULL);
    
    for (const auto& pair : g_deviceMap) {
        const auto& device = pair.second;
        std::cout << "Device " << index++ << ":\n";
        std::cout << "  Device ID: " << device.deviceID << "\n";
        std::cout << "  Serial Number: " << device.serialNumber << "\n";
        std::cout << "  Channel: " << device.channelNo << "\n";
        std::cout << "  Status: " << (device.isStreaming ? "Streaming" : "Not streaming") << "\n";
        std::cout << "  Last data received: " << difftime(currentTime, device.lastDataTime) << " seconds ago\n\n";
    }
    std::cout << "===========================================\n";
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
    
    // Step 1: Initialize the SDK
    std::cout << "Initializing Hikvision SDK...\n";
    if (!NET_ESTREAM_Init())
    {
        std::cout << "Failed to initialize the SDK! Error: " << NET_ESTREAM_GetLastError() << "\n";
        return -1;
    }
    
    // Step 2: Set log file
    if (!NET_ESTREAM_SetLogToFile(3, const_cast<char*>("./logs"), TRUE))
    {
        std::cout << "Failed to set log file! Error: " << NET_ESTREAM_GetLastError() << "\n";
        // Continue anyway - non-critical
    }
    
    // Step 3: Configure and start the preview listener
    std::cout << "Configuring preview listener...\n";
    NET_EHOME_LISTEN_PREVIEW_CFG struPreviewListenParam;
    memset(&struPreviewListenParam, 0, sizeof(struPreviewListenParam));
    
    // Set the listening IP to 0.0.0.0 (all interfaces)
    strcpy(struPreviewListenParam.struIPAdress.szIP, "0.0.0.0");
    struPreviewListenParam.struIPAdress.wPort = listenPort;
    
    // Set connection callback
    struPreviewListenParam.fnNewLinkCB = PreviewNewLinkCB;
    struPreviewListenParam.byLinkMode = 0;  // TCP mode
    
    // Start listening for preview connections
    std::cout << "Starting preview listener...\n";
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
    int deviceCheckCounter = 0;
    while (!g_bExit)
    {
        // Sleep to avoid CPU usage
        sleep(1);
        
        // Periodically display connected devices summary (every 30 seconds)
        if (++deviceCheckCounter >= 30) {
            DisplayConnectedDevices();
            deviceCheckCounter = 0;
        }
    }
    
    // Cleanup all connections
    std::cout << "Cleaning up and exiting...\n";
    for (const auto& pair : g_deviceMap) {
        NET_ESTREAM_StopPreview(pair.first);
    }
    g_deviceMap.clear();
    
    // Stop listening
    if (g_lListenHandle >= 0)
    {
        NET_ESTREAM_StopListenPreview(g_lListenHandle);
        g_lListenHandle = -1;
    }
    
    // Uninitialize SDK
    NET_ESTREAM_Fini();
    std::cout << "Application terminated normally.\n";
    
    return 0;
}
