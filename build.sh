#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Generate build files
cmake ..

# Build the application
make

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "\n\033[32mBuild successful!\033[0m"
    echo -e "You can run the application with: \033[1m./hikvision_device_info\033[0m"
else
    echo -e "\n\033[31mBuild failed!\033[0m"
    exit 1
fi 