#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <functional>
#include <thread>
#include <chrono>

// Hikvision SDK headers
#include "HCISUPPublic.h"
#include "HCISUPCMS.h"

// Define command codes if not in SDK headers
#define NET_EHOME_GET_DEVICE_INFO      0x1000
#define NET_EHOME_GET_VERSION_INFO     0x1006
#define NET_EHOME_SET_SERVER_INFO      0x2000

// Device registration callback
BOOL CALLBACK DeviceRegisterCallback(LONG lUserID, DWORD dwDataType, void *pOutBuffer, DWORD dwOutLen, 
                                     void *pInBuffer, DWORD dwInLen, void *pUser);

// Function to print box with borders
void printBoxedInfo(const std::string& title, const std::string& content) {
    int width = 60;
    std::string border(width, '-');
    
    std::cout << "+" << border << "+" << std::endl;
    
    // Print title centered
    int spaces = (width - title.length()) / 2;
    std::cout << "|" << std::string(spaces, ' ') << title << std::string(width - title.length() - spaces, ' ') << "|" << std::endl;
    
    std::cout << "|" << std::string(width, ' ') << "|" << std::endl;
    
    // Split content by newlines and print each line with padding
    size_t pos = 0;
    std::string line;
    std::string remainingContent = content;
    
    while ((pos = remainingContent.find('\n')) != std::string::npos) {
        line = remainingContent.substr(0, pos);
        std::cout << "| " << std::left << std::setw(width-2) << line << " |" << std::endl;
        remainingContent.erase(0, pos + 1);
    }
    
    // Print the last line if any
    if (!remainingContent.empty()) {
        std::cout << "| " << std::left << std::setw(width-2) << remainingContent << " |" << std::endl;
    }
    
    std::cout << "+" << border << "+" << std::endl;
}

// Function to pretty print device info
void prettyPrintDeviceInfo(const NET_EHOME_DEV_REG_INFO& deviceInfo) {
    std::string info;
    
    // Format device ID
    char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
    memcpy(deviceID, deviceInfo.byDeviceID, MAX_DEVICE_ID_LEN);
    
    // Format firmware version
    char firmwareVersion[25] = {0};
    memcpy(firmwareVersion, deviceInfo.byFirmwareVersion, 24);
    
    // Format device serial
    char deviceSerial[NET_EHOME_SERIAL_LEN + 1] = {0};
    memcpy(deviceSerial, deviceInfo.sDeviceSerial, NET_EHOME_SERIAL_LEN);
    
    // Create formatted device info
    info = "Device ID: " + std::string(deviceID) + "\n";
    info += "Firmware Version: " + std::string(firmwareVersion) + "\n";
    info += "Serial Number: " + std::string(deviceSerial) + "\n";
    info += "Device Type: " + std::to_string(deviceInfo.dwDevType) + "\n";
    info += "Manufacturer: " + std::to_string(deviceInfo.dwManufacture) + "\n";
    info += "IP Address: " + std::to_string(deviceInfo.struDevAdd.szIP[0]) + "." +
                             std::to_string(deviceInfo.struDevAdd.szIP[1]) + "." +
                             std::to_string(deviceInfo.struDevAdd.szIP[2]) + "." +
                             std::to_string(deviceInfo.struDevAdd.szIP[3]) + "\n";
    info += "Port: " + std::to_string(deviceInfo.struDevAdd.wPort);
    
    printBoxedInfo("DEVICE CONNECTED", info);
}

