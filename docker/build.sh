#!/usr/bin/env bash

dir=$(dirname $(readlink -f $0))

source $dir/env.sh

opts=

PROXY=http://192.168.1.200:3128

if [[ -n $PROXY ]] ; then
    opts="$opts --build-arg http_proxy=$PROXY"
    opts="$opts --build-arg https_proxy=$PROXY"
    opts="$opts --build-arg HTTP_PROXY=$PROXY"
    opts="$opts --build-arg HTTPS_PROXY=$PROXY"
fi

CARD=$tag
CARD_ML=$card_ml
CARD_VIDEO=$card_video

VAI_VER=
PROTOBUF_VER=

XRT_PACKAGE_URL=
XRT_PACKAGE=
XRT_APU_PACKAGE_URL=
XRT_APU_PACKAGE=
XRM_PACKAGE_URL=
XRM_PACKAGE=

case $tag in
    v70)
        VAI_VER=30
        PROTOBUF_VER=3.6.1
        XRT_PACKAGE=xrt_202220.2.14.418_20.04-amd64-xrt.deb
        XRT_APU_PACKAGE=xrt-apu_202220.2.14.418_petalinux_all.deb
        XRM_PACKAGE_URL="https://www.xilinx.com/bin/public/openDownload?filename=xrm_202220.1.5.212_20.04-x86_64.deb"
        ;;
    u30_u5x)
        VAI_VER=25
        PROTOBUF_VER=3.19.4
        #XRT_PACKAGE_URL="https://github.com/Xilinx/video-sdk/raw/release_2.0/release/U30_Ubuntu_20.04_v2.0/xrt_202110.2.11.691_20.04-amd64-xrt.deb"
        XRT_PACKAGE_URL="https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0.1/xrt_202110.2.11.722_20.04-amd64-xrt.deb"
        XRM_PACKAGE_URL="https://www.xilinx.com/bin/public/openDownload?filename=xrm_202120.1.3.29_20.04-x86_64.deb"
        ;;
    *)
        exit
        ;;
esac

cd $dir
docker buildx build \
    $opts \
    --build-arg CARD=$CARD \
    --build-arg CARD_ML=$CARD_ML \
    --build-arg CARD_VIDEO=$CARD_VIDEO \
    --build-arg VAI_VER=$VAI_VER \
    --build-arg PROTOBUF_VER=$PROTOBUF_VER \
    --build-arg XRT_PACKAGE_URL=$XRT_PACKAGE_URL \
    --build-arg XRT_PACKAGE=$XRT_PACKAGE \
    --build-arg XRT_APU_PACKAGE_URL=$XRT_APU_PACKAGE_URL \
    --build-arg XRT_APU_PACKAGE=$XRT_APU_PACKAGE \
    --build-arg XRM_PACKAGE_URL=$XRM_PACKAGE_URL \
    --build-arg XRM_PACKAGE=$XRM_PACKAGE \
    --target demo \
    -t anjn/alveo-video-analytics-demo:$tag .

