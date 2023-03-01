#!/usr/bin/env bash
set -xe

mkdir /tmp/u55c
wget "https://www.xilinx.com/bin/public/openDownload?filename=xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz" -O /tmp/u55c/xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz
tar xf /tmp/u55c/xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz -C /tmp/u55c
sudo apt install -y /tmp/u55c/*.deb

#echo
#echo Flash card if needed
#echo "  command: sudo /opt/xilinx/xrt/bin/xball --device-filter u55c xbmgmt program --base --force"
