#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <atomic>
#include <ctime>

// Hikvision SDK headers
#include "HCISUPPublic.h"
#include "HCISUPCMS.h"
#include "HCISUPStream.h"

// Global variables for recording
std::ofstream g_recordFile;
std::atomic<bool> g_isRecording(false);
std::chrono::time_point<std::chrono::system_clock> g_recordStartTime;
std::mutex g_mtx;
std::condition_variable g_cv;
bool g_exitApp = false;

// Device registration callback
BOOL CALLBACK DeviceRegisterCallback(LONG lUserID, DWORD dwDataType, void *pOutBuffer, DWORD dwOutLen, 
                                     void *pInBuffer, DWORD dwInLen, void *pUser);

// Preview data callback
void CALLBACK PreviewDataCallback(LONG iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData);

// Function to print box with borders
void printBoxedInfo(const std::string& title, const std::string& content) {
    int width = 60;
    std::string border(width, '-');
    
    std::cout << "\n+" << border << "+" << std::endl;
    
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
        info += "Audio Channels: " + std::to_string(deviceInfo.dwAudioChanNum);
        
        printBoxedInfo("DETAILED DEVICE INFORMATION", info);
    } else {
        std::cout << "Failed to get detailed device information. Error code: " 
                  << NET_ECMS_GetLastError() << std::endl;
    }
}

// Function to start recording from a device
void startRecording(LONG lUserID, const char* deviceID) {
    // Create a filename with timestamp
    char filename[128] = {0};
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    sprintf(filename, "recording_%s_%04d%02d%02d_%02d%02d%02d.mp4", 
            deviceID,
            timeinfo->tm_year + 1900, 
            timeinfo->tm_mon + 1, 
            timeinfo->tm_mday,
            timeinfo->tm_hour, 
            timeinfo->tm_min, 
            timeinfo->tm_sec);
    
    // Open file for writing
    g_recordFile.open(filename, std::ios::binary);
    if (!g_recordFile.is_open()) {
        std::cout << "Failed to open file for recording: " << filename << std::endl;
        return;
    }
    
    printBoxedInfo("RECORDING STARTED", "Recording to: " + std::string(filename));
    
    // Initialize stream library
    if (!NET_ESTREAM_Init()) {
        std::cout << "Failed to initialize stream library. Error: " << NET_ESTREAM_GetLastError() << std::endl;
        g_recordFile.close();
        return;
    }
    
    // Set preview callback function
    NET_EHOME_PREVIEW_DATA_CB_PARAM previewCBParam = {0};
    previewCBParam.fnPreviewDataCB = PreviewDataCallback;
    previewCBParam.byStreamFormat = 0; // PS format
    
    // Set start time
    g_recordStartTime = std::chrono::system_clock::now();
    g_isRecording = true;
    
    // Start preview
    NET_EHOME_PREVIEWINFO_IN previewInfoIn = {0};
    NET_EHOME_PREVIEWINFO_OUT previewInfoOut = {0};
    
    previewInfoIn.iChannel = 1; // First channel
    previewInfoIn.dwStreamType = 0; // Main stream
    previewInfoIn.dwLinkMode = 0; // TCP mode
    
    if (!NET_ECMS_StartGetRealStream(lUserID, &previewInfoIn, &previewInfoOut)) {
        std::cout << "Failed to start preview. Error: " << NET_ECMS_GetLastError() << std::endl;
        g_recordFile.close();
        g_isRecording = false;
        NET_ESTREAM_Fini();
        return;
    }
    
    // Record for 5 seconds
    std::thread([lUserID, previewInfoOut]() {
        // Wait for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Stop recording
        g_isRecording = false;
        
        // Stop stream
        NET_ECMS_StopGetRealStream(lUserID, previewInfoOut.lSessionID);
        
        // Close file
        if (g_recordFile.is_open()) {
            g_recordFile.close();
            printBoxedInfo("RECORDING COMPLETE", "Video has been saved to file.");
        }
        
        // Cleanup
        NET_ESTREAM_Fini();
    }).detach();
}

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
    
    // Setup authentication for device (if needed)
    NET_EHOME_DEV_SESSIONKEY deviceKey = {0};
    // strcpy reinterpret_cast<char*>(deviceKey.sDeviceID), "your_device_id"); // Uncomment if needed
    // strcpy reinterpret_cast<char*>(deviceKey.sSessionKey), "verification_code"); // Uncomment if needed
    // NET_ECMS_SetDeviceSessionKey(&deviceKey); // Uncomment if needed
    
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
    listenParam.pUserData = NULL;
    
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
    
    // Wait for user input or exit signal
    std::unique_lock<std::mutex> lock(g_mtx);
    g_cv.wait(lock, []{ return g_exitApp || std::cin.get() == '\n'; });
    
    // Stop listening and clean up
    NET_ECMS_StopListen(listenHandle);
    NET_ECMS_Fini();
    
    std::cout << "Application exited." << std::endl;
    
    return 0;
}

// Preview data callback function
void CALLBACK PreviewDataCallback(LONG iPreviewHandle, NET_EHOME_PREVIEW_CB_MSG *pPreviewCBMsg, void *pUserData) {
    if (pPreviewCBMsg == NULL || !g_isRecording || !g_recordFile.is_open()) {
        return;
    }
    
    // Write data to file
    if (pPreviewCBMsg->byDataType == NET_EHOME_SYSHEAD || pPreviewCBMsg->byDataType == NET_EHOME_STREAMDATA) {
        g_recordFile.write(static_cast<char*>(pPreviewCBMsg->pRecvdata), pPreviewCBMsg->dwDataLen);
        
        // Check if recording time exceeded
        auto currentTime = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - g_recordStartTime).count() >= 5) {
            g_isRecording = false;
        }
    }
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
            
            // Get detailed device information
            getDeviceDetailedInfo(lUserID);
            
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
            
            if (!NET_ECMS_SetDevConfig(lUserID, 3 /*NET_EHOME_SET_SERVER_INFO*/, &config, sizeof(NET_EHOME_CONFIG))) {
                std::cout << "Failed to set server info. Error code: " 
                          << NET_ECMS_GetLastError() << std::endl;
            }
            else {
                printBoxedInfo("CONNECTION SUCCESS", 
                    "Device ID: " + std::string(reinterpret_cast<char*>(pDevInfo->byDeviceID)) + "\n" +
                    "Server info configured successfully\n" +
                    "Connection established and ready");
                
                // Start recording from this device
                std::thread([lUserID, pDevInfo]() {
                    // Wait a moment to ensure device is fully registered
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    
                    char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
                    memcpy(deviceID, pDevInfo->byDeviceID, MAX_DEVICE_ID_LEN);
                    
                    // Start recording
                    startRecording(lUserID, deviceID);
                }).detach();
            }
            
            return TRUE;
        }
    } 
    else if (dwDataType == ENUM_DEV_OFF) {
        // Device went offline
        std::cout << "Device disconnected. User ID: " << lUserID << std::endl;
        return TRUE;
    } 
    else if (dwDataType == ENUM_DEV_ADDRESS_CHANGED) {
        // Device address changed
        std::cout << "Device address changed. User ID: " << lUserID << std::endl;
        return TRUE;
    }
    
    return FALSE;
} 