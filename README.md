# Hikvision Video Streaming Example

This is a simple example application demonstrating how to receive video streams from Hikvision cameras using the Hikvision EHome SDK.

## Prerequisites

- Linux operating system
- GCC/G++ compiler
- Hikvision EHome SDK (included in the `hikvision_sdk` directory)
- Hikvision IP cameras or network video recorders that support the EHome protocol
- CMake (optional, for easier building)

## Building the Application

### Using CMake (Recommended)

```bash
mkdir -p build
cd build
cmake ..
make
```

This will build both examples:
- `stream_viewer` - Basic example that displays stream information
- `stream_saver` - Advanced example that saves streams to files

### Manual Compilation

If you prefer to compile manually:

```bash
# For the basic example
mkdir -p logs
g++ -o stream_viewer main.cpp -I. -L./hikvision_sdk/lib -lHCISUPStream -Wl,-rpath=./hikvision_sdk/lib

# For the advanced example
g++ -o stream_saver main_save_to_file.cpp -I. -L./hikvision_sdk/lib -lHCISUPStream -Wl,-rpath=./hikvision_sdk/lib
```

## Running the Application

### Basic Example

```bash
./stream_viewer <listen_port>
```

Where `<listen_port>` is the port number you want the application to listen on (e.g., 8000).

This example will display information about received streams but does not save them.

### Advanced Example (Save to File)

```bash
./stream_saver <listen_port>
```

This example saves each stream to a separate file named `stream_<deviceID>_channel_<channelNumber>.ps`. The PS format can be played with media players like VLC.

## How It Works

This application:

1. Initializes the Hikvision SDK
2. Sets up a listener on the specified port
3. Waits for Hikvision devices to connect and send video streams
4. Processes the incoming video stream data through callbacks
5. Either displays information about the stream data (basic example) or saves it to a file (advanced example)

## Device Configuration

For Hikvision cameras or NVRs to connect to this application, you need to configure them:

1. Access the device's web interface
2. Navigate to Configuration → Network → Advanced Settings → Platform Access
3. Enable EHome protocol
4. Set the Server IP to the IP address of the computer running this application
5. Set the Server Port to the port number specified when running the application
6. Save the settings

## Working with the Saved Files

The saved files are in PS (Program Stream) format, which can be played with:

- VLC Media Player
- FFmpeg
- FFplay

Example with FFplay:
```bash
ffplay stream_DEVICE123_channel_1.ps
```

Example with VLC:
```bash
vlc stream_DEVICE123_channel_1.ps
```

You can also convert the PS file to MP4 or other formats using FFmpeg:
```bash
ffmpeg -i stream_DEVICE123_channel_1.ps -c copy output.mp4
```

## Notes

- The basic example only prints information about the received video data
- The advanced example saves the raw data to files
- To terminate either application gracefully, press Ctrl+C
- The SDK supports multiple simultaneous connections from different devices or channels

## Troubleshooting

- Check that the library path is correctly set
- Ensure the device is correctly configured to use the EHome protocol
- Verify network connectivity between the device and the computer
- Check firewall settings to ensure the port is accessible
- Make sure you have write permissions for the directory where you're running the application (for the file-saving example)
- If streams are not saving correctly, check disk space and permissions 