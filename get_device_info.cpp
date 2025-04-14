#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include "hikvision_sdk/incEn/HCISUPPublic.h"
#include "hikvision_sdk/incEn/HCISUPStream.h"
#include "hikvision_sdk/incEn/HCISUPSS.h"

// Structure to store device information
struct DeviceInfo {
    std::string deviceID;
    std::string serialNumber;
    int channelNo;
    bool isOnline;
};

// Global storage for collected device info
std::vector<DeviceInfo> g_deviceInfoList;
bool g_bListeningActive = true;

// Callback function for new device connections
BOOL CALLBACK DeviceNewLinkCB(LONG iLinkHandle, NET_EHOME_NEWLINK_CB_MSG *pNewLinkCBMsg, void *pUserData)
{
    if (pNewLinkCBMsg == NULL) {
        std::cout << "Error: NULL callback message\n";
        return FALSE;
    }

    DeviceInfo info;
    info.deviceID = reinterpret_cast<char*>(pNewLinkCBMsg->szDeviceID);
    info.serialNumber = pNewLinkCBMsg->sDeviceSerial;
    info.channelNo = pNewLinkCBMsg->dwChannelNo;
    info.isOnline = true;

    // Store the device info
    g_deviceInfoList.push_back(info);

    // Display the information
    std::cout << "\n--- New Device Connected ---\n";
    std::cout << "Device ID: " << info.deviceID << "\n";
    std::cout << "Serial Number: " << info.serialNumber << "\n";
    std::cout << "Channel: " << info.channelNo << "\n";
    std::cout << "Stream Type: " << (int)pNewLinkCBMsg->byStreamType << "\n";
    std::cout << "Stream Format: " << (int)pNewLinkCBMsg->byStreamFormat << "\n";
    std::cout << "Session ID: " << pNewLinkCBMsg->iSessionID << "\n";
    std::cout << "---------------------------\n";

    // We've collected the info, no need to keep the connection open
    // Signal that we want to stop listening after a short delay
    g_bListeningActive = false;

    return TRUE;
}

int main(int argc, char* argv[])
{
    // Check command line arguments
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <listen_port>\n";
        return -1;
    }
    
    int listenPort = atoi(argv[1]);
    
    // Initialize the SDK
    if (!NET_ESTREAM_Init()) {
        std::cout << "Failed to initialize SDK! Error: " << NET_ESTREAM_GetLastError() << "\n";
        return -1;
    }
    
    std::cout << "Scanning for Hikvision devices...\n";
    std::cout << "Please make sure devices are configured to connect to this IP on port " << listenPort << "\n";
    
    // Configure the preview listener
    NET_EHOME_LISTEN_PREVIEW_CFG struPreviewListenParam;
    memset(&struPreviewListenParam, 0, sizeof(struPreviewListenParam));
    
    // Listen on all interfaces
    strcpy(struPreviewListenParam.struIPAdress.szIP, "0.0.0.0");
    struPreviewListenParam.struIPAdress.wPort = listenPort;
    
    // Set the callback for device connections
    struPreviewListenParam.fnNewLinkCB = DeviceNewLinkCB;
    struPreviewListenParam.byLinkMode = 0;  // TCP mode
    
    // Start listening
    LONG lListenHandle = NET_ESTREAM_StartListenPreview(&struPreviewListenParam);
    if (lListenHandle < 0) {
        std::cout << "Failed to start listener! Error: " << NET_ESTREAM_GetLastError() << "\n";
        NET_ESTREAM_Fini();
        return -1;
    }
    
    std::cout << "Listening for device connections. Waiting 30 seconds...\n";
    
    // Wait for devices to connect (maximum 30 seconds)
    int secondsWaited = 0;
    while (g_bListeningActive && secondsWaited < 30) {
        sleep(1);
        secondsWaited++;
        std::cout << "." << std::flush;
    }
    
    std::cout << "\n\nScan complete. " << g_deviceInfoList.size() << " device(s) found.\n\n";
    
    // Display a summary of all found devices
    if (!g_deviceInfoList.empty()) {
        std::cout << "=== Device Information Summary ===\n";
        for (size_t i = 0; i < g_deviceInfoList.size(); i++) {
            const auto& device = g_deviceInfoList[i];
            std::cout << "Device " << (i+1) << ":\n";
            std::cout << "  Device ID: " << device.deviceID << "\n";
            std::cout << "  Serial Number: " << device.serialNumber << "\n";
            std::cout << "  Channel: " << device.channelNo << "\n";
            std::cout << "  Status: " << (device.isOnline ? "Online" : "Offline") << "\n";
            std::cout << "\n";
        }
        std::cout << "================================\n";
    } else {
        std::cout << "No devices found. Please check your network and device configuration.\n";
    }
    
    // Clean up
    NET_ESTREAM_StopListenPreview(lListenHandle);
    NET_ESTREAM_Fini();
    
    return 0;
} 