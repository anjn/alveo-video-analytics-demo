#!/usr/bin/env bash
set -xe

command=/bin/bash
if [[ $# -gt 0 ]] ; then
    command="$*"
fi

tag="u30-u5x"
card="u30|u5[05]"
prj_dir=$(dirname $(dirname $(readlink -f $0)))

# Find device
cmd=scan # for older xrt
#cmd=examine
xclmgmt=
for x in $(/opt/xilinx/xrt/bin/xbmgmt $cmd | grep -E "$card" | sed -r 's/^.*inst=([0-9]*).*/\1/'); do
    xclmgmt+="--device /dev/xclmgmt${x} "
done

xocl=
for x in $(/opt/xilinx/xrt/bin/xbutil $cmd | grep -E "$card" | sed -r 's/^.*inst=([0-9]*).*/\1/'); do
    xocl+="--device /dev/dri/renderD${x} "
done

if [[ -z $xocl ]] || [[ -z $xclmgmt ]] ; then
    echo "Error: Couldn't find $card"
    exit 1
fi

# Enable X11
xhost +local:root || true

#opts+="--network none "

docker run \
    --rm \
    -it \
    --name "alveo-video-analytics-demo" \
    $xclmgmt $xocl \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -v $prj_dir:/workspace/demo \
    -p 8554:8554 \
    -p 8555:8555 \
    $opts \
    anjn/alveo-video-analytics-demo:$tag \
    $command