// Function to get detailed device information
void getDeviceDetailedInfo(LONG lUserID) {
    // Query device info
    NET_EHOME_DEVICE_INFO deviceInfo = {0};
    deviceInfo.dwSize = sizeof(NET_EHOME_DEVICE_INFO);
    
    NET_EHOME_CONFIG config = {0};
    config.pOutBuf = &deviceInfo;
    config.dwOutSize = sizeof(NET_EHOME_DEVICE_INFO);
    
    if (NET_ECMS_GetDevConfig(lUserID, NET_EHOME_GET_DEVICE_INFO, &config, sizeof(NET_EHOME_CONFIG))) {
        std::string info;
        
        // Add fields to info string
        info = "Channels: " + std::to_string(deviceInfo.dwChannelNumber) + "\n";
        info += "Total Channels: " + std::to_string(deviceInfo.dwChannelAmount) + "\n";
        info += "Device Type: " + std::to_string(deviceInfo.dwDevType) + "\n";
        info += "Disk Number: " + std::to_string(deviceInfo.dwDiskNumber) + "\n";
        
        // Format serial number
        char serialNumber[MAX_SERIALNO_LEN + 1] = {0};
        memcpy(serialNumber, deviceInfo.sSerialNumber, MAX_SERIALNO_LEN);
        info += "Serial Number: " + std::string(serialNumber) + "\n";
        
        info += "Alarm In Ports: " + std::to_string(deviceInfo.dwAlarmInPortNum) + "\n";
        info += "Alarm Out Ports: " + std::to_string(deviceInfo.dwAlarmOutPortNum) + "\n";
        info += "Start Channel: " + std::to_string(deviceInfo.dwStartChannel) + "\n";
        info += "Audio Channels: " + std::to_string(deviceInfo.dwAudioChanNum) + "\n";
        info += "Max Digital Channels: " + std::to_string(deviceInfo.dwMaxDigitChannelNum) + "\n";
        info += "Audio Encoding Type: " + std::to_string(deviceInfo.dwAudioEncType) + "\n";
        info += "Zero Channels Support: " + std::to_string(deviceInfo.dwSupportZeroChan);
        
        printBoxedInfo("DETAILED DEVICE INFORMATION", info);
    } else {
        std::cout << "Failed to get detailed device information. Error code: " 
                  << NET_ECMS_GetLastError() << std::endl;
    }
    
    // Get version info
    NET_EHOME_VERSION_INFO versionInfo = {0};
    versionInfo.dwSize = sizeof(NET_EHOME_VERSION_INFO);
    
    config.pOutBuf = &versionInfo;
    config.dwOutSize = sizeof(NET_EHOME_VERSION_INFO);
    
    if (NET_ECMS_GetDevConfig(lUserID, NET_EHOME_GET_VERSION_INFO, &config, sizeof(NET_EHOME_CONFIG))) {
        std::string info;
        
        // Format software version
        char softwareVersion[MAX_VERSION_LEN + 1] = {0};
        memcpy(softwareVersion, versionInfo.sSoftwareVersion, MAX_VERSION_LEN);
        info = "Software Version: " + std::string(softwareVersion) + "\n";
        
        // Format DSP software version
        char dspVersion[MAX_VERSION_LEN + 1] = {0};
        memcpy(dspVersion, versionInfo.sDSPSoftwareVersion, MAX_VERSION_LEN);
        info += "DSP Version: " + std::string(dspVersion) + "\n";
        
        // Format panel version
        char panelVersion[MAX_VERSION_LEN + 1] = {0};
        memcpy(panelVersion, versionInfo.sPanelVersion, MAX_VERSION_LEN);
        info += "Panel Version: " + std::string(panelVersion) + "\n";
        
        // Format hardware version
        char hardwareVersion[MAX_VERSION_LEN + 1] = {0};
        memcpy(hardwareVersion, versionInfo.sHardwareVersion, MAX_VERSION_LEN);
        info += "Hardware Version: " + std::string(hardwareVersion);
        
        printBoxedInfo("VERSION INFORMATION", info);
    } else {
        std::cout << "Failed to get version information. Error code: " 
                  << NET_ECMS_GetLastError() << std::endl;
    }
}

