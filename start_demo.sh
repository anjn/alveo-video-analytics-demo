#!/usr/bin/bash

# Setup Video SDK
LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &
source /opt/xilinx/xcdr/setup.sh

# Build program
mkdir -p /tmp/build
pushd /tmp/build
cmake /workspace/demo
make -j$(nproc) video_server

set -xe

# Wait servicies up
for port in 5555 5556 ; do
    until nc -z $ML_SERVER_IP $port ; do
        sleep 1
    done
done

for port in 8554 ; do
    until nc -z $RTSP_SERVER_IP $port ; do
        sleep 1
    done
done

# Copy config file
cp /workspace/demo/config.toml .
sed -i "s/RTSP_SERVER_IP/$RTSP_SERVER_IP/" config.toml

# Count device
max_devices=$(/opt/xilinx/xrt/bin/xbutil examine | grep u30 | wc -l)

# Run
ulimit -n 2048
export GST_DEBUG=WARNING
./video_server --max-devices $max_devices
