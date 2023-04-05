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
    make -j$(nproc) test_rtspclientsink
    
    # Run
    export GST_DEBUG=WARNING
    /tmp/build/test_rtspclientsink $*
else
    if [[ -z $HOST_SERVER_IP ]] ; then
        HOST_SERVER_IP=$(ip route get 1.2.3.4 | head -n 1 | awk '{print $7}')
    fi

    # Run docker
    cd $dir
    ./docker/run.sh --card v70 --port 8554,8888,8889,8189/udp --env HOST_SERVER_IP=$HOST_SERVER_IP ./test/test_rtspclientsink.sh -- $*
fi


