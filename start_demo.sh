#!/usr/bin/bash

source /opt/xilinx/xcdr/setup.sh

set -e

# Wait servicies up
for port in 8554 5555 5556 ; do
    until nc -z 127.0.0.1 $port ; do
        sleep 1
    done
done

/tmp/build/video_server
