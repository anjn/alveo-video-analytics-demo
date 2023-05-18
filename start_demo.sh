#!/usr/bin/bash
#set -x

if [[ $CARD_VIDEO == v70 ]] ; then

    export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
    
    source /opt/xilinx/xrt/setup.sh
    source /opt/xilinx/xrm/setup.sh
    echo -e "[Runtime]\nxgq_polling=true" | sudo tee /opt/xilinx/xrt/xrt.ini
    export XRT_INI_PATH=/opt/xilinx/xrt/xrt.ini
    export XCLBIN_PATH=/opt/xilinx/xclbin
    export XLNX_VART_FIRMWARE=/opt/xilinx/xclbin/v70.xclbin
    export XLNX_ENABLE_DEVICES=0
    export XLNX_ENABLE_FINGERPRINT_CHECK=0
    
    xrmd &
    source /opt/xilinx/vvas/setup.sh
    export VVAS_CORE_LOG_FILE_PATH=CONSOLE
    #Needed for RawTensor graph runner multi threaded use case 
    sudo cp -rf /opt/xilinx/xclbin/image_processing.cfg /opt/xilinx/vvas/share

elif [[ $CARD_VIDEO == u30 ]] ; then

    # Setup Video SDK
    export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
    LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &
    source /opt/xilinx/xcdr/setup.sh

fi

# Count available device
export num_devices=$(/opt/xilinx/xrt/bin/xbutil examine | grep $CARD_VIDEO | wc -l)
    
DEBUG=0

set -e

# Build program
if [[ $DEBUG -eq 1 ]] ; then
    mkdir -p debug
    pushd debug
    cmake -DCMAKE_BUILD_TYPE=Debug /workspace/demo
else
    mkdir -p build
    pushd build
    cmake /workspace/demo
fi

make -j$(nproc) video_server

set -xe

# Wait servicies up
for port in 5555 ; do
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

# Run
ulimit -n 2048
export GST_DEBUG=WARNING
if [[ $DEBUG -eq 1 ]] ; then
    gdb -ex run --args ./video_server --num-devices $num_devices
    #valgrind --leak-check=no ./video_server --num-devices $num_devices
else
    ./video_server --num-devices $num_devices
fi
