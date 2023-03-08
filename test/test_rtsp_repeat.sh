#!/usr/bin/env bash
dir=$(dirname $(dirname $(readlink -f $0)))

if [[ $dir == "/workspace/demo" ]] ; then
    # Setup Video SDK
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
    ./docker/run.sh --card u30 --port 8555 ./test/test_rtsp_repeat.sh
fi

