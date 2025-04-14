# Hikvision Device Info

This application connects to Hikvision devices using the ISUP protocol and displays detailed device information when a device connects.

## Features

- Listens for Hikvision device connections using the ISUP protocol
- Displays detailed device information when a device connects
- Pretty-prints the device information in a formatted box
- Shows both basic and detailed device information
- Supports device online/offline detection

## Prerequisites

- CMake 3.10 or higher
- C++11 compatible compiler
- Hikvision SDK (included in the `/hikvision_sdk` directory)

## Building the Application

1. Create a build directory and navigate to it:

```bash
mkdir build && cd build
```

2. Generate the build files with CMake:

```bash
cmake ..
```

3. Build the application:

```bash
make
```

## Running the Application

The application listens on port 7660 by default for device connections:

```bash
./hikvision_device_info
```

The application will start and display:
```
HikVision SDK initialized successfully.
Listening for device connections on port 7660...
Press Enter to exit.
```

## Configuring Hikvision Devices

To connect your Hikvision devices to this application, you need to configure them to use the ISUP protocol:

1. Access the device's web interface
2. Go to Configuration > Network > Advanced Settings > Platform Access
3. Enable ISUP protocol
4. Set the following parameters:
   - Server Address: [IP of the computer running this application]
   - Server Port: 7660
   - Device ID: [Unique ID for this device]
5. Click Save

## Output Format

When a device connects, the application will display information in a formatted box like:

```
+------------------------------------------------------------+
|                      DEVICE CONNECTED                      |
|                                                            |
| Device ID: SAMPLE_DEVICE_123                               |
| Firmware Version: V1.2.3                                   |
| Serial Number: DS-2CD2142FWD-I20170123AACH123456789       |
| Device Type: 30                                            |
| Manufacturer: 1                                            |
| IP Address: 192.168.1.100                                  |
| Port: 8000                                                 |
+------------------------------------------------------------+
```

Followed by detailed device information.

## License

This project uses Hikvision SDK which is subject to Hikvision's licensing terms. 