// Main function
int main() {
    // Initialize the SDK
    if (!NET_ECMS_Init()) {
        std::cout << "Failed to initialize HikVision SDK. Error code: " 
                  << NET_ECMS_GetLastError() << std::endl;
        return -1;
    }
    
    std::cout << "HikVision SDK initialized successfully." << std::endl;
    
    // Set log parameters
    NET_ECMS_SetLogToFile(3, const_cast<char*>("./logs"), TRUE);
    
    // Setup listen parameters for device registration
    NET_EHOME_CMS_LISTEN_PARAM listenParam = {0};
    
    // Set the local IP and port to listen on
    listenParam.struAddress.szIP[0] = 0; // 0.0.0.0 means all interfaces
    listenParam.struAddress.szIP[1] = 0;
    listenParam.struAddress.szIP[2] = 0;
    listenParam.struAddress.szIP[3] = 0;
    listenParam.struAddress.wPort = 7660; // Standard Hikvision ISUP port
    
    // Set the callback function
    listenParam.fnCB = DeviceRegisterCallback;
    
    // Start listening for device connections
    LONG listenHandle = NET_ECMS_StartListen(&listenParam);
    
    if (listenHandle < 0) {
        std::cout << "Failed to start listening for device connections. Error code: " 
                  << NET_ECMS_GetLastError() << std::endl;
        NET_ECMS_Fini();
        return -1;
    }
    
    std::cout << "Listening for device connections on port 7660..." << std::endl;
    std::cout << "Press Enter to exit." << std::endl;
    
    // Wait for user input to exit
    std::cin.get();
    
    // Stop listening and clean up
    NET_ECMS_StopListen(listenHandle);
    NET_ECMS_Fini();
    
    std::cout << "Application exited." << std::endl;
    
    return 0;
}

// Device registration callback function
BOOL CALLBACK DeviceRegisterCallback(LONG lUserID, DWORD dwDataType, void *pOutBuffer, DWORD dwOutLen, 
                                     void *pInBuffer, DWORD dwInLen, void *pUser) {
    if (dwDataType == ENUM_DEV_ON) {
        // Device is online
        if (pOutBuffer != nullptr && dwOutLen == sizeof(NET_EHOME_DEV_REG_INFO)) {
            NET_EHOME_DEV_REG_INFO *pDevInfo = (NET_EHOME_DEV_REG_INFO*)pOutBuffer;
            
            // Print device info in a nice format
            prettyPrintDeviceInfo(*pDevInfo);
            
            // Get and print detailed device information
            std::thread([lUserID]() {
                // Wait a bit for the device to fully register
                std::this_thread::sleep_for(std::chrono::seconds(1));
                getDeviceDetailedInfo(lUserID);
            }).detach();
            
            // Set server info for the connected device
            NET_EHOME_SERVER_INFO serverInfo = {0};
            serverInfo.dwSize = sizeof(NET_EHOME_SERVER_INFO);
            serverInfo.dwAlarmServerType = 1; // Support TCP and UDP
            serverInfo.dwKeepAliveSec = 15;   // 15 seconds keep-alive
            
            // Set alarm server address (same as CMS)
            serverInfo.struTCPAlarmSever.szIP[0] = 0;
            serverInfo.struTCPAlarmSever.szIP[1] = 0;
            serverInfo.struTCPAlarmSever.szIP[2] = 0;
            serverInfo.struTCPAlarmSever.szIP[3] = 0;
            serverInfo.struTCPAlarmSever.wPort = 7660;
            
            // Set alarm server info
            NET_EHOME_CONFIG config = {0};
            config.pInBuf = &serverInfo;
            config.dwInSize = sizeof(NET_EHOME_SERVER_INFO);
            
            if (!NET_ECMS_SetDevConfig(lUserID, NET_EHOME_SET_SERVER_INFO, &config, sizeof(NET_EHOME_CONFIG))) {
                std::cout << "Failed to set server info. Error code: " 
                          << NET_ECMS_GetLastError() << std::endl;
            }
            else {
                // Print success message in a nice box
                printBoxedInfo("CONNECTION SUCCESS", 
                    "Device ID: " + std::string((char*)pDevInfo->byDeviceID) + "\n" +
                    "Server info configured successfully\n" +
                    "Connection established and ready");
            }
            
            return TRUE;
        }
    } else if (dwDataType == ENUM_DEV_OFF) {
        // Device went offline
        std::cout << "Device disconnected. User ID: " << lUserID << std::endl;
        return TRUE;
    } else if (dwDataType == ENUM_DEV_ADDRESS_CHANGED) {
        // Device address changed
        std::cout << "Device address changed. User ID: " << lUserID << std::endl;
        return TRUE;
    }
    
    return FALSE;
} 