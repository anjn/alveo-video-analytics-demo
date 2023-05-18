#!/usr/bin/env bash
dir=$(dirname $(dirname $(readlink -f $0)))

if [[ $dir == "/workspace/demo" ]] ; then

    # Setup Video SDK
    export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
    LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &
    source /opt/xilinx/xcdr/setup.sh

    ./start_rtsp.sh &
    sleep 3
    
    # Build program
    mkdir -p build
    pushd build
    cmake /workspace/demo
    make -j$(nproc) test_rtspclientsink
    
    # Run
    export GST_DEBUG=WARNING
    #export DISABLE_HW_ENC=1
    ./test_rtspclientsink $*

else

    source $dir/docker/env.sh

    if [[ -z $HOST_SERVER_IP ]] ; then
        HOST_SERVER_IP=$(ip route get 1.2.3.4 | head -n 1 | awk '{print $7}')
    fi

    # Run docker
    cd $dir
    ./docker/run.sh --card $card_video --port 8554,8888,8889,8189/udp --env HOST_SERVER_IP=$HOST_SERVER_IP ./test/test_rtspclientsink.sh -- $*

fi


