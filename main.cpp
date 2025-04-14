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
#include <vector>
#include <arpa/inet.h>

// Hikvision SDK headers
#include "HCISUPPublic.h"
#include "HCISUPCMS.h"
#include "HCISUPStream.h"

// Define the structure for verification param (not found in headers)
typedef NET_EHOME_DEV_REG_INFO_V12 NET_EHOME_VERIFICATION_PARAM;

// Global variables for recording
std::ofstream g_recordFile;
std::atomic<bool> g_isRecording(false);
std::chrono::system_clock::time_point g_recordStartTime;
std::mutex g_mtx;
std::condition_variable g_cv;
bool g_exitApp = false;

// Store verification keys for devices
struct DeviceKey {
    char deviceID[MAX_DEVICE_ID_LEN + 1];
    char key[64];
};
std::vector<DeviceKey> g_deviceKeys;

// Additional global variables for device connection status
std::atomic<bool> g_isDeviceConnected(false);
std::string g_deviceID;
LONG g_linkHandle = -1;

// Device registration callback
BOOL CALLBACK DeviceRegisterCallback(LONG lUserID, DWORD dwDataType, void *pOutBuffer, DWORD dwOutLen, void *pInBuffer, DWORD dwInLen, void *pUser) {
    std::cout << "Device register callback. Data type: " << dwDataType << std::endl;
    
    switch (dwDataType) {
        case ENUM_DEV_ON: {
            NET_EHOME_DEV_REG_INFO_V12 *pDevInfo = (NET_EHOME_DEV_REG_INFO_V12*)pOutBuffer;
            char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
            memcpy(deviceID, pDevInfo->struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN);
            deviceID[MAX_DEVICE_ID_LEN] = 0;
            
            std::cout << "Device online. ID: " << deviceID << ", IP: " 
                      << pDevInfo->struRegInfo.struDevAdd.szIP 
                      << ":" << pDevInfo->struRegInfo.struDevAdd.wPort 
                      << ", Link handle: " << lUserID << std::endl;
            
            // Update globals for connection status
            g_isDeviceConnected = true;
            g_deviceID = deviceID;
            g_linkHandle = lUserID;
            g_cv.notify_all();
            break;
        }
        case ENUM_DEV_OFF: {
            NET_EHOME_DEV_REG_INFO_V12 *pDevInfo = (NET_EHOME_DEV_REG_INFO_V12*)pOutBuffer;
            char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
            memcpy(deviceID, pDevInfo->struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN);
            deviceID[MAX_DEVICE_ID_LEN] = 0;
            
            std::cout << "Device offline. ID: " << deviceID << std::endl;
            g_isDeviceConnected = false;
            g_deviceID = "";
            g_linkHandle = -1;
            break;
        }
        case ENUM_DEV_AUTH: {
            // Handle device authentication
            NET_EHOME_VERIFICATION_PARAM *pVerifyParam = (NET_EHOME_VERIFICATION_PARAM*)pOutBuffer;
            char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
            memcpy(deviceID, pVerifyParam->struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN);
            deviceID[MAX_DEVICE_ID_LEN] = 0;
            
            std::cout << "Device authentication request. ID: " << deviceID << std::endl;
            
            // Find the key for this device
            const char* key = nullptr;
            for (const auto& device : g_deviceKeys) {
                if (strcmp(device.deviceID, deviceID) == 0) {
                    key = device.key;
                    break;
                }
            }
            
            if (key) {
                // Set the verification key in the bySessionKey field
                memset(pVerifyParam->struRegInfo.bySessionKey, 0, MAX_MASTER_KEY_LEN);
                memcpy(pVerifyParam->struRegInfo.bySessionKey, key, strlen(key));
                std::cout << "Device authentication: setting verification key for " << deviceID << std::endl;
            } else {
                std::cout << "ERROR: No authentication key found for device ID: " << deviceID << std::endl;
            }
            break;
        }
        case ENUM_DEV_SESSIONKEY: {
            NET_EHOME_DEV_SESSIONKEY *pSessionKey = (NET_EHOME_DEV_SESSIONKEY*)pInBuffer;
            NET_EHOME_DEV_REG_INFO_V12 *pDevInfo = (NET_EHOME_DEV_REG_INFO_V12*)pOutBuffer;
            
            char deviceID[MAX_DEVICE_ID_LEN + 1] = {0};
            memcpy(deviceID, pDevInfo->struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN);
            deviceID[MAX_DEVICE_ID_LEN] = 0;
            
            // Find the key for this device
            const char* key = nullptr;
            for (const auto& device : g_deviceKeys) {
                if (strcmp(device.deviceID, deviceID) == 0) {
                    key = device.key;
                    break;
                }
            }
            
            if (key && pSessionKey) {
                // Copy device ID to session key structure
                memset(pSessionKey->sDeviceID, 0, MAX_DEVICE_ID_LEN);
                memcpy(pSessionKey->sDeviceID, deviceID, strlen(deviceID));
                
                // Copy the key to session key
                memset(pSessionKey->sSessionKey, 0, MAX_MASTER_KEY_LEN);
                memcpy(pSessionKey->sSessionKey, key, strlen(key));
                
                std::cout << "Setting session key for device: " << deviceID << std::endl;
                
                NET_EHOME_DEV_SESSIONKEY sessionKey = {0};
                memset(sessionKey.sDeviceID, 0, MAX_DEVICE_ID_LEN);
                memcpy(sessionKey.sDeviceID, deviceID, strlen(deviceID));
                memset(sessionKey.sSessionKey, 0, MAX_MASTER_KEY_LEN);
                memcpy(sessionKey.sSessionKey, key, strlen(key));
                NET_ECMS_SetDeviceSessionKey(&sessionKey);
                
                return TRUE;
            } else {
                std::cout << "ERROR: No session key found for device ID: " << deviceID << std::endl;
            }
            break;
        }
        default:
            std::cout << "Unknown data type: " << dwDataType << std::endl;
            break;
    }
    
    return TRUE;
}

// Preview data callback
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
    
    // Add known device key
    DeviceKey knownDevice;
    strcpy(knownDevice.deviceID, "k26311722");
    strcpy(knownDevice.key, "qq14253689");
    g_deviceKeys.push_back(knownDevice);
    
    printBoxedInfo("AUTHENTICATION SETUP", "Using verification key: qq14253689\nHandling auth directly in callback");
    
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
    listenParam.pUserData = nullptr;
    
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