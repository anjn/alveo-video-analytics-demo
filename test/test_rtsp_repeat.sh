#!/usr/bin/env bash
dir=$(dirname $(dirname $(readlink -f $0)))

if [[ $dir == "/workspace/demo" ]] ; then
    # Setup Video SDK
    export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
    LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &
    source /opt/xilinx/xcdr/setup.sh

    ./start_rtsp.sh &
    
    # Build program
    mkdir -p /tmp/build
    pushd /tmp/build
    cmake /workspace/demo
    make -j$(nproc) test_rtsp_repeat
    
    # Run
    export GST_DEBUG=WARNING
    /tmp/build/test_rtsp_repeat
else
    # Run docker
    cd $dir
    ./docker/run.sh --card u30 --port 8554,8555,8888 ./test/test_rtsp_repeat.sh
fi

