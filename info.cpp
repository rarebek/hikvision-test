#include <iostream>
#include "hikvision_sdk/incEn/HCISUPPublic.h"  // Replace with the actual header provided by your SDK

// Hypothetical structure to hold device information.
// Adjust the field names and types as per the actual SDK documentation.
struct DeviceInfo {
    char model[64];
    char firmwareVersion[32];
    char serialNumber[64];
    // Add additional fields if needed.
};

int main() {
    // Step 1: Initialize the SDK.
    int initResult = ISUPSDK_Initialize(); // Hypothetical initialization function
    if (initResult != 0) {
        std::cerr << "Failed to initialize ISUPSDK. Error code: " << initResult << std::endl;
        return -1;
    }

    // Step 2: Set device connection parameters.
    const char* deviceIP = "192.168.1.100";  // Set your device IP here
    int devicePort = 8000;                   // Default port for many Hikvision devices
    const char* username = "admin";          // Replace with your device username
    const char* password = "your_password";  // Replace with your device password

    // Step 3: Login to the Hikvision device.
    // Assume DeviceHandle is a typedef defined in ISUPSDK.h (e.g., a pointer or integer handle)
    DeviceHandle deviceHandle = ISUPSDK_Login(deviceIP, devicePort, username, password);
    if (deviceHandle == nullptr) {
        std::cerr << "Login failed for device at " << deviceIP << ":" << devicePort << std::endl;
        ISUPSDK_Cleanup();
        return -1;
    }

    // Step 4: Retrieve the device information.
    DeviceInfo devInfo;
    int ret = ISUPSDK_GetDeviceInfo(deviceHandle, &devInfo);
    if (ret != 0) {
        std::cerr << "Failed to retrieve device information. Error code: " << ret << std::endl;
        ISUPSDK_Logout(deviceHandle);
        ISUPSDK_Cleanup();
        return -1;
    }

    // Step 5: Print the device information to the console in a pretty format.
    std::cout << "===========================================" << std::endl;
    std::cout << "         Connected Hikvision Device        " << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Model           : " << devInfo.model << std::endl;
    std::cout << "Firmware Version: " << devInfo.firmwareVersion << std::endl;
    std::cout << "Serial Number   : " << devInfo.serialNumber << std::endl;
    std::cout << "IP Address      : " << deviceIP << std::endl;
    std::cout << "Port            : " << devicePort << std::endl;
    std::cout << "===========================================" << std::endl;

    // Step 6: Logout and cleanup the SDK resources.
    ISUPSDK_Logout(deviceHandle);
    ISUPSDK_Cleanup();

    return 0;
}
