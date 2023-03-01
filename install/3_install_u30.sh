#!/usr/bin/env bash
set -xe

mkdir /tmp/u30
wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-sc-fw-u30_6.3.8-1.cd35f69_all.deb" -O /tmp/u30/xilinx-sc-fw-u30_6.3.8-1.cd35f69_all.deb
wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-u30-gen3x4-base_2-3391496_all.deb" -O /tmp/u30/xilinx-u30-gen3x4-base_2-3391496_all.deb
wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-u30-gen3x4-validate_2-3380610_all.deb" -O /tmp/u30/xilinx-u30-gen3x4-validate_2-3380610_all.deb
sudo apt install -y /tmp/u30/*.deb

# sudo /opt/xilinx/xrt/bin/xball --device-filter u30 xbmgmt program --base --force
