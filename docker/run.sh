#!/usr/bin/env bash
set -e

VALID_ARGS=$(getopt -o h --long help,tag:,name:,port:,env:,card: -- "$@")
if [[ $? -ne 0 ]]; then
    exit 1;
fi

tag="u30-u5x"
name=demo-video
port=
card="u30|u5[05]"

eval set -- "$VALID_ARGS"
while [ : ]; do
  case "$1" in
    --tag)
        tag=$2 ; shift 2 ;;
    --name)
        name=$2 ; shift 2 ;;
    --port)
        port=$2 ; shift 2 ;;
    --env)
        env=$2 ; shift 2 ;;
    --card)
        card=$2 ; shift 2 ;;
    -h | --help)
        echo "$0 [-h|--help] [--tag TAG] [--name NAME] [--port PORT[,PORT...]] [--env KEY=VALUE[,KEY=VALUE...]] [--card CARD]" ; exit ;;
    --)
        shift ; break ;;
  esac
done

command=/bin/bash
if [[ $# -gt 0 ]] ; then
    command="$*"
fi

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

# Ports
for p in $(echo $port | sed 's/,/ /g') ; do
    opts+="-p $p:$p "
done

# Envs
for e in $(echo $env | sed 's/,/ /g') ; do
    opts+="-e $e "
done

#opts+="--network none "

set -x

docker run \
    --rm \
    -it \
    --name $name \
    $xclmgmt $xocl \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -v $prj_dir:/workspace/demo \
    $opts \
    anjn/alveo-video-analytics-demo:$tag \
    $command

