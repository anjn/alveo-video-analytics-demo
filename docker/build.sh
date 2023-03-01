#!/usr/bin/env bash

dir=$(dirname $(readlink -f $0))

opts=

#PROXY=http://192.168.1.200:3128

if [[ -n $PROXY ]] ; then
    opts="$opts --build-arg http_proxy=$PROXY"
    opts="$opts --build-arg https_proxy=$PROXY"
    opts="$opts --build-arg HTTP_PROXY=$PROXY"
    opts="$opts --build-arg HTTPS_PROXY=$PROXY"
fi

cd $dir
docker buildx build $opts -t anjn/alveo-video-analytics-demo:u30-u5x .
